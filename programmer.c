/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
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

#include <stdlib.h>
#include "flash.h"

/* No-op shutdown() for programmers which don't need special handling */
int noop_shutdown(void)
{
	return 0;
}

/* Fallback map() for programmers which don't need special handling */
void *fallback_map(const char *descr, unsigned long phys_addr, size_t len)
{
	/* FIXME: Should return phys_addr. */
	return 0;
}

/* No-op/fallback unmap() for programmers which don't need special handling */
void fallback_unmap(void *virt_addr, size_t len)
{
}

/* No-op chip_writeb() for drivers not supporting addr/data pair accesses */
uint8_t noop_chip_readb(const chipaddr addr)
{
	return 0xff;
}

/* No-op chip_writeb() for drivers not supporting addr/data pair accesses */
void noop_chip_writeb(uint8_t val, chipaddr addr)
{
}

/* Little-endian fallback for drivers not supporting 16 bit accesses */
void fallback_chip_writew(uint16_t val, chipaddr addr)
{
	chip_writeb(val & 0xff, addr);
	chip_writeb((val >> 8) & 0xff, addr + 1);
}

/* Little-endian fallback for drivers not supporting 16 bit accesses */
uint16_t fallback_chip_readw(const chipaddr addr)
{
	uint16_t val;
	val = chip_readb(addr);
	val |= chip_readb(addr + 1) << 8;
	return val;
}

/* Little-endian fallback for drivers not supporting 32 bit accesses */
void fallback_chip_writel(uint32_t val, chipaddr addr)
{
	chip_writew(val & 0xffff, addr);
	chip_writew((val >> 16) & 0xffff, addr + 2);
}

/* Little-endian fallback for drivers not supporting 32 bit accesses */
uint32_t fallback_chip_readl(const chipaddr addr)
{
	uint32_t val;
	val = chip_readw(addr);
	val |= chip_readw(addr + 2) << 16;
	return val;
}

void fallback_chip_writen(uint8_t *buf, chipaddr addr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		chip_writeb(buf[i], addr + i);
	return;
}

void fallback_chip_readn(uint8_t *buf, chipaddr addr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		buf[i] = chip_readb(addr + i);
	return;
}
