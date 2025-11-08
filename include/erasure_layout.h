/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2022 Aarya Chaumal
 */

#ifndef __ERASURE_LAYOUT_H__
#define __ERASURE_LAYOUT_H__ 1

struct eraseblock_data {
	chipoff_t start_addr;
	chipoff_t end_addr;
	bool selected;
	size_t block_num;
	size_t first_sub_block_index;
	size_t last_sub_block_index;
};

struct erase_layout {
	struct eraseblock_data* layout_list;
	size_t block_count;
	const struct block_eraser *eraser;
};

void free_erase_layout(struct erase_layout *layout, unsigned int erasefn_count);
int create_erase_layout(struct flashctx *const flashctx, struct erase_layout **erase_layout);
int erase_write(struct flashctx *const flashctx, chipoff_t region_start, chipoff_t region_end,
		uint8_t* curcontents, uint8_t* newcontents,
		struct erase_layout *erase_layout, bool *all_skipped);

#endif		/* !__ERASURE_LAYOUT_H__ */
