TESTS = files(
  'demux.cpp',
  'decode.cpp',
  'ring_buffer.cpp',
  'frontend.cpp',
  'parse.cpp',
  'frac.cpp',
  'prof.cpp',
  'persist_buffer.cpp',
  'frame.cpp',
  'view_generator.cpp',
  'str.cpp',
  'crypto.cpp',
  'store.cpp',
  'vec.cpp',
  'inplace.cpp',
)

subdir('net')

fs = import('fs')

test_deps = [
  dependency('gtest'),
  dependency('gtest_main'),
  dependency('gmock'),
  libplai_dep,
]

foreach t: TESTS
  path = t.full_path()
  path = fs.relative_to(path, '.')
  path = path.split('.')[0]
  parts = path.split('/')
  nm = '.'.join(parts)
  e = executable(nm, t, dependencies: test_deps)
  test(nm, e)
endforeach
