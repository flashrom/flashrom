/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Carl-Daniel Hailfinger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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

/* Driver for the NVIDIA MCP6x/MCP7x MCP6X_SPI controller.
 * Based on clean room reverse engineered docs from
 * http://www.flashrom.org/pipermail/flashrom/2009-December/001180.html
 * created by Michael Karcher.
 */

#if defined(__i386__) || defined(__x86_64__)

#include <stdlib.h>
#include <ctype.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

/* Bit positions for each pin. */

#define MCP6X_SPI_CS		1
#define MCP6X_SPI_SCK		2
#define MCP6X_SPI_MOSI		3
#define MCP6X_SPI_MISO		4
#define MCP6X_SPI_REQUEST	0
#define MCP6X_SPI_GRANT		8

void *mcp6x_spibar = NULL;

/* Cached value of last GPIO state. */
static uint8_t mcp_gpiostate;

static void mcp6x_request_spibus(void)
{
	mcp_gpiostate = mmio_readb(mcp6x_spibar + 0x530);
	mcp_gpiostate |= 1 << MCP6X_SPI_REQUEST;
	mmio_writeb(mcp_gpiostate, mcp6x_spibar + 0x530);

	/* Wait until we are allowed to use the SPI bus. */
	while (!(mmio_readw(mcp6x_spibar + 0x530) & (1 << MCP6X_SPI_GRANT))) ;

	/* Update the cache. */
	mcp_gpiostate = mmio_readb(mcp6x_spibar + 0x530);
}

static void mcp6x_release_spibus(void)
{
	mcp_gpiostate &= ~(1 << MCP6X_SPI_REQUEST);
	mmio_writeb(mcp_gpiostate, mcp6x_spibar + 0x530);
}

static void mcp6x_bitbang_set_cs(int val)
{
	mcp_gpiostate &= ~(1 << MCP6X_SPI_CS);
	mcp_gpiostate |= (val << MCP6X_SPI_CS);
	mmio_writeb(mcp_gpiostate, mcp6x_spibar + 0x530);
}

static void mcp6x_bitbang_set_sck(int val)
{
	mcp_gpiostate &= ~(1 << MCP6X_SPI_SCK);
	mcp_gpiostate |= (val << MCP6X_SPI_SCK);
	mmio_writeb(mcp_gpiostate, mcp6x_spibar + 0x530);
}

static void mcp6x_bitbang_set_mosi(int val)
{
	mcp_gpiostate &= ~(1 << MCP6X_SPI_MOSI);
	mcp_gpiostate |= (val << MCP6X_SPI_MOSI);
	mmio_writeb(mcp_gpiostate, mcp6x_spibar + 0x530);
}

static int mcp6x_bitbang_get_miso(void)
{
	mcp_gpiostate = mmio_readb(mcp6x_spibar + 0x530);
	return (mcp_gpiostate >> MCP6X_SPI_MISO) & 0x1;
}

static const struct bitbang_spi_master bitbang_spi_master_mcp6x = {
	.type = BITBANG_SPI_MASTER_MCP,
	.set_cs = mcp6x_bitbang_set_cs,
	.set_sck = mcp6x_bitbang_set_sck,
	.set_mosi = mcp6x_bitbang_set_mosi,
	.get_miso = mcp6x_bitbang_get_miso,
	.request_bus = mcp6x_request_spibus,
	.release_bus = mcp6x_release_spibus,
	.half_period = 0,
};

int mcp6x_spi_init(int want_spi)
{
	uint16_t status;
	uint32_t mcp6x_spibaraddr;
	struct pci_dev *smbusdev;

	/* Look for the SMBus device (SMBus PCI class) */
	smbusdev = pci_dev_find_vendorclass(0x10de, 0x0c05);
	if (!smbusdev) {
		if (want_spi) {
			msg_perr("ERROR: SMBus device not found. Not enabling "
				 "SPI.\n");
			return 1;
		} else {
			msg_pinfo("Odd. SMBus device not found.\n");
			return 0;
		}
	}
	msg_pdbg("Found SMBus device %04x:%04x at %02x:%02x:%01x\n",
		smbusdev->vendor_id, smbusdev->device_id,
		smbusdev->bus, smbusdev->dev, smbusdev->func);


	/* Locate the BAR where the SPI interface lives. */
	mcp6x_spibaraddr = pci_read_long(smbusdev, 0x74);
	/* BAR size is 64k, bits 15..4 are zero, bit 3..0 declare a
	 * 32-bit non-prefetchable memory BAR.
	 */
	mcp6x_spibaraddr &= ~0xffff;
	msg_pdbg("MCP SPI BAR is at 0x%08x\n", mcp6x_spibaraddr);

	/* Accessing a NULL pointer BAR is evil. Don't do it. */
	if (!mcp6x_spibaraddr && want_spi) {
		msg_perr("Error: Chipset is strapped for SPI, but MCP SPI BAR is invalid.\n");
		return 1;
	} else if (!mcp6x_spibaraddr && !want_spi) {
		msg_pdbg("MCP SPI is not used.\n");
		return 0;
	} else if (mcp6x_spibaraddr && !want_spi) {
		msg_pdbg("Strange. MCP SPI BAR is valid, but chipset apparently doesn't have SPI enabled.\n");
		/* FIXME: Should we enable SPI anyway? */
		return 0;
	}
	/* Map the BAR. Bytewise/wordwise access at 0x530 and 0x540. */
	mcp6x_spibar = rphysmap("NVIDIA MCP6x SPI", mcp6x_spibaraddr, 0x544);
	if (mcp6x_spibar == ERROR_PTR)
		return 1;

	status = mmio_readw(mcp6x_spibar + 0x530);
	msg_pdbg("SPI control is 0x%04x, req=%i, gnt=%i\n",
		 status, (status >> MCP6X_SPI_REQUEST) & 0x1,
		 (status >> MCP6X_SPI_GRANT) & 0x1);
	mcp_gpiostate = status & 0xff;

	if (register_spi_bitbang_master(&bitbang_spi_master_mcp6x)) {
		/* This should never happen. */
		msg_perr("MCP6X bitbang SPI master init failed!\n");
		return 1;
	}

	return 0;
}

#endif
