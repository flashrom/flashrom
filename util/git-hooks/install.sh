#!/bin/sh -e

root=$(git rev-parse --show-cdup 2>/dev/null) || \
     { echo "Not under git control. Cannot install git hooks." >&2 ; exit 0 ; }

[ -z "${root}" ] || \
     { echo "Not in root directory. Can only run from git root." >&2 ; exit 1 ; }

src=util/git-hooks/ # relative to root
hooks=$(cd "${src}" && git ls-files -c | grep -Ev 'install.sh|wrapper.sh')

if [ "$(git rev-parse --git-path 2>/dev/null)" = "--git-path" ]; then
	# very old git, we have to guess
	dst=".git/hooks/"
	rel="../../"
else
	dst=$(git rev-parse --git-path hooks/)
	rel=$(git rev-parse --prefix "${dst}" --show-cdup)
fi

for h in $hooks; do
	# Test if hook is not already installed, i.e. doesn't point at the wrapper
	if [ ! "${dst}$h" -ef "${src}wrapper.sh" ]; then
		# preserve custom hooks if any
		if [ -e "${dst}$h" ]; then
			mv "${dst}$h" "${dst}$h.local"
		fi
		ln -s "${rel}${src}wrapper.sh" "${dst}$h"
	fi
done
