project('flashromutils', 'c',
  version : run_command('util/getversion.sh', '--version', check : true).stdout().strip(),
  license : 'GPL-2.0',
  meson_version : '>=0.50.0',
  default_options : [
    'warning_level=2',
    'c_std=c99',
    'werror=true',
    'optimization=s',
    'debug=false',
  ],
)

# libtool versioning
lt_current = '1'
lt_revision = '0'
lt_age = '0'
lt_version = '@0@.@1@.@2@'.format(lt_current, lt_age, lt_revision)

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

conf = configuration_data()

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments(warning_flags), language : 'c')
add_project_arguments('-D_DEFAULT_SOURCE', language : 'c')
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language : 'c') # required for fileno, nanosleep, and strndup
add_project_arguments('-D_BSD_SOURCE', language : 'c') # required for glibc < v2.19
add_project_arguments('-DFLASHROM_VERSION="' + meson.project_version() + '"', language : 'c')

# get defaults from configure
config_atahpt = get_option('config_atahpt')
config_atapromise = get_option('config_atapromise')
config_atavia = get_option('config_atavia')
config_buspirate_spi = get_option('config_buspirate_spi')
config_ch341a_spi = get_option('config_ch341a_spi')
config_dediprog = get_option('config_dediprog')
config_developerbox_spi = get_option('config_developerbox_spi')
config_digilent_spi = get_option('config_digilent_spi')
config_jlink_spi = get_option('config_jlink_spi')
config_drkaiser = get_option('config_drkaiser')
config_dummy = get_option('config_dummy')
config_ft2232_spi = get_option('config_ft2232_spi')
config_gfxnvidia = get_option('config_gfxnvidia')
config_raiden_debug_spi = get_option('config_raiden_debug_spi')
config_internal = get_option('config_internal')
config_it8212 = get_option('config_it8212')
config_linux_mtd = get_option('config_linux_mtd')
config_linux_spi = get_option('config_linux_spi')
config_mstarddc_spi = get_option('config_mstarddc_spi')
config_nic3com = get_option('config_nic3com')
config_nicintel_eeprom = get_option('config_nicintel_eeprom')
config_nicintel = get_option('config_nicintel')
config_nicintel_spi = get_option('config_nicintel_spi')
config_nicnatsemi = get_option('config_nicnatsemi')
config_nicrealtek = get_option('config_nicrealtek')
config_ogp_spi = get_option('config_ogp_spi')
config_pickit2_spi = get_option('config_pickit2_spi')
config_pony_spi = get_option('config_pony_spi')
config_rayer_spi = get_option('config_rayer_spi')
config_satamv = get_option('config_satamv')
config_satasii = get_option('config_satasii')
config_serprog = get_option('config_serprog')
config_usbblaster_spi = get_option('config_usbblaster_spi')
config_stlinkv3_spi = get_option('config_stlinkv3_spi')
config_parade_lspcon = get_option('config_parade_lspcon')
config_mediatek_i2c_spi = get_option('config_mediatek_i2c_spi')
config_realtek_mst_i2c_spi = get_option('config_realtek_mst_i2c_spi')
config_print_wiki= get_option('print_wiki')
config_default_programmer_name = get_option('default_programmer_name')
config_default_programmer_args = get_option('default_programmer_args')

cargs = []
deps = []
srcs = files(
  '82802ab.c',
  'at45db.c',
  'bitbang_spi.c',
  'edi.c',
  'en29lv640b.c',
  'flashchips.c',
  'flashrom.c',
  'fmap.c',
  'helpers.c',
  'helpers_fileio.c',
  'ich_descriptors.c',
  'jedec.c',
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

subdir('platform')

host_is_x86 = ['x86', 'x86_64'].contains(host_machine.cpu_family())

need_serial = [
  config_buspirate_spi, config_pony_spi, config_serprog,
].contains(true)
need_bitbang_spi = [
  config_internal, config_nicintel_spi, config_ogp_spi,
  config_pony_spi, config_rayer_spi,
].contains(true)
need_raw_mem_access = [
  config_atapromise, config_drkaiser, config_gfxnvidia, config_internal,
  config_it8212, config_nicintel, config_nicintel_eeprom, config_nicintel_spi,
  config_ogp_spi, config_satamv, config_satasii,
].contains(true)
# Internal programmer uses x86 features if the system is x86
need_x86_msr = config_internal and host_is_x86
need_x86_port_io = [
  config_atahpt, config_atapromise, config_internal and host_is_x86,
  config_nic3com, config_nicnatsemi, config_nicrealtek, config_rayer_spi,
  config_satamv,
].contains(true)
need_libpci = [
  config_atahpt, config_atapromise, config_atavia,
  config_drkaiser, config_gfxnvidia, config_internal, config_it8212,
  config_nic3com, config_nicintel, config_nicintel_eeprom, config_nicintel_spi,
  config_nicnatsemi, config_nicrealtek, config_ogp_spi, config_satamv,
  config_satasii,
].contains(true)
need_libusb1 = [
  config_ch341a_spi, config_dediprog, config_developerbox_spi,
  config_digilent_spi, config_pickit2_spi, config_raiden_debug_spi,
  config_stlinkv3_spi,
].contains(true)
need_libftdi1 = [
  config_ft2232_spi, config_usbblaster_spi,
].contains(true)
need_libjaylink = config_jlink_spi

if (need_x86_port_io or need_x86_msr) and not host_is_x86
  error('one or more enabled programmer only supports x86 and target is not')
endif


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

# some programmers require libusb
if get_option('usb')
  srcs += files('usbdev.c', 'usb_device.c')
  deps += dependency('libusb-1.0')
elif need_libusb1
  error('usb is disabled but one or more enabled programmer requires USB access')
endif

# some programmers require libpci
if get_option('pciutils')
  srcs += files('pcidev.c')
  deps += dependency('libpci')
elif need_libpci
  error('pciutils is disabled but one or more enabled programmer requires PCI access')
endif

if need_libftdi1
  deps += dependency('libftdi1')
endif

if need_libjaylink
  deps += dependency('libjaylink')
endif

# set defines for configured programmers
if config_atahpt
  srcs += files('atahpt.c')
  cargs += '-DCONFIG_ATAHPT=1'
endif
if config_atapromise
  srcs += files('atapromise.c')
  cargs += '-DCONFIG_ATAPROMISE=1'
endif
if config_atavia
  srcs += files('atavia.c')
  cargs += '-DCONFIG_ATAVIA=1'
endif
if config_buspirate_spi
  srcs += files('buspirate_spi.c')
  cargs += '-DCONFIG_BUSPIRATE_SPI=1'
endif
if config_ch341a_spi
  srcs += files('ch341a_spi.c')
  cargs += '-DCONFIG_CH341A_SPI=1'
endif
if config_dediprog
  srcs += files('dediprog.c')
  cargs += '-DCONFIG_DEDIPROG=1'
endif
if config_developerbox_spi
  srcs += files('developerbox_spi.c')
  cargs += '-DCONFIG_DEVELOPERBOX_SPI=1'
endif
if config_digilent_spi
  srcs += files('digilent_spi.c')
  cargs += '-DCONFIG_DIGILENT_SPI=1'
endif
if config_jlink_spi
  srcs += files('jlink_spi.c')
  cargs += '-DCONFIG_JLINK_SPI=1'
endif
if config_drkaiser
  srcs += files('drkaiser.c')
  cargs += '-DCONFIG_DRKAISER=1'
endif
if config_dummy
  srcs += files('dummyflasher.c')
  cargs += '-DCONFIG_DUMMY=1'
endif
if config_ft2232_spi
  srcs += files('ft2232_spi.c')
  cargs += '-DCONFIG_FT2232_SPI=1'
  cargs += '-DHAVE_FT232H=1'
endif
if config_gfxnvidia
  srcs += files('gfxnvidia.c')
  cargs += '-DCONFIG_GFXNVIDIA=1'
endif
if config_raiden_debug_spi
  srcs += files('raiden_debug_spi.c')
  cargs += '-DCONFIG_RAIDEN_DEBUG_SPI=1'
endif
if config_internal
  srcs += files(
    'board_enable.c',
    'cbtable.c',
    'chipset_enable.c',
    'internal.c',
    'processor_enable.c',
    'known_boards.c',
  )
  if host_is_x86
    srcs += files(
      'amd_imc.c',
      'dmi.c',
      'ichspi.c',
      'it87spi.c',
      'mcp6x_spi.c',
      'sb600spi.c',
      'wbsio_spi.c',
    )
  endif
  cargs += '-DCONFIG_INTERNAL=1'
  if get_option('config_internal_dmi')
    # Use internal DMI/SMBIOS decoder by default instead of relying on dmidecode.
    cargs += '-DCONFIG_INTERNAL_DMI=1'
  endif
endif
if config_it8212
  srcs += files('it8212.c')
  cargs += '-DCONFIG_IT8212=1'
endif
if config_linux_mtd
  srcs += files('linux_mtd.c')
  cargs += '-DCONFIG_LINUX_MTD=1'
endif
if config_linux_spi
  srcs += files('linux_spi.c')
  cargs += '-DCONFIG_LINUX_SPI=1'
endif
if config_mstarddc_spi
  srcs += files('mstarddc_spi.c')
  cargs += '-DCONFIG_MSTARDDC_SPI=1'
endif
if config_nic3com
  srcs += files('nic3com.c')
  cargs += '-DCONFIG_NIC3COM=1'
endif
if config_nicintel
  srcs += files('nicintel.c')
  cargs += '-DCONFIG_NICINTEL=1'
endif
if config_nicintel_eeprom
  srcs += files('nicintel_eeprom.c')
  cargs += '-DCONFIG_NICINTEL_EEPROM=1'
endif
if config_nicintel_spi
  srcs += files('nicintel_spi.c')
  cargs += '-DCONFIG_NICINTEL_SPI=1'
endif
if config_nicnatsemi
  srcs += files('nicnatsemi.c')
  cargs += '-DCONFIG_NICNATSEMI=1'
endif
if config_nicrealtek
  srcs += files('nicrealtek.c')
  cargs += '-DCONFIG_NICREALTEK=1'
endif
if config_ogp_spi
  srcs += files('ogp_spi.c')
  cargs += '-DCONFIG_OGP_SPI=1'
endif
if config_pickit2_spi
  srcs += files('pickit2_spi.c')
  cargs += '-DCONFIG_PICKIT2_SPI=1'
endif
if config_pony_spi
  srcs += files('pony_spi.c')
  cargs += '-DCONFIG_PONY_SPI=1'
endif
if config_rayer_spi
  srcs += files('rayer_spi.c')
  cargs += '-DCONFIG_RAYER_SPI=1'
endif
if config_satamv
  srcs += files('satamv.c')
  cargs += '-DCONFIG_SATAMV=1'
endif
if config_satasii
  srcs += files('satasii.c')
  cargs += '-DCONFIG_SATASII=1'
endif
if config_serprog
  srcs += files('serprog.c')
  cargs += '-DCONFIG_SERPROG=1'
endif
if config_usbblaster_spi
  srcs += files('usbblaster_spi.c')
  cargs += '-DCONFIG_USBBLASTER_SPI=1'
endif
if config_stlinkv3_spi
  srcs += files('stlinkv3_spi.c')
  cargs += '-DCONFIG_STLINKV3_SPI=1'
endif
if config_parade_lspcon
  srcs += files('parade_lspcon.c')
  cargs += '-DCONFIG_PARADE_LSPCON=1'
endif
if config_mediatek_i2c_spi
  srcs += files('mediatek_i2c_spi.c')
  cargs += '-DCONFIG_MEDIATEK_I2C_SPI=1'
endif
if config_realtek_mst_i2c_spi
  srcs += files('realtek_mst_i2c_spi.c')
  cargs += '-DCONFIG_REALTEK_MST_I2C_SPI=1'
endif

# bitbanging SPI infrastructure
if need_bitbang_spi
  srcs += files('bitbang_spi.c')
  cargs += '-DCONFIG_BITBANG_SPI=1'
endif

if host_machine.system() == 'linux'
  srcs += files('i2c_helper_linux.c')
endif

if need_raw_mem_access
  srcs += files('hwaccess_physmap.c')
endif

if need_x86_port_io
  srcs += files('hwaccess_x86_io.c')
  cargs += '-D__FLASHROM_HAVE_OUTB__=1'
endif

if need_x86_msr
  srcs += files('hwaccess_x86_msr.c')
endif

# raw serial IO
if need_serial
  srcs += files('serial.c')
  if host_machine.system() == 'linux'
    srcs += files('custom_baud_linux.c')
  else
    srcs += files('custom_baud.c')
  endif
endif

if config_print_wiki
  srcs += files('print_wiki.c')
  cargs += '-DCONFIG_PRINT_WIKI=1'
endif

if config_default_programmer_name != ''
  cargs += '-DCONFIG_DEFAULT_PROGRAMMER_NAME=&programmer_' + config_default_programmer_name
else
  cargs += '-DCONFIG_DEFAULT_PROGRAMMER_NAME=NULL'
endif

cargs += '-DCONFIG_DEFAULT_PROGRAMMER_ARGS="' + config_default_programmer_args + '"'

install_headers([
    'include/libflashrom.h',
  ],
)

include_dir = include_directories('include')

mapfile = 'libflashrom.map'
vflag = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), mapfile)
libflashrom = both_libraries(
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
  link_args : vflag,
  link_depends : mapfile,
)

version = meson.project_version()
#strip leading characters
if version.startswith('v')
  version = version.split('v')[1]
endif
if version.startswith('p')
  version = version.split('p')[1]
endif

pkgg = import('pkgconfig')
pkgg.generate(
  libraries : libflashrom,
  version : version,
  name : 'flashrom',
  filebase : 'flashrom',
  description : 'library to interact with flashrom',
)

conf.set('VERSION', version)
conf.set('MAN_DATE', run_command('util/getversion.sh', '--man-date', check : true).stdout().strip())
configure_file(
  input : 'flashrom.8.tmpl',
  output : 'flashrom.8',
  configuration : conf,
  install: true,
  install_dir: join_paths(get_option('mandir'), 'man8'),
)

executable(
  'flashrom',
  files(
    'cli_classic.c',
    'cli_common.c',
    'cli_output.c',
  ),
  c_args : cargs,
  include_directories : include_dir,
  install : true,
  install_dir : get_option('sbindir'),
  link_with : libflashrom.get_static_lib(), # flashrom needs internal symbols of libflashrom
)

if get_option('ich_descriptors_tool').auto() or get_option('ich_descriptors_tool').enabled()
  subdir('util/ich_descriptors_tool')
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
