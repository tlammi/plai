
tests = files(
  'pkt_count.cpp',
  'void_decoder.cpp',
  'hwaccel_list.cpp',
  'player.cpp',
  'blend_player.cpp',
  'rest.cpp',
  'watermark_player.cpp',
  'store.cpp',
  'periodic_task.cpp',
)

fs = import('fs')

foreach t: tests
  nm = fs.name(t)
  exec = nm.split('.')[0]
  exec = exec.replace('_', '-')
  executable(exec, t, dependencies: libplai_dep)
endforeach
