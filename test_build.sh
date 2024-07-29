#!/usr/bin/env sh
set -e

# This script will only work on Linux with all dependencies installed.

is_scan_build_env=0

meson_programmer_opts="all auto group_ftdi group_i2c group_jlink group_pci group_serial group_usb	\
			atahpt atapromise atavia buspirate_spi ch341a_spi ch347_spi dediprog		\
			developerbox_spi digilent_spi dirtyjtag_spi drkaiser dummy ft2232_spi		\
			gfxnvidia internal it8212 jlink_spi linux_mtd linux_spi parade_lspcon		\
			mediatek_i2c_spi mstarddc_spi nic3com nicintel nicintel_eeprom nicintel_spi	\
			nicnatsemi nicrealtek ogp_spi pickit2_spi pony_spi raiden_debug_spi rayer_spi	\
			realtek_mst_i2c_spi satamv satasii serprog stlinkv3_spi usbblaster_spi asm106x"


if [ "$(basename "${CC}")" = "ccc-analyzer" ] || [ -n "${COVERITY_OUTPUT}" ]; then
	is_scan_build_env=1
fi


run_linter() {
	./util/lint/lint-extended-020-signed-off-by
}


build_meson () {
	build_dir=out
	meson_opts="-Dtests=enabled -Dman-pages=enabled -Ddocumentation=enabled"
	ninja_opts="-j $(nproc)"

	rm -rf ${build_dir}

	for programmer in ${meson_programmer_opts}; do
		programmer_dir="${build_dir}/${programmer}"

		# In case of clang analyzer we don't want to run it on
		# each programmer individually. Thus, just return here.
		if [ ${is_scan_build_env} -eq 1 ] && [ "${programmer}" != "all" ]; then
			return
		fi

		meson setup ${programmer_dir} ${meson_opts} -Dprogrammer=${programmer}
		ninja ${ninja_opts} -C ${programmer_dir}
		ninja ${ninja_opts} -C ${programmer_dir} test
	done
}


run_linter

build_meson
