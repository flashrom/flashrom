srcs += files(
  ('endian_' + host_machine.endian() + '.c'),
  'memaccess.c',
)

if host_machine.endian() == 'little'
  add_project_arguments('-D__FLASHROM_LITTLE_ENDIAN__=1', language : 'c')
endif
if host_machine.endian() == 'big'
  add_project_arguments('-D__FLASHROM_BIG_ENDIAN__=1', language : 'c')
endif
