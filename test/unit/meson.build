tests = files(
  'demux.cpp',
  'decode.cpp',
  'ring_buffer.cpp',
  'frontend.cpp',
  'parse.cpp',
  'frac.cpp',
  'queue.cpp',
  'prof.cpp',
  'persist_buffer.cpp',
)

fs = import('fs')

test_deps = [
  dependency('gtest'),
  dependency('gtest_main'),
  libplai_dep,
]

foreach t: tests
  nm = fs.name(t)
  nm = nm.split('.')[0]
  test_nm = '@0@_test'.format(nm)
  e = executable(test_nm, t, dependencies: test_deps)
  test(test_nm, e)
endforeach
