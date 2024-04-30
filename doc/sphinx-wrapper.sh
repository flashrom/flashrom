#!/bin/sh

OUTPUT_HOME="$1"
MAN_OUTPUTS="$2"
SPHINXBUILD="$3"
shift 3

"${SPHINXBUILD}" "$@" || exit $?

SPHINXBUILD_MAJOR="$("${SPHINXBUILD}" --version | cut -d' ' -f2 | cut -d'.' -f1)"
if [ "${SPHINXBUILD_MAJOR}" -ge 4 ]; then
	exit 0
fi

# sphinx-build 3.x outputs man pages to "8" directory instead of expected "man8".
# The following block checks for "man8" (and other output paths in ${MAN_OUTPUTS})
# and, if that is missing, but "8" dir exists instead, it renames "8" to "man8"
# and creates a symlink named "8" that points to "man8", so that anyone is happy
# during the rest of current build and subsequent builds as well.

for MAN_OUTPUT in ${MAN_OUTPUTS}; do
	PATH_TARGET="${OUTPUT_HOME}/${MAN_OUTPUT}"
	PATH_ACTUAL="${OUTPUT_HOME}/${MAN_OUTPUT#man}"
	if [ -d "${PATH_ACTUAL}" -a ! -L "${PATH_ACTUAL}" ]; then
		rm -rf "${PATH_TARGET}"
		mv "${PATH_ACTUAL}" "${PATH_TARGET}"
		ln -s "${MAN_OUTPUT}" "${PATH_ACTUAL}"
	fi
done
