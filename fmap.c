/*
 * Copyright 2010, Google LLC.
 * Copyright 2018-present, Facebook Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "flash.h"
#include "fmap.h"

static size_t fmap_size(const struct fmap *fmap)
{
	return sizeof(*fmap) + (fmap->nareas * sizeof(struct fmap_area));
}

/* Make a best-effort assessment if the given fmap is real */
static int is_valid_fmap(const struct fmap *fmap)
{
	if (memcmp(fmap, FMAP_SIGNATURE, strlen(FMAP_SIGNATURE)) != 0)
		return 0;
	/* strings containing the magic tend to fail here */
	if (fmap->ver_major != FMAP_VER_MAJOR)
		return 0;
	/* a basic consistency check: flash address space size should be larger
	 * than the size of the fmap data structure */
	if (fmap->size < fmap_size(fmap))
		return 0;

	/* fmap-alikes along binary data tend to fail on having a valid,
	 * null-terminated string in the name field.*/
	int i;
	for (i = 0; i < FMAP_STRLEN; i++) {
		if (fmap->name[i] == 0)
			break;
		if (!isgraph(fmap->name[i]))
			return 0;
		if (i == FMAP_STRLEN - 1) {
			/* name is specified to be null terminated single-word string
			 * without spaces. We did not break in the 0 test, we know it
			 * is a printable spaceless string but we're seeing FMAP_STRLEN
			 * symbols, which is one too many.
			 */
			 return 0;
		}
	}
	return 1;

}

/**
 * @brief Do a brute-force linear search for fmap in provided buffer
 *
 * @param[in] buffer The buffer to search
 * @param[in] len Length (in bytes) to search
 *
 * @return offset in buffer where fmap is found if successful
 *         -1 to indicate that fmap was not found
 *         -2 to indicate fmap is truncated or exceeds buffer + len
 */
static ssize_t fmap_lsearch(const uint8_t *buf, size_t len)
{
	ssize_t offset;
	bool fmap_found = false;

	if (len < sizeof(struct fmap))
		return -1;

	for (offset = 0; offset <= (ssize_t)(len - sizeof(struct fmap)); offset++) {
		if (is_valid_fmap((struct fmap *)&buf[offset])) {
			fmap_found = true;
			break;
		}
	}

	if (!fmap_found)
		return -1;

	if (offset + fmap_size((struct fmap *)&buf[offset]) > len) {
		msg_gerr("fmap size exceeds buffer boundary.\n");
		return -2;
	}

	return offset;
}

/**
 * @brief Read fmap from provided buffer and copy it to fmap_out
 *
 * @param[out] fmap_out Double-pointer to location to store fmap contents.
 *                      Caller must free allocated fmap contents.
 * @param[in] buf Buffer to search
 * @param[in] len Length (in bytes) to search
 *
 * @return 0 if successful
 *         1 to indicate error
 *         2 to indicate fmap is not found
 */
int fmap_read_from_buffer(struct fmap **fmap_out, const uint8_t *const buf, size_t len)
{
	ssize_t offset = fmap_lsearch(buf, len);
	if (offset < 0) {
		msg_gdbg("Unable to find fmap in provided buffer.\n");
		return 2;
	}
	msg_gdbg("Found fmap at offset 0x%06zx\n", (size_t)offset);

	const struct fmap *fmap = (const struct fmap *)(buf + offset);
	*fmap_out = malloc(fmap_size(fmap));
	if (*fmap_out == NULL) {
		msg_gerr("Out of memory.\n");
		return 1;
	}

	memcpy(*fmap_out, fmap, fmap_size(fmap));
	return 0;
}

static int fmap_lsearch_rom(struct fmap **fmap_out,
		struct flashctx *const flashctx, size_t rom_offset, size_t len)
{
	int ret = -1;
	uint8_t *buf;

	if (prepare_flash_access(flashctx, true, false, false, false))
		goto _finalize_ret;

	/* likely more memory than we need, but it simplifies handling and
	 * printing offsets to keep them uniform with what's on the ROM */
	buf = malloc(rom_offset + len);
	if (!buf) {
		msg_gerr("Out of memory.\n");
		goto _finalize_ret;
	}

	ret = read_flash(flashctx, buf + rom_offset, rom_offset, len);
	if (ret) {
		msg_pdbg("Cannot read ROM contents.\n");
		goto _free_ret;
	}

	ret = fmap_read_from_buffer(fmap_out, buf + rom_offset, len);
_free_ret:
	free(buf);
_finalize_ret:
	finalize_flash_access(flashctx);
	return ret;
}

static int fmap_bsearch_rom(struct fmap **fmap_out, struct flashctx *const flashctx,
		size_t rom_offset, size_t len, size_t min_stride)
{
	size_t stride, fmap_len = 0;
	int ret = 1;
	bool fmap_found = false;
	bool check_offset_0 = true;
	struct fmap *fmap;
	const unsigned int chip_size = flashctx->chip->total_size * 1024;
	const int sig_len = strlen(FMAP_SIGNATURE);

	if (rom_offset + len > flashctx->chip->total_size * 1024)
		return 1;

	if (len < sizeof(*fmap))
		return 1;

	if (prepare_flash_access(flashctx, true, false, false, false))
		return 1;

	fmap = malloc(sizeof(*fmap));
	if (!fmap) {
		msg_gerr("Out of memory.\n");
		goto _free_ret;
	}

	/*
	 * For efficient operation, we start with the largest stride possible
	 * and then decrease the stride on each iteration. Also, check for a
	 * remainder when modding the offset with the previous stride. This
	 * makes it so that each offset is only checked once.
	 *
	 * Zero (rom_offset == 0) is a special case and is handled using a
	 * variable to track whether or not we've checked it.
	 */
	size_t offset;
	for (stride = chip_size / 2; stride >= min_stride; stride /= 2) {
		if (stride > len)
			continue;

		for (offset = rom_offset;
		     offset <= rom_offset + len - sizeof(struct fmap);
		     offset += stride) {
			if ((offset % (stride * 2) == 0) && (offset != 0))
				continue;
			if (offset == 0 && !check_offset_0)
				continue;
			check_offset_0 = false;

			/* Read errors are considered non-fatal since we may
			 * encounter locked regions and want to continue. */
			if (read_flash(flashctx, (uint8_t *)fmap, offset, sig_len)) {
				/*
				 * Print in verbose mode only to avoid excessive
				 * messages for benign errors. Subsequent error
				 * prints should be done as usual.
				 */
				msg_cdbg("Cannot read %d bytes at offset %zu\n", sig_len, offset);
				continue;
			}

			if (memcmp(fmap, FMAP_SIGNATURE, sig_len) != 0)
				continue;

			if (read_flash(flashctx, (uint8_t *)fmap + sig_len,
						offset + sig_len, sizeof(*fmap) - sig_len)) {
				msg_cerr("Cannot read %zu bytes at offset %06zx\n",
						sizeof(*fmap) - sig_len, offset + sig_len);
				continue;
			}

			if (is_valid_fmap(fmap)) {
				msg_gdbg("fmap found at offset 0x%06zx\n", offset);
				fmap_found = true;
				break;
			}
			msg_gerr("fmap signature found at %zu but header is invalid.\n", offset);
			ret = 2;
		}

		if (fmap_found)
			break;
	}

	if (!fmap_found)
		goto _free_ret;

	fmap_len = fmap_size(fmap);
	struct fmap *tmp = fmap;
	fmap = realloc(fmap, fmap_len);
	if (!fmap) {
		msg_gerr("Failed to realloc.\n");
		free(tmp);
		goto _free_ret;
	}

	if (read_flash(flashctx, (uint8_t *)fmap + sizeof(*fmap),
				offset + sizeof(*fmap), fmap_len - sizeof(*fmap))) {
		msg_cerr("Cannot read %zu bytes at offset %06zx\n",
				fmap_len - sizeof(*fmap), offset + sizeof(*fmap));
		/* Treat read failure to be fatal since this
		 * should be a valid, usable fmap. */
		ret = 2;
		goto _free_ret;
	}

	*fmap_out = fmap;
	ret = 0;
_free_ret:
	if (ret)
		free(fmap);
	finalize_flash_access(flashctx);
	return ret;
}

/**
 * @brief Read fmap from ROM
 *
 * @param[out] fmap_out Double-pointer to location to store fmap contents.
 *                      Caller must free allocated fmap contents.
 * @param[in] flashctx Flash context
 * @param[in] rom_offset Offset in ROM to begin search
 * @param[in] len Length to search relative to rom_offset
 *
 * @return 0 on success,
 *         2 if the fmap couldn't be read,
 *         1 on any other error.
 */
int fmap_read_from_rom(struct fmap **fmap_out,
	struct flashctx *const flashctx, size_t rom_offset, size_t len)
{
	int ret;

	if (!flashctx || !flashctx->chip)
		return 1;

	/*
	 * Binary search is used at first to see if we can find an fmap quickly
	 * in a usual location (often at a power-of-2 offset). However, once we
	 * reach a small enough stride the transaction overhead will reverse the
	 * speed benefit of using bsearch at which point we need to use brute-
	 * force instead.
	 *
	 * TODO: Since flashrom is often used with high-latency external
	 * programmers we should not be overly aggressive with bsearch.
	 */
	ret = fmap_bsearch_rom(fmap_out, flashctx, rom_offset, len, 256);
	if (ret) {
		msg_gdbg("Binary search failed, trying linear search...\n");
		ret = fmap_lsearch_rom(fmap_out, flashctx, rom_offset, len);
	}

	return ret;
}
