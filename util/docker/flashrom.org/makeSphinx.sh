#!/usr/bin/env bash

if [ "$1" == "livehtml" ]; then
  echo "Starting live documentation build"
  cd /data-in/ && sphinx-autobuild -b html doc /tmp/build/html
else
  echo "Starting production documentation build"
  cd /data-in/ \
    && sphinx-build -b html doc /tmp/build/html \
    && rm -rf /data-out/* \
    && mv /tmp/build/html/* /data-out/
fi
