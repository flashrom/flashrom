#!/usr/bin/env bash
set -e
cd /data-in/
meson setup /tmp/build -Ddocumentation=enabled -Dgenerate_authors_list=enabled
ninja -C /tmp/build doc/html
rm -rf /data-out/*
mv /tmp/build/doc/html/* /data-out/
rm -rf /tmp/build
