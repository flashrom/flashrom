/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Abdelkader Boudih <coreboot@seuros.com>
 */

#include <stdlib.h>
#include <string.h>

#include "flash.h"
#include "libflashrom.h"
#include "read_extended.h"

struct read_candidate {
	int count;
	unsigned char *buf;
};

int read_repeated(struct flashctx *const flash, int count,
			   unsigned char **result_buf, size_t *result_size)
{
	const size_t size = flashrom_flash_getsize(flash);
	int ret = 0;
	int num_candidates = 0;

	/* Each read is compared against known candidates via memcmp.
	 * A new buffer is only allocated when previously unseen content
	 * is encountered.  In the common case (stable chip) only one
	 * buffer is allocated regardless of count. */
	struct read_candidate *candidates = calloc(count, sizeof(*candidates));
	unsigned char *buf = calloc(size, 1);
	if (!candidates || !buf) {
		msg_gerr("Memory allocation failed!\n");
		ret = 1;
		goto free_out;
	}

	msg_ginfo("Performing %d reads to check connection stability...\n", count);

	for (int i = 0; i < count; i++) {
		msg_ginfo("Read %d/%d...\n", i + 1, count);
		ret = flashrom_image_read(flash, buf, size);
		if (ret) {
			msg_gerr("Read %d failed!\n", i + 1);
			goto free_out;
		}

		int found = -1;
		for (int j = 0; j < num_candidates; j++) {
			if (memcmp(candidates[j].buf, buf, size) == 0) {
				found = j;
				break;
			}
		}

		if (found >= 0) {
			candidates[found].count++;
		} else {
			unsigned char *stored = malloc(size);
			if (!stored) {
				msg_gerr("Memory allocation failed!\n");
				ret = 1;
				goto free_out;
			}
			memcpy(stored, buf, size);
			candidates[num_candidates].count = 1;
			candidates[num_candidates].buf = stored;
			num_candidates++;
		}
	}

	if (num_candidates == 0) {
		msg_gerr("All %d reads failed.\n", count);
		ret = 1;
		goto free_out;
	}

	/* Find the most frequent read. */
	int best = 0;
	for (int j = 1; j < num_candidates; j++) {
		if (candidates[j].count > candidates[best].count)
			best = j;
	}

	const int best_count = candidates[best].count;
	msg_ginfo("%d/%d reads matched.\n", best_count, count);

	/* Require a strict majority (> 50%).  A tie is treated as a failure
	 * since it gives no confidence about which read is correct. */
	if (best_count * 2 <= count) {
		msg_gerr("No majority read found — connection is too unstable to trust.\n");
		ret = 1;
	} else {
		if (best_count < count)
			msg_gwarn("Warning: only %d/%d reads matched — connection may be flaky.\n",
				  best_count, count);
		else
			msg_ginfo("All reads match. Flash connection looks stable.\n");

		/* Hand ownership of the winning buffer to the caller. */
		*result_buf = candidates[best].buf;
		*result_size = size;
		candidates[best].buf = NULL; /* prevent free below */
	}

free_out:
	free(buf);
	for (int j = 0; j < num_candidates; j++)
		free(candidates[j].buf);
	free(candidates);
	return ret;
}
