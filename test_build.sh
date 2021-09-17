#!/usr/bin/env sh
set -e

make CONFIG_EVERYTHING=yes WARNERROR=yes


builddir=out
meson $builddir
ninja -C $builddir
ninja -C $builddir test
