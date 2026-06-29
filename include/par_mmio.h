/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef __PAR_MMIO_H__
#define __PAR_MMIO_H__ 1

#include "flash.h"
#include <stdint.h>

/*
 * Shared state for memory-mapped parallel programmers. Embed it as the first
 * member of the driver data struct so that par_mmio_chip_readb()/writeb() can
 * recover the mapping from par.data.
 */
struct par_mmio_data {
	uint8_t *bar;
	uint32_t mask;
};

void par_mmio_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr);
uint8_t par_mmio_chip_readb(const struct flashctx *flash, const chipaddr addr);

#endif /* !__PAR_MMIO_H__ */
