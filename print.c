/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2009 Carl-Daniel Hailfinger
 * Copyright (C) 2011-2013 Stefan Tauner
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#if HAVE_UTSNAME == 1
#include <sys/utsname.h>
#endif
#if IS_WINDOWS
#include <windows.h>
#undef min
#undef max
#endif

#include "flash.h"
#include "programmer.h"

static const char *test_state_to_text(enum test_state test_state)
{
	switch (test_state) {
	case OK: return "OK";
	case BAD: return "Not working";
	case NA: return "N/A";
	case DEP: return "Config-dependent";
	case NT:
	default: return "Untested";
	}
}

static int print_supported_chips(void)
{
	const char *delim = "/";
	const int mintoklen = 5;
	const int border = 2;
	int i, chipcount = 0;
	int maxvendorlen = strlen("Vendor") + 1;
	int maxchiplen = strlen("Device") + 1;
	int maxtypelen = strlen("Type") + 1;
	const struct flashchip *chip;
	char *s;
	char *ven, *dev;
	char *tmpven, *tmpdev, *tmpven_save, *tmpdev_save;
	int tmpvenlen, tmpdevlen, curvenlen, curdevlen;

	/* calculate maximum column widths and by iterating over all chips */
	for (chip = flashchips; chip->name != NULL; chip++) {
		/* Ignore generic entries. */
		if (!strncmp(chip->vendor, "Unknown", 7) ||
		    !strncmp(chip->vendor, "Programmer", 10) ||
		    !strncmp(chip->name, "unknown", 7))
			continue;
		chipcount++;

		/* Find maximum vendor length (respecting line splitting). */
		tmpven = (char *)chip->vendor;
		do {
			/* and take minimum token lengths into account */
			tmpvenlen = 0;
			do {
				tmpvenlen += strcspn(tmpven, delim);
				/* skip to the address after the first token */
				tmpven += tmpvenlen;
				if (tmpven[0] == '\0')
					break;
				tmpven++;
			} while (tmpvenlen < mintoklen);
			maxvendorlen = max(maxvendorlen, tmpvenlen);
			if (tmpven[0] == '\0')
				break;
		} while (1);

		/* same for device name */
		tmpdev = (char *)chip->name;
		do {
			tmpdevlen = 0;
			do {
				tmpdevlen += strcspn(tmpdev, delim);
				tmpdev += tmpdevlen;
				if (tmpdev[0] == '\0')
					break;
				tmpdev++;
			} while (tmpdevlen < mintoklen);
			maxchiplen = max(maxchiplen, tmpdevlen);
			if (tmpdev[0] == '\0')
				break;
		} while (1);

		s = flashbuses_to_text(chip->bustype);
		if (s == NULL) {
			msg_gerr("Out of memory!\n");
			return 1;
		}
		maxtypelen = max(maxtypelen, strlen(s));
		free(s);
	}
	maxvendorlen += border;
	maxchiplen += border;
	maxtypelen += border;

	msg_ginfo("Supported flash chips (total: %d):\n\n", chipcount);
	msg_ginfo("Vendor");
	for (i = strlen("Vendor"); i < maxvendorlen; i++)
		msg_ginfo(" ");
	msg_ginfo("Device");
	for (i = strlen("Device"); i < maxchiplen; i++)
		msg_ginfo(" ");

	msg_ginfo("Test ");
	for (i = 0; i < border; i++)
		msg_ginfo(" ");
	msg_ginfo("Known");
	for (i = 0; i < border; i++)
		msg_ginfo(" ");
	msg_ginfo(" Size ");
	for (i = 0; i < border; i++)
		msg_ginfo(" ");

	msg_ginfo("Type");
	for (i = strlen("Type"); i < maxtypelen; i++)
		msg_ginfo(" ");
	msg_gdbg("Voltage");
	msg_ginfo("\n");

	for (i = 0; i < maxvendorlen + maxchiplen; i++)
		msg_ginfo(" ");
	msg_ginfo("OK   ");
	for (i = 0; i < border; i++)
		msg_ginfo(" ");
	msg_ginfo("Broken");
	for (i = 0; i < border; i++)
		msg_ginfo(" ");
	msg_ginfo("[kB] ");
	for (i = 0; i < border + maxtypelen; i++)
		msg_ginfo(" ");
	msg_gdbg("range [V]");
	msg_ginfo("\n\n");
	msg_ginfo("(P = PROBE, R = READ, E = ERASE, W = WRITE, B = block-protect, - = N/A)\n\n");

	for (chip = flashchips; chip->name != NULL; chip++) {
		/* Don't print generic entries. */
		if (!strncmp(chip->vendor, "Unknown", 7) ||
		    !strncmp(chip->vendor, "Programmer", 10) ||
		    !strncmp(chip->name, "unknown", 7))
			continue;

		/* support for multiline vendor names:
		 * - make a copy of the original vendor name
		 * - use strok to put the first token in tmpven
		 * - keep track of the length of all tokens on the current line
		 *   for ' '-padding in curvenlen
		 * - check if additional tokens should be printed on the current
		 *   line
		 * - after all other values are printed print the surplus tokens
		 *   on fresh lines
		 */
		ven = malloc(strlen(chip->vendor) + 1);
		if (ven == NULL) {
			msg_gerr("Out of memory!\n");
			return 1;
		}
		strcpy(ven, chip->vendor);

		tmpven = strtok_r(ven, delim, &tmpven_save);
		msg_ginfo("%s", tmpven);
		curvenlen = strlen(tmpven);
		while ((tmpven = strtok_r(NULL, delim, &tmpven_save)) != NULL) {
			msg_ginfo("%s", delim);
			curvenlen++;
			tmpvenlen = strlen(tmpven);
			if (tmpvenlen >= mintoklen)
				break; /* big enough to be on its own line */
			msg_ginfo("%s", tmpven);
			curvenlen += tmpvenlen;
		}

		for (i = curvenlen; i < maxvendorlen; i++)
			msg_ginfo(" ");

		/* support for multiline device names as above */
		dev = malloc(strlen(chip->name) + 1);
		if (dev == NULL) {
			msg_gerr("Out of memory!\n");
			free(ven);
			return 1;
		}
		strcpy(dev, chip->name);

		tmpdev = strtok_r(dev, delim, &tmpdev_save);
		msg_ginfo("%s", tmpdev);
		curdevlen = strlen(tmpdev);
		while ((tmpdev = strtok_r(NULL, delim, &tmpdev_save)) != NULL) {
			msg_ginfo("%s", delim);
			curdevlen++;
			tmpdevlen = strlen(tmpdev);
			if (tmpdevlen >= mintoklen)
				break; /* big enough to be on its own line */
			msg_ginfo("%s", tmpdev);
			curdevlen += tmpdevlen;
		}

		for (i = curdevlen; i < maxchiplen; i++)
			msg_ginfo(" ");

		if (chip->tested.probe == OK)
			msg_ginfo("P");
		else if (chip->tested.probe == NA)
			msg_ginfo("-");
		else
			msg_ginfo(" ");
		if (chip->tested.read == OK)
			msg_ginfo("R");
		else if (chip->tested.read == NA)
			msg_ginfo("-");
		else
			msg_ginfo(" ");
		if (chip->tested.erase == OK)
			msg_ginfo("E");
		else if (chip->tested.erase == NA)
			msg_ginfo("-");
		else
			msg_ginfo(" ");
		if (chip->tested.write == OK)
			msg_ginfo("W");
		else if (chip->tested.write == NA)
			msg_ginfo("-");
		else
			msg_ginfo(" ");
		if (chip->tested.wp == OK)
			msg_ginfo("B");
		else if (chip->tested.wp == NA)
			msg_ginfo("-");
		else
			msg_ginfo(" ");
		for (i = 0; i < border; i++)
			msg_ginfo(" ");

		if (chip->tested.probe == BAD)
			msg_ginfo("P");
		else
			msg_ginfo(" ");
		if (chip->tested.read == BAD)
			msg_ginfo("R");
		else
			msg_ginfo(" ");
		if (chip->tested.erase == BAD)
			msg_ginfo("E");
		else
			msg_ginfo(" ");
		if (chip->tested.write == BAD)
			msg_ginfo("W");
		else
			msg_ginfo(" ");
		if (chip->tested.wp == BAD)
			msg_ginfo("B");
		else
			msg_ginfo(" ");
		for (i = 0; i < border + 1; i++)
			msg_ginfo(" ");

		msg_ginfo("%6d", chip->total_size);
		for (i = 0; i < border; i++)
			msg_ginfo(" ");

		s = flashbuses_to_text(chip->bustype);
		if (s == NULL) {
			msg_gerr("Out of memory!\n");
			free(ven);
			free(dev);
			return 1;
		}
		msg_ginfo("%s", s);
		for (i = strlen(s); i < maxtypelen; i++)
			msg_ginfo(" ");
		free(s);

		if (chip->voltage.min == 0 && chip->voltage.max == 0)
			msg_gdbg("no info");
		else
			msg_gdbg("%0.02f;%0.02f",
				 chip->voltage.min/(double)1000,
				 chip->voltage.max/(double)1000);

		/* print surplus vendor and device name tokens */
		while (tmpven != NULL || tmpdev != NULL) {
			msg_ginfo("\n");
			if (tmpven != NULL){
				msg_ginfo("%s", tmpven);
				curvenlen = strlen(tmpven);
				while ((tmpven = strtok_r(NULL, delim, &tmpven_save)) != NULL) {
					msg_ginfo("%s", delim);
					curvenlen++;
					tmpvenlen = strlen(tmpven);
					/* big enough to be on its own line */
					if (tmpvenlen >= mintoklen)
						break;
					msg_ginfo("%s", tmpven);
					curvenlen += tmpvenlen;
				}
			} else
				curvenlen = 0;

			for (i = curvenlen; i < maxvendorlen; i++)
				msg_ginfo(" ");

			if (tmpdev != NULL){
				msg_ginfo("%s", tmpdev);
				curdevlen = strlen(tmpdev);
				while ((tmpdev = strtok_r(NULL, delim, &tmpdev_save)) != NULL) {
					msg_ginfo("%s", delim);
					curdevlen++;
					tmpdevlen = strlen(tmpdev);
					/* big enough to be on its own line */
					if (tmpdevlen >= mintoklen)
						break;
					msg_ginfo("%s", tmpdev);
					curdevlen += tmpdevlen;
				}
			}
		}
		msg_ginfo("\n");
		free(ven);
		free(dev);
	}

	return 0;
}

#if CONFIG_INTERNAL == 1
static void print_supported_chipsets(void)
{
	unsigned int i, chipsetcount = 0;
	const struct penable *c = chipset_enables;
	size_t maxvendorlen = strlen("Vendor") + 1;
	size_t maxchipsetlen = strlen("Chipset") + 1;

	for (c = chipset_enables; c->vendor_name != NULL; c++) {
		chipsetcount++;
		maxvendorlen = MAX(maxvendorlen, strlen(c->vendor_name));
		maxchipsetlen = MAX(maxchipsetlen, strlen(c->device_name));
	}
	maxvendorlen++;
	maxchipsetlen++;

	msg_ginfo("Supported chipsets (total: %u):\n\n", chipsetcount);

	msg_ginfo("Vendor");
	for (i = strlen("Vendor"); i < maxvendorlen; i++)
		msg_ginfo(" ");

	msg_ginfo("Chipset");
	for (i = strlen("Chipset"); i < maxchipsetlen; i++)
		msg_ginfo(" ");

	msg_ginfo("PCI IDs    Status\n\n");

	for (c = chipset_enables; c->vendor_name != NULL; c++) {
		msg_ginfo("%s", c->vendor_name);
		for (i = 0; i < maxvendorlen - strlen(c->vendor_name); i++)
			msg_ginfo(" ");
		msg_ginfo("%s", c->device_name);
		for (i = 0; i < maxchipsetlen - strlen(c->device_name); i++)
			msg_ginfo(" ");
		msg_ginfo("%04x:%04x  %s\n", c->vendor_id, c->device_id,
		       test_state_to_text(c->status));
	}
}

static void print_supported_boards_helper(const struct board_info *boards,
				   const char *devicetype)
{
	unsigned int i;
	unsigned int boardcount_good = 0, boardcount_bad = 0, boardcount_nt = 0;
	const struct board_match *e = board_matches;
	const struct board_info *b = boards;
	size_t maxvendorlen = strlen("Vendor") + 1;
	size_t maxboardlen = strlen("Board") + 1;

	for (b = boards; b->vendor != NULL; b++) {
		maxvendorlen = max(maxvendorlen, strlen(b->vendor));
		maxboardlen = max(maxboardlen, strlen(b->name));
		if (b->working == OK)
			boardcount_good++;
		else if (b->working == NT)
			boardcount_nt++;
		else
			boardcount_bad++;
	}
	maxvendorlen++;
	maxboardlen++;

	msg_ginfo("%d known %s (good: %d, untested: %d, bad: %d):\n\n",
		  boardcount_good + boardcount_nt + boardcount_bad,
		  devicetype, boardcount_good, boardcount_nt, boardcount_bad);

	msg_ginfo("Vendor");
	for (i = strlen("Vendor"); i < maxvendorlen; i++)
		msg_ginfo(" ");

	msg_ginfo("Board");
	for (i = strlen("Board"); i < maxboardlen; i++)
		msg_ginfo(" ");

	msg_ginfo("Status  Required value for\n");
	for (i = 0; i < maxvendorlen + maxboardlen + strlen("Status  "); i++)
		msg_ginfo(" ");
	msg_ginfo("-p internal:mainboard=\n");

	for (b = boards; b->vendor != NULL; b++) {
		msg_ginfo("%s", b->vendor);
		for (i = 0; i < maxvendorlen - strlen(b->vendor); i++)
			msg_ginfo(" ");
		msg_ginfo("%s", b->name);
		for (i = 0; i < maxboardlen - strlen(b->name); i++)
			msg_ginfo(" ");

		switch (b->working) {
		case OK:  msg_ginfo("OK      "); break;
		case NT:  msg_ginfo("NT      "); break;
		case DEP: msg_ginfo("DEP     "); break;
		case NA:  msg_ginfo("N/A     "); break;
		case BAD:
		default:  msg_ginfo("BAD     "); break;
		}

		for (e = board_matches; e->vendor_name != NULL; e++) {
			if (strcmp(e->vendor_name, b->vendor)
			    || strcmp(e->board_name, b->name))
				continue;
			if (e->lb_vendor == NULL)
				msg_ginfo("(autodetected)");
			else
				msg_ginfo("%s:%s", e->lb_vendor,
						   e->lb_part);
		}
		msg_ginfo("\n");
	}
}
#endif

static void print_supported_devs(const struct programmer_entry *const prog, const char *const type,
				int* num_devs)
{
	const struct dev_entry *const devs = prog->devs.dev;
	msg_ginfo("\nSupported %s devices for the %s programmer:\n", type, prog->name);
	unsigned int maxvendorlen = strlen("Vendor") + 1;
	unsigned int maxdevlen = strlen("Device") + 1;

	unsigned int i;
	for (i = 0; devs[i].vendor_name != NULL; i++) {
		maxvendorlen = max(maxvendorlen, strlen(devs[i].vendor_name));
		maxdevlen = max(maxdevlen, strlen(devs[i].device_name));
	}
	maxvendorlen++;
	maxdevlen++;

	msg_ginfo("Vendor");
	for (i = strlen("Vendor"); i < maxvendorlen; i++)
		msg_ginfo(" ");

	msg_ginfo("Device");
	for (i = strlen("Device"); i < maxdevlen; i++)
		msg_ginfo(" ");

	msg_ginfo(" %s IDs    Status\n", type);

	for (i = 0; devs[i].vendor_name != NULL; i++) {
		msg_ginfo("%s", devs[i].vendor_name);
		unsigned int j;
		for (j = strlen(devs[i].vendor_name); j < maxvendorlen; j++)
			msg_ginfo(" ");
		msg_ginfo("%s", devs[i].device_name);
		for (j = strlen(devs[i].device_name); j < maxdevlen; j++)
			msg_ginfo(" ");

		msg_pinfo(" %04x:%04x  %s\n", devs[i].vendor_id, devs[i].device_id,
			  test_state_to_text(devs[i].status));
		if (devs[i].status == OK || devs[i].status == NT || devs[i].status == DEP)
			*num_devs += 1;
	}
}

int print_supported(void)
{
	unsigned int i;
	int num_pci_devs = 0;
	int num_usb_devs = 0;

	if (print_supported_chips())
		return 1;

	msg_ginfo("\nSupported programmers:\n");
	list_programmers_linebreak(0, 80, 0);
	msg_ginfo("\n");
#if CONFIG_INTERNAL == 1
	msg_ginfo("\nSupported devices for the internal programmer:\n\n");
	print_supported_chipsets();
	msg_ginfo("\n");
	print_supported_boards_helper(boards_known, "mainboards");
	msg_ginfo("\n");
	print_supported_boards_helper(laptops_known, "mobile devices");
#endif
	for (i = 0; i < programmer_table_size; i++) {
		const struct programmer_entry *const prog = programmer_table[i];
		switch (prog->type) {
		case USB:
			print_supported_devs(prog, "USB", &num_usb_devs);
			break;
		case PCI:
			print_supported_devs(prog, "PCI", &num_pci_devs);
			break;
		case OTHER:
			if (prog->devs.note != NULL) {
				msg_ginfo("\nSupported devices for the %s programmer:\n", prog->name);
				msg_ginfo("%s", prog->devs.note);
			}
			break;
		default:
			msg_gerr("\n%s: %s: Uninitialized programmer type! Please report a bug at "
				 "flashrom@flashrom.org\n", __func__, prog->name);
			break;
		}
	}

	msg_ginfo("\nSupported USB devices, total %d\nSupported PCI devices, total %d\n",
			num_usb_devs, num_pci_devs);

	return 0;
}

static void print_sysinfo(void)
{
#if IS_WINDOWS
	SYSTEM_INFO si = { 0 };
	OSVERSIONINFOEX osvi = { 0 };

	msg_ginfo(" on Windows");
	/* Tell Windows which version of the structure we want. */
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (GetVersionEx((OSVERSIONINFO*) &osvi))
		msg_ginfo(" %lu.%lu", (unsigned long)osvi.dwMajorVersion,
			(unsigned long)osvi.dwMinorVersion);
	else
		msg_ginfo(" unknown version");
	GetSystemInfo(&si);
	switch (si.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64:
		msg_ginfo(" (x86_64)");
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		msg_ginfo(" (x86)");
		break;
	default:
		msg_ginfo(" (unknown arch)");
		break;
	}
#elif HAVE_UTSNAME == 1
	struct utsname osinfo;

	uname(&osinfo);
	msg_ginfo(" on %s %s (%s)", osinfo.sysname, osinfo.release,
		  osinfo.machine);
#else
	msg_ginfo(" on unknown machine");
#endif
}

void print_buildinfo(void)
{
	msg_gdbg("flashrom was built with");
#ifdef __clang__
	msg_gdbg(" LLVM Clang");
#ifdef __clang_version__
	msg_gdbg(" %s,", __clang_version__);
#else
	msg_gdbg(" unknown version (before r102686),");
#endif
#elif defined(__GNUC__)
	msg_gdbg(" GCC");
#ifdef __VERSION__
	msg_gdbg(" %s,", __VERSION__);
#else
	msg_gdbg(" unknown version,");
#endif
#else
	msg_gdbg(" unknown compiler,");
#endif
#if defined (__FLASHROM_LITTLE_ENDIAN__)
	msg_gdbg(" little endian");
#elif defined (__FLASHROM_BIG_ENDIAN__)
	msg_gdbg(" big endian");
#else
#error Endianness could not be determined
#endif
	msg_gdbg("\n");
}

void print_version(void)
{
	msg_ginfo("flashrom %s", flashrom_version_info());
	print_sysinfo();
	msg_ginfo("\n");
}

void print_banner(void)
{
	msg_ginfo("flashrom is free software, get the source code at "
		  "https://flashrom.org\n");
	msg_ginfo("\n");
}
