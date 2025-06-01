

# DEPS = [
#   # segfaults occur with older versions
#   dependency('libavcodec', version: '>=61'),
#   dependency('libavformat', version: '>=61'),
#   dependency('libavutil', version: '>=59'),
#   dependency('libswscale', version: '>=8.1.100'),
#   dependency('sdl2', version: '>=2.30'),
#   dependency('threads'),
#   dependency('sqlite3'),
#   dependency('openssl'),
#   reflectcpp_dep,
# ]
# DEPS = [
#   # segfaults occur with older versions
#   dependency('libavcodec', version: '>=60'),
#   dependency('libavformat', version: '>=60'),
#   dependency('libavutil', version: '>=58'),
#   dependency('libswscale', version: '>=7.5.100'),
#   dependency('sdl2', version: '>=2.30'),
#   dependency('threads'),
#   dependency('sqlite3'),
#   dependency('openssl'),
#   reflectcpp_dep,
# ]
DEPS = [
  # segfaults occur with older versions
  dependency('libavcodec', version: '>=59'),
  dependency('libavformat', version: '>=59'),
  dependency('libavutil', version: '>=57'),
  dependency('libswscale', version: '>=6.7.100'),
  dependency('sdl2', version: '>=2.26'),
  dependency('threads'),
  dependency('sqlite3'),
  dependency('openssl'),
  reflectcpp_dep,
]

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'gcc'
  if compiler.version().version_compare('<14')
    add_project_arguments('-DPLAI_USE_STDFORMAT=0', language: 'cpp')
    DEPS += dependency('fmt')
  endif
endif

SRCS = []

incdir = include_directories('include')

subdir('src')

libplai = library('plai', SRCS, dependencies: DEPS, include_directories: incdir)
libplai_dep = declare_dependency(link_with: libplai, include_directories: incdir, dependencies: DEPS)

