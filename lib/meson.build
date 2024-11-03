

DEPS = [
  # segfaults occur with older versions
  dependency('libavcodec', version: '>=61'),
  dependency('libavformat', version: '>=61'),
  dependency('libavutil', version: '>=59'),
  dependency('libswscale', version: '>=8.1.100'),
  dependency('sdl2', version: '>=2.30'),
  httplib_dep,
]

SRCS = []

incdir = include_directories('include')

subdir('src')

libplai = library('plai', SRCS, dependencies: DEPS, include_directories: incdir)
libplai_dep = declare_dependency(link_with: libplai, include_directories: incdir, dependencies: DEPS)

