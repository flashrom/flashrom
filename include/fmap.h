/*
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 * SPDX-FileCopyrightText: 2010, Google LLC.
 * SPDX-FileCopyrightText: 2018-present, Facebook Inc. All rights reserved.
 */

#ifndef __FMAP_H__
#define __FMAP_H__ 1

#include <inttypes.h>
#include <stdbool.h>

#define FMAP_SIGNATURE		"__FMAP__"
#define FMAP_VER_MAJOR		1	/* this header's FMAP minor version */
#define FMAP_VER_MINOR		1	/* this header's FMAP minor version */
#define FMAP_STRLEN		32	/* maximum length for strings */

struct fmap_area {
	uint32_t offset;		/* offset relative to base */
	uint32_t size;			/* size in bytes */
	uint8_t  name[FMAP_STRLEN];	/* descriptive name */
	uint16_t flags;			/* flags for this area */
}  __attribute__((packed));

struct fmap {
	uint8_t  signature[8];		/* "__FMAP__" */
	uint8_t  ver_major;		/* major version */
	uint8_t  ver_minor;		/* minor version */
	uint64_t base;			/* address of the firmware binary */
	uint32_t size;			/* size of firmware binary in bytes */
	uint8_t  name[FMAP_STRLEN];	/* name of this firmware binary */
	uint16_t nareas;		/* number of areas described by
					   fmap_areas[] below */
	struct fmap_area areas[];
}  __attribute__((packed));

int fmap_read_from_buffer(struct fmap **fmap_out, const uint8_t *buf, size_t len);
int fmap_read_from_rom(struct fmap **fmap_out, struct flashctx *const flashctx, size_t rom_offset, size_t len);


#endif	/* __FMAP_H__*/
