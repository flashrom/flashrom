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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "flash.h"
#include "programmer.h"

/* No-op shutdown() for programmers which don't need special handling */
int noop_shutdown(void)
{
	return 0;
}

/* Fallback map() for programmers which don't need special handling */
void *fallback_map(const char *descr, uintptr_t phys_addr, size_t len)
{
	/* FIXME: Should return phys_addr. */
	return NULL;
}

/* No-op/fallback unmap() for programmers which don't need special handling */
void fallback_unmap(void *virt_addr, size_t len)
{
}

/* No-op chip_writeb() for parallel style drivers not supporting writes */
void noop_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
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

void fallback_chip_writen(const struct flashctx *flash, uint8_t *buf,
			  chipaddr addr, size_t len)
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

int register_par_programmer(const struct par_programmer *pgm,
			    const enum chipbustype buses)
{
	struct registered_programmer rpgm;
	if (!pgm->chip_writeb || !pgm->chip_writew || !pgm->chip_writel ||
	    !pgm->chip_writen || !pgm->chip_readb || !pgm->chip_readw ||
	    !pgm->chip_readl || !pgm->chip_readn) {
		msg_perr("%s called with incomplete programmer definition. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return ERROR_FLASHROM_BUG;
	}

	rpgm.buses_supported = buses;
	rpgm.par = *pgm;
	return register_programmer(&rpgm);
}

/* The limit of 4 is totally arbitrary. */
#define PROGRAMMERS_MAX 4
struct registered_programmer registered_programmers[PROGRAMMERS_MAX];
int registered_programmer_count = 0;

/* This function copies the struct registered_programmer parameter. */
int register_programmer(struct registered_programmer *pgm)
{
	if (registered_programmer_count >= PROGRAMMERS_MAX) {
		msg_perr("Tried to register more than %i programmer "
			 "interfaces.\n", PROGRAMMERS_MAX);
		return ERROR_FLASHROM_LIMIT;
	}
	registered_programmers[registered_programmer_count] = *pgm;
	registered_programmer_count++;

	return 0;
}

enum chipbustype get_buses_supported(void)
{
	int i;
	enum chipbustype ret = BUS_NONE;

	for (i = 0; i < registered_programmer_count; i++)
		ret |= registered_programmers[i].buses_supported;

	return ret;
}
