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
 */

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

struct pci_dev *pci_dev_find_vendorclass(uint16_t vendor, uint16_t devclass)
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
			if (tmp2 == devclass)
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

int force_boardenable = 0;
int force_boardmismatch = 0;

#if IS_X86
void probe_superio(void)
{
	probe_superio_winbond();
	/* ITE probe causes SMSC LPC47N217 to power off the serial UART.
	 * Always probe for SMSC first, and if a SMSC Super I/O is detected
	 * at a given I/O port, do _not_ probe that port with the ITE probe.
	 * This means SMSC probing must be done before ITE probing.
	 */
	//probe_superio_smsc();
	probe_superio_ite();
}

int superio_count = 0;
#define SUPERIO_MAX_COUNT 3

struct superio superios[SUPERIO_MAX_COUNT];

int register_superio(struct superio s)
{
	if (superio_count == SUPERIO_MAX_COUNT)
		return 1;
	superios[superio_count++] = s;
	return 0;
}

#endif

int is_laptop = 0;
int laptop_ok = 0;

static void internal_chip_writeb(const struct flashctx *flash, uint8_t val,
				 chipaddr addr);
static void internal_chip_writew(const struct flashctx *flash, uint16_t val,
				 chipaddr addr);
static void internal_chip_writel(const struct flashctx *flash, uint32_t val,
				 chipaddr addr);
static uint8_t internal_chip_readb(const struct flashctx *flash,
				   const chipaddr addr);
static uint16_t internal_chip_readw(const struct flashctx *flash,
				    const chipaddr addr);
static uint32_t internal_chip_readl(const struct flashctx *flash,
				    const chipaddr addr);
static void internal_chip_readn(const struct flashctx *flash, uint8_t *buf,
				const chipaddr addr, size_t len);
static const struct par_master par_master_internal = {
		.chip_readb		= internal_chip_readb,
		.chip_readw		= internal_chip_readw,
		.chip_readl		= internal_chip_readl,
		.chip_readn		= internal_chip_readn,
		.chip_writeb		= internal_chip_writeb,
		.chip_writew		= internal_chip_writew,
		.chip_writel		= internal_chip_writel,
		.chip_writen		= fallback_chip_writen,
};

enum chipbustype internal_buses_supported = BUS_NONE;

int internal_init(void)
{
	int ret = 0;
	int force_laptop = 0;
	int not_a_laptop = 0;
	char *board_vendor = NULL;
	char *board_model = NULL;
#if IS_X86
	const char *cb_vendor = NULL;
	const char *cb_model = NULL;
#endif
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
	if (arg && !strcmp(arg, "force_I_want_a_brick"))
		force_laptop = 1;
	else if (arg && !strcmp(arg, "this_is_not_a_laptop"))
		not_a_laptop = 1;
	else if (arg && !strlen(arg)) {
		msg_perr("Missing argument for laptop.\n");
		free(arg);
		return 1;
	} else if (arg) {
		msg_perr("Unknown argument for laptop: %s\n", arg);
		free(arg);
		return 1;
	}
	free(arg);

	arg = extract_programmer_param("mainboard");
	if (arg && strlen(arg)) {
		if (board_parse_parameter(arg, &board_vendor, &board_model)) {
			free(arg);
			return 1;
		}
	} else if (arg && !strlen(arg)) {
		msg_perr("Missing argument for mainboard.\n");
		free(arg);
		return 1;
	}
	free(arg);

	if (rget_io_perms()) {
		ret = 1;
		goto internal_init_exit;
	}

	/* Default to Parallel/LPC/FWH flash devices. If a known host controller
	 * is found, the host controller init routine sets the
	 * internal_buses_supported bitfield.
	 */
	internal_buses_supported = BUS_NONSPI;

	if (try_mtd() == 0) {
		ret = 0;
		goto internal_init_exit;
	}

	/* Initialize PCI access for flash enables */
	if (pci_init_common() != 0) {
		ret = 1;
		goto internal_init_exit;
	}

	if (processor_flash_enable()) {
		msg_perr("Processor detection/init failed.\n"
			 "Aborting.\n");
		ret = 1;
		goto internal_init_exit;
	}

#if IS_X86
	if ((cb_parse_table(&cb_vendor, &cb_model) == 0) && (board_vendor != NULL) && (board_model != NULL)) {
		if (strcasecmp(board_vendor, cb_vendor) || strcasecmp(board_model, cb_model)) {
			msg_pwarn("Warning: The mainboard IDs set by -p internal:mainboard (%s:%s) do not\n"
				  "         match the current coreboot IDs of the mainboard (%s:%s).\n",
				  board_vendor, board_model, cb_vendor, cb_model);
			if (!force_boardmismatch) {
				ret = 1;
				goto internal_init_exit;
			}
			msg_pinfo("Continuing anyway.\n");
		}
	}
#endif

#if IS_X86
	is_laptop = 2; /* Assume that we don't know by default. */

	dmi_init();

	/* In case Super I/O probing would cause pretty explosions. */
	board_handle_before_superio();

	/* Probe for the Super I/O chip and fill global struct superio. */
	probe_superio();
#else
	/* FIXME: Enable cbtable searching on all non-x86 platforms supported
	 *        by coreboot.
	 * FIXME: Find a replacement for DMI on non-x86.
	 * FIXME: Enable Super I/O probing once port I/O is possible.
	 */
#endif

	/* Check laptop whitelist. */
	board_handle_before_laptop();

	/*
	 * Disable all internal buses by default if we are not sure
	 * this isn't a laptop. Board-enables may override this,
	 * non-legacy buses (SPI and opaque atm) are probed anyway.
	 */
	if (is_laptop && !(laptop_ok || force_laptop || (not_a_laptop && is_laptop == 2)))
		internal_buses_supported = BUS_NONE;

	/* try to enable it. Failure IS an option, since not all motherboards
	 * really need this to be done, etc., etc.
	 */
	ret = chipset_flash_enable();
	if (ret == -2) {
		msg_perr("WARNING: No chipset found. Flash detection "
			 "will most likely fail.\n");
	} else if (ret == ERROR_FATAL) {
		goto internal_init_exit;
	}

#if IS_X86
	/* Probe unconditionally for ITE Super I/O chips. This enables LPC->SPI translation on IT87* and
	 * parallel writes on IT8705F. Also, this handles the manual chip select for Gigabyte's DualBIOS. */
	init_superio_ite();

	if (board_flash_enable(board_vendor, board_model, cb_vendor, cb_model)) {
		msg_perr("Aborting to be safe.\n");
		ret = 1;
		goto internal_init_exit;
	}
#endif

	if (internal_buses_supported & BUS_NONSPI)
		register_par_master(&par_master_internal, internal_buses_supported);

	/* Report if a non-whitelisted laptop is detected that likely uses a legacy bus. */
	if (is_laptop && !laptop_ok) {
		msg_pinfo("========================================================================\n");
		if (is_laptop == 1) {
			msg_pinfo("You seem to be running flashrom on an unknown laptop. Some\n"
				  "internal buses have been disabled for safety reasons.\n\n");
		} else {
			msg_pinfo("You may be running flashrom on an unknown laptop. We could not\n"
				  "detect this for sure because your vendor has not set up the SMBIOS\n"
				  "tables correctly. Some internal buses have been disabled for\n"
				  "safety reasons. You can enforce using all buses by adding\n"
				  "  -p internal:laptop=this_is_not_a_laptop\n"
				  "to the command line, but please read the following warning if you\n"
				  "are not sure.\n\n");
		}
		msg_perr("Laptops, notebooks and netbooks are difficult to support and we\n"
			 "recommend to use the vendor flashing utility. The embedded controller\n"
			 "(EC) in these machines often interacts badly with flashing.\n"
			 "See the manpage and https://flashrom.org/Laptops for details.\n\n"
			 "If flash is shared with the EC, erase is guaranteed to brick your laptop\n"
			 "and write may brick your laptop.\n"
			 "Read and probe may irritate your EC and cause fan failure, backlight\n"
			 "failure and sudden poweroff.\n"
			 "You have been warned.\n"
			 "========================================================================\n");
	}

	ret = 0;

internal_init_exit:
	free(board_vendor);
	free(board_model);

	return ret;
}

static void internal_chip_writeb(const struct flashctx *flash, uint8_t val,
				 chipaddr addr)
{
	mmio_writeb(val, (void *) addr);
}

static void internal_chip_writew(const struct flashctx *flash, uint16_t val,
				 chipaddr addr)
{
	mmio_writew(val, (void *) addr);
}

static void internal_chip_writel(const struct flashctx *flash, uint32_t val,
				 chipaddr addr)
{
	mmio_writel(val, (void *) addr);
}

static uint8_t internal_chip_readb(const struct flashctx *flash,
				   const chipaddr addr)
{
	return mmio_readb((void *) addr);
}

static uint16_t internal_chip_readw(const struct flashctx *flash,
				    const chipaddr addr)
{
	return mmio_readw((void *) addr);
}

static uint32_t internal_chip_readl(const struct flashctx *flash,
				    const chipaddr addr)
{
	return mmio_readl((void *) addr);
}

static void internal_chip_readn(const struct flashctx *flash, uint8_t *buf,
				const chipaddr addr, size_t len)
{
	mmio_readn((void *)addr, buf, len);
	return;
}
