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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* Datasheets are not public (yet?) */
#if defined(__i386__) || defined(__x86_64__)

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"

uint8_t *mv_bar;
uint16_t mv_iobar;

const struct pcidev_status satas_mv[] = {
	/* 88SX6041 and 88SX6042 are the same according to the datasheet. */
	{0x11ab, 0x7042, OK, "Marvell", "88SX7042 PCI-e 4-port SATA-II"},

	{},
};

#define NVRAM_PARAM			0x1045c
#define FLASH_PARAM			0x1046c
#define EXPANSION_ROM_BAR_CONTROL	0x00d2c
#define PCI_BAR2_CONTROL		0x00c08
#define GPIO_PORT_CONTROL		0x104f0

static int satamv_shutdown(void *data)
{
	physunmap(mv_bar, 0x20000);
	pci_cleanup(pacc);
	release_io_perms();
	return 0;
}

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
int satamv_init(void)
{
	uintptr_t addr;
	uint32_t tmp;

	get_io_perms();

	/* BAR0 has all internal registers memory mapped. */
	/* No need to check for errors, pcidev_init() will not return in case
	 * of errors.
	 */
	addr = pcidev_init(PCI_BASE_ADDRESS_0, satas_mv);

	mv_bar = physmap("Marvell 88SX7042 registers", addr, 0x20000);
	if (mv_bar == ERROR_PTR)
		goto error_out;

	if (register_shutdown(satamv_shutdown, NULL))
		return 1;

	tmp = pci_mmio_readl(mv_bar + FLASH_PARAM);
	msg_pspew("Flash Parameters:\n");
	msg_pspew("TurnOff=0x%01x\n", (tmp >> 0) & 0x7);
	msg_pspew("Acc2First=0x%01x\n", (tmp >> 3) & 0xf);
	msg_pspew("Acc2Next=0x%01x\n", (tmp >> 7) & 0xf);
	msg_pspew("ALE2Wr=0x%01x\n", (tmp >> 11) & 0x7);
	msg_pspew("WrLow=0x%01x\n", (tmp >> 14) & 0x7);
	msg_pspew("WrHigh=0x%01x\n", (tmp >> 17) & 0x7);
	msg_pspew("Reserved[21:20]=0x%01x\n", (tmp >> 20) & 0x3);
	msg_pspew("TurnOffExt=0x%01x\n", (tmp >> 22) & 0x1);
	msg_pspew("Acc2FirstExt=0x%01x\n", (tmp >> 23) & 0x1);
	msg_pspew("Acc2NextExt=0x%01x\n", (tmp >> 24) & 0x1);
	msg_pspew("ALE2WrExt=0x%01x\n", (tmp >> 25) & 0x1);
	msg_pspew("WrLowExt=0x%01x\n", (tmp >> 26) & 0x1);
	msg_pspew("WrHighExt=0x%01x\n", (tmp >> 27) & 0x1);
	msg_pspew("Reserved[31:28]=0x%01x\n", (tmp >> 28) & 0xf);

	tmp = pci_mmio_readl(mv_bar + EXPANSION_ROM_BAR_CONTROL);
	msg_pspew("Expansion ROM BAR Control:\n");
	msg_pspew("ExpROMSz=0x%01x\n", (tmp >> 19) & 0x7);

	/* Enable BAR2 mapping to flash */
	tmp = pci_mmio_readl(mv_bar + PCI_BAR2_CONTROL);
	msg_pspew("PCI BAR2 (Flash/NVRAM) Control:\n");
	msg_pspew("Bar2En=0x%01x\n", (tmp >> 0) & 0x1);
	msg_pspew("BAR2TransAttr=0x%01x\n", (tmp >> 1) & 0x1f);
	msg_pspew("BAR2Sz=0x%01x\n", (tmp >> 19) & 0x7);
	tmp &= 0xffffffc0;
	tmp |= 0x0000001f;
	pci_rmmio_writel(tmp, mv_bar + PCI_BAR2_CONTROL);

	/* Enable flash: GPIO Port Control Register 0x104f0 */
	tmp = pci_mmio_readl(mv_bar + GPIO_PORT_CONTROL);
	msg_pspew("GPIOPortMode=0x%01x\n", (tmp >> 0) & 0x3);
	if (((tmp >> 0) & 0x3) != 0x2)
		msg_pinfo("Warning! Either the straps are incorrect or you "
			  "have no flash or someone overwrote the strap "
			  "values!\n");
	tmp &= 0xfffffffc;
	tmp |= 0x2;
	pci_rmmio_writel(tmp, mv_bar + GPIO_PORT_CONTROL);

	/* Get I/O BAR location. */
	tmp = pci_read_long(pcidev_dev, PCI_BASE_ADDRESS_2) &
	      PCI_BASE_ADDRESS_IO_MASK;
	/* Truncate to reachable range.
	 * FIXME: Check if the I/O BAR is actually reachable.
	 * This is an arch specific check.
	 */
	mv_iobar = tmp & 0xffff;
	msg_pspew("Activating I/O BAR at 0x%04x\n", mv_iobar);

	buses_supported = BUS_PARALLEL;

	/* 512 kByte with two 8-bit latches, and
	 * 4 MByte with additional 3-bit latch. */
	max_rom_decode.parallel = 4 * 1024 * 1024;

	return 0;

error_out:
	pci_cleanup(pacc);
	release_io_perms();
	return 1;
}

/* BAR2 (MEM) can map NVRAM and flash. We set it to flash in the init function.
 * If BAR2 is disabled, it still can be accessed indirectly via BAR1 (I/O).
 * This code only supports indirect accesses for now.
 */

/* Indirect access to via the I/O BAR1. */
static void satamv_indirect_chip_writeb(uint8_t val, chipaddr addr)
{
	/* 0x80000000 selects BAR2 for remapping. */
	OUTL(((uint32_t)addr | 0x80000000) & 0xfffffffc, mv_iobar);
	OUTB(val, mv_iobar + 0x80 + (addr & 0x3));
}

/* Indirect access to via the I/O BAR1. */
static uint8_t satamv_indirect_chip_readb(const chipaddr addr)
{
	/* 0x80000000 selects BAR2 for remapping. */
	OUTL(((uint32_t)addr | 0x80000000) & 0xfffffffc, mv_iobar);
	return INB(mv_iobar + 0x80 + (addr & 0x3));
}

/* FIXME: Prefer direct access to BAR2 if BAR2 is active. */
void satamv_chip_writeb(uint8_t val, chipaddr addr)
{
	satamv_indirect_chip_writeb(val, addr);
}

/* FIXME: Prefer direct access to BAR2 if BAR2 is active. */
uint8_t satamv_chip_readb(const chipaddr addr)
{
	return satamv_indirect_chip_readb(addr);
}

#else
#error PCI port I/O access is not supported on this architecture yet.
#endif
