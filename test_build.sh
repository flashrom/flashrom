#!/usr/bin/env sh
set -e

make CONFIG_EVERYTHING=yes WARNERROR=yes

meson out
(cd out && ninja)
(cd out && ninja test)
