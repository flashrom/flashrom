project('flashromutils', 'c',
  version : run_command('cat', 'VERSION', check: true).stdout().strip(),
  license : 'GPL-2.0',
  meson_version : '>=0.53.0',
  default_options : [
    'warning_level=2',
    'c_std=c99',
    'werror=true',
    'optimization=s',
    'debug=false',
    'default_library=both'
  ],
)

if get_option('classic_cli').enabled() and get_option('default_library') == 'shared'
  error('''
    Cannot build cli_classic with shared libflashrom. Use \'-Dclassic_cli=disabled\' to disable the cli,
    or use \'--default-library=both\' to also build the classic_cli
  ''')
endif

# libtool versioning
lt_current = '1'
lt_revision = '0'
lt_age = '0'
lt_version = '@0@.@1@.@2@'.format(lt_current, lt_age, lt_revision)

flashrom_version = meson.project_version()
git = find_program('git', native : true, required : false)
if git.found()
  version_git = run_command('git', 'describe', check : false)
  if version_git.returncode() == 0
    flashrom_version += ' (git:@0@)'.format(version_git.stdout().strip())
  endif
endif

subdir('doc')

# hide/enable some warnings
warning_flags = [
  '-Wshadow',
  '-Wmissing-prototypes',
  '-Wwrite-strings',
  '-Wno-unused-parameter',
  '-Wno-address-of-packed-member',
  '-Wno-enum-conversion',
  '-Wno-missing-braces',
]

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments(warning_flags), language : 'c')
add_project_arguments('-D_DEFAULT_SOURCE', language : 'c')
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language : 'c') # required for fileno, nanosleep, and strndup
add_project_arguments('-D_BSD_SOURCE', language : 'c') # required for glibc < v2.19
add_project_arguments('-D__BSD_VISIBLE', language : 'c') # required for u_char, u_int, u_long on FreeBSD
add_project_arguments('-D__XSI_VISIBLE', language : 'c') # required for gettimeofday() on FreeBSD
add_project_arguments('-D_NETBSD_SOURCE', language : 'c') # required for indirect include of strings.h on NetBSD
add_project_arguments('-D_DARWIN_C_SOURCE', language : 'c') # required for indirect include of strings.h on MacOS
add_project_arguments('-DFLASHROM_VERSION="' + flashrom_version + '"', language : 'c')

# get defaults from configure
config_print_wiki= get_option('classic_cli_print_wiki')
config_default_programmer_name = get_option('default_programmer_name')
config_default_programmer_args = get_option('default_programmer_args')

cargs = []
link_args = []
deps = []
srcs = files(
  '82802ab.c',
  'at45db.c',
  'bitbang_spi.c',
  'edi.c',
  'en29lv640b.c',
  'erasure_layout.c',
  'flashchips.c',
  'flashrom.c',
  'fmap.c',
  'helpers.c',
  'helpers_fileio.c',
  'ich_descriptors.c',
  'jedec.c',
  'printlock.c',
  'layout.c',
  'libflashrom.c',
  'opaque.c',
  'parallel.c',
  'print.c',
  'programmer.c',
  'programmer_table.c',
  's25f.c',
  'sfdp.c',
  'spi25.c',
  'spi25_statusreg.c',
  'spi95.c',
  'spi.c',
  'sst28sf040.c',
  'sst49lfxxxc.c',
  'sst_fwhub.c',
  'stm50.c',
  'udelay.c',
  'w29ee011.c',
  'w39.c',
  'writeprotect.c',
  'writeprotect_ranges.c',
)

# check for required symbols
if cc.has_function('clock_gettime')
  add_project_arguments('-DHAVE_CLOCK_GETTIME=1', language : 'c')
endif
if cc.has_function('strnlen')
  add_project_arguments('-DHAVE_STRNLEN=1', language : 'c')
endif
if cc.check_header('sys/utsname.h')
  add_project_arguments('-DHAVE_UTSNAME=1', language : 'c')
endif
if host_machine.system() in ['cygwin', 'windows']
  add_project_arguments('-DIS_WINDOWS=1', language : 'c')
else
  add_project_arguments('-DIS_WINDOWS=0', language : 'c')
endif

if host_machine.system() == 'linux'
  custom_baud_c = 'custom_baud_linux.c'
elif host_machine.system() == 'darwin'
  custom_baud_c = 'custom_baud_darwin.c'
else
  custom_baud_c = 'custom_baud.c'
endif

systems_hwaccess   = [ 'linux', 'openbsd', 'freebsd', 'dragonfly', 'netbsd', 'dos' ]
systems_serial     = [ 'linux', 'openbsd', 'freebsd', 'dragonfly', 'netbsd', 'darwin', 'windows' ]

cpus_port_io = [ 'x86', 'x86_64' ]

group_ftdi   = get_option('programmer').contains('group_ftdi')
group_pci    = get_option('programmer').contains('group_pci')
group_usb    = get_option('programmer').contains('group_usb')
group_i2c    = get_option('programmer').contains('group_i2c')
group_serial = get_option('programmer').contains('group_serial')
group_jlink  = get_option('programmer').contains('group_jlink')
group_internal = get_option('programmer').contains('group_internal')
group_external = get_option('programmer').contains('group_external')

libpci     = dependency('libpci', required : group_pci, version : '>=2.2.0',
  static : (host_machine.system() == 'openbsd' ? true : false)) # On openbsd a static version of libpci is needed to get also -libz
libusb1    = dependency('libusb-1.0', required : group_usb)
libftdi1   = dependency('libftdi1', required : group_ftdi)
libjaylink = dependency('libjaylink', required : group_jlink, version : '>=0.3.0')

subdir('platform')

if systems_hwaccess.contains(host_machine.system())
  srcs += files('hwaccess_physmap.c')
  if ['x86', 'x86_64'].contains(host_machine.cpu_family())
    srcs += files('hwaccess_x86_msr.c', 'hwaccess_x86_io.c')
  endif
endif

# Pseudo dependencies
linux_headers = \
  cc.has_header('linux/i2c.h')     and \
  cc.has_header('linux/i2c-dev.h') and \
  cc.has_header('mtd/mtd-user.h')  and \
  cc.has_header('linux/spi/spidev.h') ? declare_dependency() : dependency('', required : false)

# '<programmer_name>' : {
#   'system'      : list[string],  # default: ['all']
#   'cpu_families : list[string],  # default: ['all']
#   'deps'        : list[dep],     # default: []
#   'groups       : list[boolean], # default: []
#   'srcs'        : list[file],    # default: []
#   'flags'       : list[string],  # default: []
#   'default'     : boolean,       # default: true
#   'active'      : boolean,       # added on runtime
# }
programmer = {
  'asm106x' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('asm106x.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_ASM106X=1' ],
  },
  'atahpt' : {
    'systems' : systems_hwaccess,
    'cpu_families' : cpus_port_io,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('atahpt.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_ATAHPT=1' ],
    'default' : false, # not yet working
  },
  'atapromise' : {
    'systems' : systems_hwaccess,
    'cpu_families' : cpus_port_io,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('atapromise.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_ATAPROMISE=1' ],
    'default' : false,
  },
  'atavia' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('atavia.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_ATAVIA=1' ],
  },
  'buspirate_spi' : {
    'systems' : systems_serial,
    'groups'  : [ group_serial, group_external ],
    'srcs'    : files('buspirate_spi.c', 'serial.c', custom_baud_c),
    'flags'   : [ '-DCONFIG_BUSPIRATE_SPI=1' ],
  },
  'ch341a_spi' : {
    'deps'    : [ libusb1 ],
    'groups'  : [ group_usb, group_external ],
    'srcs'    : files('ch341a_spi.c'),
    'test_srcs' : files('tests/ch341a_spi.c'),
    'flags'   : [ '-DCONFIG_CH341A_SPI=1' ],
  },
  'ch347_spi' : {
    'deps'    : [ libusb1 ],
    'groups'  : [ group_usb, group_external ],
    'srcs'    : files('ch347_spi.c'),
    'flags'   : [ '-DCONFIG_CH347_SPI=1' ],
  },
  'dediprog' : {
    'deps'    : [ libusb1 ],
    'groups'  : [ group_usb, group_external ],
    'srcs'    : files('dediprog.c', 'usbdev.c'),
    'test_srcs' : files('tests/dediprog.c'),
    'flags'   : [ '-DCONFIG_DEDIPROG=1' ],
  },
  'developerbox_spi' : {
    'deps'    : [ libusb1 ],
    'groups'  : [ group_usb, group_external ],
    'srcs'    : files('developerbox_spi.c', 'usbdev.c'),
    'flags'   : [ '-DCONFIG_DEVELOPERBOX_SPI=1' ],
  },
  'digilent_spi' : {
    'deps'    : [ libusb1 ],
    'groups'  : [ group_usb, group_external ],
    'srcs'    : files('digilent_spi.c'),
    'flags'   : [ '-DCONFIG_DIGILENT_SPI=1' ],
  },
  'dirtyjtag_spi' : {
    'deps'    : [ libusb1 ],
    'groups'  : [ group_usb, group_external ],
    'srcs'    : files('dirtyjtag_spi.c'),
    'flags'   : [ '-DCONFIG_DIRTYJTAG_SPI=1' ],
  },
  'drkaiser' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('drkaiser.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_DRKAISER=1' ],
  },
  'dummy'     : {
    'srcs'    : files('dummyflasher.c'),
    'test_srcs' : files('tests/dummyflasher.c'),
    'flags'   : [ '-DCONFIG_DUMMY=1' ],
  },
  'ft2232_spi' : {
    'deps'    : [ libftdi1 ],
    'groups'  : [ group_ftdi, group_external ],
    'srcs'    : files('ft2232_spi.c' ),
    'flags'   : [ '-DCONFIG_FT2232_SPI=1' ],
  },
  'gfxnvidia' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('gfxnvidia.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_GFXNVIDIA=1' ],
  },
  'internal' : {
    'systems' : systems_hwaccess + ['linux'],
    'cpu_families' : (host_machine.system() == 'linux' ? [host_machine.cpu_family()] : ['x86', 'x86_64']),
    'deps'    : [ libpci ],
    'groups'  : [ group_internal ],
    'srcs'    : (host_machine.cpu_family() in ['x86', 'x86_64'] ? files(
      'processor_enable.c',
      'chipset_enable.c',
      'board_enable.c',
      'cbtable.c',
      'internal.c',
      'internal_par.c',
      'it87spi.c',
      'sb600spi.c',
      'superio.c',
      'amd_imc.c',
      'wbsio_spi.c',
      'mcp6x_spi.c',
      'ichspi.c',
      'dmi.c',
      'pcidev.c',
      'known_boards.c',
    ) : files(
      'board_enable.c',
      'cbtable.c',
      'chipset_enable.c',
      'internal.c',
      'internal_par.c',
      'processor_enable.c',
      'pcidev.c',
      'known_boards.c',
    )),
    'flags' : [
      '-DCONFIG_INTERNAL=1',
      '-DCONFIG_INTERNAL_DMI=' + (get_option('use_internal_dmi') ? '1' : '0'),
    ]
  },
  'it8212' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('it8212.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_IT8212=1' ],
  },
  'jlink_spi' : {
    'deps'    : [ libjaylink ],
    'groups'  : [ group_jlink, group_external ],
    'srcs'    : files('jlink_spi.c'),
    'flags'   : [ '-DCONFIG_JLINK_SPI=1' ],
  },
  'linux_mtd' : {
    'systems' : [ 'linux' ],
    'deps'    : [ linux_headers ],
    'groups'  : [ group_internal ],
    'srcs'    : files('linux_mtd.c'),
    'test_srcs' : files('tests/linux_mtd.c'),
    'flags'   : [ '-DCONFIG_LINUX_MTD=1' ],
  },
  'linux_spi' : {
    'systems' : [ 'linux' ],
    'deps'    : [ linux_headers ],
              # internal / external?
    'srcs'    : files('linux_spi.c'),
    'test_srcs' : files('tests/linux_spi.c'),
    'flags'   : [ '-DCONFIG_LINUX_SPI=1' ],
  },
  'parade_lspcon' : {
    'systems' : [ 'linux' ],
    'deps'    : [ linux_headers ],
    'groups'  : [ group_i2c ],
    'srcs'    : files('parade_lspcon.c', 'i2c_helper_linux.c'),
    'test_srcs' : files('tests/parade_lspcon.c'),
    'flags'   : [ '-DCONFIG_PARADE_LSPCON=1' ],
    'default' : false
  },
  'mediatek_i2c_spi' : {
    'systems' : [ 'linux' ],
    'deps'    : [ linux_headers ],
    'groups'  : [ group_i2c ],
    'srcs'    : files('mediatek_i2c_spi.c', 'i2c_helper_linux.c'),
    'test_srcs' : files('tests/mediatek_i2c_spi.c'),
    'flags'   : [ '-DCONFIG_MEDIATEK_I2C_SPI=1' ],
    'default' : false,
  },
  'mstarddc_spi' : {
    'systems' : [ 'linux' ],
    'deps'    : [ linux_headers ],
    'groups'  : [ group_i2c ],
    'srcs'    : files('mstarddc_spi.c', 'i2c_helper_linux.c'),
    'flags'   : [ '-DCONFIG_MSTARDDC_SPI=1' ],
    'default' : false
  },
  'nic3com' : {
    'systems' : systems_hwaccess,
    'cpu_families' : cpus_port_io,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('nic3com.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_NIC3COM=1' ],
  },
  'nicintel' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('nicintel.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_NICINTEL=1' ],
  },
  'nicintel_eeprom' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('nicintel_eeprom.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_NICINTEL_EEPROM=1' ],
  },
  'nicintel_spi' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('nicintel_spi.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_NICINTEL_SPI=1' ],
  },
  'nicnatsemi' : {
    'systems' : systems_hwaccess,
    'cpu_families' : cpus_port_io,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('nicnatsemi.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_NICNATSEMI=1' ],
    'default' : false, # not complete nor tested
  },
  'nicrealtek' : {
    'systems' : systems_hwaccess,
    'cpu_families' : cpus_port_io,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('nicrealtek.c', 'pcidev.c'),
    'test_srcs' : files('tests/nicrealtek.c'),
    'flags'   : [ '-DCONFIG_NICREALTEK=1' ],
  },
  'ogp_spi' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('ogp_spi.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_OGP_SPI=1' ],
  },
  'pickit2_spi' : {
    'deps'    : [ libusb1 ],
    'groups'  : [ group_usb, group_external ],
    'srcs'    : files('pickit2_spi.c'),
    'flags'   : [ '-DCONFIG_PICKIT2_SPI=1' ],
  },
  'pony_spi' : {
    'systems' : systems_serial,
    'groups'  : [ group_serial, group_external ],
    'srcs'    : files('pony_spi.c', 'serial.c', custom_baud_c),
    'flags'   : [ '-DCONFIG_PONY_SPI=1' ],
  },
  'raiden_debug_spi' : {
    'deps'    : [ libusb1 ],
    'groups'  : [ group_usb, group_external ],
    'srcs'    : files('raiden_debug_spi.c', 'usb_device.c'),
    'test_srcs' : files('tests/raiden_debug_spi.c'),
    'flags'   : [ '-DCONFIG_RAIDEN_DEBUG_SPI=1' ],
  },
  'rayer_spi' : {
    'systems' : systems_hwaccess,
    'cpu_families' : cpus_port_io,
    'groups'  : [ group_internal ],
    'srcs'    : files('rayer_spi.c'),
    'flags'   : [ '-DCONFIG_RAYER_SPI=1' ],
  },
  'realtek_mst_i2c_spi' : {
    'systems' : [ 'linux' ],
    'deps'    : [ linux_headers ],
    'groups'  : [ group_i2c ],
    'srcs'    : files('realtek_mst_i2c_spi.c', 'i2c_helper_linux.c'),
    'test_srcs' : files('tests/realtek_mst_i2c_spi.c'),
    'flags'   : [ '-DCONFIG_REALTEK_MST_I2C_SPI=1' ],
    'default' : false,
  },
  'satamv' : {
    'systems' : systems_hwaccess,
    'cpu_families' : cpus_port_io,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('satamv.c', 'pcidev.c'),
    'flags'   : ['-DCONFIG_SATAMV=1'],
  },
  'satasii' : {
    'systems' : systems_hwaccess,
    'deps'    : [ libpci ],
    'groups'  : [ group_pci, group_internal ],
    'srcs'    : files('satasii.c', 'pcidev.c'),
    'flags'   : [ '-DCONFIG_SATASII=1' ],
  },
  'serprog' : {
    'systems' : systems_serial,
    'groups'  : [ group_serial, group_external ],
    'srcs'    : files('serprog.c', 'serial.c', custom_baud_c),
    'flags'   : [ '-DCONFIG_SERPROG=1' ],
  },
  'stlinkv3_spi' : {
    'deps'    : [ libusb1 ],
    'groups'  : [ group_usb, group_external ],
    'srcs'    : files('stlinkv3_spi.c', 'usbdev.c'),
    'flags'   : [ '-DCONFIG_STLINKV3_SPI=1' ],
  },
  'usbblaster_spi' : {
    'deps'    : [ libftdi1 ],
    'groups'  : [ group_ftdi, group_external ],
    'srcs'    : files('usbblaster_spi.c'),
    'flags'   : [ '-DCONFIG_USBBLASTER_SPI=1' ],
  },
}

active_programmer_count = 0
foreach p_name, p_data : programmer
  p_data += {
    'systems' : p_data.get('systems', ['all']),
    'cpu_families' : p_data.get('cpu_families', ['all']),
    'deps' : p_data.get('deps', []),
    'groups' : p_data.get('groups', []),
    'srcs' : p_data.get('srcs', []),
    'test_srcs': p_data.get('test_srcs', []),
    'flags' : p_data.get('flags', []),
    'default' : p_data.get('default', true),
  }

  active        = false
  deps_found    = true
  not_found_dep = ''
  not_active_message = ''
  selected_hard = p_name in get_option('programmer')
  selected_soft = p_data.get('groups').contains(true) or \
                  'all' in get_option('programmer') or \
                  'auto' in get_option('programmer') and p_data.get('default')
  available     = (p_data.get('systems').contains('all') or p_data.get('systems').contains(host_machine.system())) \
                  and (p_data.get('cpu_families').contains('all') or p_data.get('cpu_families').contains(host_machine.cpu_family()))

  foreach dep : p_data.get('deps')
    if not dep.found()
      deps_found = false
      not_found_dep = dep.name()
      break
    endif
  endforeach

  if selected_hard
    if not available
      error(p_name + ' selected but not supported on this platform')
    elif not deps_found
      error(p_name + ' selected but dependency ' + not_found_dep +'not found')
    else
      active = true
    endif
  elif selected_soft
    if not available
      not_active_message = 'Not available on platform'
    elif not deps_found
      not_active_message = 'dependency ' + not_found_dep + ' not found'
    else
      active = true
    endif
  else
    not_active_message = 'not selected'
  endif

  p_data += {
    'active' : active,
    'summary' : not_active_message,
  }
  programmer += {p_name : p_data}
  if active
    active_programmer_count += 1
  endif
endforeach

if active_programmer_count == 0
  error('At least one programmer must be selected')
endif

# add srcs, cargs & deps from active programmer to global srcs, cargs & deps
foreach p_name, p_data : programmer
  if p_data.get('active')
    srcs += p_data.get('srcs')
    cargs += p_data.get('flags')
    deps += p_data.get('deps')
  endif
endforeach

if config_print_wiki.enabled()
  if get_option('classic_cli').disabled()
    error('`classic_cli_print_wiki` can not be enabled without `classic_cli`')
  else
    srcs += files('print_wiki.c')
    cargs += '-DCONFIG_PRINT_WIKI=1'
  endif
endif

if config_default_programmer_name != ''
  cargs += '-DCONFIG_DEFAULT_PROGRAMMER_NAME=&programmer_' + config_default_programmer_name
else
  cargs += '-DCONFIG_DEFAULT_PROGRAMMER_NAME=NULL'
endif

cargs += '-DCONFIG_DEFAULT_PROGRAMMER_ARGS="' + config_default_programmer_args + '"'

if get_option('llvm_cov').enabled()
  cargs += ['-fprofile-instr-generate', '-fcoverage-mapping']
  link_args += ['-fprofile-instr-generate', '-fcoverage-mapping']
endif

install_headers([
    'include/libflashrom.h',
  ],
)

include_dir = include_directories('include')

mapfile = 'libflashrom.map'
if host_machine.system() == 'darwin'
  vflag = ''
else
  vflag = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), mapfile)
endif
libflashrom = library(
  'flashrom',
  sources : [
    srcs,
  ],
  include_directories : include_dir,
  soversion : lt_current,
  version : lt_version,
  dependencies : [
    deps,
  ],
  c_args : [
    cargs,
  ],
  install : true,
  link_args : link_args + [vflag],
  link_depends : mapfile,
)

pkgg = import('pkgconfig')
pkgg.generate(
  libraries : libflashrom,
  version : flashrom_version.split()[0], # cut off the git version
  name : 'flashrom',
  filebase : 'flashrom',
  description : 'library to interact with flashrom',
)

if get_option('classic_cli').enabled() or get_option('classic_cli').auto() and not get_option('default_library') == 'shared'

  cli_srcs = files(
    'cli_classic.c',
    'cli_common.c',
    'cli_output.c'
  )

  if not cc.has_function('getopt_long')
    cli_srcs += files('cli_getopt.c')
  endif

  classic_cli = executable(
    'flashrom',
    cli_srcs,
    c_args : cargs,
    include_directories : include_dir,
    install : true,
    install_dir : get_option('sbindir'),
    link_args : link_args,
    # flashrom needs internal symbols of libflashrom
    link_with : get_option('default_library') == 'static' ? libflashrom : libflashrom.get_static_lib(),
  )
  if get_option('llvm_cov').enabled()
    run_target('llvm-cov-cli', command : ['scripts/llvm-cov', classic_cli])
  endif
endif

if get_option('ich_descriptors_tool').auto() or get_option('ich_descriptors_tool').enabled()
  subdir('util/ich_descriptors_tool')
endif

if get_option('bash_completion').auto() or get_option('bash_completion').enabled()
  if get_option('classic_cli').disabled()
    if get_option('bash_completion').enabled()
      error('`bash_completion` can not be enabled without `classic_cli`')
    endif
  else
    bash_comp = dependency('bash-completion', required : false)
    if bash_comp.found()
      bash_comp_install_dir = bash_comp.get_variable(
        pkgconfig : 'completionsdir',
        pkgconfig_define : ['datadir', get_option('datadir')]
      )
    else
      bash_comp_install_dir = join_paths(get_option('datadir'), 'bash-completion', 'completions')
    endif

    programmer_names_active_str = ''
    foreach p_name, p_data : programmer
      if p_data.get('active')
        programmer_names_active_str += p_name + ' '
      endif
    endforeach

    configure_file(
      input : 'util/flashrom.bash-completion.tmpl',
      output : 'flashrom.bash',
      configuration : {
        'PROGRAMMERS' : programmer_names_active_str,
      },
      install: true,
      install_dir: bash_comp_install_dir,
    )
  endif
endif

# Use `.auto() or .enabled()` instead of `.allowed()` to keep the minimum meson version as low as possible.
# `.allowed()` gets introduced in 0.59.0
if get_option('tests').auto() or get_option('tests').enabled()
  # unit-test framework
  cmocka_dep = dependency(
    'cmocka',
    fallback: ['cmocka', 'cmocka_dep'],
    required : get_option('tests')
  )

  flashrom_test_dep = declare_dependency(
    include_directories : include_dir,
    sources : [
      srcs,
      'cli_common.c',
      'cli_output.c',
      'flashrom.c',
    ],
    compile_args : [
      '-includestdlib.h',
      '-includeunittest_env.h',
    ],
    dependencies : [
      deps,
    ],
  )

  if cmocka_dep.found()
    subdir('tests')
  endif
endif

programmer_names_active     = []
programmer_names_not_active = []
foreach p_name, p_data : programmer
  if p_data.get('active')
    programmer_names_active += p_name
  else
    programmer_names_not_active += p_name + ' (' + p_data.get('summary', '') + ')'
  endif
endforeach

summary({
  'active' : [programmer_names_active],
  'non active' : [programmer_names_not_active],
}, section : 'Programmer')
