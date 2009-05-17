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

#if defined(__FreeBSD__) || defined(__DragonFly__)
int io_fd;
#endif

struct pci_dev *pci_dev_find_filter(struct pci_filter filter)
{
	struct pci_dev *temp;

	for (temp = pacc->devices; temp; temp = temp->next)
		if (pci_filter_match(&filter, temp))
			return temp;

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

void get_io_perms(void)
{
#if defined (__sun) && (defined(__i386) || defined(__amd64))
	if (sysi86(SI86V86, V86SC_IOPL, PS_IOPL) != 0) {
#elif defined(__FreeBSD__) || defined (__DragonFly__)
	if ((io_fd = open("/dev/io", O_RDWR)) < 0) {
#else
	if (iopl(3) != 0) {
#endif
		fprintf(stderr, "ERROR: Could not get I/O privileges (%s).\n"
			"You need to be root.\n", strerror(errno));
		exit(1);
	}
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

	/* try to enable it. Failure IS an option, since not all motherboards
	 * really need this to be done, etc., etc.
	 */
	ret = chipset_flash_enable();
	if (ret == -2) {
		printf("WARNING: No chipset found. Flash detection "
		       "will most likely fail.\n");
	}

	board_flash_enable(lb_vendor, lb_part);

	return ret; 
}

int internal_shutdown(void)
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
	close(io_fd);
#endif

	return 0;
}

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

void mmio_writeb(uint8_t val, void *addr)
{
	*(volatile uint8_t *) addr = val;
}

void mmio_writew(uint16_t val, void *addr)
{
	*(volatile uint16_t *) addr = val;
}

void mmio_writel(uint32_t val, void *addr)
{
	*(volatile uint32_t *) addr = val;
}

uint8_t mmio_readb(void *addr)
{
	return *(volatile uint8_t *) addr;
}

uint16_t mmio_readw(void *addr)
{
	return *(volatile uint16_t *) addr;
}

uint32_t mmio_readl(void *addr)
{
	return *(volatile uint32_t *) addr;
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
