project('plai', 'cpp', default_options: {'cpp_std': 'c++23', 'cpp_rtti': false})

cmake = import('cmake')

add_project_arguments('-Wfatal-errors', language:'cpp')


reflectcpp_dep = dependency('reflect-cpp', 'reflectcpp', required: false)

if not reflectcpp_dep.found()
  opts = cmake.subproject_options()
  opts.add_cmake_defines({'BUILD_SHARED_LIBS': 'OFF', 'CMAKE_POSITION_INDEPENDENT_CODE': 'ON'})
  proj = cmake.subproject('reflect-cpp', options: opts)
  reflectcpp_dep = proj.get_variable('reflectcpp_dep')
endif

if get_option('b_sanitize') == 'address'
  add_project_arguments('-DPLAI_SDL_NO_QUIT', language: 'cpp')
endif

subdir('lib')
subdir('bin')
if get_option('tests')
  subdir('test')
endif
