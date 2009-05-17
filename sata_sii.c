/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Rudolf Marek <r.marek@assembler.cz>
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
 * Foundation, , 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* Datasheets can be found on http://www.siliconimage.com. Great thanks! */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "flash.h"

#define PCI_VENDOR_ID_SII	0x1095

uint8_t *sii_bar;
uint16_t id;

struct pcidev_status satas_sii[] = {
	{0x1095, 0x0680, PCI_NT, "Silicon Image", "PCI0680 Ultra ATA-133 Host Controller"},
	{0x1095, 0x3114, PCI_OK, "Silicon Image", "SiI 3114 [SATALink/SATARaid] Serial ATA Controller"},
	{0x1095, 0x3124, PCI_NT, "Silicon Image", "SiI 3124 PCI-X Serial ATA Controller"},
	{0x1095, 0x3132, PCI_OK, "Silicon Image", "SiI 3132 Serial ATA Raid II Controller"},
	{0x1095, 0x3512, PCI_NT, "Silicon Image", "SiI 3512 [SATALink/SATARaid] Serial ATA Controller"},
	{},
};

int satasii_init(void)
{
	uint32_t addr;
	uint16_t reg_offset;

	get_io_perms();

	pcidev_init(PCI_VENDOR_ID_SII, satas_sii);

	id = pcidev_dev->device_id;

	if ((id == 0x3132) || (id == 0x3124)) {
		/* BAR 0, offset 0x70 */
		addr = pci_read_long(pcidev_dev, PCI_IO_BASE_ADDRESS) & ~0x07;
		reg_offset = 0x70;
	} else {
		/* BAR 5, offset 0x50 */
		addr = pci_read_long(pcidev_dev, PCI_IO_BASE_ADDRESS + (5 * 4)) & ~0x07;
		reg_offset = 0x50;
	}

	sii_bar = physmap("SATA SIL registers", addr, 0x100);
	sii_bar += reg_offset;

	/* check if rom cycle are OK */
	if (!(mmio_readl(sii_bar)) & (1 << 26)) {
		printf("Warning: Flash seems unconnected\n");
	}

	return 0;
}

int satasii_shutdown(void)
{

	free(pcidev_bdf);
	pci_cleanup(pacc);
#if defined(__FreeBSD__) || defined(__DragonFly__)
	close(io_fd);
#endif
	return 0;
}

void *satasii_map(const char *descr, unsigned long phys_addr, size_t len)
{
	return 0;
}

void satasii_unmap(void *virt_addr, size_t len)
{
}

void satasii_chip_writeb(uint8_t val, chipaddr addr)
{

	uint32_t ctrl_reg, addr_reg;

	while ((ctrl_reg = mmio_readl(sii_bar)) & (1 << 25)) ;

	/* Mask out unused/reserved bits, set writes and start transaction */
	ctrl_reg &= 0xfcf80000;
	ctrl_reg |= (1 << 25) | (0 << 24) | ((uint32_t) addr & 0x7ffff);

	addr_reg = (mmio_readl((sii_bar + 4)) & ~0xff) | val;
	mmio_writel(addr_reg, (sii_bar + 4));
	mmio_writel(ctrl_reg, sii_bar);

	while (mmio_readl(sii_bar) & (1 << 25)) ;

}

uint8_t satasii_chip_readb(const chipaddr addr)
{
	uint32_t ctrl_reg;

	while ((ctrl_reg = mmio_readl(sii_bar)) & (1 << 25)) ;

	/* Mask out unused/reserved bits, set reads and start transaction */
	ctrl_reg &= 0xfcf80000;
	ctrl_reg |= (1 << 25) | (1 << 24) | ((uint32_t) addr & 0x7ffff);

	mmio_writel(ctrl_reg, sii_bar);

	while ((mmio_readl(sii_bar)) & (1 << 25)) ;

	return (mmio_readl(sii_bar + 4)) & 0xff;
}
