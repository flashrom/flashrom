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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "flash.h"

#if NEED_PCI == 1
struct pci_dev *pci_dev_find_filter(struct pci_filter filter)
{
	struct pci_dev *temp;

	for (temp = pacc->devices; temp; temp = temp->next)
		if (pci_filter_match(&filter, temp))
			return temp;

	return NULL;
}

struct pci_dev *pci_dev_find_vendorclass(uint16_t vendor, uint16_t class)
{
	struct pci_dev *temp;
	struct pci_filter filter;
	uint16_t tmp2;

	pci_filter_init(NULL, &filter);
	filter.vendor = vendor;

	for (temp = pacc->devices; temp; temp = temp->next)
		if (pci_filter_match(&filter, temp)) {
			/* Read PCI class */
			tmp2 = pci_read_word(temp, 0x0a);
			if (tmp2 == class)
				return temp;
		}

	return NULL;
}

struct pci_dev *pci_dev_find(uint16_t vendor, uint16_t device)
{
	struct pci_dev *temp;
	struct pci_filter filter;

	pci_filter_init(NULL, &filter);
	filter.vendor = vendor;
	filter.device = device;

	for (temp = pacc->devices; temp; temp = temp->next)
		if (pci_filter_match(&filter, temp))
			return temp;

	return NULL;
}

struct pci_dev *pci_card_find(uint16_t vendor, uint16_t device,
			      uint16_t card_vendor, uint16_t card_device)
{
	struct pci_dev *temp;
	struct pci_filter filter;

	pci_filter_init(NULL, &filter);
	filter.vendor = vendor;
	filter.device = device;

	for (temp = pacc->devices; temp; temp = temp->next)
		if (pci_filter_match(&filter, temp)) {
			if ((card_vendor ==
			     pci_read_word(temp, PCI_SUBSYSTEM_VENDOR_ID))
			    && (card_device ==
				pci_read_word(temp, PCI_SUBSYSTEM_ID)))
				return temp;
		}

	return NULL;
}
#endif

#if INTERNAL_SUPPORT == 1
struct superio superio = {};

void probe_superio(void)
{
	superio = probe_superio_ite();
#if 0	/* Winbond SuperI/O code is not yet available. */
	if (superio.vendor == SUPERIO_VENDOR_NONE)
		superio = probe_superio_winbond();
#endif
}

int internal_init(void)
{
	int ret = 0;

	get_io_perms();

	/* Initialize PCI access for flash enables */
	pacc = pci_alloc();	/* Get the pci_access structure */
	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);		/* Initialize the PCI library */
	pci_scan_bus(pacc);	/* We want to get the list of devices */

	/* We look at the lbtable first to see if we need a
	 * mainboard specific flash enable sequence.
	 */
	coreboot_init();
	dmi_init();

	/* Probe for the SuperI/O chip and fill global struct superio. */
	probe_superio();

	/* try to enable it. Failure IS an option, since not all motherboards
	 * really need this to be done, etc., etc.
	 */
	ret = chipset_flash_enable();
	if (ret == -2) {
		printf("WARNING: No chipset found. Flash detection "
		       "will most likely fail.\n");
	}

	board_flash_enable(lb_vendor, lb_part);

	/* Even if chipset init returns an error code, we don't want to abort.
	 * The error code might have been a warning only.
	 * Besides that, we don't check the board enable return code either.
	 */
	return 0; 
}

int internal_shutdown(void)
{
	release_io_perms();

	return 0;
}
#endif

void internal_chip_writeb(uint8_t val, chipaddr addr)
{
	mmio_writeb(val, (void *) addr);
}

void internal_chip_writew(uint16_t val, chipaddr addr)
{
	mmio_writew(val, (void *) addr);
}

void internal_chip_writel(uint32_t val, chipaddr addr)
{
	mmio_writel(val, (void *) addr);
}

uint8_t internal_chip_readb(const chipaddr addr)
{
	return mmio_readb((void *) addr);
}

uint16_t internal_chip_readw(const chipaddr addr)
{
	return mmio_readw((void *) addr);
}

uint32_t internal_chip_readl(const chipaddr addr)
{
	return mmio_readl((void *) addr);
}

void internal_chip_readn(uint8_t *buf, const chipaddr addr, size_t len)
{
	memcpy(buf, (void *)addr, len);
	return;
}
