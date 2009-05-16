/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
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

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "flash.h"

#define PCI_IO_BASE_ADDRESS 0x10

uint32_t io_base_addr;
struct pci_access *pacc;
struct pci_filter filter;
char *pcidev_bdf = NULL;
struct pci_dev *pcidev_dev = NULL;

uint32_t pcidev_validate(struct pci_dev *dev, struct pcidev_status *devs)
{
	int i;
	uint32_t addr;

	for (i = 0; devs[i].device_name != NULL; i++) {
		if (dev->device_id != devs[i].device_id)
			continue;

		/* Don't use dev->base_addr[0], won't work on older libpci. */
		addr = pci_read_long(dev, PCI_IO_BASE_ADDRESS) & ~0x03;

		printf("Found \"%s %s\" (%04x:%04x, BDF %02x:%02x.%x)\n",
		       devs[i].vendor_name, devs[i].device_name, dev->vendor_id,
		       dev->device_id, dev->bus, dev->dev, dev->func);

		if (devs[i].status == PCI_NT) {
			printf("===\nThis PCI device is UNTESTED. Please email "
			       "a report including the 'flashrom -p xxxxxx'\n"
			       "output to flashrom@coreboot.org if it works "
			       "for you. Thank you for your help!\n===\n");
		}

		return addr;
	}

	return 0;
}

uint32_t pcidev_init(uint16_t vendor_id, struct pcidev_status *devs)
{
	struct pci_dev *dev;
	char *msg = NULL;
	int found = 0;
	uint32_t addr = 0, curaddr = 0;

	pacc = pci_alloc();     /* Get the pci_access structure */
	pci_init(pacc);         /* Initialize the PCI library */
	pci_scan_bus(pacc);     /* We want to get the list of devices */
	pci_filter_init(pacc, &filter);

	/* Filter by vendor and also bb:dd.f (if supplied by the user). */
	filter.vendor = vendor_id;
	if (pcidev_bdf != NULL) {
		if ((msg = pci_filter_parse_slot(&filter, pcidev_bdf))) {
			fprintf(stderr, "Error: %s\n", msg);
			exit(1);
		}
	}

	for (dev = pacc->devices; dev; dev = dev->next) {
		if (pci_filter_match(&filter, dev)) {
			if ((addr = pcidev_validate(dev, devs)) != 0) {
				curaddr = addr;
				pcidev_dev = dev;
				found++;
			}
		}
	}

	/* Only continue if exactly one supported PCI dev has been found. */
	if (found == 0) {
		fprintf(stderr, "Error: No supported PCI device found.\n");
		exit(1);
	} else if (found > 1) {
		fprintf(stderr, "Error: Multiple supported PCI devices found. "
			"Please use 'flashrom -p xxxxxx=bb:dd.f' \n"
			"to explicitly select the card with the given BDF "
			"(PCI bus, device, function).\n");
		exit(1);
	}

	return curaddr;
}

void print_supported_pcidevs(struct pcidev_status *devs)
{
	int i;

	for (i = 0; devs[i].vendor_name != NULL; i++) {
		printf("%s %s [%02x:%02x]%s\n", devs[i].vendor_name,
		       devs[i].device_name, devs[i].vendor_id,
		       devs[i].device_id,
		       (devs[i].status == PCI_NT) ? " (untested)" : "");
	}
}
