# Endian
srcs += files(
  'endian_common.c',
  'endian_' + host_machine.endian() + '.c',
)
# __FLASHROM_LITTLE_ENDIAN__ / __FLASHROM_BIG_ENDIAN__
add_project_arguments('-D__FLASHROM_' + host_machine.endian().to_upper() + '_ENDIAN__=1', language : 'c')

# Select an appropriate delay implementation for the target OS
if target_machine.system() == 'dos'
  srcs += files('udelay_dos.c')
elif target_machine.system() == 'windows'
  srcs += files('udelay_windows.c')
else
  srcs += files('udelay.c')
endif
cargs += ['-DCONFIG_DELAY_MINIMUM_SLEEP_US=@0@'.format(
  get_option('delay_minimum_sleep_us')
)]

# OpenBSD requires libi386 or libamd64 for I/O port handling
if host_machine.system() == 'openbsd'
  if host_machine.cpu_family() == 'x86'
    libi386 = cc.find_library('i386')
    deps += libi386
  elif host_machine.cpu_family() == 'x86_64'
    libamd64 = cc.find_library('amd64')
    deps += libamd64
  endif
endif

# NetBSD requires libx86 or libx86_64 for I/O port handling
if host_machine.system() == 'netbsd'
  if host_machine.cpu_family() == 'x86'
    libx86 = cc.find_library('x86')
    deps += libx86
  elif host_machine.cpu_family() == 'x86_64'
    libx86_64 = cc.find_library('x86_64')
    deps += libx86_64
  endif
endif

# SunOS requires external libraries for network sockets
# they are used to support serial devices via network
if host_machine.system() == 'sunos'
  libsocket = cc.find_library('socket')
  libnsl = cc.find_library('nsl')
  deps += [ libsocket, libnsl]
endif
