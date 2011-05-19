#!/bin/sh
#
# Copyright (C) 2010 Google Inc.
# Written by David Hendricks for Google Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
#
# This script attempts to test Flashrom partial write capability by writing
# patterns of 0xff and 0x00 bytes to the lowest 128kB of flash. 128kB is chosen
# since 64kB is usually the largest possible block size, so we will try to
# cover at least two blocks with this test.

EXIT_SUCCESS=0
EXIT_FAILURE=1

# The copy of flashrom to test. If unset, we'll assume the user wants to test
# a newly built flashrom binary in the parent directory (this script should
# reside in flashrom/util).
if [ -z "$FLASHROM" ] ; then
	FLASHROM="../flashrom"
fi
echo "testing flashrom binary: ${FLASHROM}"

OLDDIR=$(pwd)

# test data location
TMPDIR=$(mktemp -d -t flashrom_test.XXXXXXXXXX)
if [ "$?" != "0" ] ; then
	echo "Could not create temporary directory"
	exit $EXIT_FAILURE
fi

ZERO_4K="00_4k.bin"
FF_4K="ff_4k.bin"
FF_4K_TEXT="ff_4k.txt"

TESTFILE="test.bin"

which uuencode > /dev/null
if [ "$?" != "0" ] ; then
	echo "uuencode is required to use this script"
	exit $EXIT_FAILURE
fi

which flashrom > /dev/null
if [ "$?" != "0" ] ; then
	echo "Please install a stable version of flashrom in your path."
	echo "This will be used to compare the test flashrom binary and "
	echo "restore your firmware image at the end of the test."
	exit $EXIT_FAILURE
fi

# Keep a copy of flashrom with the test data
cp "$FLASHROM" "${TMPDIR}/"
cd "$TMPDIR"
echo "Running test in ${TMPDIR}"

# Make 4k worth of 0xff bytes
echo "begin 640 $FF_4K" > "$FF_4K_TEXT"
i=0
while [ $i -le 90 ] ; do
	echo "M____________________________________________________________" >> "$FF_4K_TEXT"
	i=$((${i} + 1))
done
echo "!_P``" >> "$FF_4K_TEXT"
echo "\`" >> "$FF_4K_TEXT"
echo "end" >> "$FF_4K_TEXT"
uudecode -o "$FF_4K" "$FF_4K_TEXT"
rm -f "$FF_4K_TEXT"

# Make 4k worth of 0x00 bytes
dd if=/dev/zero of="$ZERO_4K" bs=1 count=4096 2> /dev/null
echo "ffh pattern written in ${FF_4K}"
echo "00h pattern written in ${ZERO_4K}"

echo "Reading BIOS image"
BIOS="bios.bin"
flashrom ${FLASHROM_PARAM} -r "$BIOS" > /dev/null
echo "Original image saved as ${BIOS}"

# $1: exit code
do_exit() {
	if [ ${1} -eq ${EXIT_FAILURE} ] ; then
		echo "Result: FAILED"
	else
		echo "Result: PASSED"
	fi

	echo "restoring original bios image using system's flashrom"
	flashrom ${FLASHROM_PARAM} -w "$BIOS"
	echo "test files remain in ${TMPDIR}"
	cd "$OLDDIR"
	exit "$1"
}

#
# Actual tests are performed below.
#
NUM_REGIONS=16

# Make a layout - 4K regions on 4K boundaries. This will test basic
# functionality of erasing and writing specific blocks.
echo "
0x000000:0x000fff 00_0
0x001000:0x001fff ff_0

0x002000:0x002fff 00_1
0x003000:0x003fff ff_1

0x004000:0x004fff 00_2
0x005000:0x005fff ff_2

0x006000:0x006fff 00_3
0x007000:0x007fff ff_3

0x008000:0x008fff 00_4
0x009000:0x009fff ff_4

0x00a000:0x00afff 00_5
0x00b000:0x00bfff ff_5

0x00c000:0x00cfff 00_6
0x00d000:0x00dfff ff_6

0x00e000:0x00efff 00_7
0x00f000:0x00ffff ff_7

0x010000:0x010fff 00_8
0x011000:0x011fff ff_8

0x012000:0x012fff 00_9
0x013000:0x013fff ff_9

0x014000:0x014fff 00_10
0x015000:0x015fff ff_10

0x016000:0x016fff 00_11
0x017000:0x017fff ff_11

0x018000:0x018fff 00_12
0x019000:0x019fff ff_12

0x01a000:0x01afff 00_13
0x01b000:0x01bfff ff_13

0x01c000:0x01cfff 00_14
0x01d000:0x01dfff ff_14

0x01e000:0x01efff 00_15
0x01f000:0x01ffff ff_15
" > layout_4k_aligned.txt

cp "$BIOS" "$TESTFILE"
i=0
while [ $i -lt $NUM_REGIONS ] ; do
	echo -n "aligned region ${i} test: "
	offset=$((${i} * 8192))
	dd if=${ZERO_4K} of=${TESTFILE} bs=1 conv=notrunc seek=${offset} 2> /dev/null
	dd if=${FF_4K} of=${TESTFILE} bs=1 conv=notrunc seek=$((${offset} + 4096)) 2> /dev/null

	./flashrom ${FLASHROM_PARAM} -l layout_4k_aligned.txt -i 00_${i} -i ff_${i} -w "$TESTFILE" > /dev/null
	if [ "$?" != "0" ] ; then
		echo "failed to flash region"
		do_exit "$EXIT_FAILURE"
	fi

	# download the entire ROM image and use diff to compare to ensure
	# flashrom logic does not violate user-specified regions
	flashrom ${FLASHROM_PARAM} -r difftest.bin > /dev/null
	diff -q difftest.bin "$TESTFILE"
	if [ "$?" != "0" ] ; then
		echo "failed diff test"
		do_exit "$EXIT_FAILURE"
	fi
	rm -f difftest.bin

	i=$((${i} + 1))
	echo "passed"
done

# Make a layout - 4K regions on 4.5K boundaries. This will help find problems
# with logic that only operates on part of a block. For example, if a user
# wishes to re-write a fraction of a block, then:
# 1. The whole block must be erased.
# 2. The old content must be restored at unspecified offsets.
# 3. The new content must be written at specified offsets.
#
# Note: The last chunk of 0xff bytes is only 2k as to avoid overrunning a 128kB
# test image.
#
echo "
0x000800:0x0017ff 00_0
0x001800:0x0027ff ff_0

0x002800:0x0037ff 00_1
0x003800:0x0047ff ff_1

0x004800:0x0057ff 00_2
0x005800:0x0067ff ff_2

0x006800:0x0077ff 00_3
0x007800:0x0087ff ff_3

0x008800:0x0097ff 00_4
0x009800:0x00a7ff ff_4

0x00a800:0x00b7ff 00_5
0x00b800:0x00c7ff ff_5

0x00c800:0x00d7ff 00_6
0x00d800:0x00e7ff ff_6

0x00e800:0x00f7ff 00_7
0x00f800:0x0107ff ff_7

0x010800:0x0117ff 00_8
0x011800:0x0127ff ff_8

0x012800:0x0137ff 00_9
0x013800:0x0147ff ff_9

0x014800:0x0157ff 00_10
0x015800:0x0167ff ff_10

0x016800:0x0177ff 00_11
0x017800:0x0187ff ff_11

0x018800:0x0197ff 00_12
0x019800:0x01a7ff ff_12

0x01a800:0x01b7ff 00_13
0x01b800:0x01c7ff ff_13

0x01c800:0x01d7ff 00_14
0x01d800:0x01e7ff ff_14

0x01e800:0x01f7ff 00_15
0x01f800:0x01ffff ff_15
" > layout_unaligned.txt

# reset the test file and ROM to the original state
flashrom ${FLASHROM_PARAM} -w "$BIOS" > /dev/null
cp "$BIOS" "$TESTFILE"

i=0
while [ $i -lt $NUM_REGIONS ] ; do
	echo -n "unaligned region ${i} test: "

	offset=$(($((${i} * 8192)) + 2048))
	# Protect against too long write
	writelen=4096
	if [ $((${offset} + 4096 + 4096)) -ge 131072 ]; then
		writelen=$((131072 - $((${offset} + 4096))))
		if [ ${writelen} -lt 0 ]; then
			writelen=0
		fi
	fi
	dd if=${ZERO_4K} of=${TESTFILE} bs=1 conv=notrunc seek=${offset} 2> /dev/null
	dd if=${FF_4K} of=${TESTFILE} bs=1 conv=notrunc seek=$((${offset} + 4096)) count=writelen 2> /dev/null

	./flashrom ${FLASHROM_PARAM} -l layout_unaligned.txt -i 00_${i} -i ff_${i} -w "$TESTFILE" > /dev/null
	if [ "$?" != "0" ] ; then
		echo "failed to flash region"
		do_exit "$EXIT_FAILURE"
	fi

	# download the entire ROM image and use diff to compare to ensure
	# flashrom logic does not violate user-specified regions
	flashrom ${FLASHROM_PARAM} -r difftest.bin > /dev/null
	diff -q difftest.bin "$TESTFILE"
	if [ "$?" != "0" ] ; then
		echo "failed diff test"
		do_exit "$EXIT_FAILURE"
	fi
	rm -f difftest.bin

	i=$((${i} + 1))
	echo "passed"
done

do_exit "$EXIT_SUCCESS"
