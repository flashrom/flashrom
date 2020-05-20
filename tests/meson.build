root_includes = include_directories('../subprojects')

srcs = [
  'tests.c',
]

mocks = [
  '-Wl,--wrap=physunmap',
  '-Wl,--wrap=physmap',
  '-Wl,--gc-sections',
]

flashrom_tests = executable('flashrom_unit_tests',
  srcs,
  include_directories : root_includes,
  c_args : [
    cargs,
    '-ffunction-sections',
    '-fdata-sections',
    #    '-DSTANDALONE',
    '-DCONFIG_DEFAULT_PROGRAMMER=PROGRAMMER_DUMMY',
    '-DCONFIG_DEFAULT_PROGRAMMER_ARGS=""',
  ],
  export_dynamic : true,
  link_args : mocks,
  dependencies : [cmocka_dep, flashrom_test_dep],
)
test('cmocka test flashrom', flashrom_tests)
