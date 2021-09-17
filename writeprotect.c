/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"
#include "spi.h"
#include "writeprotect.h"

/*
 * The following procedures rely on look-up tables to match the user-specified
 * range with the chip's supported ranges. This turned out to be the most
 * elegant approach since diferent flash chips use different levels of
 * granularity and methods to determine protected ranges. In other words,
 * be stupid and simple since clever arithmetic will not work for many chips.
 */

struct wp_range {
	unsigned int start;	/* starting address */
	unsigned int len;	/* len */
};

enum bit_state {
	OFF	= 0,
	ON	= 1,
	X	= -1	/* don't care. Must be bigger than max # of bp. */
};

/*
 * Generic write-protection schema for 25-series SPI flash chips. This assumes
 * there is a status register that contains one or more consecutive bits which
 * determine which address range is protected.
 */

struct status_register_layout {
	int bp0_pos;	/* position of BP0 */
	int bp_bits;	/* number of block protect bits */
	int srp_pos;	/* position of status register protect enable bit */
};

/*
 * The following ranges and functions are useful for representing the
 * writeprotect schema in which there are typically 5 bits of
 * relevant information stored in status register 1:
 * m.sec: This bit indicates the units (sectors vs. blocks)
 * m.tb: The top-bottom bit indicates if the affected range is at the top of
 *       the flash memory's address space or at the bottom.
 * bp: Bitmask representing the number of affected sectors/blocks.
 */
struct wp_range_descriptor {
	struct modifier_bits m;
	unsigned int bp;		/* block protect bitfield */
	struct wp_range range;
};

struct wp_context {
	struct status_register_layout sr1;	/* status register 1 */
	struct wp_range_descriptor *descrs;

	/*
	 * Some chips store modifier bits in one or more special control
	 * registers instead of the status register like many older SPI NOR
	 * flash chips did. get_modifier_bits() and set_modifier_bits() will do
	 * any chip-specific operations necessary to get/set these bit values.
	 */
	int (*get_modifier_bits)(const struct flashctx *flash,
			struct modifier_bits *m);
	int (*set_modifier_bits)(const struct flashctx *flash,
			struct modifier_bits *m);
};

/*
 * Mask to extract write-protect enable and range bits
 *   Status register 1:
 *     SRP0:           bit 7
 *     range(BP2-BP0): bit 4-2
 *     range(BP3-BP0): bit 5-2 (large chips)
 *   Status register 2:
 *     SRP1:           bit 1
 */
#define MASK_WP_AREA (0x9C)
#define MASK_WP_AREA_LARGE (0x9C)
#define MASK_WP2_AREA (0x01)

static uint8_t do_read_status(const struct flashctx *flash)
{
	if (flash->chip->read_status)
		return flash->chip->read_status(flash);
	else
		return spi_read_status_register(flash);
}

static int do_write_status(const struct flashctx *flash, int status)
{
	if (flash->chip->write_status)
		return flash->chip->write_status(flash, status);
	else
		return spi_write_status_register(flash, status);
}

enum wp_mode get_wp_mode(const char *mode_str)
{
	enum wp_mode wp_mode = WP_MODE_UNKNOWN;

	if (!strcasecmp(mode_str, "hardware"))
		wp_mode = WP_MODE_HARDWARE;
	else if (!strcasecmp(mode_str, "power_cycle"))
		wp_mode = WP_MODE_POWER_CYCLE;
	else if (!strcasecmp(mode_str, "permanent"))
		wp_mode = WP_MODE_PERMANENT;

	return wp_mode;
}

/* Given a flash chip, this function returns its writeprotect info. */
static int generic_range_table(const struct flashctx *flash,
                           struct wp_context **wp,
                           int *num_entries)
{
	*wp = NULL;
	*num_entries = 0;

	switch (flash->chip->manufacture_id) {
	default:
		msg_cerr("%s: flash vendor (0x%x) not found, aborting\n",
		         __func__, flash->chip->manufacture_id);
		return -1;
	}

	return 0;
}

static uint8_t generic_get_bp_mask(struct wp_context *wp)
{
	return ((1 << (wp->sr1.bp0_pos + wp->sr1.bp_bits)) - 1) ^ \
		  ((1 << wp->sr1.bp0_pos) - 1);
}

static uint8_t generic_get_status_check_mask(struct wp_context *wp)
{
	return generic_get_bp_mask(wp) | 1 << wp->sr1.srp_pos;
}

/* Given a [start, len], this function finds a block protect bit combination
 * (if possible) and sets the corresponding bits in "status". Remaining bits
 * are preserved. */
static int generic_range_to_status(const struct flashctx *flash,
                        unsigned int start, unsigned int len,
                        uint8_t *status, uint8_t *check_mask)
{
	struct wp_context *wp;
	struct wp_range_descriptor *r;
	int i, range_found = 0, num_entries;
	uint8_t bp_mask;

	if (generic_range_table(flash, &wp, &num_entries))
		return -1;

	bp_mask = generic_get_bp_mask(wp);

	for (i = 0, r = &wp->descrs[0]; i < num_entries; i++, r++) {
		msg_cspew("comparing range 0x%x 0x%x / 0x%x 0x%x\n",
			  start, len, r->range.start, r->range.len);
		if ((start == r->range.start) && (len == r->range.len)) {
			*status &= ~(bp_mask);
			*status |= r->bp << (wp->sr1.bp0_pos);

			if (wp->set_modifier_bits) {
				if (wp->set_modifier_bits(flash, &r->m) < 0) {
					msg_cerr("error setting modifier bits for range.\n");
					return -1;
				}
			}

			range_found = 1;
			break;
		}
	}

	if (!range_found) {
		msg_cerr("%s: matching range not found\n", __func__);
		return -1;
	}

	*check_mask = generic_get_status_check_mask(wp);
	return 0;
}

static int generic_status_to_range(const struct flashctx *flash,
		const uint8_t sr1, unsigned int *start, unsigned int *len)
{
	struct wp_context *wp;
	struct wp_range_descriptor *r;
	int num_entries, i, status_found = 0;
	uint8_t sr1_bp;
	struct modifier_bits m;

	if (generic_range_table(flash, &wp, &num_entries))
		return -1;

	/* modifier bits may be compared more than once, so get them here */
	if (wp->get_modifier_bits && wp->get_modifier_bits(flash, &m) < 0)
			return -1;

	sr1_bp = (sr1 >> wp->sr1.bp0_pos) & ((1 << wp->sr1.bp_bits) - 1);

	for (i = 0, r = &wp->descrs[0]; i < num_entries; i++, r++) {
		if (wp->get_modifier_bits) {
			if (memcmp(&m, &r->m, sizeof(m)))
				continue;
		}
		msg_cspew("comparing  0x%02x 0x%02x\n", sr1_bp, r->bp);
		if (sr1_bp == r->bp) {
			*start = r->range.start;
			*len = r->range.len;
			status_found = 1;
			break;
		}
	}

	if (!status_found) {
		msg_cerr("matching status not found\n");
		return -1;
	}

	return 0;
}

/* Given a [start, len], this function calls generic_range_to_status() to
 * convert it to flash-chip-specific range bits, then sets into status register.
 */
static int generic_set_range(const struct flashctx *flash,
                         unsigned int start, unsigned int len)
{
	uint8_t status, expected, check_mask;

	status = do_read_status(flash);
	msg_cdbg("%s: old status: 0x%02x\n", __func__, status);

	expected = status;	/* preserve non-bp bits */
	if (generic_range_to_status(flash, start, len, &expected, &check_mask))
		return -1;

	do_write_status(flash, expected);

	status = do_read_status(flash);
	msg_cdbg("%s: new status: 0x%02x\n", __func__, status);
	if ((status & check_mask) != (expected & check_mask)) {
		msg_cerr("expected=0x%02x, but actual=0x%02x. check mask=0x%02x\n",
		          expected, status, check_mask);
		return 1;
	}
	return 0;
}

/* Set/clear the status regsiter write protect bit in SR1. */
static int generic_set_srp0(const struct flashctx *flash, int enable)
{
	uint8_t status, expected, check_mask;
	struct wp_context *wp;
	int num_entries;

	if (generic_range_table(flash, &wp, &num_entries))
		return -1;

	expected = do_read_status(flash);
	msg_cdbg("%s: old status: 0x%02x\n", __func__, expected);

	if (enable)
		expected |= 1 << wp->sr1.srp_pos;
	else
		expected &= ~(1 << wp->sr1.srp_pos);

	do_write_status(flash, expected);

	status = do_read_status(flash);
	msg_cdbg("%s: new status: 0x%02x\n", __func__, status);

	check_mask = generic_get_status_check_mask(wp);
	msg_cdbg("%s: check mask: 0x%02x\n", __func__, check_mask);
	if ((status & check_mask) != (expected & check_mask)) {
		msg_cerr("expected=0x%02x, but actual=0x%02x. check mask=0x%02x\n",
		          expected, status, check_mask);
		return -1;
	}

	return 0;
}

static int generic_enable_writeprotect(const struct flashctx *flash,
		enum wp_mode wp_mode)
{
	int ret;

	if (wp_mode != WP_MODE_HARDWARE) {
		msg_cerr("%s(): unsupported write-protect mode\n", __func__);
		return 1;
	}

	ret = generic_set_srp0(flash, 1);
	if (ret)
		msg_cerr("%s(): error=%d.\n", __func__, ret);

	return ret;
}

static int generic_disable_writeprotect(const struct flashctx *flash)
{
	int ret;

	ret = generic_set_srp0(flash, 0);
	if (ret)
		msg_cerr("%s(): error=%d.\n", __func__, ret);

	return ret;
}

static int generic_list_ranges(const struct flashctx *flash)
{
	struct wp_context *wp;
	struct wp_range_descriptor *r;
	int i, num_entries;

	if (generic_range_table(flash, &wp, &num_entries))
		return -1;

	r = &wp->descrs[0];
	for (i = 0; i < num_entries; i++) {
		msg_cinfo("start: 0x%06x, length: 0x%06x\n",
		          r->range.start, r->range.len);
		r++;
	}

	return 0;
}

static int wp_context_status(const struct flashctx *flash)
{
	uint8_t sr1;
	unsigned int start, len;
	int ret = 0;
	struct wp_context *wp;
	int num_entries, wp_en;

	if (generic_range_table(flash, &wp, &num_entries))
		return -1;

	sr1 = do_read_status(flash);
	wp_en = (sr1 >> wp->sr1.srp_pos) & 1;

	msg_cinfo("WP: status: 0x%04x\n", sr1);
	msg_cinfo("WP: status.srp0: %x\n", wp_en);
	/* FIXME: SRP1 is not really generic, but we probably should print
	 * it anyway to have consistent output. #legacycruft */
	msg_cinfo("WP: status.srp1: %x\n", 0);
	msg_cinfo("WP: write protect is %s.\n",
		          wp_en ? "enabled" : "disabled");

	msg_cinfo("WP: write protect range: ");
	if (generic_status_to_range(flash, sr1, &start, &len)) {
		msg_cinfo("(cannot resolve the range)\n");
		ret = -1;
	} else {
		msg_cinfo("start=0x%08x, len=0x%08x\n", start, len);
	}

	return ret;
}

struct wp wp_generic = {
	.list_ranges	= generic_list_ranges,
	.set_range	= generic_set_range,
	.enable		= generic_enable_writeprotect,
	.disable	= generic_disable_writeprotect,
	.wp_status	= wp_context_status,
};
