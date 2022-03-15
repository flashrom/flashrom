#!/bin/sh
#
# Copyright (C) 2016 Google Inc.
# Copyright (C) 2020 Facebook Inc.
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

EXIT_SUCCESS=0
EXIT_FAILURE=1
RC=$EXIT_SUCCESS
FATAL=0
NONFATAL=1

#
# Stuff obtained from command-line
#

# Generic options
BACKUP_IMAGE=""
OLD_FLASHROM=""
NEW_FLASHROM=""
NO_CLEAN=0
SKIP_CONSISTENCY_CHECK=0
UPLOAD_RESULTS=0

# LOCAL_FLASHROM is required if both a secondary programmer *and* a remote host
# are used since OLD_FLASHROM and NEW_FLASHROM will be called on the remote
# host and we need a local copy of flashrom to control the secondary programmer.
# By default this will be set to the result of `which flashrom`.
LOCAL_FLASHROM=""

# Primary/Secondary programmer options
PRIMARY_OPTS=""
SECONDARY_OPTS=""

# Calls preflash_hook() and postflash_hook() before and after doing a command.
CUSTOM_HOOKS_FILENAME=""

# logfile to store the script's output
SCRIPT_LOGFILE="flashrom-test_script_output.txt"

# Test type
TEST_TYPE_UNKNOWN=0
TEST_TYPE_SINGLE=1
TEST_TYPE_ENDURANCE=2
TEST_TYPE=$TEST_TYPE_UNKNOWN

# Region modes
REGION_MODE_UNKNOWN=0
REGION_MODE_CLOBBER=1
REGION_MODE_DESCRIPTOR=2
REGION_MODE_FLASHMAP=3
REGION_MODE_LAYOUT=4
REGION_MODE=$REGION_MODE_UNKNOWN
DESCRIPTOR_REGION="BIOS"
FLASHMAP_REGION="RW_SECTION_A"
LAYOUT_FILE=""
LAYOUT_REGION="RW"
SMALL_REGION=0

# Remote testing options
SSH_PORT=""
REMOTE_HOST=""
REMOTE_PORT_OPTION=""
REMOTE_ONLY=0
REMOTE_PROGRAMMER_PARAMS=""
SSH_CMD="ssh $REMOTE_PORT_OPTION root@${REMOTE_HOST} command -v"

LOCAL=0
REMOTE=1
DO_REMOTE=0	# boolean to use for cmd() and tests

# relative path from flashrom root directory
TEST_SCRIPT_PATH="util/testing/test_v2.1.sh"

# In case we need to run flashrom locally and we're not already root.
SUDO_CMD=""
if [ "$(id -u)" -ne "0" ]; then
	SUDO_CMD="sudo"
fi

# 1KB
K=1024

show_help() {
	printf "Usage:
	${0} <options>

General options:
    -b, --backup-image <path>
        Backup image to write unconditionally at end of testing.
    -h, --help
        Show this message.
    -l, --layout-file <path>
        Layout file (required if mode is \"layout\", resides locally).
    -m, --mode <arg>
        Region access mode (clobber, descriptor, flashmap, layout).
    -n, --new <path>
        Path to new version of flashrom.
    -o, --old <path>
        Path to old (stable) version of flashrom.
    -p, --primary-programmer <parameters>
        Primary programmer options.
    -r, --remote-host <host>
        Remote host to test primary programmer on.
    -s, --secondary-programmer <parameters>
        Secondary programmer options.
    -t, --type <arg>
        Test type (single, endurance).
    -u, --upload-results
        Upload results to flashrom.org.
    -v, --voltage
        Chip voltage in millivolts (usually 1800 or 3300).

Long options:
    --custom-hooks <filename>
        Supply a script with custom hooks to run before and after commands.
    --descriptor-region <name>
        Specify region to use in descriptor mode (default: $DESCRIPTOR_REGION)
    --flashmap-region <name>
        Specify region to use in flashmap mode (default: $FLASHMAP_REGION)
    --layout-region <name>
        Specify region to use in layout mode (default: $LAYOUT_REGION)
    --local-flashrom <path>
        Path to local version of flashrom when using both a secondary programmer
        and remote host (default: $($SUDO_CMD which flashrom))
    --no-clean
        Do not remove temporary files.
    --skip-consistency-check
        Skip the consistency check (two consecutive reads) at beginning.
    --small-region
        Omit tests that require large amounts of space (>16KB).
Remote connectivity options:
    --ssh-port <port>
        Use a specific SSH port.

See documentation for usage examples (TODO: Migrate https://goo.gl/3jNoL7
to flashrom wiki).\n
"
}

getopt -T
if [ $? -ne 4 ]; then
	printf "GNU-compatible getopt(1) required.\n"
	exit $EXIT_FAILURE
fi

LONGOPTS="backup-image:,help,,new:,old:,remote-host:,upload-results:"
LONGOPTS="${LONGOPTS},primary-programmer:,secondary-programmer:,local-flashrom:"
LONGOPTS="${LONGOPTS},custom-hooks:,mode:,skip-consistency-check,small-region"
LONGOPTS="${LONGOPTS},type:,voltage:"
LONGOPTS="${LONGOPTS},layout-file:,descriptor-region:,flashmap-region:,layout-region:"
LONGOPTS="${LONGOPTS},no-clean"
LONGOPTS="${LONGOPTS},ssh-port:"

ARGS=$(getopt -o b:hl:m:n:o:p:r:s:t:u -l "$LONGOPTS" -n "$0" -- "$@");
if [ $? != 0 ] ; then printf "Terminating...\n" >&2 ; exit 1 ; fi
eval set -- "$ARGS"
while true ; do
	case "$1" in
		# Generic options
		-b|--backup-image)
			shift
			BACKUP_IMAGE="$1"
			;;
		-h|--help)
			show_help
			exit $EXIT_SUCCESS
			;;
		-l|--layout-file)
			shift
			LAYOUT_FILE="$1"
			;;
		-m|--mode)
			shift
			if [ "$1" = "clobber" ]; then
				REGION_MODE=$REGION_MODE_CLOBBER
			elif [ "$1" = "descriptor" ]; then
				REGION_MODE=$REGION_MODE_DESCRIPTOR
			elif [ "$1" = "flashmap" ]; then
				REGION_MODE=$REGION_MODE_FLASHMAP
			elif [ "$1" = "layout" ]; then
				REGION_MODE=$REGION_MODE_LAYOUT
			else
				printf "Unknown mode: $1\n"
				exit $EXIT_FAILURE
			fi
			;;
		-n|--new)
			shift
			NEW_FLASHROM="$1"
			;;
		-o|--old)
			shift
			OLD_FLASHROM="$1"
			;;
		-p|--primary_programmer)
			shift
			PRIMARY_OPTS="-p $1"
			;;
		-s|--secondary_programmer)
			shift
			SECONDARY_OPTS="-p $1"
			;;
		-t|--type)
			shift
			if [ "$1" = "single" ]; then
				TEST_TYPE=$TEST_TYPE_SINGLE
			elif [ "$1" = "endurance" ]; then
				TEST_TYPE=$TEST_TYPE_ENDURANCE
			else
				printf "Unknown type: $1\n"
				exit $EXIT_FAILURE
			fi
			;;
		-r|--remote-host)
			DO_REMOTE=1
			shift
			REMOTE_HOST="$1"
			;;
		-u|--upload-results)
			UPLOAD_RESULTS=1
			;;
		-v|--voltage)
			shift
			VOLTAGE="$1"
			;;

		# Longopts only
		--custom-hooks)
			shift
			CUSTOM_HOOKS_FILENAME="$1"
			;;
		--descriptor-region)
			shift
			DESCRIPTOR_REGION="$1"
			;;
		--flashmap-region)
			shift
			FLASHMAP_REGION="$1"
			;;
		--layout-region)
			shift
			LAYOUT_REGION="$1"
			;;
		--local-flashrom)
			shift
			LOCAL_FLASHROM="$1"
			;;
		--no-clean)
			NO_CLEAN=1
			;;
		--skip-consistency-check)
			SKIP_CONSISTENCY_CHECK=1
			;;
		--small-region)
			SMALL_REGION=1
			;;

		# Remote testing options
		--ssh-port)
			shift
			REMOTE_PORT_OPTION="-p $1"
			;;

		# error handling
		--)
			shift
			if [ -n "$*" ]; then
				printf "Non-option parameters detected: '$*'\n"
				exit $EXIT_FAILURE
			fi
			break
			;;
		*)
			printf "error processing options at '$1'\n"
			exit $EXIT_FAILURE
	esac
	shift
done

# TODO: Implement this.
if [ $UPLOAD_RESULTS -eq 1 ]; then
	printf "TODO: Implement ability to upload results.\n"
	exit $EXIT_FAILURE
fi

#
# Source helper scripts
#
export REMOTE_HOST REMOTE_PORT_OPTION
export LOCAL REMOTE FATAL NONFATAL EXIT_SUCCESS EXIT_FAILURE
export CUSTOM_HOOKS_FILENAME SUDO_CMD VOLTAGE
. "$(pwd)/util/testing/cmd.sh"

# We will set up a logs directory within the tmpdirs to store
# all output logs.
LOGS="logs"

# Setup temporary working directories:
# LOCAL_TMPDIR:  Working directory on local host.
# REMOTE_TMPDIR: Working directory on remote host.
# TMPDIR:        The temporary directory in which we do most of the work. This is
#                convenient for commands that depend on $DO_REMOTE.
LOCAL_TMPDIR=$(mktemp -d --tmpdir flashrom_test.XXXXXXXX)
if [ $? -ne 0 ] ; then
	printf "Could not create temporary directory\n"
	exit $EXIT_FAILURE
fi
mkdir "${LOCAL_TMPDIR}/${LOGS}"

if [ $DO_REMOTE -eq 1 ]; then
	REMOTE_TMPDIR=$(ssh root@${REMOTE_HOST} mktemp -d --tmpdir flashrom_test.XXXXXXXX)
	if [ $? -ne 0 ] ; then
		printf "Could not create temporary directory\n"
		exit $EXIT_FAILURE
	fi
	scmd $REMOTE "mkdir ${REMOTE_TMPDIR}/${LOGS}"
fi

if [ $DO_REMOTE -eq 0 ]; then
	TMPDIR="$LOCAL_TMPDIR"
else
	TMPDIR="$REMOTE_TMPDIR"
fi

#
# Test command-line validity.
#
if [ $TEST_TYPE -eq $TEST_TYPE_UNKNOWN ]; then
	printf "Must specify a test type (-t/--type).\n"
	exit $EXIT_FAILURE
elif [ $TEST_TYPE -eq $TEST_TYPE_SINGLE ]; then
	if [ $REGION_MODE -eq $REGION_MODE_UNKNOWN ]; then
		printf "Must specify a region access mode (-m/--mode).\n"
		exit $EXIT_FAILURE
	elif [ $REGION_MODE -eq $REGION_MODE_LAYOUT ]; then
		if [ -z "$LAYOUT_FILE" ]; then
			printf "Must specify a layout file when using layout mode.\n"
			exit $EXIT_FAILURE
		fi

		scmd $DO_REMOTE "stat $LAYOUT_FILE"
		if [ $? -ne 0 ]; then
			if [ $DO_REMOTE -eq 1 ]; then
				tmp=" on remote host $REMOTE_HOST."
			else
				tmp=" on local host."
			fi
			printf "Layout file $LAYOUT_FILE not found${tmp}\n"
			exit $EXIT_FAILURE
		fi

		if [ $DO_REMOTE -eq 1 ]; then
			scp root@"${REMOTE_HOST}:$LAYOUT_FILE" "${LOCAL_TMPDIR}/" 2>&1 >/dev/null
		else
			cp "$LAYOUT_FILE" "${LOCAL_TMPDIR}/"
		fi
	fi
fi

if [ -n "$VOLTAGE" ]; then
	echo "$VOLTAGE" | grep -q '[^0-9]'
	if [ $? -ne 1 ]; then
		printf "Voltage must be an integer with units of millivolts."
		exit $EXIT_FAILURE
	fi
fi

if [ $DO_REMOTE -eq 1 ]; then
	# Test connection to remote host
	test_cmd $DO_REMOTE "ls /" $NONFATAL
	if [ $? -ne 0 ]; then
		printf "Could not connect to remote host ${REMOTE_HOST}\n"
		exit $EXIT_FAILURE
	fi
fi

# Remote host and secondary programmer are in use, so either the user must
# specify a local version of flashrom to control the secondary programmer
# or it must be found in the default path.
if [ $DO_REMOTE -eq 1 ] && [ -n "$SECONDARY_OPTS" ]; then
	if [ -z "$LOCAL_FLASHROM" ]; then
		LOCAL_FLASHROM="$($SUDO_CMD which flashrom)"
	fi

	if [ ! -e "$LOCAL_FLASHROM" ]; then
		printf "$LOCAL_FLASHROM does not exist\n"
		exit $EXIT_FAILURE
	fi
fi

#
# Dependencies
#

# cmp is used to compare files
test_cmd $DO_REMOTE "cmp" $FATAL

if [ ! -e "/dev/urandom" ]; then
	printf "This script uses /dev/urandom\n"
	exit $EXIT_FAILURE
fi

if [ ! -e "/dev/zero" ]; then
	printf "This script uses /dev/zero\n"
	exit $EXIT_FAILURE
fi

#
# Setup.
#

# Naive path check
if [ ! -e "$TEST_SCRIPT_PATH" ] ; then
	printf "Script must be run from root of flashrom directory\n"
	exit $EXIT_FAILURE
fi

if [ -z "$OLD_FLASHROM" ]; then
	if [ $DO_REMOTE -eq 1 ]; then
		OLD_FLASHROM="$(ssh root@${REMOTE_HOST} which flashrom)"
	else
		OLD_FLASHROM="$($SUDO_CMD which flashrom)"
	fi
fi
test_cmd $DO_REMOTE "$OLD_FLASHROM --help" $NONFATAL
if [ $? -ne 0 ]; then
	printf "Old flashrom binary is not usable.\n"
	exit $EXIT_FAILURE
fi

# print $1 and store it in the script log file
print_and_log()
{
	printf "$1" | tee -a "${LOCAL_TMPDIR}/${LOGS}/${SCRIPT_LOGFILE}"
}

# Copy files from local tmpdir to remote host tmpdir
copy_to_remote()
{
	for F in $@; do
		scp "${LOCAL_TMPDIR}/${F}" root@"${REMOTE_HOST}:${REMOTE_TMPDIR}" 2>&1 >/dev/null
	done
}

# Copy files from remote host tmpdir to local tmpdir
copy_from_remote()
{
	for F in $@; do
		scp root@"${REMOTE_HOST}:${REMOTE_TMPDIR}/${F}" "${LOCAL_TMPDIR}/" 2>&1 >/dev/null
	done
}

# A wrapper for scmd calls to flashrom when we want to log the output
# $1: 0 ($LOCAL) to run command locally,
#     1 ($REMOTE) to run remotely if remote host defined
# $2: arguments to be passed into scmd
# $3: context of the flashrom call (to be used in the logfile)
flashrom_log_scmd()
{
	local logfile="flashrom-${3}.txt"
	local rc

	if [ $1 -eq $REMOTE ]; then
		tmpdir=$REMOTE_TMPDIR
	else
		tmpdir=$LOCAL_TMPDIR
	fi

	scmd $1 "$2 -o ${tmpdir}/${LOGS}/${logfile}"; rc=$?
	# if the call was successful, we don't want to save the log (only save failure logs)
	if [ $rc -eq $EXIT_SUCCESS ]; then
		scmd $1 "rm -f ${tmpdir}/${LOGS}/${logfile}"
	else
		# if the log was stored remotely, we want to copy it over to local tmpdir
		if [ $1 -eq $REMOTE ]; then
			scp root@"${REMOTE_HOST}:${REMOTE_TMPDIR}/${LOGS}/${logfile}" "${LOCAL_TMPDIR}/${LOGS}" 2>&1 >/dev/null
		fi
	fi

	return $rc
}

# Read current image as backup in case one hasn't already been specified.
if [ -z "$BACKUP_IMAGE" ]; then
	backup_file="backup.bin"
	if [ $DO_REMOTE -eq 1 ]; then
		BACKUP_IMAGE="${REMOTE_TMPDIR}/${backup_file}"
	else
		BACKUP_IMAGE="${LOCAL_TMPDIR}/${backup_file}"
	fi

	print_and_log "Reading backup image..."
	flashrom_log_scmd $DO_REMOTE "$OLD_FLASHROM $PRIMARY_OPTS -r $BACKUP_IMAGE" "read_backup"

	if [ $? -ne 0 ]; then
		print_and_log "Failed to read backup image, aborting.\n"
		exit $EXIT_FAILURE
	fi

	if [ $DO_REMOTE -eq 1 ]; then
		copy_from_remote "$backup_file"
	fi
else
	if [ $DO_REMOTE -eq 1 ]; then
		scmd $DO_REMOTE "cp $BACKUP_IMAGE $REMOTE_TMPDIR"
		copy_from_remote "$(basename $BACKUP_IMAGE)"
	fi
fi

# The copy of flashrom to test. If unset, we'll assume the user wants to test
# a newly built flashrom binary in the current directory.
if [ -z "$NEW_FLASHROM" ] ; then
	if [ -x "flashrom" ]; then
		NEW_FLASHROM="flashrom"
	else
		print_and_log "Must supply new flashrom version to test\n"
		exit $EXIT_FAILURE
	fi
fi

print_and_log "Stable flashrom binary: ${OLD_FLASHROM}\n"
print_and_log "New flashrom binary to test: ${NEW_FLASHROM}\n"
print_and_log "Local temporary files will be stored in ${LOCAL_TMPDIR}\n"
if [ $DO_REMOTE -eq 1 ]; then
	print_and_log "Remote temporary files will be stored in ${REMOTE_HOST}:${REMOTE_TMPDIR}\n"
	print_and_log "Backup image: ${REMOTE_HOST}:${BACKUP_IMAGE}\n"
	print_and_log "Backup image also stored at: ${LOCAL_TMPDIR}/$(basename ${BACKUP_IMAGE})\n"
else
	print_and_log "Backup image: ${BACKUP_IMAGE}\n"
fi

#
# Now the fun begins.
#
cmd $DO_REMOTE "$OLD_FLASHROM $PRIMARY_OPTS --flash-size" "${LOCAL_TMPDIR}/old_chip_size.txt.orig"
cmd $DO_REMOTE "$NEW_FLASHROM $PRIMARY_OPTS --flash-size" "${LOCAL_TMPDIR}/new_chip_size.txt.orig"
# get rid of banner and other superfluous output from flashrom
tail -n 1 "${LOCAL_TMPDIR}/old_chip_size.txt.orig" > "${LOCAL_TMPDIR}/old_chip_size.txt"
tail -n 1 "${LOCAL_TMPDIR}/new_chip_size.txt.orig" > "${LOCAL_TMPDIR}/new_chip_size.txt"
tmp=$(cat ${LOCAL_TMPDIR}/old_chip_size.txt)
CHIP_SIZE=$(cat ${LOCAL_TMPDIR}/new_chip_size.txt)
CHIP_SIZE_KB=$(($CHIP_SIZE / $K))
CHIP_SIZE_HALF=$(($CHIP_SIZE / 2))
if [ $CHIP_SIZE -ne $tmp ]; then
	print_and_log "New flashrom and old flashrom disagree on chip size. Aborting.\n"
	exit $EXIT_FAILURE
else
	print_and_log "Chip size: $CHIP_SIZE_KB KiB\n"
fi

# Upload results
#do_upload()
#{
#	# TODO: implement this
#}

# Remove temporary files
do_cleanup()
{
	if [ $NO_CLEAN -eq 1 ]; then
		print_and_log "Skipping cleanup.\n"
		return $EXIT_SUCCESS
	fi

	rm -rf "$LOCAL_TMPDIR"
	if [ -n "$REMOTE_HOST" ]; then
		ssh root@${REMOTE_HOST} rm -rf "$REMOTE_TMPDIR"
	fi

	return $EXIT_SUCCESS
}

# $1: Message to display to user.
test_fail()
{
	print_and_log "$1\n"
	printf "Skipping cleanup (logs saved).\n"
	exit $EXIT_FAILURE
}

write_backup_image()
{
	print_and_log "Writing backup image.\n"
	flashrom_log_scmd $DO_REMOTE "$OLD_FLASHROM $PRIMARY_OPTS -w $BACKUP_IMAGE" "write_backup"
	if [ $? -ne 0 ]; then
		test_fail "Failed to write backup image.\n"
	fi
}

# Read a region twice and compare results
# $1: address of region (in bytes)
# $2: length of region (in bytes)
double_read_test()
{
	local cmp1="${TMPDIR}/read_test1.bin"
	local cmp2="${TMPDIR}/read_test2.bin"
	local layout="double_read_test_layout.txt"
	local len=$(($2 / $K))

	print_and_log "Doing double read test, size: $len KiB\n"
	# FIXME: Figure out how to do printf remotely...
	printf "%06x:%06x region\n" $1 $(($1 + $2 - 1)) > "${LOCAL_TMPDIR}/${layout}"
	if [ $DO_REMOTE -eq 1 ]; then copy_to_remote "$layout" ; fi

	flashrom_log_scmd $DO_REMOTE "$NEW_FLASHROM $PRIMARY_OPTS -l ${TMPDIR}/${layout} -i region -r ${cmp1}" "double_read_1"
	# FIXME: second (or maybe third?) read should be done using secondary programmer, if applicable.
	flashrom_log_scmd $DO_REMOTE "$NEW_FLASHROM $PRIMARY_OPTS -l ${TMPDIR}/${layout} -i region -r ${cmp2}" "double_read_2"
	scmd $DO_REMOTE "cmp $cmp1 $cmp2"
	if [ $? -ne 0 ]; then
		test_fail "Double-read test failed, aborting."
	fi
}

PARTIAL_WRITE_TEST_REGION_SIZE=0
PARTIAL_WRITE_TEST_ALIGN_SIZE_KB=0
# FIXME: Hack due to lack of region-sized file handling.
PARTIAL_WRITE_TEST_REGION_BASE_OFFSET=-1

# helper function to reduce repetitiveness of partial_write_test
partial_write_test_helper()
{
	local test_name="$1"
	local pattern_offset_kb=$2
	local pattern_size_kb=$3
	local hex="$4"
	local oct=""
	local base_offset_kb=$(($PARTIAL_WRITE_TEST_REGION_BASE_OFFSET / $K))

	oct="\\$(printf "%03o" $hex)"
	cp "${LOCAL_TMPDIR}/random_4k_test.bin" "${LOCAL_TMPDIR}/${test_name}.bin"
	dd if=/dev/zero bs=1k count=${pattern_size_kb} 2>/dev/null | tr "\000" "$oct" > "${LOCAL_TMPDIR}/${hex}_${pattern_size_kb}k.bin"

	while [ $(($(($pattern_offset_kb + $pattern_size_kb)) * $K)) -lt $PARTIAL_WRITE_TEST_REGION_SIZE ]; do
		dd if="${LOCAL_TMPDIR}/${hex}_${pattern_size_kb}k.bin" of="${LOCAL_TMPDIR}/${test_name}.bin" bs=1k count=$pattern_size_kb seek=$(($base_offset_kb + $pattern_offset_kb)) conv=notrunc 2>/dev/null
		pattern_offset_kb=$(($pattern_offset_kb + $PARTIAL_WRITE_TEST_ALIGN_SIZE_KB))
	done
}

# Regional partial write test. Given a region name, this will write patterns
# of bytes designed to test corner cases.
#
# We assume that eraseable block size can be either 4KB or 64KB and
# must test for both. For simplicity, we'll assume the region size is
# at least 256KB.
#
# $1: Region name
partial_write_test()
{
	local opts="--noverify-all"
	local secondary_opts=""		# for secondary programmer
	local region_name="$1"
	local filename=""
	local test_num=0
	local prev_test_num=0

	# FIXME: Hack due to lack of region-sized file handling.
	if [ $((PARTIAL_WRITE_TEST_REGION_SIZE)) -eq 0 ]; then
		print_and_log "Size of $region_name unknown\n"
		return $EXIT_FAILURE
	fi
	if [ $((PARTIAL_WRITE_TEST_REGION_BASE_OFFSET)) -lt 0 ]; then
		print_and_log "Offset of $region_name is unknown\n"
		return $EXIT_FAILURE
	fi

	if [ $TEST_TYPE -eq $TEST_TYPE_SINGLE ]; then
		if [ $REGION_MODE -eq $REGION_MODE_LAYOUT ]; then
			opts="$opts -l $LAYOUT_FILE"
			secondary_opts="$opts"
		elif [ $REGION_MODE -eq $REGION_MODE_CLOBBER ]; then
			printf "000000:%06x RW\n" $(($CHIP_SIZE - 1)) > "${LOCAL_TMPDIR}/clobber_mode_layout.txt"
			if [ $DO_REMOTE -eq 1 ]; then
				copy_to_remote "clobber_mode_layout.txt"
			fi
			secondary_opts="$opts -l ${LOCAL_TMPDIR}/clobber_mode_layout.txt"
			opts="$opts -l ${TMPDIR}/clobber_mode_layout.txt"
		fi
	fi

	if [ $SMALL_REGION -eq 1 ]; then
		PARTIAL_WRITE_TEST_ALIGN_SIZE_KB=16
	else
		PARTIAL_WRITE_TEST_ALIGN_SIZE_KB=256
	fi

	# FIXME: Add sanity checks.

	print_and_log "Doing region-based partial write test on region \"$region_name\"\n"
	flashrom_log_scmd $DO_REMOTE "$OLD_FLASHROM $PRIMARY_OPTS $opts -i ${region_name} -r ${TMPDIR}/${region_name}.bin" "read_region_${region_name}"
	if [ $DO_REMOTE -eq 1 ]; then
		copy_from_remote "${region_name}.bin"
	fi

	if [ $(($PARTIAL_WRITE_TEST_REGION_SIZE % $(($PARTIAL_WRITE_TEST_ALIGN_SIZE_KB)))) -ne 0 ]; then
		print_and_log "Region $region_name is not aligned to $PARTIAL_WRITE_TEST_ALIGN_SIZE_KB\n"
		return $EXIT_FAILURE
	fi

	# Test procedure:
	# Clobber region with random content first. Then do writes using the
	# following sequences for each 128KB:
	# 0-2K		: 0x00 (\000)	Partial 4KB sector, lower half
	# 2K-6K		: 0x11 (\021)	Crossover 4KB sector boundary
	# 6K-8K		: 0x22 (\042)	Partial 4KB sector, upper half
	# 8K-16K	: 0x33 (\063)	Full 4KB sectors
	#
	# Repeat the above sequence for 64KB-aligned sizes
	# 0-32K		: 0x44 (\104)	Partial 64KB block, lower half
	# 32K-96K	: 0x55 (\125)	Crossover 64KB block boundary
	# 96K-128K	: 0x66 (\146)	Partial 64KB block, upper half
	# 128K-256K	: 0x77 (\167)	Full 64KB blocks

	test_num=0
	dd if=/dev/zero of="${LOCAL_TMPDIR}/random_4k_test.bin" bs=1k count=$CHIP_SIZE_KB 2>/dev/null
	dd if=/dev/urandom of="${LOCAL_TMPDIR}/random_4k_test.bin" bs=4k count=$(($PARTIAL_WRITE_TEST_REGION_SIZE / $((4 * $K)))) seek=$(($((PARTIAL_WRITE_TEST_REGION_BASE_OFFSET)) / $((4 * $K)))) conv=notrunc 2>/dev/null

	# 0-2K		: 0x00 (\000)	Partial 4KB sector, lower half
	partial_write_test_helper "4k_test_${test_num}" 0 2 "0x00"
	prev_test_num=$test_num
	test_num=$(($test_num + 1))

	# 2K-6K		: 0x11 (\021)	Crossover 4KB sector boundary
	partial_write_test_helper "4k_test_${test_num}" 2 4 "0x11"
	test_num=$(($test_num + 1))

	# 6K-8K		: 0x22 (\042)	Partial 4KB sector, upper half
	partial_write_test_helper "4k_test_${test_num}" 6 2 "0x22"
	test_num=$(($test_num + 1))

	# 8K-16K	: 0x33 (\063)	Full 4KB sectors
	partial_write_test_helper "4k_test_${test_num}" 8 8 "0x33"

	for F in ${LOCAL_TMPDIR}/random_4k_test.bin ${LOCAL_TMPDIR}/4k_test_*.bin ; do
		filename=$(basename $F)
		if [ $DO_REMOTE -eq 1 ]; then
			copy_to_remote $filename
		fi

		flashrom_log_scmd $DO_REMOTE "$NEW_FLASHROM $PRIMARY_OPTS $opts -i ${region_name} -w ${TMPDIR}/${filename}" "write_${filename}"
		if [ $? -ne 0 ]; then
			test_fail "Failed to write $filename to $region_name"
		fi

		flashrom_log_scmd $DO_REMOTE "$OLD_FLASHROM $PRIMARY_OPTS $opts -i ${region_name} -v ${TMPDIR}/${filename}" "verify_${filename}"
		if [ $? -ne 0 ]; then
			test_fail "Failed to verify write of $filename to $region_name"
		fi

		if [ -n "$SECONDARY_OPTS" ]; then
			flashrom_log_scmd $LOCAL "$LOCAL_FLASHROM $SECONDARY_OPTS $secondary_opts -i ${region_name} -v ${LOCAL_TMPDIR}/${filename}" "verify_secondary_${filename}"
			if [ $? -ne 0 ]; then
				test_fail "Failed to verify write of $filename to $region_name using secondary programmer"
			fi
		fi

		print_and_log "\tWrote $filename to $region_name region successfully.\n"
	done

	if [ $SMALL_REGION -eq 1 ]; then
		return $EXIT_SUCCESS
	fi

	#
	# Second half: Tests for 64KB chunks
	#
	test_num=0
#	dd if=/dev/urandom of="${LOCAL_TMPDIR}/random_64k_test.bin" bs=128k count=$(($PARTIAL_WRITE_TEST_REGION_SIZE / $((128*$K)))) 2>/dev/null
	dd if=/dev/zero of="${LOCAL_TMPDIR}/random_64k_test.bin" bs=1k count=$CHIP_SIZE_KB 2>/dev/null
	dd if=/dev/urandom of="${LOCAL_TMPDIR}/random_64k_test.bin" bs=128k count=$(($PARTIAL_WRITE_TEST_REGION_SIZE / $((128 * $K)))) seek=$(($((PARTIAL_WRITE_TEST_REGION_BASE_OFFSET)) / $((128 * $K)))) conv=notrunc 2>/dev/null

	# 0-32K		: 0x44 (\104)	Partial 64KB block, lower half
	partial_write_test_helper "64k_test_${test_num}"  0 32 "0x44"
	prev_test_num=$test_num
	test_num=$(($test_num + 1))

	# 32K-96K	: 0x55 (\125)	Crossover 64KB block boundary
	partial_write_test_helper "64k_test_${test_num}" 32 64 "0x55"
	test_num=$(($test_num + 1))

	# 96K-128K	: 0x66 (\146)	Partial 64KB block, upper half
	partial_write_test_helper "64k_test_${test_num}"  96 32 "0x66"
	test_num=$(($test_num + 1))

	# 128K-256K	: 0x77 (\167)	Full 64KB blocks
	partial_write_test_helper "64k_test_${test_num}"  128 128 "0x77"

	for F in ${LOCAL_TMPDIR}/random_64k_test.bin ${LOCAL_TMPDIR}/64k_test_*.bin ; do
		filename=$(basename $F)
		if [ $DO_REMOTE -eq 1 ]; then
			copy_to_remote $filename
		fi

		flashrom_log_scmd $DO_REMOTE "$NEW_FLASHROM $PRIMARY_OPTS $opts -i ${region_name} -w ${TMPDIR}/${filename}" "write_${filename}"
		if [ $? -ne 0 ]; then
			test_fail "Failed to write $filename to $region_name"
		fi

		flashrom_log_scmd $DO_REMOTE "$OLD_FLASHROM $PRIMARY_OPTS $opts -i ${region_name} -v ${TMPDIR}/${filename}" "verify_${filename}"
		if [ $? -ne 0 ]; then
			test_fail "Failed to verify write of $filename to $region_name"
		fi

		if [ -n "$SECONDARY_OPTS" ]; then
			flashrom_log_scmd $LOCAL "$LOCAL_FLASHROM $SECONDARY_OPTS $secondary_opts -i ${region_name} -v ${LOCAL_TMPDIR}/${filename}" "verify_secondary_${filename}"
			if [ $? -ne 0 ]; then
				test_fail "Failed to verify write of $filename to $region_name using secondary programmer"
			fi
		fi

		print_and_log "\tWrote $filename to $region_name region successfully.\n"
	done

	return $EXIT_SUCCESS
}

# Before anything else, check to see if Flashrom can successfully probe
# for and find the flash chips. If not, we will abort.
flashrom_log_scmd $DO_REMOTE "$NEW_FLASHROM $PRIMARY_OPTS" "verify_probe"
if [ $? -ne 0 ]; then
	test_fail "Failed to find flash chips while probing, aborting."
fi

# Read ROM twice to test for consistency.
if [ $SKIP_CONSISTENCY_CHECK -eq 0 ]; then
	double_read_test 0 $CHIP_SIZE
fi

if [ $TEST_TYPE -eq $TEST_TYPE_SINGLE ]; then
	if [ $REGION_MODE -eq $REGION_MODE_CLOBBER ]; then
		random_file="${TMPDIR}/random_${CHIP_SIZE_KB}K.bin"
		cmp_file="${TMPDIR}/cmp.bin"

		scmd $DO_REMOTE "dd if=/dev/urandom of=${random_file} bs=1k count=${CHIP_SIZE_KB}"
		flashrom_log_scmd $DO_REMOTE "$NEW_FLASHROM $PRIMARY_OPTS -w $random_file" "clobber_write"
		flashrom_log_scmd $DO_REMOTE "$OLD_FLASHROM $PRIMARY_OPTS -r $cmp_file" "clobber_verify"
		scmd $DO_REMOTE "cmp $random_file $cmp_file"
		if [ $? -ne 0 ]; then
			write_backup_image
			test_fail "Failed to clobber entire ROM."
		fi
	elif [ $REGION_MODE -eq $REGION_MODE_DESCRIPTOR ]; then
		# FIXME: Need region-sized file handling or some way to get region info
		print_and_log "Currently broken due to lack of region-sized file handling."
		exit $EXIT_FAILURE
	elif [ $REGION_MODE -eq $REGION_MODE_FLASHMAP ]; then
		# FIXME: Need region-sized file handling or some way to get region info
		print_and_log "Currently broken due to lack of region-sized file handling."
		exit $EXIT_FAILURE
	elif [ $REGION_MODE -eq $REGION_MODE_LAYOUT ]; then
		rw_layout=""
		addr=""
		end=""
		size=""
		size_kb=""

		# Look for a region name with any amount of leading whitespace
		# and no trailing whitespace or characters.
		rw_layout=$(grep "\s${LAYOUT_REGION}$" "${LOCAL_TMPDIR}/$(basename $LAYOUT_FILE)" | head -n 1)
		if [ -z "$rw_layout" ]; then
			print_and_log "No region matching \"${LAYOUT_REGION}\" found layout file \"${LAYOUT_FILE}\"\n"
			test_fail ""
		fi

		addr="0x$(echo "$rw_layout" | cut -d ' ' -f -1 | awk -F ':' '{ print $1 }')"
		end="0x$(echo "$rw_layout" | cut -d ' ' -f -1 | awk -F ':' '{ print $2 }')"
		size="$(($end - $addr + 1))"
		size_kb="$(($size / $K))"

		# FIXME: Hack to make this work without region-sized file handling.
		PARTIAL_WRITE_TEST_REGION_BASE_OFFSET=$addr
		PARTIAL_WRITE_TEST_REGION_SIZE=$size

		print_and_log "\"$LAYOUT_REGION\" region address: ${addr}, size: $size_kb KiB\n"
		partial_write_test "$LAYOUT_REGION"
		if [ $? -ne 0 ]; then
			print_and_log "Layout mode test failed\n"
			RC=$EXIT_FAILURE
		fi
	fi
elif [ $TEST_TYPE -eq $TEST_TYPE_ENDURANCE ]; then
	iteration=1
	terminate=0
	random_file="${TMPDIR}/random_${CHIP_SIZE_KB}K.bin"
	cmp_file="${TMPDIR}/cmp.bin"
	# TODO: We can measure how long the tests take on average, throughput, etc.
	# i.e. { time $NEW_FLASHROM $PRIMARY_OPTS -w $random_file ; } 2>&1 | grep user | cut -f2

	# For this test we want to run clobber mode until failure
	while [ $terminate -eq 0 ]
	do
		print_and_log "Running iteration #${iteration}\n"

		scmd $DO_REMOTE "dd if=/dev/urandom of=${random_file} bs=1k count=${CHIP_SIZE_KB}"
		flashrom_log_scmd $DO_REMOTE "$NEW_FLASHROM $PRIMARY_OPTS -w $random_file" "endurance_write_${iteration}"
		flashrom_log_scmd $DO_REMOTE "$OLD_FLASHROM $PRIMARY_OPTS -r $cmp_file" "endurance_verify_${iteration}"
		scmd $DO_REMOTE "cmp $random_file $cmp_file"
		if [ $? -ne 0 ]; then
			terminate=1
		fi
		scmd $DO_REMOTE "rm -f $cmp_file $random_file"

		iteration=$(($iteration + 1))
	done

	# TODO: Determine what to return for the endurance test exit status
	# i.e. what constitutes a test pass and what constitutes a test fail?
	print_and_log "Failed on iteration $iteration\n"
	# TODO - Print performance metrics?
fi

# restore and cleanup
write_backup_image

if [ $RC -eq 0 ]; then
	print_and_log "Test status: PASS\n"
else
	print_and_log "Test status: FAIL\n"
fi
do_cleanup

exit $RC
