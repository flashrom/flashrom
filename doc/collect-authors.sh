#!/bin/sh

if [ $# -ne 3 ]
then
    echo "Wrong number of arguments. Usage: $0 [authors|reviewers] outfile git_dir" >&2
    exit 1
fi

case "$1" in
    authors)
        GROUP_ARGS="--group=author --group=trailer:Co-Authored-by --group=trailer:Co-Developed-by"
        ;;
    reviewers)
        GROUP_ARGS="--group=trailer:Reviewed-by"
        ;;
    *)
        echo "Unknown contributor kind: \"$1\"" >&2
        exit 1
        ;;
esac
OUTFILE="$2"
# GIT_DIR is passed explicitly so we never need to guess where
# the source directory is. It may be somewhere entirely different
# from where meson is running us.
GIT_DIR="$3"

if ! command -v git >/dev/null
then
    echo "git not available" >&2
    exit 1
fi

if [ ! -d "$GIT_DIR" ]
then
    echo "GIT_DIR ($GIT_DIR) does not exist" >&2
    exit 1
fi

git --no-pager --git-dir="$GIT_DIR" shortlog --summary --numbered $GROUP_ARGS HEAD > "$OUTFILE"
