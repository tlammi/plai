
tests = files('pkt_count.cpp', 'void_decoder.cpp', 'hwaccel_list.cpp')

fs = import('fs')

foreach t: tests
  nm = fs.name(t)
  exec = nm.split('.')[0]
  exec = exec.replace('_', '-')
  executable(exec, t, dependencies: libplai_dep)
endforeach

# executable('pkt-count', 'pkt_count.cpp', dependencies: [libplai_dep])
# executable('void-decoder', 'void_decoder.cpp', dependencies: [libplai_dep])
