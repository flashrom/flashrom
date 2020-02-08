#!/bin/sh
#
# This file is part of the flashrom project. It is derived from
# board_status.sh in coreboot.
#
# Copyright (C) 2016 Google Inc.
# Copyright (C) 2014 Sage Electronic Engineering, LLC.

USE_CUSTOM_HOOKS=0
if [ -n "$CUSTOM_HOOKS_FILENAME" ]; then
	USE_CUSTOM_HOOKS=1
	. "$CUSTOM_HOOKS_FILENAME"
	if [ $? -ne 0 ]; then
		echo "Failed to source custom hooks file."
		exit $EXIT_FAILURE
	fi

	if ! custom_hook_sanity_check; then
		echo "Failed to run sanity check for custom hooks."
		exit $EXIT_FAILURE
	fi
fi

# test a command
#
# $1: 0 ($LOCAL) to run command locally,
#     1 ($REMOTE) to run remotely if remote host defined
# $2: command to test
# $3: 0 ($FATAL) Exit with an error if the command fails
#     1 ($NONFATAL) Don't exit on command test failure
test_cmd()
{
	local rc
	local cmd__="$(echo $2 | cut -d ' ' -f -1)"
	local args="$(echo $2 | cut -d ' ' -f 2-)"

	if [ -e "$cmd__" ]; then
		return
	fi

	if [ "$1" -eq "$REMOTE" ] && [ -n "$REMOTE_HOST" ]; then
		ssh $REMOTE_PORT_OPTION root@${REMOTE_HOST} command -v "$cmd__" $args > /dev/null 2>&1
		rc=$?
	else
		command -v "$cmd__" $args >/dev/null 2>&1
		rc=$?
	fi

	if [ $rc -eq 0 ]; then
		return 0
	fi

	if [ "$3" = "1" ]; then
		return 1
	fi

	echo "$2 not found"
	exit $EXIT_FAILURE
}

# Same args as cmd() but with the addition of $4 which determines if the
# command should be totally silenced or not.
_cmd()
{
	local silent=$4
	local rc=0

	if [ -n "$3" ]; then
		pipe_location="${3}"
	else
		pipe_location="/dev/null"
	fi

	if [ $1 -eq $REMOTE ] && [ -n "$REMOTE_HOST" ]; then
		if [ $silent -eq 0 ]; then
			ssh $REMOTE_PORT_OPTION "root@${REMOTE_HOST}" "$2" > "$pipe_location" 2>/dev/null
			rc=$?
		else
			ssh $REMOTE_PORT_OPTION "root@${REMOTE_HOST}" "$2" >/dev/null 2>&1
			rc=$?
		fi
	else
		if [ $USE_CUSTOM_HOOKS -eq 1 ]; then
			preflash_hook $1 "$2" "$3" $4
		fi

		if [ $silent -eq 0 ]; then
			$SUDO_CMD $2 > "$pipe_location" 2>/dev/null
			rc=$?
		else
			$SUDO_CMD $2 >/dev/null 2>&1
			rc=$?
		fi

		if [ $USE_CUSTOM_HOOKS -eq 1 ]; then
			postflash_hook $1 "$2" "$3" $4
		fi
	fi

	return $rc
}

# run a command
#
# $1: 0 ($LOCAL) to run command locally,
#     1 ($REMOTE) to run remotely if remote host defined
# $2: command
# $3: filename to direct output of command into
cmd()
{
	_cmd $1 "$2" "$3" 0

	if [ $? -eq 0 ]; then
		return
	fi

	echo "Failed to run \"$2\", aborting"
	rm -f "$3"	# don't leave an empty file
	return $EXIT_FAILURE
}

# run a command silently
#
# $1: 0 ($LOCAL) to run command locally,
#     1 ($REMOTE) to run remotely if remote host defined
# $2: command
scmd()
{
	_cmd $1 "$2" "" 1

	if [ $? -eq 0 ]; then
		return
	fi

	echo "Failed to run \"$2\", aborting"
	return $EXIT_FAILURE
}
