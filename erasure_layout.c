/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2022 Aarya Chaumal
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

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "flash.h"
#include "layout.h"
#include "erasure_layout.h"

static size_t calculate_block_count(const struct flashchip *chip, size_t eraser_idx)
{
	size_t block_count = 0;

	chipoff_t addr = 0;
	for (size_t i = 0; addr < chip->total_size * 1024; i++) {
		const struct eraseblock *block = &chip->block_erasers[eraser_idx].eraseblocks[i];
		block_count += block->count;
		addr += block->size * block->count;
	}

	return block_count;
}

static void init_eraseblock(struct erase_layout *layout, size_t idx, size_t block_num,
		chipoff_t start_addr, chipoff_t end_addr, size_t *sub_block_index)
{
	struct eraseblock_data *edata = &layout[idx].layout_list[block_num];
	edata->start_addr = start_addr;
	edata->end_addr = end_addr;
	edata->selected = false;
	edata->block_num = block_num;

	if (!idx)
		return;

	edata->first_sub_block_index = *sub_block_index;
	struct eraseblock_data *subedata = &layout[idx - 1].layout_list[*sub_block_index];
	while (subedata->start_addr >= start_addr && subedata->end_addr <= end_addr &&
		*sub_block_index < layout[idx-1].block_count) {
		(*sub_block_index)++;
		subedata++;
	}
	edata->last_sub_block_index = *sub_block_index - 1;
}

/*
 * @brief Function to free the created erase_layout
 *
 * @param layout pointer to allocated layout
 * @param erasefn_count number of erase functions for which the layout was created
 *
 */
void free_erase_layout(struct erase_layout *layout, unsigned int erasefn_count)
{
	if (!layout)
		return;
	for (size_t i = 0; i < erasefn_count; i++) {
		free(layout[i].layout_list);
	}
	free(layout);
}

/*
 * @brief Function to create an erase layout
 *
 * @param	flashctx	flash context
 * @param	e_layout	address to the pointer to store the layout
 * @return	0 on success,
 *		-1 if layout creation fails
 *
 * This function creates a layout of which erase functions erase which regions
 * of the flash chip. This helps to optimally select the erase functions for
 * erase/write operations.
 */
int create_erase_layout(struct flashctx *const flashctx, struct erase_layout **e_layout)
{
	const struct flashchip *chip = flashctx->chip;
	const size_t erasefn_count = count_usable_erasers(flashctx);
	if (!erasefn_count) {
		msg_gerr("No erase functions supported\n");
		return 0;
	}

	struct erase_layout *layout = calloc(erasefn_count, sizeof(struct erase_layout));
	if (!layout) {
		msg_gerr("Out of memory!\n");
		return -1;
	}

	size_t layout_idx = 0;
	for (size_t eraser_idx = 0; eraser_idx < NUM_ERASEFUNCTIONS; eraser_idx++) {
		if (check_block_eraser(flashctx, eraser_idx, 0))
			continue;

		layout[layout_idx].eraser = &chip->block_erasers[eraser_idx];
		const size_t block_count = calculate_block_count(flashctx->chip, eraser_idx);
		size_t sub_block_index = 0;

		layout[layout_idx].block_count = block_count;
		layout[layout_idx].layout_list = (struct eraseblock_data *)calloc(block_count,
									sizeof(struct eraseblock_data));

		if (!layout[layout_idx].layout_list) {
			free_erase_layout(layout, layout_idx);
			return -1;
		}

		size_t block_num = 0;
		chipoff_t start_addr = 0;

		for (int i = 0; block_num < block_count;  i++) {
			const struct eraseblock *block = &chip->block_erasers[eraser_idx].eraseblocks[i];

			for (size_t num = 0; num < block->count; num++) {
				chipoff_t end_addr = start_addr + block->size - 1;
				init_eraseblock(layout, layout_idx, block_num,
						start_addr, end_addr, &sub_block_index);
				block_num += 1;
				start_addr = end_addr + 1;
			}
		}
		layout_idx++;
	}

	*e_layout = layout;
	return layout_idx;
}

/*
 * @brief	Function to align start and address of the region boundaries
 *
 * @param	layout	erase layout
 * @param	flashctx	flash context
 * @param	region_start	pointer to start address of the region to align
 * @param	region_end	pointer to end address of the region to align
 *
 * This function aligns start and end address of the region (in struct walk_info)
 * to some erase sector boundaries and modify the region start and end addresses
 * to match nearest erase sector boundaries. This function will be used in the
 * new algorithm for erase function selection.
 */
static void align_region(const struct erase_layout *layout, struct flashctx *const flashctx,
			chipoff_t *region_start, chipoff_t *region_end)
{
	chipoff_t start_diff = UINT_MAX, end_diff = UINT_MAX;
	const size_t erasefn_count = count_usable_erasers(flashctx);
	for (size_t i = 0; i < erasefn_count; i++) {
		for (size_t j = 0; j < layout[i].block_count; j++) {
			const struct eraseblock_data *ll = &layout[i].layout_list[j];
			if (ll->start_addr <= *region_start)
				start_diff = (*region_start - ll->start_addr) > start_diff ?
						start_diff : (*region_start - ll->start_addr);
			if (ll->end_addr >= *region_end)
				end_diff = (ll->end_addr - *region_end) > end_diff ?
						end_diff : (ll->end_addr - *region_end);
		}
	}

	if (start_diff) {
		msg_cinfo("Region start not sector aligned! Extending start boundaries...\n");
		*region_start = *region_start - start_diff;
	}
	if (end_diff) {
		msg_cinfo("Region end not sector aligned! Extending end boundaries...\n");
		*region_end = *region_end + end_diff;
	}
}

/*
 * @brief	Function to select the list of sectors that need erasing
 *
 * @param	flashctx	flash context
 * @param	layout		erase layout
 * @param	findex		index of the erase function
 * @param	block_num	index of the block to erase according to the erase function index
 * @param	curcontents	buffer containg the current contents of the flash
 * @param	newcontents	buffer containg the new contents of the flash
 * @param	rstart		start address of the region
 * @rend	rend		end address of the region
 */
static void select_erase_functions(struct flashctx *flashctx, const struct erase_layout *layout,
				size_t findex, size_t block_num, uint8_t *curcontents, uint8_t *newcontents,
				chipoff_t rstart, chipoff_t rend)
{
	struct eraseblock_data *ll = &layout[findex].layout_list[block_num];
	if (!findex) {
		if (ll->start_addr >= rstart && ll->end_addr <= rend) {
			chipoff_t start_addr = ll->start_addr;
			chipoff_t end_addr = ll->end_addr;
			const chipsize_t erase_len = end_addr - start_addr + 1;
			const uint8_t erased_value = ERASED_VALUE(flashctx);
			ll->selected = need_erase(curcontents + start_addr, newcontents + start_addr, erase_len,
						flashctx->chip->gran, erased_value);
		}
	} else {
		int count = 0;
		const int sub_block_start = ll->first_sub_block_index;
		const int sub_block_end = ll->last_sub_block_index;

		for (int j = sub_block_start; j <= sub_block_end; j++) {
			select_erase_functions(flashctx, layout, findex - 1, j, curcontents, newcontents,
						rstart, rend);
			if (layout[findex - 1].layout_list[j].selected)
				count++;
		}

		const int total_blocks = sub_block_end - sub_block_start + 1;
		if (count && count > total_blocks/2) {
			if (ll->start_addr >= rstart && ll->end_addr <= rend) {
				for (int j = sub_block_start; j <= sub_block_end; j++)
					layout[findex - 1].layout_list[j].selected = false;
				ll->selected = true;
			}
		}
	}
}

/*
 * @brief	wrapper to use the erase algorithm
 *
 * @param	flashctx	flash context
 * @param	region_start	start address of the region
 * @param	region_end	end address of the region
 * @param       curcontents     buffer containg the current contents of the flash
 * @param       newcontents     buffer containg the new contents of the flash
 * @param	erase_layout	erase layout
 * @param	all_skipped	pointer to the flag to chec if any block was erased
 */
int erase_write(struct flashctx *const flashctx, chipoff_t region_start, chipoff_t region_end,
		uint8_t *curcontents, uint8_t *newcontents,
		struct erase_layout *erase_layout, bool *all_skipped)
{
	const size_t erasefn_count = count_usable_erasers(flashctx);
	int ret = 0;
	size_t i;
	chipoff_t old_start = region_start, old_end = region_end;
	align_region(erase_layout, flashctx, &region_start, &region_end);

	uint8_t *old_start_buf = NULL, *old_end_buf = NULL;
	old_start_buf = (uint8_t *)malloc(old_start - region_start);
	if (!old_start_buf) {
		msg_cerr("Not enough memory!\n");
		ret = -1;
		goto _end;
	}
	old_end_buf = (uint8_t *)malloc(region_end - old_end);
	if (!old_end_buf) {
		msg_cerr("Not enough memory!\n");
		ret = -1;
		goto _end;
	}

	if (old_start - region_start) {
		read_flash(flashctx, curcontents + region_start, region_start, old_start - region_start);
		memcpy(old_start_buf, newcontents + region_start, old_start - region_start);
		memcpy(newcontents + region_start, curcontents + region_start, old_start - region_start);
	}
	if (region_end - old_end) {
		read_flash(flashctx, curcontents + old_end, old_end, region_end - old_end);
		memcpy(old_end_buf, newcontents + old_end, region_end - old_end);
		memcpy(newcontents + old_end, curcontents + old_end, region_end - old_end);
	}

	// select erase functions
	for (i = 0; i < erase_layout[erasefn_count - 1].block_count; i++) {
		if (erase_layout[erasefn_count - 1].layout_list[i].start_addr <= region_end &&
			region_start <= erase_layout[erasefn_count - 1].layout_list[i].end_addr)
			select_erase_functions(flashctx, erase_layout,
						erasefn_count - 1, i,
						curcontents, newcontents,
						region_start, region_end);
	}

	for (i = 0; i < erasefn_count; i++) {
		for (size_t j = 0; j < erase_layout[i].block_count; j++) {
			if (!erase_layout[i].layout_list[j].selected)	continue;

			// erase
			chipoff_t start_addr = erase_layout[i].layout_list[j].start_addr;
			unsigned int block_len = erase_layout[i].layout_list[j].end_addr - start_addr + 1;
			const uint8_t erased_value = ERASED_VALUE(flashctx);
			// execute erase
			erasefunc_t *erasefn = lookup_erase_func_ptr(erase_layout[i].eraser);

			if (!flashctx->flags.skip_unwritable_regions) {
				if (check_for_unwritable_regions(flashctx, start_addr, block_len))
					goto _end;
			}

			unsigned int len;
			for (unsigned int addr = start_addr; addr < start_addr + block_len; addr += len) {
				struct flash_region region;
				get_flash_region(flashctx, addr, &region);

				len = min(start_addr + block_len, region.end) - addr;

				if (region.write_prot) {
					msg_gdbg("%s: cannot erase inside %s "
						"region (%#08"PRIx32"..%#08"PRIx32"), skipping range (%#08x..%#08x).\n",
						 __func__, region.name,
						 region.start, region.end - 1,
						 addr, addr + len - 1);
					free(region.name);
					continue;
				}

				msg_gdbg("%s: %s region (%#08"PRIx32"..%#08"PRIx32") is "
					"writable, erasing range (%#08x..%#08x).\n",
					 __func__, region.name,
					 region.start, region.end - 1,
					 addr, addr + len - 1);
				free(region.name);

				if (erasefn(flashctx, addr, len)) {
					ret = -1;
					goto _end;
				}
				if (check_erased_range(flashctx, addr, len)) {
					ret = - 1;
					msg_cerr("ERASE FAILED!\n");
					goto _end;
				}
			}

			ret = erasefn(flashctx, start_addr, block_len);
			if (ret) {
				msg_cerr("Failed to execute erase command "
					"for offset %#"PRIx32" to %#"PRIx32".\n",
					start_addr, start_addr + block_len);
				ret = -1;
				goto _end;
			}

			// adjust curcontents
			memset(curcontents+start_addr, erased_value, block_len);
			// after erase make it unselected again
			erase_layout[i].layout_list[j].selected = false;
			msg_cdbg("E(%"PRIx32":%"PRIx32")", start_addr, start_addr + block_len - 1);
			// verify erase
			ret = check_erased_range(flashctx, start_addr, block_len);
			if (ret) {
				msg_cerr("Verifying flash. Erase failed for range %#"PRIx32" : %#"PRIx32", Abort.\n",
					start_addr, start_addr + block_len - 1);
				goto _end;
			}

			*all_skipped = false;
		}
	}

	// write
	unsigned int start_here = 0, len_here = 0, erase_len = region_end - region_start + 1;
	while ((len_here = get_next_write(curcontents + region_start + start_here,
					newcontents + region_start + start_here,
					erase_len - start_here, &start_here,
					flashctx->chip->gran))) {
		// execute write
		ret = write_flash(flashctx,
				newcontents + region_start + start_here,
				region_start + start_here, len_here);
		if (ret) {
			msg_cerr("Write failed at %#zx, Abort.\n", i);
			ret = -1;
			goto _end;
		}

		// adjust curcontents
		memcpy(curcontents + region_start + start_here,
			newcontents + region_start + start_here, len_here);
		msg_cdbg("W(%"PRIx32":%"PRIx32")", region_start + start_here, region_start + start_here + len_here - 1);

		*all_skipped = false;
	}
	// verify write
	ret = verify_range(flashctx, newcontents + region_start, region_start, region_end - region_start);
	if (ret) {
		msg_cerr("Verifying flash. Write failed for range %#"PRIx32" : %#"PRIx32", Abort.\n",
			region_start, region_end);
		goto _end;
	}

_end:
	memcpy(newcontents + region_start, old_start_buf, old_start - region_start);
	memcpy(newcontents + old_end, old_end_buf, region_end - old_end);

	free(old_start_buf);
	free(old_end_buf);

	msg_cinfo("Erase/write done from %"PRIx32" to %"PRIx32"\n", region_start, region_end);
	return ret;
}
