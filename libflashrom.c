/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2012, 2016 secunet Security Networks AG
 * (Written by Nico Huber <nico.huber@secunet.com> for secunet)
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
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "flash.h"
#include "fmap.h"
#include "programmer.h"
#include "layout.h"
#include "ich_descriptors.h"
#include "libflashrom.h"
#include "writeprotect.h"


/** Pointer to log callback function. */
static flashrom_log_callback *global_log_callback = NULL;

int flashrom_init(const int perform_selfcheck)
{
	if (perform_selfcheck && selfcheck())
		return 1;
	return 0;
}

int flashrom_shutdown(void)
{
	return 0; /* TODO: nothing to do? */
}

/* TODO: flashrom_set_loglevel()? do we need it?
         For now, let the user decide in their callback. */
void flashrom_set_log_callback(flashrom_log_callback *const log_callback)
{
	global_log_callback = log_callback;
}
/** @private */
int print(const enum flashrom_log_level level, const char *const fmt, ...)
{
	if (global_log_callback) {
		int ret;
		va_list args;
		va_start(args, fmt);
		ret = global_log_callback(level, fmt, args);
		va_end(args);
		return ret;
	}
	return 0;
}

void flashrom_set_progress_callback(struct flashrom_flashctx *flashctx, flashrom_progress_callback *progress_callback, struct flashrom_progress *progress_state)
{
	flashctx->progress_callback = progress_callback;
	flashctx->progress_state = progress_state;
}
/** @private */
void update_progress(struct flashrom_flashctx *flashctx, enum flashrom_progress_stage stage, size_t current, size_t total)
{
	if (flashctx->progress_callback == NULL)
		return;
	if (current > total)
		current = total;

	flashctx->progress_state->stage = stage;
	flashctx->progress_state->current = current;
	flashctx->progress_state->total = total;
	flashctx->progress_callback(flashctx);
}

const char *flashrom_version_info(void)
{
	return flashrom_version;
}

struct flashrom_flashchip_info *flashrom_supported_flash_chips(void)
{
	struct flashrom_flashchip_info *supported_flashchips =
		malloc(flashchips_size * sizeof(*supported_flashchips));

	if (!supported_flashchips) {
		msg_gerr("Memory allocation error!\n");
		return NULL;
	}

	for (unsigned int i = 0; i < flashchips_size; ++i) {
		supported_flashchips[i].vendor = flashchips[i].vendor;
		supported_flashchips[i].name = flashchips[i].name;
		supported_flashchips[i].tested.erase =
			(enum flashrom_test_state)flashchips[i].tested.erase;
		supported_flashchips[i].tested.probe =
			(enum flashrom_test_state)flashchips[i].tested.probe;
		supported_flashchips[i].tested.read =
			(enum flashrom_test_state)flashchips[i].tested.read;
		supported_flashchips[i].tested.write =
			(enum flashrom_test_state)flashchips[i].tested.write;
		supported_flashchips[i].total_size = flashchips[i].total_size;
	}

	return supported_flashchips;
}

struct flashrom_board_info *flashrom_supported_boards(void)
{
#if CONFIG_INTERNAL == 1
	int boards_known_size = 0;
	const struct board_info *binfo = boards_known;

	while ((binfo++)->vendor)
		++boards_known_size;
	binfo = boards_known;
	/* add place for {0} */
	++boards_known_size;

	struct flashrom_board_info *supported_boards =
		malloc(boards_known_size * sizeof(*supported_boards));

	if (!supported_boards) {
		msg_gerr("Memory allocation error!\n");
		return NULL;
	}

	for (int i = 0; i < boards_known_size; ++i) {
		supported_boards[i].vendor = binfo[i].vendor;
		supported_boards[i].name = binfo[i].name;
		supported_boards[i].working =
			(enum flashrom_test_state) binfo[i].working;
	}

	return supported_boards;
#else
	return NULL;
#endif
}

struct flashrom_chipset_info *flashrom_supported_chipsets(void)
{
#if CONFIG_INTERNAL == 1
	int chipset_enables_size = 0;
	const struct penable *chipset = chipset_enables;

	while ((chipset++)->vendor_name)
		++chipset_enables_size;
	chipset = chipset_enables;
	/* add place for {0}*/
	++chipset_enables_size;

	struct flashrom_chipset_info *supported_chipsets =
		malloc(chipset_enables_size * sizeof(*supported_chipsets));

	if (!supported_chipsets) {
		msg_gerr("Memory allocation error!\n");
		return NULL;
	}

	for (int i = 0; i < chipset_enables_size; ++i) {
		supported_chipsets[i].vendor = chipset[i].vendor_name;
		supported_chipsets[i].chipset = chipset[i].device_name;
		supported_chipsets[i].vendor_id = chipset[i].vendor_id;
		supported_chipsets[i].chipset_id = chipset[i].device_id;
		supported_chipsets[i].status =
			(enum flashrom_test_state) chipset[i].status;
	}

	return supported_chipsets;
#else
	return NULL;
#endif
}

int flashrom_data_free(void *const p)
{
	free(p);
	return 0;
}

int flashrom_programmer_init(struct flashrom_programmer **const flashprog,
			     const char *const prog_name, const char *const prog_param)
{
	unsigned prog;

	for (prog = 0; prog < programmer_table_size; prog++) {
		if (strcmp(prog_name, programmer_table[prog]->name) == 0)
			break;
	}
	if (prog >= programmer_table_size) {
		msg_ginfo("Error: Unknown programmer \"%s\". Valid choices are:\n", prog_name);
		list_programmers_linebreak(0, 80, 0);
		return 1;
	}
	return programmer_init(programmer_table[prog], prog_param);
}

int flashrom_programmer_shutdown(struct flashrom_programmer *const flashprog)
{
	return programmer_shutdown();
}

/* TODO: flashrom_programmer_capabilities()? */

int flashrom_flash_probe(struct flashrom_flashctx **const flashctx,
			 const struct flashrom_programmer *const flashprog,
			 const char *const chip_name)
{
	int i, ret = 2;
	struct flashrom_flashctx second_flashctx = { 0, };

	*flashctx = malloc(sizeof(**flashctx));
	if (!*flashctx)
		return 1;
	memset(*flashctx, 0, sizeof(**flashctx));

	for (i = 0; i < registered_master_count; ++i) {
		int flash_idx = -1;
		if (!ret || (flash_idx = probe_flash(&registered_masters[i], 0, *flashctx, 0, chip_name)) != -1) {
			ret = 0;
			/* We found one chip, now check that there is no second match. */
			if (probe_flash(&registered_masters[i], flash_idx + 1, &second_flashctx, 0, chip_name) != -1) {
				flashrom_layout_release(second_flashctx.default_layout);
				free(second_flashctx.chip);
				ret = 3;
				break;
			}
		}
	}
	if (ret) {
		flashrom_flash_release(*flashctx);
		*flashctx = NULL;
	}
	return ret;
}

size_t flashrom_flash_getsize(const struct flashrom_flashctx *const flashctx)
{
	return flashctx->chip->total_size * 1024;
}

void flashrom_flash_release(struct flashrom_flashctx *const flashctx)
{
	if (!flashctx)
		return;

	flashrom_layout_release(flashctx->default_layout);
	free(flashctx->chip);
	free(flashctx);
}

void flashrom_flag_set(struct flashrom_flashctx *const flashctx,
		       const enum flashrom_flag flag, const bool value)
{
	switch (flag) {
		case FLASHROM_FLAG_FORCE:			flashctx->flags.force = value; break;
		case FLASHROM_FLAG_FORCE_BOARDMISMATCH:		flashctx->flags.force_boardmismatch = value; break;
		case FLASHROM_FLAG_VERIFY_AFTER_WRITE:		flashctx->flags.verify_after_write = value; break;
		case FLASHROM_FLAG_VERIFY_WHOLE_CHIP:		flashctx->flags.verify_whole_chip = value; break;
		case FLASHROM_FLAG_SKIP_UNREADABLE_REGIONS:	flashctx->flags.skip_unreadable_regions = value; break;
		case FLASHROM_FLAG_SKIP_UNWRITABLE_REGIONS:	flashctx->flags.skip_unwritable_regions = value; break;
	}
}

bool flashrom_flag_get(const struct flashrom_flashctx *const flashctx, const enum flashrom_flag flag)
{
	switch (flag) {
		case FLASHROM_FLAG_FORCE:			return flashctx->flags.force;
		case FLASHROM_FLAG_FORCE_BOARDMISMATCH:		return flashctx->flags.force_boardmismatch;
		case FLASHROM_FLAG_VERIFY_AFTER_WRITE:		return flashctx->flags.verify_after_write;
		case FLASHROM_FLAG_VERIFY_WHOLE_CHIP:		return flashctx->flags.verify_whole_chip;
		case FLASHROM_FLAG_SKIP_UNREADABLE_REGIONS:	return flashctx->flags.skip_unreadable_regions;
		case FLASHROM_FLAG_SKIP_UNWRITABLE_REGIONS:	return flashctx->flags.skip_unwritable_regions;
		default:					return false;
	}
}

int flashrom_layout_read_from_ifd(struct flashrom_layout **const layout, struct flashctx *const flashctx,
				  const void *const dump, const size_t len)
{
#ifndef __FLASHROM_LITTLE_ENDIAN__
	return 6;
#else
	struct flashrom_layout *dump_layout = NULL, *chip_layout = NULL;
	int ret = 1;

	void *const desc = malloc(0x1000);
	if (prepare_flash_access(flashctx, true, false, false, false))
		goto _free_ret;

	msg_cinfo("Reading ich descriptor... ");
	if (read_flash(flashctx, desc, 0, 0x1000)) {
		msg_cerr("Read operation failed!\n");
		msg_cinfo("FAILED.\n");
		ret = 2;
		goto _finalize_ret;
	}
	msg_cinfo("done.\n");

	if (layout_from_ich_descriptors(&chip_layout, desc, 0x1000)) {
		msg_cerr("Couldn't parse the descriptor!\n");
		ret = 3;
		goto _finalize_ret;
	}

	if (dump) {
		if (layout_from_ich_descriptors(&dump_layout, dump, len)) {
			msg_cerr("Couldn't parse the descriptor!\n");
			ret = 4;
			goto _finalize_ret;
		}

		const struct romentry *chip_entry = layout_next(chip_layout, NULL);
		const struct romentry *dump_entry = layout_next(dump_layout, NULL);
		while (chip_entry && dump_entry && !memcmp(chip_entry, dump_entry, sizeof(*chip_entry))) {
			chip_entry = layout_next(chip_layout, chip_entry);
			dump_entry = layout_next(dump_layout, dump_entry);
		}
		flashrom_layout_release(dump_layout);
		if (chip_entry || dump_entry) {
			msg_cerr("Descriptors don't match!\n");
			ret = 5;
			goto _finalize_ret;
		}
	}

	*layout = (struct flashrom_layout *)chip_layout;
	ret = 0;

_finalize_ret:
	finalize_flash_access(flashctx);
_free_ret:
	if (ret)
		flashrom_layout_release(chip_layout);
	free(desc);
	return ret;
#endif
}

#ifdef __FLASHROM_LITTLE_ENDIAN__
static int flashrom_layout_parse_fmap(struct flashrom_layout **layout,
		struct flashctx *const flashctx, const struct fmap *const fmap)
{
	int i;
	char name[FMAP_STRLEN + 1];
	const struct fmap_area *area;
	struct flashrom_layout *l;

	if (!fmap || flashrom_layout_new(&l))
		return 1;

	for (i = 0, area = fmap->areas; i < fmap->nareas; i++, area++) {
		if (area->size == 0) {
			/* Layout regions use inclusive upper and lower bounds,
			 * so it's impossible to represent a region with zero
			 * size although it's allowed in fmap. */
			msg_gwarn("Ignoring zero-size fmap region \"%s\";"
				  " empty regions are unsupported.\n",
				  area->name);
			continue;
		}

		snprintf(name, sizeof(name), "%s", area->name);
		if (flashrom_layout_add_region(l, area->offset, area->offset + area->size - 1, name)) {
			flashrom_layout_release(l);
			return 1;
		}
	}

	*layout = l;
	return 0;
}
#endif /* __FLASHROM_LITTLE_ENDIAN__ */

int flashrom_layout_read_fmap_from_rom(struct flashrom_layout **const layout,
		struct flashctx *const flashctx, size_t offset, size_t len)
{
#ifndef __FLASHROM_LITTLE_ENDIAN__
	return 3;
#else
	struct fmap *fmap = NULL;
	int ret = 0;

	msg_gdbg("Attempting to read fmap from ROM content.\n");
	if (fmap_read_from_rom(&fmap, flashctx, offset, len)) {
		msg_gerr("Failed to read fmap from ROM.\n");
		return 1;
	}

	msg_gdbg("Adding fmap layout to global layout.\n");
	if (flashrom_layout_parse_fmap(layout, flashctx, fmap)) {
		msg_gerr("Failed to add fmap regions to layout.\n");
		ret = 1;
	}

	free(fmap);
	return ret;
#endif
}

int flashrom_layout_read_fmap_from_buffer(struct flashrom_layout **const layout,
		struct flashctx *const flashctx, const uint8_t *const buf, size_t size)
{
#ifndef __FLASHROM_LITTLE_ENDIAN__
	return 3;
#else
	struct fmap *fmap = NULL;
	int ret = 1;

	if (!buf || !size)
		goto _ret;

	msg_gdbg("Attempting to read fmap from buffer.\n");
	if (fmap_read_from_buffer(&fmap, buf, size)) {
		msg_gerr("Failed to read fmap from buffer.\n");
		goto _ret;
	}

	msg_gdbg("Adding fmap layout to global layout.\n");
	if (flashrom_layout_parse_fmap(layout, flashctx, fmap)) {
		msg_gerr("Failed to add fmap regions to layout.\n");
		goto _free_ret;
	}

	ret = 0;
_free_ret:
	free(fmap);
_ret:
	return ret;
#endif
}

void flashrom_layout_set(struct flashrom_flashctx *const flashctx, const struct flashrom_layout *const layout)
{
	flashctx->layout = layout;
}

enum flashrom_wp_result flashrom_wp_cfg_new(struct flashrom_wp_cfg **cfg)
{
	*cfg = calloc(1, sizeof(**cfg));
	return *cfg ? 0 : FLASHROM_WP_ERR_OTHER;
}

void flashrom_wp_cfg_release(struct flashrom_wp_cfg *cfg)
{
	free(cfg);
}

void flashrom_wp_set_mode(struct flashrom_wp_cfg *cfg, enum flashrom_wp_mode mode)
{
	cfg->mode = mode;
}

enum flashrom_wp_mode flashrom_wp_get_mode(const struct flashrom_wp_cfg *cfg)
{
	return cfg->mode;
}

void flashrom_wp_set_range(struct flashrom_wp_cfg *cfg, size_t start, size_t len)
{
	cfg->range.start = start;
	cfg->range.len = len;
}

void flashrom_wp_get_range(size_t *start, size_t *len, const struct flashrom_wp_cfg *cfg)
{
	*start = cfg->range.start;
	*len = cfg->range.len;
}

enum flashrom_wp_result flashrom_wp_write_cfg(struct flashctx *flash, const struct flashrom_wp_cfg *cfg)
{
	if (flash->mst->buses_supported & BUS_PROG && flash->mst->opaque.wp_write_cfg)
		return flash->mst->opaque.wp_write_cfg(flash, cfg);

	if (wp_operations_available(flash))
		return wp_write_cfg(flash, cfg);

	return FLASHROM_WP_ERR_OTHER;
}

enum flashrom_wp_result flashrom_wp_read_cfg(struct flashrom_wp_cfg *cfg, struct flashctx *flash)
{
	if (flash->mst->buses_supported & BUS_PROG && flash->mst->opaque.wp_read_cfg)
		return flash->mst->opaque.wp_read_cfg(cfg, flash);

	if (wp_operations_available(flash))
		return wp_read_cfg(cfg, flash);

	return FLASHROM_WP_ERR_OTHER;
}

enum flashrom_wp_result flashrom_wp_get_available_ranges(struct flashrom_wp_ranges **list, struct flashrom_flashctx *flash)
{
	if (flash->mst->buses_supported & BUS_PROG && flash->mst->opaque.wp_get_ranges)
		return flash->mst->opaque.wp_get_ranges(list, flash);

	if (wp_operations_available(flash))
		return wp_get_available_ranges(list, flash);

	return FLASHROM_WP_ERR_OTHER;
}

size_t flashrom_wp_ranges_get_count(const struct flashrom_wp_ranges *list)
{
	return list->count;
}

enum flashrom_wp_result flashrom_wp_ranges_get_range(size_t *start, size_t *len, const struct flashrom_wp_ranges *list, unsigned int index)
{
	if (index >= list->count)
		return FLASHROM_WP_ERR_OTHER;

	*start = list->ranges[index].start;
	*len = list->ranges[index].len;

	return 0;
}

void flashrom_wp_ranges_release(struct flashrom_wp_ranges *list)
{
	if (!list)
		return;

	free(list->ranges);
	free(list);
}
