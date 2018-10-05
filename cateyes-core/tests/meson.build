test_sources = [
  'test-system.vala',
  'test-injector.vala',
  'test-agent.vala',
  'test-agent-glue.c',
  'test-gadget.vala',
  'test-host-session.vala',
  'runner.vala',
  'runner-glue.c',
  'labrats.vala',
  'async-harness.vala',
  'process.vala',
  'process-resource-usage.c',
]

subdir('labrats')

system_vala_args = []

if host_os_family == 'windows'
  test_sources += ['process-windows.c']
else
  test_sources += ['process-unix.c']

  system_vala_args += ['--pkg=posix']
endif

cateyes_tests = static_library('cateyes-tests', test_sources + [labrats_stamp],
  vala_args: system_vala_args,
  dependencies: [gmodule_dep, json_glib_dep, core_dep],
)

runner_name = 'cateyes-tests'
if host_os_family == 'darwin'
  runner_name = 'cateyes-tests-unsigned'
endif

runner_link_args = []
runner_link_depends = []
if host_os == 'macos'
  runner_plist = 'cateyes-tests.plist'
  runner_link_args += ['-Wl,-sectcreate,__TEXT,__info_plist,' + join_paths(meson.current_source_dir(), runner_plist)]
  runner_link_depends += [runner_plist]
endif

runner = executable(runner_name, 'main.vala',
  link_args: runner_link_args,
  link_depends: runner_link_depends,
  link_with: cateyes_tests,
  dependencies: [core_dep],
)

if host_os_family == 'darwin'
  custom_target('cateyes-tests-signed',
    input: [
      runner,
      'cateyes-tests.xcent',
    ],
    output: 'cateyes-tests',
    command: [
      files('sign.sh'),
      host_os,
      '@INPUT@',
      '@OUTPUT@',
    ],
    build_by_default: true,
  )
endif
