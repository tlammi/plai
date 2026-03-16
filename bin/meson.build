
srcs = files('plai.cpp', 'cli.cpp')
deps = [dependency('CLI11', fallback: ['cli11', 'CLI11_dep']), libplai_dep]
executable('plai', srcs, dependencies: deps, install: true)
executable('plai_new', files('plai_new.cpp'), dependencies: deps)
executable('testing', files('testing.cpp'), dependencies: deps)
