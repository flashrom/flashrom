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
#include "flash.h"
#include "programmer.h"

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

#if CONFIG_INTERNAL == 1
int force_boardenable = 0;
int force_boardmismatch = 0;

#if defined(__i386__) || defined(__x86_64__)
struct superio superio = {};

void probe_superio(void)
{
	superio = probe_superio_ite();
#if 0
	/* Winbond Super I/O code is not yet available. */
	if (superio.vendor == SUPERIO_VENDOR_NONE)
		superio = probe_superio_winbond();
#endif
}
#endif

int is_laptop = 0;

int internal_init(void)
{
#if __FLASHROM_LITTLE_ENDIAN__
	int ret = 0;
#endif
	int force_laptop = 0;
	char *arg;

	arg = extract_programmer_param("boardenable");
	if (arg && !strcmp(arg,"force")) {
		force_boardenable = 1;
	} else if (arg && !strlen(arg)) {
		msg_perr("Missing argument for boardenable.\n");
		free(arg);
		return 1;
	} else if (arg) {
		msg_perr("Unknown argument for boardenable: %s\n", arg);
		free(arg);
		return 1;
	}
	free(arg);

	arg = extract_programmer_param("boardmismatch");
	if (arg && !strcmp(arg,"force")) {
		force_boardmismatch = 1;
	} else if (arg && !strlen(arg)) {
		msg_perr("Missing argument for boardmismatch.\n");
		free(arg);
		return 1;
	} else if (arg) {
		msg_perr("Unknown argument for boardmismatch: %s\n", arg);
		free(arg);
		return 1;
	}
	free(arg);

	arg = extract_programmer_param("laptop");
	if (arg && !strcmp(arg,"force_I_want_a_brick")) {
		force_laptop = 1;
	} else if (arg && !strlen(arg)) {
		msg_perr("Missing argument for laptop.\n");
		free(arg);
		return 1;
	} else if (arg) {
		msg_perr("Unknown argument for laptop: %s\n", arg);
		free(arg);
		return 1;
	}
	free(arg);

	get_io_perms();

	/* Initialize PCI access for flash enables */
	pacc = pci_alloc();	/* Get the pci_access structure */
	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);		/* Initialize the PCI library */
	pci_scan_bus(pacc);	/* We want to get the list of devices */

	if (processor_flash_enable()) {
		msg_perr("Processor detection/init failed.\n"
			 "Aborting.\n");
		return 1;
	}

#if defined(__i386__) || defined(__x86_64__)
	/* We look at the cbtable first to see if we need a
	 * mainboard specific flash enable sequence.
	 */
	coreboot_init();

	dmi_init();

	/* Probe for the Super I/O chip and fill global struct superio. */
	probe_superio();
#else
	/* FIXME: Enable cbtable searching on all non-x86 platforms supported
	 *        by coreboot.
	 * FIXME: Find a replacement for DMI on non-x86.
	 * FIXME: Enable Super I/O probing once port I/O is possible.
	 */
#endif

	/* Warn if a laptop is detected. */
	if (is_laptop) {
		msg_perr("========================================================================\n"
			 "WARNING! You seem to be running flashrom on a laptop.\n"
			 "Laptops, notebooks and netbooks are difficult to support and we recommend\n"
			 "to use the vendor flashing utility. The embedded controller (EC) in these\n"
			 "machines often interacts badly with flashing.\n"
			 "See http://www.flashrom.org/Laptops for details.\n\n"
			 "If flash is shared with the EC, erase is guaranteed to brick your laptop\n"
			 "and write may brick your laptop.\n"
			 "Read and probe may irritate your EC and cause fan failure, backlight\n"
			 "failure and sudden poweroff.\n"
			 "You have been warned.\n"
			 "========================================================================\n");
		if (force_laptop) {
			msg_perr("Proceeding anyway because user specified "
				 "laptop=force_I_want_a_brick\n");
		} else {
			msg_perr("Aborting.\n");
			exit(1);
		}
	}

#if __FLASHROM_LITTLE_ENDIAN__
	/* try to enable it. Failure IS an option, since not all motherboards
	 * really need this to be done, etc., etc.
	 */
	ret = chipset_flash_enable();
	if (ret == -2) {
		msg_perr("WARNING: No chipset found. Flash detection "
			 "will most likely fail.\n");
	}

#if defined(__i386__) || defined(__x86_64__)
	/* Probe unconditionally for IT87* LPC->SPI translation and for
	 * IT87* Parallel write enable.
	 */
	init_superio_ite();
#endif

	board_flash_enable(lb_vendor, lb_part);

	/* Even if chipset init returns an error code, we don't want to abort.
	 * The error code might have been a warning only.
	 * Besides that, we don't check the board enable return code either.
	 */
#if defined(__i386__) || defined(__x86_64__) || defined (__mips)
	return 0;
#else
	msg_perr("Your platform is not supported yet for the internal "
		 "programmer due to missing\n"
		 "flash_base and top/bottom alignment information.\n"
		 "Aborting.\n");
	return 1;
#endif
#else
	/* FIXME: Remove this unconditional abort once all PCI drivers are
	 * converted to use little-endian accesses for memory BARs.
	 */
	msg_perr("Your platform is not supported yet for the internal "
		 "programmer because it has\n"
		 "not been converted from native endian to little endian "
		 "access yet.\n"
		 "Aborting.\n");
	return 1;
#endif
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
