#!/bin/sh
# NB: Supposed to be POSIX compatible but at least the usage of 'local' is not.
#
# This file is part of the flashrom project.
#
# Copyright (C) 2005 coresystems GmbH <stepan@coresystems.de>
# Copyright (C) 2009,2010 Carl-Daniel Hailfinger
# Copyright (C) 2010 Chromium OS Authors
# Copyright (C) 2013-2016 Stefan Tauner
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

EXIT_SUCCESS=0
EXIT_FAILURE=1

# Make sure we don't get translated output
export LC_ALL=C
# nor local times or dates
export TZ=UTC0

# Helper functions
# First argument is the path to inspect (usually optional; without
# it the whole repository will be considered)
git_has_local_changes() {
	git update-index -q --refresh >/dev/null
	! git diff-index --quiet HEAD -- "$1"
}

git_last_commit() {
	# git rev-parse --short HEAD would suffice if repository as a whole is of interest (no $1)
	git log --pretty=format:"%h" -1 -- "$1"
}

git_is_file_tracked() {
	git ls-files --error-unmatch -- "$1" >/dev/null 2>&1
}

is_file_tracked() {
	git_is_file_tracked "$1"
}

# Tries to find a remote source for the changes committed locally.
# This includes the URL of the remote repository including the last commit and a suitable branch name.
# Takes one optional argument: the path to inspect
git_url() {
	last_commit=$(git_last_commit "$1")
	# get all remote branches containing the last commit (excluding origin/HEAD)
	branches=$(git branch -r --contains $last_commit | sed '/\//!d;/.*->.*/d;s/[\t ]*//')
	if [ -z "$branches" ] ; then
		echo "No remote branch contains a suitable commit">&2
		return
	fi

	# find "nearest" branch
	local mindiff=9000
	local target=
	for branch in $branches ; do
		curdiff=$(git rev-list --count $last_commit..$branch)
		if [ $curdiff -ge $mindiff ] ; then
			continue
		fi
		mindiff=$curdiff
		target=$branch
	done

	echo "$(git ls-remote --exit-code --get-url ${target%/*}) ${target#*/}"
}

# Returns a string indicating where others can get the current source code (excluding uncommitted changes).
# Takes one optional argument: the path to inspect
scm_url() {
	local url=

	if git_is_file_tracked "$1" ; then
		url="$(git_url "$1")"
	else
		return ${EXIT_FAILURE}
	fi

	echo "${url}"
}

# Retrieve timestamp since last modification. If the sources are pristine,
# then the timestamp will match that of the SCM's most recent modification
# date.
timestamp() {
	local t

	# date syntaxes are manifold:
	# gnu		date [-d input]... [+FORMAT]
	# netbsd	date [-ajnu] [-d date] [-r seconds] [+format] [[[[[[CC]yy]mm]dd]HH]MM[.SS]]
	# freebsd	date [-jnu]  [-d dst] [-r seconds] [-f fmt date | [[[[[cc]yy]mm]dd]HH]MM[.ss]] [+format] [...]
	# dragonflybsd	date [-jnu]  [-d dst] [-r seconds] [-f fmt date | [[[[[cc]yy]mm]dd]HH]MM[.ss]] [+format] [...]
	# openbsd	date [-aju]  [-d dst] [-r seconds] [+format] [[[[[[cc]yy]mm]dd]HH]MM[.SS]] [...]
	if git_is_file_tracked "$2" ; then
		# are there local changes?
		if git_has_local_changes "$2" ; then
			t=$(date -u "${1}")
		else
			# No local changes, get date of the last commit
			case $(uname) in
			# Most BSD dates do not support parsing date values from user input with -d but all of
			# them support parsing epoch seconds with -r. Thanks to git we can easily use that:
			NetBSD|OpenBSD|DragonFly|FreeBSD)
				t=$(date -u -r "$(git log --pretty=format:%ct -1 -- $2)"  "$1" 2>/dev/null);;
			*)
				t=$(date -d "$(git log --pretty=format:%cD -1 -- $2)" -u "$1" 2>/dev/null);;
			esac
		fi
	else
		t=$(date -u "$1")
	fi

	if [ -z "$t" ]; then
		echo "Warning: Could not determine timestamp." 2>/dev/null
	fi
	echo "${t}"
}

revision() {
	local r
	if git_is_file_tracked "$1" ; then
		r=$(git describe $(git_last_commit "$1"))
		if git_has_local_changes "$1" ; then
			r="$r-dirty"
		fi
	else
		r="unknown"
	fi

	echo "${r}"
}

is_tracked() {
	is_file_tracked "$1"
}

show_help() {
	echo "Usage:
	${0} <command> [path]

Commands
    -h or --help
        this message
    -c or --check
        test if path is under version control at all
    -r or --revision
        return unique revision information including an indicator for
        uncommitted changes
    -U or --url
        URL associated with the latest commit
    -d or --date
        date of most recent modification
    -t or --timestamp
        timestamp of most recent modification
"
	return
}

check_action() {
	if [ -n "$action" ]; then
		echo "Error: Multiple actions given.">&2
		exit ${EXIT_FAILURE}
	fi
}

main() {
	local query_path=
	local action=

	# Argument parser loop
	while [ $# -gt 0 ];
	do
		case ${1} in
		-h|--help)
			action=show_help;
			shift;;
		-U|--url)
			check_action $1
			action=scm_url
			shift;;
		-d|--date)
			check_action $1
			action="timestamp +%Y-%m-%d" # refrain from suffixing 'Z' to indicate it's UTC
			shift;;
		-r|--revision)
			check_action $1
			action=revision
			shift;;
		-t|--timestamp)
			check_action $1
			action="timestamp +%Y-%m-%dT%H:%M:%SZ" # There is only one valid time format! ISO 8601
			shift;;
		-c|--check)
			check_action $1
			action=is_tracked
			shift;;
		-*)
			show_help;
			echo "Error: Invalid option: ${1}"
			exit ${EXIT_FAILURE};;
		*)
			if [ -z "$query_path" ] ; then
				if [ ! -e "$1" ] ; then
					echo "Error: Path \"${1}\" does not exist.">&2
					exit ${EXIT_FAILURE}
				fi
				query_path=$1
			else
				echo "Warning: Ignoring overabundant parameter: \"${1}\"">&2
			fi
			shift;;
		esac;
	done

	# default to current directory (usually equals the whole repository)
	if [ -z "$query_path" ] ; then
		query_path=.
	fi
	if [ -z "$action" ] ; then
		show_help
		echo "Error: No actions specified"
		exit ${EXIT_FAILURE}
	fi

	$action "$query_path"
}

main $@
