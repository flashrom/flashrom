/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Abdelkader Boudih <coreboot@seuros.com>
 */

#ifndef __READ_EXTENDED_H__
#define __READ_EXTENDED_H__

#include "flash.h"

/**
 * Read a flash chip multiple times and use majority voting to determine
 * the most likely correct content. A new buffer is only allocated when
 * previously unseen content is encountered, so on a stable connection
 * only one buffer is allocated regardless of count.
 *
 * @param  flash       Flash chip context (must be probed).
 * @param  count       Number of reads (>= 3).
 * @param  result_buf  On success, receives a malloc'd buffer with the
 *                     majority content. Caller must free with flashrom_data_free().
 * @param  result_size On success, receives the flash size in bytes.
 * @return 0 on success, non-zero on failure.
 */
int read_repeated(struct flashctx *flash, int count,
			   unsigned char **result_buf, size_t *result_size);

#endif /* __READ_EXTENDED_H__ */
