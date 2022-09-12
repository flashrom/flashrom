#!/usr/bin/env sh
set -e

# This script will only work on Linux with all dependencies installed.

is_scan_build_env=0

make_programmer_opts="INTERNAL INTERNAL_X86 SERPROG RAYER_SPI RAIDEN_DEBUG_SPI PONY_SPI NIC3COM		\
		      GFXNVIDIA SATASII ATAHPT ATAVIA ATAPROMISE FT2232_SPI USBBLASTER_SPI MSTARDDC_SPI	\
		      PICKIT2_SPI STLINKV3_SPI PARADE_LSPCON MEDIATEK_I2C_SPI REALTEK_MST_I2C_SPI DUMMY	\
		      DRKAISER NICREALTEK NICNATSEMI NICINTEL NICINTEL_SPI NICINTEL_EEPROM OGP_SPI	\
		      BUSPIRATE_SPI DEDIPROG DEVELOPERBOX_SPI SATAMV LINUX_MTD LINUX_SPI IT8212		\
		      CH341A_SPI DIGILENT_SPI JLINK_SPI"


if [ "$(basename "${CC}")" = "ccc-analyzer" ] || [ -n "${COVERITY_OUTPUT}" ]; then
	is_scan_build_env=1
fi


build_make () {
	make clean
	make CONFIG_EVERYTHING=yes

	# In case of clang analyzer we don't want to run it on
	# each programmer individually. Thus, just return here.
	if [ ${is_scan_build_env} -eq 1 ]; then
		return
	fi

	for option in ${make_programmer_opts}; do
		echo "Building ${option}"
		make clean
		make CONFIG_NOTHING=yes CONFIG_${option}=yes
	done
}


build_meson () {
	build_dir=out

	rm -rf ${build_dir}

	meson $build_dir -Dtests=enabled
	ninja -C $build_dir
	ninja -C $build_dir test
}


build_make
build_meson
