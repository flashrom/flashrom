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
#include <sys/types.h>
#include "flash.h"

int dummy_init(void)
{
	int i;
	msg_pspew("%s\n", __func__);

	/* "all" is equivalent to specifying no type. */
	if (programmer_param && (!strcmp(programmer_param, "all"))) {
		free(programmer_param);
		programmer_param = NULL;
	}
	if (!programmer_param)
		programmer_param = strdup("parallel,lpc,fwh,spi");
	/* Convert the parameters to lowercase. */
	for (i = 0; programmer_param[i] != '\0'; i++)
		programmer_param[i] = (char)tolower(programmer_param[i]);

	buses_supported = CHIP_BUSTYPE_NONE;
	if (strstr(programmer_param, "parallel")) {
		buses_supported |= CHIP_BUSTYPE_PARALLEL;
		msg_pdbg("Enabling support for %s flash.\n", "parallel");
	}
	if (strstr(programmer_param, "lpc")) {
		buses_supported |= CHIP_BUSTYPE_LPC;
		msg_pdbg("Enabling support for %s flash.\n", "LPC");
	}
	if (strstr(programmer_param, "fwh")) {
		buses_supported |= CHIP_BUSTYPE_FWH;
		msg_pdbg("Enabling support for %s flash.\n", "FWH");
	}
	if (strstr(programmer_param, "spi")) {
		buses_supported |= CHIP_BUSTYPE_SPI;
		spi_controller = SPI_CONTROLLER_DUMMY;
		msg_pdbg("Enabling support for %s flash.\n", "SPI");
	}
	if (buses_supported == CHIP_BUSTYPE_NONE)
		msg_pdbg("Support for all flash bus types disabled.\n");
	free(programmer_param);
	return 0;
}

int dummy_shutdown(void)
{
	msg_pspew("%s\n", __func__);
	return 0;
}

void *dummy_map(const char *descr, unsigned long phys_addr, size_t len)
{
	msg_pspew("%s: Mapping %s, 0x%lx bytes at 0x%08lx\n",
		  __func__, descr, (unsigned long)len, phys_addr);
	return (void *)phys_addr;
}

void dummy_unmap(void *virt_addr, size_t len)
{
	msg_pspew("%s: Unmapping 0x%lx bytes at %p\n",
		  __func__, (unsigned long)len, virt_addr);
}

void dummy_chip_writeb(uint8_t val, chipaddr addr)
{
	msg_pspew("%s: addr=0x%lx, val=0x%02x\n", __func__, addr, val);
}

void dummy_chip_writew(uint16_t val, chipaddr addr)
{
	msg_pspew("%s: addr=0x%lx, val=0x%04x\n", __func__, addr, val);
}

void dummy_chip_writel(uint32_t val, chipaddr addr)
{
	msg_pspew("%s: addr=0x%lx, val=0x%08x\n", __func__, addr, val);
}

void dummy_chip_writen(uint8_t *buf, chipaddr addr, size_t len)
{
	size_t i;
	msg_pspew("%s: addr=0x%lx, len=0x%08lx, writing data (hex):",
		  __func__, addr, (unsigned long)len);
	for (i = 0; i < len; i++) {
		if ((i % 16) == 0)
			msg_pspew("\n");
		msg_pspew("%02x ", buf[i]);
	}
}

uint8_t dummy_chip_readb(const chipaddr addr)
{
	msg_pspew("%s:  addr=0x%lx, returning 0xff\n", __func__, addr);
	return 0xff;
}

uint16_t dummy_chip_readw(const chipaddr addr)
{
	msg_pspew("%s:  addr=0x%lx, returning 0xffff\n", __func__, addr);
	return 0xffff;
}

uint32_t dummy_chip_readl(const chipaddr addr)
{
	msg_pspew("%s:  addr=0x%lx, returning 0xffffffff\n", __func__, addr);
	return 0xffffffff;
}

void dummy_chip_readn(uint8_t *buf, const chipaddr addr, size_t len)
{
	msg_pspew("%s:  addr=0x%lx, len=0x%lx, returning array of 0xff\n",
		  __func__, addr, (unsigned long)len);
	memset(buf, 0xff, len);
	return;
}

int dummy_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		      const unsigned char *writearr, unsigned char *readarr)
{
	int i;

	msg_pspew("%s:", __func__);

	msg_pspew(" writing %u bytes:", writecnt);
	for (i = 0; i < writecnt; i++)
		msg_pspew(" 0x%02x", writearr[i]);

	msg_pspew(" reading %u bytes:", readcnt);
	for (i = 0; i < readcnt; i++) {
		msg_pspew(" 0xff");
		readarr[i] = 0xff;
	}

	msg_pspew("\n");
	return 0;
}
