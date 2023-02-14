/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010,2011 Carl-Daniel Hailfinger
 * Written by Carl-Daniel Hailfinger for Angelbird Ltd.
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

/* Datasheets are not public (yet?) */

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_x86_io.h"
#include "hwaccess_physmap.h"
#include "platform/pci.h"

struct satamv_data {
	uint8_t *bar;
	uint16_t iobar;
};

static const struct dev_entry satas_mv[] = {
	/* 88SX6041 and 88SX6042 are the same according to the datasheet. */
	{0x11ab, 0x7042, OK, "Marvell", "88SX7042 PCI-e 4-port SATA-II"},

	{0},
};

#define NVRAM_PARAM			0x1045c
#define FLASH_PARAM			0x1046c
#define EXPANSION_ROM_BAR_CONTROL	0x00d2c
#define PCI_BAR2_CONTROL		0x00c08
#define GPIO_PORT_CONTROL		0x104f0

/* BAR2 (MEM) can map NVRAM and flash. We set it to flash in the init function.
 * If BAR2 is disabled, it still can be accessed indirectly via BAR1 (I/O).
 * This code only supports indirect accesses for now.
 */

/* Indirect access to via the I/O BAR1. */
static void satamv_indirect_chip_writeb(uint8_t val, chipaddr addr, uint16_t iobar)
{
	/* 0x80000000 selects BAR2 for remapping. */
	OUTL(((uint32_t)addr | 0x80000000) & 0xfffffffc, iobar);
	OUTB(val, iobar + 0x80 + (addr & 0x3));
}

/* Indirect access to via the I/O BAR1. */
static uint8_t satamv_indirect_chip_readb(const chipaddr addr, uint16_t iobar)
{
	/* 0x80000000 selects BAR2 for remapping. */
	OUTL(((uint32_t)addr | 0x80000000) & 0xfffffffc, iobar);
	return INB(iobar + 0x80 + (addr & 0x3));
}

/* FIXME: Prefer direct access to BAR2 if BAR2 is active. */
static void satamv_chip_writeb(const struct flashctx *flash, uint8_t val,
			       chipaddr addr)
{
	const struct satamv_data *data = flash->mst->par.data;

	satamv_indirect_chip_writeb(val, addr, data->iobar);
}

/* FIXME: Prefer direct access to BAR2 if BAR2 is active. */
static uint8_t satamv_chip_readb(const struct flashctx *flash,
				 const chipaddr addr)
{
	const struct satamv_data *data = flash->mst->par.data;

	return satamv_indirect_chip_readb(addr, data->iobar);
}

static int satamv_shutdown(void *par_data)
{
	free(par_data);
	return 0;
}

static const struct par_master par_master_satamv = {
	.chip_readb	= satamv_chip_readb,
	.chip_writeb	= satamv_chip_writeb,
	.shutdown	= satamv_shutdown,
};

/*
 * Random notes:
 * FCE#		Flash Chip Enable
 * FWE#		Flash Write Enable
 * FOE#		Flash Output Enable
 * FALE[1:0]	Flash Address Latch Enable
 * FAD[7:0]	Flash Multiplexed Address/Data Bus
 * FA[2:0]	Flash Address Low
 *
 * GPIO[15,2]	GPIO Port Mode
 * GPIO[4:3]	Flash Size
 *
 * 0xd2c	Expansion ROM BAR Control
 * 0xc08	PCI BAR2 (Flash/NVRAM) Control
 * 0x1046c	Flash Parameters
 */
static int satamv_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;
	uintptr_t addr;
	uint32_t tmp;
	uint16_t iobar;
	uint8_t *bar;

	if (rget_io_perms())
		return 1;

	/* BAR0 has all internal registers memory mapped. */
	dev = pcidev_init(cfg, satas_mv, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!addr)
		return 1;

	bar = rphysmap("Marvell 88SX7042 registers", addr, 0x20000);
	if (bar == ERROR_PTR)
		return 1;

	tmp = pci_mmio_readl(bar + FLASH_PARAM);
	msg_pspew("Flash Parameters:\n");
	msg_pspew("TurnOff=0x%01"PRIx32"\n", (tmp >> 0) & 0x7);
	msg_pspew("Acc2First=0x%01"PRIx32"\n", (tmp >> 3) & 0xf);
	msg_pspew("Acc2Next=0x%01"PRIx32"\n", (tmp >> 7) & 0xf);
	msg_pspew("ALE2Wr=0x%01"PRIx32"\n", (tmp >> 11) & 0x7);
	msg_pspew("WrLow=0x%01"PRIx32"\n", (tmp >> 14) & 0x7);
	msg_pspew("WrHigh=0x%01"PRIx32"\n", (tmp >> 17) & 0x7);
	msg_pspew("Reserved[21:20]=0x%01"PRIx32"\n", (tmp >> 20) & 0x3);
	msg_pspew("TurnOffExt=0x%01"PRIx32"\n", (tmp >> 22) & 0x1);
	msg_pspew("Acc2FirstExt=0x%01"PRIx32"\n", (tmp >> 23) & 0x1);
	msg_pspew("Acc2NextExt=0x%01"PRIx32"\n", (tmp >> 24) & 0x1);
	msg_pspew("ALE2WrExt=0x%01"PRIx32"\n", (tmp >> 25) & 0x1);
	msg_pspew("WrLowExt=0x%01"PRIx32"\n", (tmp >> 26) & 0x1);
	msg_pspew("WrHighExt=0x%01"PRIx32"\n", (tmp >> 27) & 0x1);
	msg_pspew("Reserved[31:28]=0x%01"PRIx32"\n", (tmp >> 28) & 0xf);

	tmp = pci_mmio_readl(bar + EXPANSION_ROM_BAR_CONTROL);
	msg_pspew("Expansion ROM BAR Control:\n");
	msg_pspew("ExpROMSz=0x%01"PRIx32"\n", (tmp >> 19) & 0x7);

	/* Enable BAR2 mapping to flash */
	tmp = pci_mmio_readl(bar + PCI_BAR2_CONTROL);
	msg_pspew("PCI BAR2 (Flash/NVRAM) Control:\n");
	msg_pspew("Bar2En=0x%01"PRIx32"\n", (tmp >> 0) & 0x1);
	msg_pspew("BAR2TransAttr=0x%01"PRIx32"\n", (tmp >> 1) & 0x1f);
	msg_pspew("BAR2Sz=0x%01"PRIx32"\n", (tmp >> 19) & 0x7);
	tmp &= 0xffffffc0;
	tmp |= 0x0000001f;
	pci_rmmio_writel(tmp, bar + PCI_BAR2_CONTROL);

	/* Enable flash: GPIO Port Control Register 0x104f0 */
	tmp = pci_mmio_readl(bar + GPIO_PORT_CONTROL);
	msg_pspew("GPIOPortMode=0x%01"PRIx32"\n", (tmp >> 0) & 0x3);
	if (((tmp >> 0) & 0x3) != 0x2)
		msg_pinfo("Warning! Either the straps are incorrect or you "
			  "have no flash or someone overwrote the strap "
			  "values!\n");
	tmp &= 0xfffffffc;
	tmp |= 0x2;
	pci_rmmio_writel(tmp, bar + GPIO_PORT_CONTROL);

	/* Get I/O BAR location. */
	addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_2);
	if (!addr)
		return 1;

	/* Truncate to reachable range.
	 * FIXME: Check if the I/O BAR is actually reachable.
	 * This is an arch specific check.
	 */
	iobar = addr & 0xffff;
	msg_pspew("Activating I/O BAR at 0x%04x\n", iobar);

	struct satamv_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		return 1;
	}
	data->bar = bar;
	data->iobar = iobar;

	/* 512 kByte with two 8-bit latches, and
	 * 4 MByte with additional 3-bit latch. */
	max_rom_decode.parallel = 4 * 1024 * 1024;
	return register_par_master(&par_master_satamv, BUS_PARALLEL, data);
}

const struct programmer_entry programmer_satamv = {
	.name			= "satamv",
	.type			= PCI,
	.devs.dev		= satas_mv,
	.init			= satamv_init,
};
