/**********************************************************************/
/*                     DO NOT MODIFY THIS HEADER                      */
/* MAGPIE - Mesoscale Atomistic Glue Program for Integrated Execution */
/*                                                                    */
/*            Copyright 2017 Battelle Energy Alliance, LLC            */
/*                        ALL RIGHTS RESERVED                         */
/**********************************************************************/
#ifdef GSL_ENABLED

#include "PolyatomicDisplacementFunction.h"

// mytrim includes
#include <mytrim/simconf.h>
#include <mytrim/ion.h>
#include <mytrim/element.h>

// general includes
#include <assert.h>
#include <limits>
#include <exception>

// gsl includes
#include "gsl/gsl_sf_legendre.h"
#include "gsl/gsl_integration.h"

PolyatomicDisplacementFunction::PolyatomicDisplacementFunction(
    std::vector<MyTRIM_NS::Element> polyatomic_material,
    nrt_type damage_function_type,
    std::vector<std::vector<Real>> Ecap)
  : PolyatomicDisplacementFunctionBase(polyatomic_material, damage_function_type, Ecap),
    _total_displacement_function(_damage_function_type == TOTAL)
{
  if (damage_function_type != TOTAL && damage_function_type != NET)
    throw std::exception();

  // set the number of indices
  _n_indices = 2;

  Real Edisp_min = std::numeric_limits<Real>::max();
  for (unsigned int j = 0; j < _n_species; ++j)
    if (_material->_element[j]._Edisp < Edisp_min)
      Edisp_min = _material->_element[j]._Edisp;
  _energy_history[0] = Edisp_min;

  // set initial conditions for _displacement_function,
  // note: for net displacement function, the gii = 1, for total displacement function nij = 0
  if (_damage_function_type == NET)
    for (unsigned int i = 0; i < _n_species; ++i)
      _displacement_function[0][mapIndex(i, i, 0)] = 1;
}

std::vector<Real>
PolyatomicDisplacementFunction::getRHS(Real energy)
{
  std::vector<Real> f(_problem_size);
  for (unsigned int i = 0; i < nSpecies(); ++i)
  {
    Real stopping_power = stoppingPower(i, energy);
    for (unsigned int j = 0; j < nSpecies(); ++j)
    {
      // working on the right hand side for nu_ij
      unsigned int n = mapIndex(i, j, 0);
      f[n] = 0;

      for (unsigned int k = 0; k < nSpecies(); ++k)
        f[n] += numberFraction(k) *
                (integralTypeI(energy, i, j, k) + integralTypeII(energy, i, j, k)) / stopping_power;
    }
  }
  return f;
}

Real
PolyatomicDisplacementFunction::integralTypeI(Real energy,
                                              unsigned int i,
                                              unsigned int j,
                                              unsigned int k) const
{
  Real upper_integration_limit = energy * _lambda[i][k];
  Real integral = 0;
  Real delta_kj = k == j && _total_displacement_function ? 1 : 0;
  Real Ebind = _material->_element[k]._Elbind;

  // the integration follows the already existing energies
  for (unsigned int l = 1; l < _energy_history.size(); ++l)
  {
    // adjust integration limits to be within E_{l-1} ... min(E_l, upper_integration_limit)
    Real lower = _energy_history[l - 1];
    Real upper = std::min(_energy_history[l], upper_integration_limit);

    // nothing to integrate
    if (lower > upper)
      continue;

    // now integrate from lower to upper
    Real f = 0.5 * (upper - lower);
    for (unsigned int qp = 0; qp < _quad_order; ++qp)
    {
      Real recoil_energy = f * (_quad_points[qp] + 1) + lower;
      integral += f * _quad_weights[qp] * scatteringCrossSection(i, k, energy, recoil_energy) *
                  displacementProbability(k, recoil_energy) *
                  (delta_kj + linearInterpolation(recoil_energy - Ebind, k, j));
    }
  }
  return integral;
}

Real
PolyatomicDisplacementFunction::integralTypeII(Real energy,
                                               unsigned int i,
                                               unsigned int j,
                                               unsigned int k) const
{
  Real upper_integration_limit = energy * _lambda[i][k];
  Real threshold = std::min(_asymptotic_threshold, energy * _lambda[i][k]);

  // store the current displacement function value
  Real current_value = linearInterpolation(energy, i, j);

  // estimate the derivative d(nu_i) / dE:
  Real dE = _energy_history.back() - _energy_history[_energy_history.size() - 2];
  Real derivative = (current_value - linearInterpolation(energy - dE, i, j)) / dE;

  // integrate up to threshold and multiply by estimate of the derivative
  Real integral = -weightedScatteringIntegral(energy, threshold, i, k) * derivative;

  if (energy * _lambda[i][k] <= _asymptotic_threshold)
    return integral;

  // the integration follows the already existing energies
  for (unsigned int l = 0; l < _energy_history.size(); ++l)
  {
    // adjust integration limits to be within
    Real lower;
    if (l == 0)
      lower = _asymptotic_threshold;
    else
      lower = std::max(_energy_history[l - 1], _asymptotic_threshold);

    Real upper = std::min(_energy_history[l], upper_integration_limit);

    // nothing to integrate
    if (lower > upper)
      continue;

    // now integrate from lower to upper
    Real f = 0.5 * (upper - lower);
    for (unsigned int qp = 0; qp < _quad_order; ++qp)
    {
      Real recoil_energy = f * (_quad_points[qp] + 1) + lower;
      integral += f * _quad_weights[qp] * scatteringCrossSection(i, k, energy, recoil_energy) *
                  (nonCaptureProbability(i, k, energy, recoil_energy) *
                       linearInterpolation(energy - recoil_energy, i, j) -
                   current_value);
    }
  }
  return integral;
}

#endif
