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
