/**********************************************************************/
/*                     DO NOT MODIFY THIS HEADER                      */
/* MAGPIE - Mesoscale Atomistic Glue Program for Integrated Execution */
/*                                                                    */
/*            Copyright 2017 Battelle Energy Alliance, LLC            */
/*                        ALL RIGHTS RESERVED                         */
/**********************************************************************/

#include "PKAGeneratorBase.h"
#include "MagpieUtils.h"
#include <algorithm>

InputParameters
PKAGeneratorBase::validParams()
{
  InputParameters params = DiscreteElementUserObject::validParams();
  params.addClassDescription("PKA generator user object base class.\n Takes pdf and samples PKAs "
                             "due to various interactions.");
  return params;
}

PKAGeneratorBase::PKAGeneratorBase(const InputParameters & parameters)
  : DiscreteElementUserObject(parameters)
{
  setRandomResetFrequency(EXEC_TIMESTEP_END);
}

void
PKAGeneratorBase::setPosition(MyTRIM_NS::IonBase & ion) const
{
  ion._pos = MagpieUtils::randomElementPoint(*_current_elem, getRandomPoint());
}

int
PKAGeneratorBase::ionTag(const MyTRIMRasterizer::PKAParameters & pka_parameters,
                         Real Z,
                         Real m) const
{
  // this function relies on the exact representation of whole numbers in IEEE floating point
  // numbers up to a reasonable upper limit [Z < m < 300]

  const auto & mZ = pka_parameters._mass_charge_tuple;

  // element not found in rasterizer table
  if (pka_parameters._index_Z[Z].first == 0)
    return -1;

  // only one isotope of this element is present
  if (pka_parameters._index_Z[Z].first == 1)
  {
    auto & t = mZ[pka_parameters._index_Z[Z].second];
    if (std::abs(m - std::get<0>(t)) < std::get<2>(t))
      return pka_parameters._index_Z[Z].second;
    else
      return -1;
  }

  // start the search at the first matching Z
  for (auto i = pka_parameters._index_Z[Z].second; i < mZ.size(); ++i)
    if (std::get<1>(mZ[i]) == Z && std::abs(m - std::get<0>(mZ[i])) < std::get<2>(mZ[i]))
      return i;

  // no matching mass (isotope) found for the given Z
  return -1;
}

void
PKAGeneratorBase::setRandomDirection(MyTRIM_NS::IonBase & ion) const
{
  Real nsq, x1, x2;

  // Marsaglia's method for uniformly sampling the surface of the sphere
  do
  {
    x1 = 2 * getRandomReal() - 1.0;
    x2 = 2 * getRandomReal() - 1.0;
    nsq = x1 * x1 + x2 * x2;
  } while (nsq >= 1);

  // construct normalized direction vector
  ion._dir(0) = 2.0 * x1 * std::sqrt(1.0 - nsq);
  ion._dir(1) = 2.0 * x2 * std::sqrt(1.0 - nsq);
  ion._dir(2) = 1.0 - 2.0 * nsq;
}
