project('plai', 'cpp', default_options: {'cpp_std': 'c++23', 'cpp_rtti': false})


httplib_proj = subproject('cpp-httplib')
httplib_dep = httplib_proj.get_variable('cpp_httplib_dep')

if get_option('b_sanitize') == 'address'
  add_project_arguments('-DPLAI_SDL_NO_QUIT', language: 'cpp')
endif

subdir('lib')
subdir('bin')
subdir('test')
