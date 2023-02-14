/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Jonathan Kollasch <jakllsch@kollasch.net>
 * Copyright (C) 2012-2013 Stefan Tauner
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

#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "programmer.h"
#include "platform/pci.h"

#define PCI_VENDOR_ID_VIA	0x1106

#define VIA_MAX_RETRIES		300

#define BROM_ADDR		0x60

#define BROM_DATA		0x64

#define BROM_ACCESS		0x68
#define BROM_TRIGGER		0x80
#define BROM_WRITE		0x40
#define BROM_SIZE_MASK		0x30
#define BROM_SIZE_64K		0x00
#define BROM_SIZE_32K		0x10
#define BROM_SIZE_16K		0x20
#define BROM_SIZE_0K		0x30
#define BROM_BYTE_ENABLE_MASK	0x0f

#define BROM_STATUS		0x69
#define BROM_ERROR_STATUS	0x80

/* Select the byte we want to access. This is done by clearing the bit corresponding to the byte we want to
 * access, leaving the others set (yes, really). */
#define ENABLE_BYTE(address)	((~(1 << ((address) & 3))) & BROM_BYTE_ENABLE_MASK)
#define BYTE_OFFSET(address)	(((address) & 3) * 8)

static const struct dev_entry ata_via[] = {
	{PCI_VENDOR_ID_VIA, 0x3249, DEP, "VIA", "VT6421A"},

	{0},
};

static void *atavia_offset = NULL;
static struct pci_dev *dev = NULL;

static void atavia_prettyprint_access(uint8_t access)
{
	uint8_t bmask = access & BROM_BYTE_ENABLE_MASK;
	uint8_t size = access & BROM_SIZE_MASK;

	msg_pspew("Accessing byte(s):%s%s%s%s\n",
		  ((bmask & (1<<3)) == 0) ? " 3" : "",
		  ((bmask & (1<<2)) == 0) ? " 2" : "",
		  ((bmask & (1<<1)) == 0) ? " 1" : "",
		  ((bmask & (1<<0)) == 0) ? " 0" : "");
	if (size == BROM_SIZE_0K) {
		msg_pspew("No ROM device found.\n");
	} else
		msg_pspew("ROM device with %s kB attached.\n",
			  (size == BROM_SIZE_64K) ? ">=64" :
			  (size == BROM_SIZE_32K) ? "32" : "16");
	msg_pspew("Access is a %s.\n", (access & BROM_WRITE) ? "write" : "read");
	msg_pspew("Device is %s.\n", (access & BROM_TRIGGER) ? "busy" : "ready");
}

static bool atavia_ready(struct pci_dev *pcidev_dev)
{
	int try;
	uint8_t access, status;
	bool ready = false;

	for (try = 0; try < VIA_MAX_RETRIES; try++) {
		access = pci_read_byte(pcidev_dev, BROM_ACCESS);
		status = pci_read_byte(pcidev_dev, BROM_STATUS);
		if (((access & BROM_TRIGGER) == 0) && (status & BROM_ERROR_STATUS) == 0) {
			ready = true;
			break;
		} else {
			default_delay(1);
			continue;
		}
	}

	msg_pdbg2("\n%s: %s after %d tries (access=0x%02x, status=0x%02x)\n",
		  __func__, ready ? "succeeded" : "failed", try, access, status);
	atavia_prettyprint_access(access);
	return ready;
}

static void *atavia_map(const char *descr, uintptr_t phys_addr, size_t len)
{
	return (atavia_offset != 0) ? atavia_offset : (void *)phys_addr;
}

static void atavia_chip_writeb(const struct flashctx *flash, uint8_t val, const chipaddr addr)
{
	msg_pspew("%s: 0x%02x to 0x%*" PRIxPTR ".\n", __func__, val, PRIxPTR_WIDTH, addr);
	pci_write_long(dev, BROM_ADDR, (addr & ~3));
	pci_write_long(dev, BROM_DATA, val << BYTE_OFFSET(addr));
	pci_write_byte(dev, BROM_ACCESS, BROM_TRIGGER | BROM_WRITE | ENABLE_BYTE(addr));

	if (!atavia_ready(dev)) {
		msg_perr("not ready after write\n");
	}
}

static uint8_t atavia_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	pci_write_long(dev, BROM_ADDR, (addr & ~3));
	pci_write_byte(dev, BROM_ACCESS, BROM_TRIGGER | ENABLE_BYTE(addr));

	if (!atavia_ready(dev)) {
		msg_perr("not ready after read\n");
	}

	uint8_t val = (pci_read_long(dev, BROM_DATA) >> BYTE_OFFSET(addr)) & 0xff;
	msg_pspew("%s: 0x%02x from 0x%*" PRIxPTR ".\n", __func__, val, PRIxPTR_WIDTH, addr);
	return val;
}

static const struct par_master lpc_master_atavia = {
	.map_flash_region	= atavia_map,
	.chip_readb	= atavia_chip_readb,
	.chip_writeb	= atavia_chip_writeb,
};

static int atavia_init(const struct programmer_cfg *cfg)
{
	char *arg = extract_programmer_param_str(cfg, "offset");
	if (arg) {
		if (strlen(arg) == 0) {
			msg_perr("Missing argument for offset.\n");
			free(arg);
			return ERROR_FLASHROM_FATAL;
		}
		char *endptr;
		atavia_offset = (void *)strtoul(arg, &endptr, 0);
		if (*endptr) {
			msg_perr("Error: Invalid offset specified: \"%s\".\n", arg);
			free(arg);
			return ERROR_FLASHROM_FATAL;
		}
		msg_pinfo("Mapping addresses to base %p.\n", atavia_offset);
	}
	free(arg);

	dev = pcidev_init(cfg, ata_via, PCI_ROM_ADDRESS); /* Actually no BAR setup needed at all. */
	if (!dev)
		return 1;

	/* Test if a flash chip is attached. */
	pci_write_long(dev, PCI_ROM_ADDRESS, (uint32_t)PCI_ROM_ADDRESS_MASK);
	default_delay(90);
	uint32_t base = pci_read_long(dev, PCI_ROM_ADDRESS);
	msg_pdbg2("BROM base=0x%08"PRIx32"\n", base);
	if ((base & PCI_ROM_ADDRESS_MASK) == 0) {
		msg_pwarn("Controller thinks there is no ROM attached.\n");
	}

	if (!atavia_ready(dev)) {
		msg_perr("Controller not ready.\n");
		return 1;
	}

	return register_par_master(&lpc_master_atavia, BUS_LPC, NULL);
}

const struct programmer_entry programmer_atavia = {
	.name			= "atavia",
	.type			= PCI,
	.devs.dev		= ata_via,
	.init			= atavia_init,
};
