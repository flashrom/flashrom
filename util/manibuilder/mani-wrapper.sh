#!/bin/sh

cd /home/mani/flashrom/

if [ $# -eq 0 ]; then
	exec "${DEVSHELL}"
else
	exec "${DEVSHELL}" -c "$*"
fi
