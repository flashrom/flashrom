#!/bin/sh -e

root=$(git rev-parse --show-cdup 2>/dev/null) || \
     { echo "Not under git control. Cannot install git hooks." >&2 ; exit 0 ; }

dst="${root}"$(git rev-parse --git-path hooks/)
src=util/git-hooks/ # relative to root
hooks=$(cd "${root}${src}" && git ls-files -c | grep -Ev 'install.sh|wrapper.sh')

for h in $hooks; do
	# Test if hook is not already installed, i.e. doesn't point at the wrapper
	if [ ! "${dst}$h" -ef "${root}${src}wrapper.sh" ]; then
		# preserve custom hooks if any
		if [ -e "${dst}$h" ]; then
			mv "${dst}$h" "${dst}$h.local"
		fi
		ln -s "$(git rev-parse --prefix $(git rev-parse --git-path hooks/) --show-cdup)${src}wrapper.sh" \
		      "${dst}$h"
	fi
done
