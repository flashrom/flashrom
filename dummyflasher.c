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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "flash.h"

char *dummytype = NULL;

int dummy_init(void)
{
	int i;
	printf_debug("%s\n", __func__);

	/* "all" is equivalent to specifying no type. */
	if (!strcmp(dummytype, "all")) {
		free(dummytype);
		dummytype = NULL;
	}
	if (!dummytype)
		dummytype = strdup("parallel,lpc,fwh,spi");
	/* Convert the parameters to lowercase. */
	for (i = 0; dummytype[i] != '\0'; i++)
		dummytype[i] = (char)tolower(dummytype[i]);

	buses_supported = CHIP_BUSTYPE_NONE;
	if (strstr(dummytype, "parallel")) {
		buses_supported |= CHIP_BUSTYPE_PARALLEL;
		printf_debug("Enabling support for %s flash.\n", "parallel");
	}
	if (strstr(dummytype, "lpc")) {
		buses_supported |= CHIP_BUSTYPE_LPC;
		printf_debug("Enabling support for %s flash.\n", "LPC");
	}
	if (strstr(dummytype, "fwh")) {
		buses_supported |= CHIP_BUSTYPE_FWH;
		printf_debug("Enabling support for %s flash.\n", "FWH");
	}
	if (strstr(dummytype, "spi")) {
		buses_supported |= CHIP_BUSTYPE_SPI;
		spi_controller = SPI_CONTROLLER_DUMMY;
		printf_debug("Enabling support for %s flash.\n", "SPI");
	}
	if (buses_supported == CHIP_BUSTYPE_NONE)
		printf_debug("Support for all flash bus types disabled.\n");
	free(dummytype);
	return 0;
}

int dummy_shutdown(void)
{
	printf_debug("%s\n", __func__);
	return 0;
}

void *dummy_map(const char *descr, unsigned long phys_addr, size_t len)
{
	printf_debug("%s: Mapping %s, 0x%lx bytes at 0x%08lx\n",
		__func__, descr, (unsigned long)len, phys_addr);
	return (void *)phys_addr;
}

void dummy_unmap(void *virt_addr, size_t len)
{
	printf_debug("%s: Unmapping 0x%lx bytes at %p\n",
		__func__, (unsigned long)len, virt_addr);
}

void dummy_chip_writeb(uint8_t val, chipaddr addr)
{
	printf_debug("%s: addr=0x%lx, val=0x%02x\n", __func__, addr, val);
}

void dummy_chip_writew(uint16_t val, chipaddr addr)
{
	printf_debug("%s: addr=0x%lx, val=0x%04x\n", __func__, addr, val);
}

void dummy_chip_writel(uint32_t val, chipaddr addr)
{
	printf_debug("%s: addr=0x%lx, val=0x%08x\n", __func__, addr, val);
}

uint8_t dummy_chip_readb(const chipaddr addr)
{
	printf_debug("%s:  addr=0x%lx, returning 0xff\n", __func__, addr);
	return 0xff;
}

uint16_t dummy_chip_readw(const chipaddr addr)
{
	printf_debug("%s:  addr=0x%lx, returning 0xffff\n", __func__, addr);
	return 0xffff;
}

uint32_t dummy_chip_readl(const chipaddr addr)
{
	printf_debug("%s:  addr=0x%lx, returning 0xffffffff\n", __func__, addr);
	return 0xffffffff;
}

int dummy_spi_command(unsigned int writecnt, unsigned int readcnt,
		      const unsigned char *writearr, unsigned char *readarr)
{
	int i;

	printf_debug("%s:", __func__);

	printf_debug(" writing %u bytes:", writecnt);
	for (i = 0; i < writecnt; i++)
		printf_debug(" 0x%02x", writearr[i]);

	printf_debug(" reading %u bytes:", readcnt);
	for (i = 0; i < readcnt; i++) {
		printf_debug(" 0xff");
		readarr[i] = 0xff;
	}

	printf_debug("\n");
	return 0;
}
