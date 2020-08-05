[Mesh]
  type = MyTRIMMesh
  dim = 3
  xmin = -5
  xmax = 5
  ymin = -5
  ymax = 5
  zmin = -5
  zmax = 5
  nx = 63
  ny = 63
  nz = 63
[]

[Problem]
  type = FFTProblem
[]

[AuxVariables]
  [./epsilon_aux_var]
    order = CONSTANT
    family = MONOMIAL
  [../]

  [./stress_aux_var]
    order = CONSTANT
    family = MONOMIAL
  [../]

  [./elastic_aux_var]
    order = CONSTANT
    family = MONOMIAL

  [../]

  [./index_buffer_aux_var]
    order = CONSTANT
    family = MONOMIAL
  [../]

  [./stiffness_ratio_aux]
    order = CONSTANT
    family = MONOMIAL
    [./InitialCondition]
      type = SmoothSuperellipsoidIC
      x1 = 0
      y1 = 0
      z1 = 0
      a = 2
      b = 2
      c = 2
      n = 2
      int_width = 2
      invalue = 1
      outvalue = 10.0
    [../]
  [../]
[]

[UserObjects]
  # Buffers
  [./epsilon]
    type = RankTwoTensorFFTWBuffer
  [../]
  [./stress]
    type = RankTwoTensorFFTWBuffer
  [../]
  # Reciprocal space: Elastic tensor
  [./gamma]
    type = RankFourTensorFFTWBuffer
  [../]
  [./elastic]
    type = RankFourTensorFFTWBuffer
  [../]
  [./stiffness_ratio]
     type = RealFFTWBuffer
     moose_variable = stiffness_ratio_aux
  []
  [./index_buffer]
     type = RealFFTWBuffer
  []
[]

[AuxKernels]
  [./epsilon_aux]
    type = FFTBufferAux
    variable = epsilon_aux_var
    fft_buffer = epsilon
    execute_on = final
    component = '0 1'
  [../]

  [./stress_aux]
    type = FFTBufferAux
    variable = stress_aux_var
    fft_buffer = stress
    execute_on = final
    component = '0 1'
  [../]

  [./elastic_aux]
    type = FFTBufferAux
    variable = elastic_aux_var
    fft_buffer = elastic
    execute_on = final
    component = '0 1 0 1'
  [../]

  [./stiffness_aux]
    type = FFTBufferAux
    variable = stiffness_ratio_aux
    fft_buffer = stiffness_ratio
    execute_on = final
  [../]

  [./index_aux]
    type = FFTBufferAux
    variable = index_buffer_aux_var
    fft_buffer = index_buffer
    execute_on = final
  [../]
[]

[Executioner]
  type = SpectralExecutionerLinearElastic

  time_step = 1.0
  number_steps = 300
  initial_shear_strain = 0.0001
  young_modulus = 1e4
  poisson_ratio = 0.3
  average_material_factor = 3.3
[]

[VectorPostprocessors]
  [./linevalue]
    type = LineValueSampler
    variable = 'stress_aux_var'
    start_point = '-4.9999999999 0 0'
    end_point = '4.9999999999 0 0'
    num_points = 101
    sort_by = x
    execute_on = final
  [../]
[]

[Outputs]
  exodus = true
  execute_on = 'INITIAL FINAL'
  perf_graph = true
  [./comp]
    type = CSV
  [../]
[]
