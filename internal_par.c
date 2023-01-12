/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Carl-Daniel Hailfinger
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

#include "programmer.h"
#include "hwaccess_physmap.h"

static void internal_chip_writeb(const struct flashctx *flash, uint8_t val,
				 chipaddr addr)
{
	mmio_writeb(val, (void *) addr);
}

static void internal_chip_writew(const struct flashctx *flash, uint16_t val,
				 chipaddr addr)
{
	mmio_writew(val, (void *) addr);
}

static void internal_chip_writel(const struct flashctx *flash, uint32_t val,
				 chipaddr addr)
{
	mmio_writel(val, (void *) addr);
}

static uint8_t internal_chip_readb(const struct flashctx *flash,
				   const chipaddr addr)
{
	return mmio_readb((void *) addr);
}

static uint16_t internal_chip_readw(const struct flashctx *flash,
				    const chipaddr addr)
{
	return mmio_readw((void *) addr);
}

static uint32_t internal_chip_readl(const struct flashctx *flash,
				    const chipaddr addr)
{
	return mmio_readl((void *) addr);
}

static void internal_chip_readn(const struct flashctx *flash, uint8_t *buf,
				const chipaddr addr, size_t len)
{
	mmio_readn((void *)addr, buf, len);
	return;
}

static const struct par_master par_master_internal = {
	.map_flash_region	= physmap,
	.unmap_flash_region	= physunmap,
	.chip_readb	= internal_chip_readb,
	.chip_readw	= internal_chip_readw,
	.chip_readl	= internal_chip_readl,
	.chip_readn	= internal_chip_readn,
	.chip_writeb	= internal_chip_writeb,
	.chip_writew	= internal_chip_writew,
	.chip_writel	= internal_chip_writel,
};

void internal_par_init(enum chipbustype buses)
{
	if (buses & BUS_NONSPI)
		register_par_master(&par_master_internal, internal_buses_supported, NULL);
}
