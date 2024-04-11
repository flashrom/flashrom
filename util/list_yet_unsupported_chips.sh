#!/bin/sh

scriptname=$(readlink -f "$0") 2>/dev/null
path=$(dirname "$scriptname")/.. 2>/dev/null
if [ ! -e "$path/flashchips.c" -o ! -e "$path/include/flashchips.h" ]; then
	echo "Warning: could not calculate flashchips.[ch]'s directory. Trying current..."
	path="."
	if [ ! -e "$path/flashchips.c" -o ! -e "$path/include/flashchips.h" ]; then
		echo "Nope, sorry!"
		exit 1
	fi
fi

chips=$(sed -re '/#define [A-Z]/ !d' -e '/_ID\s/d' -e 's/\s*#define\s+([[:alnum:]_]+)\s+.*/\1/' "$path/include/flashchips.h")
for c in $chips ; do
	if ! grep "$c" "$path/flashchips.c" >/dev/null ; then
		if [ -n "$1" ]; then
			grep -o "$c.*" "$path/include/flashchips.h"
		else
			echo "$c"
		fi
	fi
done
