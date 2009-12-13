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

void release_io_perms(void)
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
	close(io_fd);
#endif
}

#if INTERNAL_SUPPORT == 1
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

void internal_delay(int usecs)
{
	/* If the delay is >1 s, use usleep because timing does not need to
	 * be so precise.
	 */
	if (usecs > 1000000) {
		usleep(usecs);
	} else {
		myusec_delay(usecs);
	}
}

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
