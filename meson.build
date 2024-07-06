project('plai', 'cpp', default_options: {'cpp_std': 'c++23', 'cpp_rtti': false})


if get_option('b_sanitize') == 'address'
  add_project_arguments('-DPLAI_SDL_NO_QUIT', language: 'cpp')
endif

subdir('lib')
#subdir('bin')
subdir('test')
