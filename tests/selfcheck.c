/*
 * This file is part of the flashrom project.
 *
 * Copyright 2022 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "flash.h"
#include "string.h"

#include "include/test.h"
#include "programmer.h"
#include "tests.h"
#include <cmocka.h>


#define assert_table(assertion, message, index, name)                                                          \
	do {                                                                                                   \
		if (!(assertion))                                                                              \
			fail_msg(message " for index:%zu name:%s", (index), (name) ? (name) : "unknown");      \
	} while (0)


void selfcheck_programmer_table(void **state)
{
	(void)state; /* unused */

	size_t i;
	for (i = 0; i < programmer_table_size; i++) {
		const struct programmer_entry *const p = programmer_table[i];
		assert_table(p, "programmer entry is null", i, "unknown");
		assert_table(p->name, "programmer name is null", i, p->name);
		bool type_good = false;
		switch (p->type) {
		case PCI:
		case USB:
		case OTHER:
			type_good = true;
		}
		assert_table(type_good, "programmer type is invalid", i, p->name);
		/* internal has its device list stored separately. */
		if (strcmp("internal", p->name) != 0)
			assert_table(p->devs.note, "programmer devs.note is null", i, p->name);
		assert_table(p->init, "programmer init is null", i, p->name);
	}
}

void selfcheck_flashchips_table(void **state)
{
	(void)state; /* unused */

	size_t i;
	assert_true(flashchips_size > 1);
	assert_true(flashchips[flashchips_size - 1].name == NULL);
	for (i = 0; i < flashchips_size - 1; i++) {
		const struct flashchip *chip = &flashchips[i];
		assert_table(chip->vendor, "chip vendor is null", i, chip->name);
		assert_table(chip->name, "chip name is null", i, chip->name);
		assert_table(chip->bustype != BUS_NONE, "chip bustype is BUS_NONE", i, chip->name);
	}
}

void selfcheck_eraseblocks(void **state)
{
	(void)state; /* unused */

	size_t chip_index;
	for (chip_index = 0; chip_index < flashchips_size - 1; chip_index++) {
		size_t i, j, k;
		const struct flashchip *chip = &flashchips[chip_index];
		unsigned int prev_eraseblock_count = chip->total_size * 1024;

		for (k = 0; k < NUM_ERASEFUNCTIONS; k++) {
			unsigned int done = 0;
			struct block_eraser eraser = chip->block_erasers[k];
			unsigned int curr_eraseblock_count = 0;

			for (i = 0; i < NUM_ERASEREGIONS; i++) {
				/* Blocks with zero size are bugs in flashchips.c. */
				if (eraser.eraseblocks[i].count && !eraser.eraseblocks[i].size) {
					fail_msg("Flash chip %s erase function %zu region %zu has size 0",
						 chip->name, k, i);
				}
				/* Blocks with zero count are bugs in flashchips.c. */
				if (!eraser.eraseblocks[i].count && eraser.eraseblocks[i].size) {
					fail_msg("Flash chip %s erase function %zu region %zu has count 0",
						 chip->name, k, i);
				}
				done += eraser.eraseblocks[i].count * eraser.eraseblocks[i].size;
				curr_eraseblock_count += eraser.eraseblocks[i].count;
			}
			/* Empty eraseblock definition with erase function.  */
			if (!done && eraser.block_erase) {
				printf("Strange: Empty eraseblock definition with non-empty erase function chip %s function %zu. Not an error.\n",
				       chip->name, k);
			}

			if (!done)
				continue;
			if (done != chip->total_size * 1024) {
				fail_msg(
					"Flash chip %s erase function %zu region walking resulted in 0x%06x bytes total, expected 0x%06x bytes.",
					chip->name, k, done, chip->total_size * 1024);
				assert_true(false);
			}

			if (!eraser.block_erase)
				continue;
			/* Check if there are identical erase functions for different
			 * layouts. That would imply "magic" erase functions. The
			 * easiest way to check this is with function pointers.
			 */
			for (j = k + 1; j < NUM_ERASEFUNCTIONS; j++) {
				if (eraser.block_erase == chip->block_erasers[j].block_erase) {
					fail_msg("Flash chip %s erase function %zu and %zu are identical.",
						 chip->name, k, j);
				}
			}
			if (curr_eraseblock_count > prev_eraseblock_count) {
				fail_msg("Flash chip %s erase function %zu is not in order", chip->name, k);
			}
			prev_eraseblock_count = curr_eraseblock_count;
		}
	}
}

#if CONFIG_INTERNAL == 1
void selfcheck_board_matches_table(void **state)
{
	(void)state; /* unused */

	size_t i;

	assert_true(board_matches_size > 0);
	assert_true(board_matches[board_matches_size - 1].vendor_name == NULL);
	for (i = 0; i < board_matches_size - 1; i++) {
		const struct board_match *b = &board_matches[i];
		assert_table(b->vendor_name, "board vendor_name is null", i, b->board_name);
		assert_table(b->board_name, "board boad_name is null", i, b->board_name);
		if ((!b->first_vendor || !b->first_device || !b->second_vendor || !b->second_device)
		    || ((!b->lb_vendor) ^ (!b->lb_part)) || (!b->max_rom_decode_parallel && !b->enable))
			fail_msg("Board enable for %s %s is misdefined.\n", b->vendor_name, b->board_name);
	}
}

#else
	SKIP_TEST(selfcheck_board_matches_table)
#endif /* CONFIG_INTERNAL */
