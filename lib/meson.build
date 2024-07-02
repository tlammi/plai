

DEPS = [
  dependency('libavcodec'),
  dependency('libavformat'),
  dependency('libavutil'),
]

SRCS = []

incdir = include_directories('include')

subdir('src')

libplai = library('plai', SRCS, dependencies: DEPS, include_directories: incdir)
libplai_dep = declare_dependency(link_with: libplai, include_directories: incdir, dependencies: DEPS)

