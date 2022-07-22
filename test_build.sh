#!/usr/bin/env sh
set -e


build_make () {
	make CONFIG_EVERYTHING=yes
}


build_meson () {
	builddir=out
	meson $builddir
	ninja -C $builddir
	ninja -C $builddir test
}


build_make
build_meson
