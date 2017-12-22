#!/bin/sh

cd

[ "${IDENT}" ] || IDENT=$(mktemp -u XXXXXXXX)

CCACHE=.ccache/anita-${IDENT}.img

[ -f ${CCACHE} ] || zcat cache.img.gz >${CCACHE}

if [ $# -eq 0 ]; then
	exec anita --vmm-args "-hdb ${CCACHE}" interact ${INST_IMG}
else
	exec anita --vmm-args "-hdb ${CCACHE}" --persist \
		--run ". ./init && manitest \"$*\"" \
		boot ${INST_IMG}
fi
