project('flashromutils', 'c',
  version : '1.0',
  license : 'GPL-2.0+',
  meson_version : '>=0.47.0',
  default_options : ['warning_level=2', 'c_std=c99'],
)

# libtool versioning
lt_current = '1'
lt_revision = '0'
lt_age = '0'
lt_version = '@0@.@1@.@2@'.format(lt_current, lt_age, lt_revision)

# hide some warnings
warning_flags = [
  '-Wno-unused-parameter',
  '-Wno-sign-compare',
  '-Wno-address-of-packed-member',
  '-Wno-enum-conversion',
  '-Wno-missing-field-initializers',
  '-Wno-missing-braces',
]

conf = configuration_data()

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments(warning_flags), language : 'c')
add_project_arguments('-D_DEFAULT_SOURCE', language : 'c')
add_project_arguments('-D_POSIX_C_SOURCE', language : 'c') # required for fileno
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
config_drkaiser = get_option('config_drkaiser')
config_dummy = get_option('config_dummy')
config_ft2232_spi = get_option('config_ft2232_spi')
config_gfxnvidia = get_option('config_gfxnvidia')
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

cargs = []
deps = []
srcs = []

need_libusb0 = false
need_raw_access = false
need_serial = false

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

# some programmers require libusb
if get_option('usb')
  srcs += 'usbdev.c'
  deps += dependency('libusb-1.0')
else
  config_ch341a_spi = false
  config_dediprog = false
  config_digilent_spi = false
  config_developerbox_spi = false
endif

# some programmers require libpci
if get_option('pciutils')
  srcs += 'pcidev.c'
  deps += dependency('libpci')
  cargs += '-DNEED_PCI=1'
else
  config_atahpt = false
  config_atapromise = false
  config_atavia = false
  config_drkaiser = false
  config_gfxnvidia = false
  config_internal = false
  config_it8212 = false
  config_nic3com = false
  config_nicintel_eeprom = false
  config_nicintel = false
  config_nicintel_spi = false
  config_nicnatsemi = false
  config_nicrealtek = false
  config_ogp_spi = false
  config_rayer_spi = false
  config_satamv = false
  config_satasii = false
endif

# set defines for configured programmers
if config_atahpt
  srcs += 'atahpt.c'
  cargs += '-DCONFIG_ATAHPT=1'
endif
if config_atapromise
  srcs += 'atapromise.c'
  cargs += '-DCONFIG_ATAPROMISE=1'
endif
if config_atavia
  srcs += 'atavia.c'
  cargs += '-DCONFIG_ATAVIA=1'
endif
if config_buspirate_spi
  srcs += 'buspirate_spi.c'
  cargs += '-DCONFIG_BUSPIRATE_SPI=1'
  need_serial = true
endif
if config_ch341a_spi
  srcs += 'ch341a_spi.c'
  cargs += '-DCONFIG_CH341A_SPI=1'
endif
if config_dediprog
  srcs += 'dediprog.c'
  cargs += '-DCONFIG_DEDIPROG=1'
endif
if config_developerbox_spi
  srcs += 'developerbox_spi.c'
  cargs += '-DCONFIG_DEVELOPERBOX_SPI=1'
endif
if config_digilent_spi
  srcs += 'digilent_spi.c'
  cargs += '-DCONFIG_DIGILENT_SPI=1'
endif
if config_drkaiser
  srcs += 'drkaiser.c'
  cargs += '-DCONFIG_DRKAISER=1'
endif
if config_dummy
  srcs += 'dummyflasher.c'
  cargs += '-DCONFIG_DUMMY=1'
endif
if config_ft2232_spi
  srcs += 'ft2232_spi.c'
  cargs += '-DCONFIG_FT2232_SPI=1'
  deps += dependency('libftdi1')
  cargs += '-DHAVE_FT232H=1'
endif
if config_gfxnvidia
  srcs += 'gfxnvidia.c'
  cargs += '-DCONFIG_GFXNVIDIA=1'
endif
if config_internal
  srcs += 'board_enable.c'
  srcs += 'cbtable.c'
  srcs += 'chipset_enable.c'
  srcs += 'internal.c'
  srcs += 'processor_enable.c'
  if target_machine.cpu_family() == 'x86' or target_machine.cpu_family() == 'x86_64'
    srcs += 'amd_imc.c'
    srcs += 'dmi.c'
    srcs += 'ichspi.c'
    srcs += 'it85spi.c'
    srcs += 'it87spi.c'
    srcs += 'mcp6x_spi.c'
    srcs += 'sb600spi.c'
    srcs += 'wbsio_spi.c'
  endif
  config_bitbang_spi = true
  cargs += '-DCONFIG_INTERNAL=1'
  if get_option('config_internal_dmi')
    # Use internal DMI/SMBIOS decoder by default instead of relying on dmidecode.
    cargs += '-DCONFIG_INTERNAL_DMI=1'
  endif
endif
if config_it8212
  srcs += 'it8212.c'
  cargs += '-DCONFIG_IT8212=1'
endif
if config_linux_mtd
  srcs += 'linux_mtd.c'
  cargs += '-DCONFIG_LINUX_MTD=1'
endif
if config_linux_spi
  srcs += 'linux_spi.c'
  cargs += '-DCONFIG_LINUX_SPI=1'
endif
if config_mstarddc_spi
  srcs += 'mstarddc_spi.c'
  cargs += '-DCONFIG_MSTARDDC_SPI=1'
endif
if config_nic3com
  srcs += 'nic3com.c'
  cargs += '-DCONFIG_NIC3COM=1'
endif
if config_nicintel
  srcs += 'nicintel.c'
  cargs += '-DCONFIG_NICINTEL=1'
endif
if config_nicintel_eeprom
  srcs += 'nicintel_eeprom.c'
  cargs += '-DCONFIG_NICINTEL_EEPROM=1'
endif
if config_nicintel_spi
  srcs += 'nicintel_spi.c'
  config_bitbang_spi = true
  cargs += '-DCONFIG_NICINTEL_SPI=1'
endif
if config_nicnatsemi
  srcs += 'nicnatsemi.c'
  cargs += '-DCONFIG_NICNATSEMI=1'
endif
if config_nicrealtek
  srcs += 'nicrealtek.c'
  cargs += '-DCONFIG_NICREALTEK=1'
endif
if config_ogp_spi
  config_bitbang_spi = true
  srcs += 'ogp_spi.c'
  cargs += '-DCONFIG_OGP_SPI=1'
endif
if config_pickit2_spi
  srcs += 'pickit2_spi.c'
  cargs += '-DCONFIG_PICKIT2_SPI=1'
  need_libusb0 = true
endif
if config_pony_spi
  srcs += 'pony_spi.c'
  need_serial = true
  config_bitbang_spi = true
  cargs += '-DCONFIG_PONY_SPI=1'
endif
if config_rayer_spi
  srcs += 'rayer_spi.c'
  config_bitbang_spi = true
  need_raw_access = true
  cargs += '-DCONFIG_RAYER_SPI=1'
endif
if config_satamv
  srcs += 'satamv.c'
  cargs += '-DCONFIG_SATAMV=1'
endif
if config_satasii
  srcs += 'satasii.c'
  cargs += '-DCONFIG_SATASII=1'
endif
if config_serprog
  srcs += 'serprog.c'
  cargs += '-DCONFIG_SERPROG=1'
  need_serial = true
endif
if config_usbblaster_spi
  srcs += 'usbblaster_spi.c'
  cargs += '-DCONFIG_USBBLASTER_SPI=1'
endif

# bitbanging SPI infrastructure
if config_bitbang_spi
  srcs += 'bitbang_spi.c'
  cargs += '-DCONFIG_BITBANG_SPI=1'
endif

# raw memory, MSR or PCI port I/O access
if need_raw_access
  srcs += 'hwaccess.c'
  srcs += 'physmap.c'
  cargs += '-DNEED_RAW_ACCESS=1'
endif

# raw serial IO
if need_serial
  srcs += 'custom_baud.c'
  srcs += 'serial.c'
endif

# raw deprecated and old USB library
if need_libusb0
  deps += dependency('libusb')
endif

prefix = get_option('prefix')
sbindir = join_paths(prefix, get_option('sbindir'))
libdir = join_paths(prefix, get_option('libdir'))

install_headers([
    'libflashrom.h',
  ],
)

# core modules needed by both the library and the CLI
srcs += '82802ab.c'
srcs += 'at45db.c'
srcs += 'edi.c'
srcs += 'en29lv640b.c'
srcs += 'flashchips.c'
srcs += 'flashrom.c'
srcs += 'fmap.c'
srcs += 'helpers.c'
srcs += 'ich_descriptors.c'
srcs += 'jedec.c'
srcs += 'layout.c'
srcs += 'libflashrom.c'
srcs += 'opaque.c'
srcs += 'print.c'
srcs += 'programmer.c'
srcs += 'sfdp.c'
srcs += 'spi25.c'
srcs += 'spi25_statusreg.c'
srcs += 'spi.c'
srcs += 'sst28sf040.c'
srcs += 'sst49lfxxxc.c'
srcs += 'sst_fwhub.c'
srcs += 'stm50.c'
srcs += 'udelay.c'
srcs += 'w29ee011.c'
srcs += 'w39.c'

mapfile = 'libflashrom.map'
vflag = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), mapfile)
flashrom = shared_library(
  'flashrom',
  sources : [
    srcs,
  ],
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

pkgg = import('pkgconfig')
pkgg.generate(
  libraries : flashrom,
  version : meson.project_version(),
  name : 'libflashrom',
  filebase : 'libflashrom',
  description : 'libflashrom',
)

flashrom_dep = declare_dependency(
  link_with : flashrom,
  include_directories : include_directories('.'),
  dependencies : deps
)

# we can't just link_with libflashrom as we require all the internal symbols...
executable(
  'flashrom',
  sources : [
    srcs,
    'cli_classic.c',
    'cli_common.c',
    'cli_output.c',
    'flashrom.c',
  ],
  dependencies : [
    deps,
  ],
  c_args : [
    cargs,
    '-DSTANDALONE',
    '-DCONFIG_DEFAULT_PROGRAMMER=PROGRAMMER_INVALID',
    '-DCONFIG_DEFAULT_PROGRAMMER_ARGS=""',
  ],
  install : true,
  install_dir : sbindir,
)

subdir('util')
