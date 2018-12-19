executable(
  'ich_descriptors_tool',
  sources : [
    'ich_descriptors_tool.c',
    '../../ich_descriptors.c',
  ],
  dependencies : [
    deps,
  ],
  include_directories : include_directories('../..'),
  c_args : [
    '-DICH_DESCRIPTORS_FROM_DUMP_ONLY',
  ],
)
