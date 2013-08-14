#!/bin/sh
#
# This file is part of the flashrom project.
#
# Copyright (C) 2005 coresystems GmbH <stepan@coresystems.de>
# Copyright (C) 2009,2010 Carl-Daniel Hailfinger
# Copyright (C) 2010 Chromium OS Authors
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

svn_revision() {
	LC_ALL=C svnversion -cn . 2>/dev/null | \
		sed -e "s/.*://" -e "s/\([0-9]*\).*/\1/" | \
		grep "[0-9]" ||
	LC_ALL=C svn info . 2>/dev/null | \
		awk '/^Revision:/ {print $$2 }' | \
		grep "[0-9]" ||
	LC_ALL=C git svn info . 2>/dev/null | \
		awk '/^Revision:/ {print $$2 }' | \
		grep "[0-9]" ||
	echo ''
}

svn_url() {
	echo $(LC_ALL=C svn info 2>/dev/null |
	       grep URL: |
		   sed 's/.*URL:[[:blank:]]*//' |
		   grep ^.
	      )
}

svn_timestamp() {
	local date_format="+%Y-%m-%d %H:%M:%S"
	local timestamp

	# are there local changes in the client?
	if svn status | egrep '^ *[ADMR] *' > /dev/null ; then
		timestamp=$(date "${date_format} +")
	else
		# No local changes, get date of the last log record.
		local last_commit_date=$(svn info | grep '^Last Changed Date:' | \
		                         awk '{print $4" "$5" "$6}')
		timestamp=$(date --utc --date "${last_commit_date}" \
			        "${date_format} UTC")
	fi

	echo "${timestamp}"
}

git_revision() {
	echo $(git log --oneline | head -1 | awk '{print $1}')
}

# Retrieve svn revision using git log data (for git mirrors)
gitsvn_revision() {
	local r

	git log|
		grep git-svn-id|
		sed 's/.*git-svn-id:[[:blank:]]*\([^@]\+\)@[0-9]\+[[:blank:]]\+\([^[:blank:]]\+\)/\1 \2/'|
		sort|
		uniq|
		read url uuid

	r=$(git log --grep="git-svn-id.*$uuid"| grep git-svn-id | \
		sed 's/.*@//;s/[[:blank:]].*//'| \
		sort -n | \
		tail -1)

	echo "${r}"
}

git_timestamp() {
	local date_format="+%b %d %Y %H:%M:%S"
	local timestamp

	# are there local changes in the client?
	if git status | \
	   egrep '^# Change(s to be committed|d but not updated):$' > /dev/null
	then
		timestamp=$(date "${date_format} +")
	else
		# No local changes, get date of the last log record.
		local last_commit_date=$(git log | head -3 | grep '^Date:' | \
		                         awk '{print $3" "$4" "$6" "$5" "$7}')
		timestamp=$(date --utc --date "${last_commit_date}" \
		            "${date_format} UTC")
	fi

	echo "${timestamp}"
}

git_url() {
	# Only the first line of `git remote' is considered.
	echo $(LC_ALL=C git remote show origin 2>/dev/null |
	       grep 'Fetch URL:' |
	       sed 's/.*URL:[[:blank:]]*//' |
	       grep ^.
	      )
}

scm_url() {
	local url

	if [ -d ".svn" ] ; then
		url=$(svn_url)
	elif [ -d ".git" ] ; then
		url=$(git_url)
	fi

	echo "${url}"
}

# Retrieve timestamp since last modification. If the sources are pristine,
# then the timestamp will match that of the SCM's more recent modification
# date.
timestamp() {
	local t

	if [ -d ".svn" ] ; then
		t=$(svn_timestamp)
	elif [ -d ".git" ] ; then
		t=$(git_timestamp)
	fi

	echo ${t}
}

# Retrieve local SCM revision info. This is useful if we're working in
# a even different SCM than upstream.
#
# If local copy is svn, then there is nothing to do here.
# If local copy is git, then retrieve useful git revision info
local_revision() {
	local r

	if [ -d ".git" ] ; then
		r=$(git_revision)
	fi

	echo ${r}
}

# Get the upstream flashrom revision stored in SVN metadata.
#
# If the local copy is svn, then use svnversion
# If the local copy is git, then scrape upstream revision from git logs
upstream_revision() {
	local r

	if [ -d ".svn" ] ; then
		r=$(svn_revision)
	elif [ -d ".git" ] ; then
		r=$(gitsvn_revision)
	fi

	echo "${r}"
}

show_help() {
	echo "Usage:
	${0} <option>

Options
    -h or --help
        Display this message.
    -u or --upstream
        upstream flashrom revision
    -l or --local
        local revision (if different from upstream)
    -t or --timestamp
        timestamp of most recent modification
    -U or --url
        url associated with local copy of flashrom
	"
	return
}

if [ ! -n "${1}" ]
then
	show_help;
	echo "No options specified";
	exit ${EXIT_SUCCESS}
fi

# The is the main loop
while [[ ${1} = -* ]];
do
	case ${1} in
	-h|--help)
		show_help;
		shift;;
	-u|--upstream)
		echo "$(upstream_revision)";
		shift;;
	-l|--local)
		echo "$(local_revision)";
		shift;;
	-t|--timestamp)
		echo "$(timestamp)";
		shift;;
	-U|--url)
		echo "$(scm_url)";
		shift;;
	*)
		show_help;
		echo "invalid option: ${1}"
		exit ${EXIT_FAILURE}
	esac;
done

exit ${EXIT_SUCCESS}
