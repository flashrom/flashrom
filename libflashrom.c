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
/**
 * @mainpage
 *
 * Have a look at the Modules section for a function reference.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "flash.h"
#include "fmap.h"
#include "programmer.h"
#include "layout.h"
#include "hwaccess.h"
#include "ich_descriptors.h"
#include "libflashrom.h"

/**
 * @defgroup flashrom-general General
 * @{
 */

/** Pointer to log callback function. */
static flashrom_log_callback *global_log_callback = NULL;

/**
 * @brief Initialize libflashrom.
 *
 * @param perform_selfcheck If not zero, perform a self check.
 * @return 0 on success
 */
int flashrom_init(const int perform_selfcheck)
{
	if (perform_selfcheck && selfcheck())
		return 1;
	myusec_calibrate_delay();
	return 0;
}

/**
 * @brief Shut down libflashrom.
 * @return 0 on success
 */
int flashrom_shutdown(void)
{
	return 0; /* TODO: nothing to do? */
}

/* TODO: flashrom_set_loglevel()? do we need it?
         For now, let the user decide in his callback. */

/**
 * @brief Set the log callback function.
 *
 * Set a callback function which will be invoked whenever libflashrom wants
 * to output messages. This allows frontends to do whatever they see fit with
 * such messages, e.g. write them to syslog, or to file, or print them in a
 * GUI window, etc.
 *
 * @param log_callback Pointer to the new log callback function.
 */
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

/** @} */ /* end flashrom-general */



/**
 * @defgroup flashrom-query Querying
 * @{
 */

/**
 * @brief Returns flashrom version
 * @return Flashrom version
 */
const char *flashrom_version_info(void)
{
	return flashrom_version;
}

/**
 * @brief Returns list of supported programmers
 * @return List of supported programmers
 */
const char **flashrom_supported_programmers(void)
{
	enum programmer p = 0;
	const char **supported_programmers = NULL;
	supported_programmers = malloc((PROGRAMMER_INVALID + 1) * sizeof(char*));

	if (supported_programmers != NULL) {
		for (; p < PROGRAMMER_INVALID; ++p) {
			supported_programmers[p] = programmer_table[p].name;
		}
	} else {
		msg_gerr("Memory allocation error!\n");
	}

	return supported_programmers;
}

/**
 * @brief Returns list of supported flash chips
 * @return List of supported flash chips
 */
flashrom_flashchip_info *flashrom_supported_flash_chips(void)
{
	int i = 0;
	flashrom_flashchip_info *supported_flashchips = malloc(flashchips_size * sizeof(flashrom_flashchip_info));

	if (supported_flashchips != NULL) {
		for (; i < flashchips_size; ++i) {
			supported_flashchips[i].vendor = flashchips[i].vendor;
			supported_flashchips[i].name = flashchips[i].name;
			supported_flashchips[i].tested.erase = (flashrom_test_state) flashchips[i].tested.erase;
			supported_flashchips[i].tested.probe =  (flashrom_test_state) flashchips[i].tested.probe;
			supported_flashchips[i].tested.read = (flashrom_test_state) flashchips[i].tested.read;
			supported_flashchips[i].tested.write = (flashrom_test_state) flashchips[i].tested.write;
			supported_flashchips[i].total_size = flashchips[i].total_size;
		}
	} else {
		msg_gerr("Memory allocation error!\n");
	}

	return supported_flashchips;
}

/**
 * @brief Returns list of supported mainboards
 * @return List of supported mainboards
 */
flashrom_board_info *flashrom_supported_boards(void)
{
	int boards_known_size = 0;
	int i = 0;
	const struct board_info *binfo = boards_known;

	while ((binfo++)->vendor)
		++boards_known_size;
	binfo = boards_known;
	/* add place for {0} */
	++boards_known_size;

	flashrom_board_info *supported_boards = malloc(boards_known_size * sizeof(flashrom_board_info));

	if (supported_boards != NULL) {
		for (; i < boards_known_size; ++i) {
			supported_boards[i].vendor = binfo[i].vendor;
			supported_boards[i].name = binfo[i].name;
			supported_boards[i].working = binfo[i].working;
		}
	} else {
		msg_gerr("Memory allocation error!\n");
	}

	return supported_boards;
}

/**
 * @brief Returns list of supported chipsets
 * @return List of supported chipsets
 */
flashrom_chipset_info *flashrom_supported_chipsets(void)
{
	int chipset_enables_size = 0;
	int i = 0;
	const struct penable *chipset = chipset_enables;

	while ((chipset++)->vendor_name)
		++chipset_enables_size;
	chipset = chipset_enables;
	/* add place for {0}*/
	++chipset_enables_size;

	flashrom_chipset_info *supported_chipsets = malloc(chipset_enables_size * sizeof(flashrom_chipset_info));

	if (supported_chipsets != NULL) {
		for (; i < chipset_enables_size; ++i) {
			supported_chipsets[i].vendor = chipset[i].vendor_name;
			supported_chipsets[i].chipset = chipset[i].device_name;
			supported_chipsets[i].vendor_id = chipset[i].vendor_id;
			supported_chipsets[i].chipset_id = chipset[i].device_id;
			supported_chipsets[i].status = chipset[i].status;
	  }
	} else {
		msg_gerr("Memory allocation error!\n");
	}

	return supported_chipsets;
}

/**
 * @brief Frees memory allocated by libflashrom API
 * @param Pointer to block of memory which should be freed
 * @return 0 on success
 *         1 on null pointer error
 */
int flashrom_data_free(void *const p)
{
	if (!p) {
		msg_gerr("flashrom_data_free - Null pointer!\n");
		return 1;
	} else {
		free(p);
		return 0;
	}
}

/** @} */ /* end flashrom-query */



/**
 * @defgroup flashrom-prog Programmers
 * @{
 */

/**
 * @brief Initialize the specified programmer.
 *
 * Currently, only one programmer may be initialized at a time.
 *
 * @param[out] flashprog Points to a pointer of type struct flashrom_programmer
 *                       that will be set if programmer initialization succeeds.
 *                       *flashprog has to be shutdown by the caller with @ref
 *                       flashrom_programmer_shutdown.
 * @param[in] prog_name Name of the programmer to initialize.
 * @param[in] prog_param Pointer to programmer specific parameters.
 * @return 0 on success
 */
int flashrom_programmer_init(struct flashrom_programmer **const flashprog,
			     const char *const prog_name, const char *const prog_param)
{
	unsigned prog;

	for (prog = 0; prog < PROGRAMMER_INVALID; prog++) {
		if (strcmp(prog_name, programmer_table[prog].name) == 0)
			break;
	}
	if (prog >= PROGRAMMER_INVALID) {
		msg_ginfo("Error: Unknown programmer \"%s\". Valid choices are:\n", prog_name);
		list_programmers_linebreak(0, 80, 0);
		return 1;
	}
	return programmer_init(prog, prog_param);
}

/**
 * @brief Shut down the initialized programmer.
 *
 * @param flashprog The programmer to shut down.
 * @return 0 on success
 */
int flashrom_programmer_shutdown(struct flashrom_programmer *const flashprog)
{
	return programmer_shutdown();
}

/* TODO: flashrom_programmer_capabilities()? */

/** @} */ /* end flashrom-prog */



/**
 * @defgroup flashrom-flash Flash chips
 * @{
 */

/**
 * @brief Probe for a flash chip.
 *
 * Probes for a flash chip and returns a flash context, that can be used
 * later with flash chip and @ref flashrom-ops "image operations", if
 * exactly one matching chip is found.
 *
 * @param[out] flashctx Points to a pointer of type struct flashrom_flashctx
 *                      that will be set if exactly one chip is found. *flashctx
 *                      has to be freed by the caller with @ref flashrom_flash_release.
 * @param[in] flashprog The flash programmer used to access the chip.
 * @param[in] chip_name Name of a chip to probe for, or NULL to probe for
 *                      all known chips.
 * @return 0 on success,
 *         3 if multiple chips were found,
 *         2 if no chip was found,
 *         or 1 on any other error.
 */
int flashrom_flash_probe(struct flashrom_flashctx **const flashctx,
			 const struct flashrom_programmer *const flashprog,
			 const char *const chip_name)
{
	int i, ret = 2;
	struct flashrom_flashctx second_flashctx = { 0, };

	chip_to_probe = chip_name; /* chip_to_probe is global in flashrom.c */

	*flashctx = malloc(sizeof(**flashctx));
	if (!*flashctx)
		return 1;
	memset(*flashctx, 0, sizeof(**flashctx));

	for (i = 0; i < registered_master_count; ++i) {
		int flash_idx = -1;
		if (!ret || (flash_idx = probe_flash(&registered_masters[i], 0, *flashctx, 0)) != -1) {
			ret = 0;
			/* We found one chip, now check that there is no second match. */
			if (probe_flash(&registered_masters[i], flash_idx + 1, &second_flashctx, 0) != -1) {
				ret = 3;
				break;
			}
		}
	}
	if (ret) {
		free(*flashctx);
		*flashctx = NULL;
	}
	return ret;
}

/**
 * @brief Returns the size of the specified flash chip in bytes.
 *
 * @param flashctx The queried flash context.
 * @return Size of flash chip in bytes.
 */
size_t flashrom_flash_getsize(const struct flashrom_flashctx *const flashctx)
{
	return flashctx->chip->total_size * 1024;
}

/**
 * @brief Free a flash context.
 *
 * @param flashctx Flash context to free.
 */
void flashrom_flash_release(struct flashrom_flashctx *const flashctx)
{
	free(flashctx);
}

/**
 * @brief Set a flag in the given flash context.
 *
 * @param flashctx Flash context to alter.
 * @param flag	   Flag that is to be set / cleared.
 * @param value	   Value to set.
 */
void flashrom_flag_set(struct flashrom_flashctx *const flashctx,
		       const enum flashrom_flag flag, const bool value)
{
	switch (flag) {
		case FLASHROM_FLAG_FORCE:		flashctx->flags.force = value; break;
		case FLASHROM_FLAG_FORCE_BOARDMISMATCH:	flashctx->flags.force_boardmismatch = value; break;
		case FLASHROM_FLAG_VERIFY_AFTER_WRITE:	flashctx->flags.verify_after_write = value; break;
		case FLASHROM_FLAG_VERIFY_WHOLE_CHIP:	flashctx->flags.verify_whole_chip = value; break;
	}
}

/**
 * @brief Return the current value of a flag in the given flash context.
 *
 * @param flashctx Flash context to read from.
 * @param flag	   Flag to be read.
 * @return Current value of the flag.
 */
bool flashrom_flag_get(const struct flashrom_flashctx *const flashctx, const enum flashrom_flag flag)
{
	switch (flag) {
		case FLASHROM_FLAG_FORCE:		return flashctx->flags.force;
		case FLASHROM_FLAG_FORCE_BOARDMISMATCH:	return flashctx->flags.force_boardmismatch;
		case FLASHROM_FLAG_VERIFY_AFTER_WRITE:	return flashctx->flags.verify_after_write;
		case FLASHROM_FLAG_VERIFY_WHOLE_CHIP:	return flashctx->flags.verify_whole_chip;
		default:				return false;
	}
}

/** @} */ /* end flashrom-flash */



/**
 * @defgroup flashrom-layout Layout handling
 * @{
 */

/**
 * @brief Read a layout from the Intel ICH descriptor in the flash.
 *
 * Optionally verify that the layout matches the one in the given
 * descriptor dump.
 *
 * @param[out] layout Points to a struct flashrom_layout pointer that
 *                    gets set if the descriptor is read and parsed
 *                    successfully.
 * @param[in] flashctx Flash context to read the descriptor from flash.
 * @param[in] dump     The descriptor dump to compare to or NULL.
 * @param[in] len      The length of the descriptor dump.
 *
 * @return 0 on success,
 *         6 if descriptor parsing isn't implemented for the host,
 *         5 if the descriptors don't match,
 *         4 if the descriptor dump couldn't be parsed,
 *         3 if the descriptor on flash couldn't be parsed,
 *         2 if the descriptor on flash couldn't be read,
 *         1 on any other error.
 */
int flashrom_layout_read_from_ifd(struct flashrom_layout **const layout, struct flashctx *const flashctx,
				  const void *const dump, const size_t len)
{
#ifndef __FLASHROM_LITTLE_ENDIAN__
	return 6;
#else
	struct ich_layout dump_layout;
	int ret = 1;

	void *const desc = malloc(0x1000);
	struct ich_layout *const chip_layout = malloc(sizeof(*chip_layout));
	if (!desc || !chip_layout) {
		msg_gerr("Out of memory!\n");
		goto _free_ret;
	}

	if (prepare_flash_access(flashctx, true, false, false, false))
		goto _free_ret;

	msg_cinfo("Reading ich descriptor... ");
	if (flashctx->chip->read(flashctx, desc, 0, 0x1000)) {
		msg_cerr("Read operation failed!\n");
		msg_cinfo("FAILED.\n");
		ret = 2;
		goto _finalize_ret;
	}
	msg_cinfo("done.\n");

	if (layout_from_ich_descriptors(chip_layout, desc, 0x1000)) {
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

		if (chip_layout->base.num_entries != dump_layout.base.num_entries ||
		    memcmp(chip_layout->entries, dump_layout.entries, sizeof(dump_layout.entries))) {
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
		free(chip_layout);
	free(desc);
	return ret;
#endif
}

#ifdef __FLASHROM_LITTLE_ENDIAN__
static int flashrom_layout_parse_fmap(struct flashrom_layout **layout,
		struct flashctx *const flashctx, const struct fmap *const fmap)
{
	int i;
	struct flashrom_layout *l = get_global_layout();

	if (!fmap || !l)
		return 1;

	if (l->num_entries + fmap->nareas > MAX_ROMLAYOUT) {
		msg_gerr("Cannot add fmap entries to layout - Too many entries.\n");
		return 1;
	}

	for (i = 0; i < fmap->nareas; i++) {
		l->entries[l->num_entries].start = fmap->areas[i].offset;
		l->entries[l->num_entries].end = fmap->areas[i].offset + fmap->areas[i].size - 1;
		l->entries[l->num_entries].included = false;
		l->entries[l->num_entries].name =
			strndup((const char *)fmap->areas[i].name, FMAP_STRLEN);
		if (!l->entries[l->num_entries].name) {
			msg_gerr("Error adding layout entry: %s\n", strerror(errno));
			return 1;
		}
		msg_gdbg("fmap %08x - %08x named %s\n",
			l->entries[l->num_entries].start,
			l->entries[l->num_entries].end,
			l->entries[l->num_entries].name);
		l->num_entries++;
	}

	*layout = l;
	return 0;
}
#endif /* __FLASHROM_LITTLE_ENDIAN__ */

/**
 * @brief Read a layout by searching the flash chip for fmap.
 *
 * @param[out] layout Points to a struct flashrom_layout pointer that
 *                    gets set if the fmap is read and parsed successfully.
 * @param[in] flashctx Flash context
 * @param[in] offset Offset to begin searching for fmap.
 * @param[in] offset Length of address space to search.
 *
 * @return 0 on success,
 *         3 if fmap parsing isn't implemented for the host,
 *         2 if the fmap couldn't be read,
 *         1 on any other error.
 */
int flashrom_layout_read_fmap_from_rom(struct flashrom_layout **const layout,
		struct flashctx *const flashctx, off_t offset, size_t len)
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

/**
 * @brief Read a layout by searching buffer for fmap.
 *
 * @param[out] layout Points to a struct flashrom_layout pointer that
 *                    gets set if the fmap is read and parsed successfully.
 * @param[in] flashctx Flash context
 * @param[in] buffer Buffer to search in
 * @param[in] size Size of buffer to search
 *
 * @return 0 on success,
 *         3 if fmap parsing isn't implemented for the host,
 *         2 if the fmap couldn't be read,
 *         1 on any other error.
 */
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

/**
 * @brief Set the active layout for a flash context.
 *
 * Note: This just sets a pointer. The caller must not release the layout
 *       as long as he uses it through the given flash context.
 *
 * @param flashctx Flash context whose layout will be set.
 * @param layout   Layout to bet set.
 */
void flashrom_layout_set(struct flashrom_flashctx *const flashctx, const struct flashrom_layout *const layout)
{
	flashctx->layout = layout;
}

/** @} */ /* end flashrom-layout */
