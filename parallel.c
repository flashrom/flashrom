/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2004 Tyan Corp <yhlu@tyan.com>
 * Copyright (C) 2005-2008 coresystems GmbH
 * Copyright (C) 2008,2009 Carl-Daniel Hailfinger
 * Copyright (C) 2016 secunet Security Networks AG
 * (Written by Nico Huber <nico.huber@secunet.com> for secunet)
 * Copyright (C) 2009,2010,2011 Carl-Daniel Hailfinger
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
#include "programmer.h"

void chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	flash->mst->par.chip_writeb(flash, val, addr);
}

/* Little-endian fallback for drivers not supporting 16 bit accesses */
static void fallback_chip_writew(const struct flashctx *flash, uint16_t val,
			  chipaddr addr)
{
	chip_writeb(flash, val & 0xff, addr);
	chip_writeb(flash, (val >> 8) & 0xff, addr + 1);
}

void chip_writew(const struct flashctx *flash, uint16_t val, chipaddr addr)
{
	if (flash->mst->par.chip_writew)
		flash->mst->par.chip_writew(flash, val, addr);
	fallback_chip_writew(flash, val, addr);
}

/* Little-endian fallback for drivers not supporting 32 bit accesses */
static void fallback_chip_writel(const struct flashctx *flash, uint32_t val,
			  chipaddr addr)
{
	chip_writew(flash, val & 0xffff, addr);
	chip_writew(flash, (val >> 16) & 0xffff, addr + 2);
}

void chip_writel(const struct flashctx *flash, uint32_t val, chipaddr addr)
{
	if (flash->mst->par.chip_writel)
		flash->mst->par.chip_writel(flash, val, addr);
	fallback_chip_writel(flash, val, addr);
}

static void fallback_chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		chip_writeb(flash, buf[i], addr + i);
	return;
}

void chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len)
{
	if (flash->mst->par.chip_writen)
		flash->mst->par.chip_writen(flash, buf, addr, len);
	fallback_chip_writen(flash, buf, addr, len);
}

uint8_t chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	return flash->mst->par.chip_readb(flash, addr);
}

/* Little-endian fallback for drivers not supporting 16 bit accesses */
static uint16_t fallback_chip_readw(const struct flashctx *flash, const chipaddr addr)
{
	uint16_t val;
	val = chip_readb(flash, addr);
	val |= chip_readb(flash, addr + 1) << 8;
	return val;
}

uint16_t chip_readw(const struct flashctx *flash, const chipaddr addr)
{
	if (flash->mst->par.chip_readw)
		return flash->mst->par.chip_readw(flash, addr);
	return fallback_chip_readw(flash, addr);
}

/* Little-endian fallback for drivers not supporting 32 bit accesses */
static uint32_t fallback_chip_readl(const struct flashctx *flash, const chipaddr addr)
{
	uint32_t val;
	val = chip_readw(flash, addr);
	val |= chip_readw(flash, addr + 2) << 16;
	return val;
}

uint32_t chip_readl(const struct flashctx *flash, const chipaddr addr)
{
	if (flash->mst->par.chip_readl)
		return flash->mst->par.chip_readl(flash, addr);
	return fallback_chip_readl(flash, addr);
}

static void fallback_chip_readn(const struct flashctx *flash, uint8_t *buf,
			 chipaddr addr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		buf[i] = chip_readb(flash, addr + i);
	return;
}

void chip_readn(const struct flashctx *flash, uint8_t *buf, chipaddr addr,
		size_t len)
{
	if (flash->mst->par.chip_readn)
		flash->mst->par.chip_readn(flash, buf, addr, len);
	fallback_chip_readn(flash, buf, addr, len);
}

int register_par_master(const struct par_master *mst,
			    const enum chipbustype buses,
			    void *data)
{
	struct registered_master rmst = {0};

	if (mst->shutdown) {
		if (register_shutdown(mst->shutdown, data)) {
			mst->shutdown(data); /* cleanup */
			return 1;
		}
	}

	/* Bus masters supporting FWH/LPC cannot use chip physical maps, distinct
	 * mappings are needed to support chips with FEATURE_REGISTERMAP
	 */
	if ((buses & (BUS_FWH | BUS_LPC)) && !mst->map_flash_region) {
		msg_perr("%s called with incomplete master definition. "
			 "FWH/LPC masters must provide memory mappings. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return ERROR_FLASHROM_BUG;
	}

	if (!mst->chip_writeb || !mst->chip_readb) {
		msg_perr("%s called with incomplete master definition. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return ERROR_FLASHROM_BUG;
	}

	rmst.buses_supported = buses;
	rmst.par = *mst;
	if (data)
		rmst.par.data = data;
	return register_master(&rmst);
}
