/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010,2011 Carl-Daniel Hailfinger
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

#include "flash.h"
#include "programmer.h"

/* Fallback map() for programmers which don't need special handling */
void *fallback_map(const char *descr, uintptr_t phys_addr, size_t len)
{
	/* A result of NULL causes mapped addresses to be chip physical
	 * addresses, assuming only a single region is mapped (the entire flash
	 * space).  Chips with a second region (like a register map) require a
	 * real memory mapping to distinguish the different ranges.  Those chips
	 * are FWH/LPC, so the bus master provides a real mapping.
	 */
	return NULL;
}

/* No-op/fallback unmap() for programmers which don't need special handling */
void fallback_unmap(void *virt_addr, size_t len)
{
}

/* Little-endian fallback for drivers not supporting 16 bit accesses */
void fallback_chip_writew(const struct flashctx *flash, uint16_t val,
			  chipaddr addr)
{
	chip_writeb(flash, val & 0xff, addr);
	chip_writeb(flash, (val >> 8) & 0xff, addr + 1);
}

/* Little-endian fallback for drivers not supporting 16 bit accesses */
uint16_t fallback_chip_readw(const struct flashctx *flash, const chipaddr addr)
{
	uint16_t val;
	val = chip_readb(flash, addr);
	val |= chip_readb(flash, addr + 1) << 8;
	return val;
}

/* Little-endian fallback for drivers not supporting 32 bit accesses */
void fallback_chip_writel(const struct flashctx *flash, uint32_t val,
			  chipaddr addr)
{
	chip_writew(flash, val & 0xffff, addr);
	chip_writew(flash, (val >> 16) & 0xffff, addr + 2);
}

/* Little-endian fallback for drivers not supporting 32 bit accesses */
uint32_t fallback_chip_readl(const struct flashctx *flash, const chipaddr addr)
{
	uint32_t val;
	val = chip_readw(flash, addr);
	val |= chip_readw(flash, addr + 2) << 16;
	return val;
}

void fallback_chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		chip_writeb(flash, buf[i], addr + i);
	return;
}

void fallback_chip_readn(const struct flashctx *flash, uint8_t *buf,
			 chipaddr addr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		buf[i] = chip_readb(flash, addr + i);
	return;
}

/* The limit of 4 is totally arbitrary. */
#define MASTERS_MAX 4
struct registered_master registered_masters[MASTERS_MAX];
int registered_master_count = 0;

/* This function copies the struct registered_master parameter. */
int register_master(const struct registered_master *mst)
{
	if (registered_master_count >= MASTERS_MAX) {
		msg_perr("Tried to register more than %i master "
			 "interfaces.\n", MASTERS_MAX);
		return ERROR_FLASHROM_LIMIT;
	}
	registered_masters[registered_master_count] = *mst;
	registered_master_count++;

	return 0;
}

enum chipbustype get_buses_supported(void)
{
	int i;
	enum chipbustype ret = BUS_NONE;

	for (i = 0; i < registered_master_count; i++)
		ret |= registered_masters[i].buses_supported;

	return ret;
}
