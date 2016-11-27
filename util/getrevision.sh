#!/bin/sh
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

# List of important upstream branches...
upstream_branches="stable staging"
upstream_url="https://flashrom.org/git/flashrom.git"
upstream_patterns="github\.com.flashrom/flashrom(\.git)?|flashrom\.org.git/flashrom(\.git)?"

upcache_prefix="refs/flashrom_org/"
# Generate upcache_refs
for b in $upstream_branches ; do
	upcache_refs="$upcache_refs ${upcache_prefix}$b"
done

# We need to update our upstream information sometimes so that we can create detailed version information.
# To that end the code below fetches parts of the upstream repository via https.
# This takes about one second or less under normal circumstances.
#
# It can be called manually, but is usually called via
#  - the Makefile (implicitly via revision()) when there is no upstream information in any existing remote
forced_update() {
	local rev_remote_refs
	for ref in $upcache_refs ; do
		rev_remote_refs="$rev_remote_refs +${ref##*/}:$ref"
	done
	git fetch -q "$upstream_url" --tags $rev_remote_refs && echo "Success."
}

update() {
	offline=$(git config flashrom.offline-builds 2>/dev/null)
	if [ -z "$offline" ]; then
		echo "To produce useful version information the build process needs access to the commit
history from an upstream repository. If no git remote is pointing to one we
can store the necessary information out of sight and update it on every build.
To enable this functionality and fetch the upstream commits from $upstream_url
please execute 'git config flashrom.offline-builds false' or add one of the
upstream repositories as git remote to rely on that information.
However, if you want to work completely offline and generate possibly meaningless
version strings then disable it with 'git config flashrom.offline-builds true'
You can force updating the local commit cache with '$0 --forced-update'">&2
		return 1
	elif [ "x$offline" = "xfalse" ]; then
		echo "Fetching commit history from upstream repository $upstream_url
To disable any network activity execute 'git config flashrom.offline-builds true'.">&2
		forced_update >/dev/null
	else
		echo "Fetching commit history from upstream is disabled - version strings might be misleading.
To ensure proper version strings and allow network access run 'git config flashrom.offline-builds false'.">&2
	fi
	return 0
}

# Helper functions
# First argument is the path to inspect (usually optional; w/o it the whole repository will be considered)
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
	# get all remote branches containing the last commit (excluding origin/HEAD and git-svn branches/tags)
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

# Returns a string indicating where others can get the current source code (excluding uncommitted changes)
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

tag() {
	local t=

	if git_is_file_tracked "$1" ; then
		local sha=$(git_last_commit "$1")
		t=$(git describe --abbrev=0 "$sha")
	fi
	if [ -z "$t" ]; then
		t="unknown" # default to unknown
	fi
	echo "${t}"
}

find_upremote() {
	# Try to find upstream's remote name
	for remote in $(git remote) ; do
		local url=$(git ls-remote --get-url $remote)
		if echo "$url" | grep -q -E "$upstream_patterns" ; then
			echo "$remote"
			return
		fi
	done
}

revision() {
	local sha=$(git_last_commit "$1" 2>/dev/null)
	# No git no fun
	if [ -z "$sha" ]; then
		echo "unknown"
		return
	fi

	local r="$sha"
	if git_has_local_changes "$1" ; then
		r="$r-dirty"
	fi

	# sha + possibly dirty info is not exactly verbose, therefore the code below tries to use tags and
	# branches from the upstream repos to derive a more previse version string.
	# To that end we try to use the existing remotes first.
	# If the upstream repos (and its mirrors) are not available as remotes, use a shadow copy instead.

	local up_refs
	local up_remote=$(find_upremote)
	if [ -n "$up_remote" ]; then
		for b in $upstream_branches ; do
			up_refs="$up_refs ${up_remote}/${b}"
		done
	else
		update || { echo "offline" ; return ; }
		up_refs=$upcache_refs
	fi

	# Find nearest commit contained in this branch that is also in any of the up_refs, i.e. the branch point
	# of the current branch. This might be the latest commit if it's in any of the upstream branches.
	local merge_point=$(git merge-base ${sha} ${up_refs})
	local upstream_branch
	if [ -z "$merge_point" ]; then
		echo "$sha"
		return
	fi

	# If the current commit is reachable from any remote branch, append the branch name and its
	# distance to the nearest earlier tag to that tag name itself (tag-distance-branch).
	# If multiple branches are reachable then we use the newest one (by commit date).
	# If none is reachable we use the nearest tag and ? for distances and remote branch name.

	local cnt_upstream_branch2sha=$(git rev-list --count "${merge_point}..${sha}" 2>/dev/null)

	local lasttag=$(git describe --abbrev=0 "$merge_point" 2>/dev/null)
	if [ -z "$lasttag" ]; then
		echo "Could not find tag reachable from merge point!">&2
		echo "$sha"
		return
	fi

	local cnt_tag2upstream_branch
	for ref in $up_refs ; do
		if git merge-base --is-ancestor ${merge_point} ${ref}; then
			upstream_branch=${ref##*/} # remove everything till last /
			cnt_tag2upstream_branch=$(git rev-list --count "${lasttag}..${merge_point}" 2>/dev/null)
			break
		fi
	done

	if [ "$cnt_upstream_branch2sha" -gt 0 ]; then
		r="$cnt_upstream_branch2sha-$r"
	fi
	if [ "$cnt_tag2upstream_branch" -gt 0 ]; then
		if [ -n "$upstream_branch" ]; then
			r="$upstream_branch-$r"
		fi
		r="$cnt_tag2upstream_branch-$r"
	fi
	r="$lasttag-$r"

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
    -T or --tag
        returns the name of the last release/tag
    -r or --revision
        return unique revision information including the last tag and an indicator for uncommitted changes
    -u or --url
        URL associated with the latest commit
    -d or --date
        date of most recent modification
    -t or --timestamp
        timestamp of most recent modification
    -U or --update
        update local shadow copy of upstream commits if need be and offline builds are not enforced
          --forced-update
        force updating the local shadow copy of upstream commits
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
		-T|--tag)
			check_action $1
			action=tag
			shift;;
		-r|--revision)
			check_action $1
			action=revision
			shift;;
		-u|--url)
			check_action $1
			action=scm_url
			shift;;
		-d|--date)
			check_action $1
			action="timestamp +%Y-%m-%d" # refrain from suffixing 'Z' to indicate it's UTC
			shift;;
		-t|--timestamp)
			check_action $1
			action="timestamp +%Y-%m-%dT%H:%M:%SZ" # There is only one valid time format! ISO 8601
			shift;;
		-U|--update)
			check_action $1
			action=update
			shift;;
		--forced-update)
			check_action $1
			action=forced_update
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
