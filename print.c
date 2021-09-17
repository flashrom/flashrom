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

	msg_ginfo("Test");
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
	msg_ginfo("OK  ");
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
	msg_ginfo("(P = PROBE, R = READ, E = ERASE, W = WRITE, - = N/A)\n\n");

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
		for (i = 0; i < border + 1; i++)
			msg_ginfo(" ");

		msg_ginfo("%6d", chip->total_size);
		for (i = 0; i < border; i++)
			msg_ginfo(" ");

		s = flashbuses_to_text(chip->bustype);
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

static void print_supported_devs(const struct programmer_entry *const prog, const char *const type)
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
	}
}

int print_supported(void)
{
	unsigned int i;
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
			print_supported_devs(prog, "USB");
			break;
		case PCI:
			print_supported_devs(prog, "PCI");
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
	return 0;
}

#if CONFIG_INTERNAL == 1

#ifdef CONFIG_PRINT_WIKI
#define B(vendor, name, status, url, note) { vendor, name, status, url, note }
#else
#define B(vendor, name, status, url, note) { vendor, name, status }
#endif

/* Please keep this list alphabetically ordered by vendor/board. */
const struct board_info boards_known[] = {
#if defined(__i386__) || defined(__x86_64__)
	B("A-Trend",	"ATC-6220",		OK, "http://www.motherboard.cz/mb/atrend/atc6220.htm", NULL),
	B("abit",	"A-S78H",		OK, NULL, NULL),
	B("abit",	"AN-M2",		OK, NULL, NULL),
	B("abit",	"AV8",			OK, NULL, NULL),
	B("abit",	"AX8",			OK, NULL, NULL),
	B("abit",	"BF6",			OK, NULL, NULL),
	B("abit",	"BM6",			OK, NULL, NULL),
	B("abit",	"BX6 2.0",		OK, NULL, NULL),
	B("abit",	"Fatal1ty F-I90HD",	OK, NULL, NULL),
	B("abit",	"IC7",			OK, NULL, NULL),
	B("abit",	"IP35",			OK, NULL, NULL),
	B("abit",	"IP35 Pro",		OK, NULL, NULL),
	B("abit",	"IS-10",		BAD, NULL, "Reported by deejkuba@aol.com to flashrom@coreboot.org, no public archive. Missing board enable and/or M50FW040 unlocking. May work now."),
	B("abit",	"KN8 Ultra",		OK, NULL, NULL),
	B("abit",	"KN9 Ultra",		OK, NULL, NULL),
	B("abit",	"NF-M2 nView",		OK, NULL, NULL),
	B("abit",	"NF-M2S",		OK, NULL, NULL),
	B("abit",	"NF7-S",		OK, NULL, NULL),
	B("abit",	"VA6",			OK, NULL, NULL),
	B("abit",	"VT6X4",		OK, NULL, NULL),
	B("Acer",	"V75-M",		OK, NULL, "This is an OEM board used by IBM in e.g. Aptiva 2170-G"),
	B("Acer",	"EM61SM/EM61PM",	OK, NULL, "Used in Acer Aspire T180 and E380. Seems to be an OEM variant of abit's NF-M2S."),
	B("Acorp",	"6A815EPD",		OK, "http://web.archive.org/web/20021206163652/www.acorp.com.tw/English/default.asp", NULL),
	B("Acorp",	"6M810C",		OK, NULL, NULL),
	B("ADLINK",	"Express-HR",		OK, "http://www.adlinktech.com/PD/web/PD_detail.php?pid=1012", NULL),
	B("Advantech",	"PCM-5820",		OK, "http://www.emacinc.com/sbc_pc_compatible/pcm_5820.htm", NULL),
	B("agami",	"Aruma",		OK, "http://web.archive.org/web/20080212111524/http://www.agami.com/site/ais-6000-series", NULL),
	B("Albatron",	"PM266A Pro",		OK, "http://www.albatron.com.tw/English/Product/MB/pro_detail.asp?rlink=Overview&no=56", NULL), /* FIXME */
	B("Alienware",	"Aurora-R2",		BAD, NULL, "Mainboard model is 0RV30W. Probing works (Macronix MX25L3205, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("AOpen",	"i945GMx-VFX",		OK, NULL, "This is (also?) an OEM board from FSC (used in e.g. ESPRIMO Q5010 with designation D2544-B1)."),
	B("AOpen",	"UK79G-1394",		OK, "http://global.aopen.com/products_detail.aspx?auno=9", "Used in EZ18 barebones"),
	B("AOpen",	"vKM400Am-S",		OK, "http://global.aopen.com/products_detail.aspx?Auno=824", NULL),
	B("Artec Group","DBE61",		OK, "http://wiki.thincan.org/DBE61", NULL),
	B("Artec Group","DBE62",		OK, "http://wiki.thincan.org/DBE62", NULL),
	B("ASI",	"MB-5BLMP",		OK, "http://www.hojerteknik.com/winnet.htm", "Used in the IGEL WinNET III thin client."),
	B("ASRock",	"4CoreDual-VSTA",	OK, "http://www.asrock.com/mb/overview.asp?Model=4CoreDual-VSTA", "W39V040FB"),
	B("ASRock",	"775Dual-VSTA",		OK, "http://www.asrock.com/mb/overview.asp?Model=775Dual-VSTA", NULL),
	B("ASRock",	"775i65G",		OK, "http://www.asrock.com/mb/overview.asp?Model=775i65G", NULL),
	B("ASRock",	"880G Pro3",		OK, "http://www.asrock.com/mb/overview.asp?Model=880G%20Pro3", NULL),
	B("ASRock",	"890GX Extreme3",	OK, "http://www.asrock.com/mb/overview.asp?Model=890GX%20Extreme3", NULL),
	B("ASRock",	"939A785GMH/128M",	OK, "http://www.asrock.com/mb/overview.asp?Model=939A785GMH/128M", NULL),
	B("ASRock",	"960GM-GS3 FX",		OK, "http://www.asrock.com/mb/overview.asp?Model=960GM-GS3%20FX", NULL),
	B("ASRock",	"A330GC",		OK, "http://www.asrock.com/mb/overview.asp?Model=A330GC", NULL),
	B("ASRock",	"A770CrossFire",	OK, "http://www.asrock.com/mb/overview.asp?Model=A770CrossFire", NULL),
	B("ASRock",	"A780FullHD",		OK, "http://www.asrock.com/mb/overview.asp?Model=A780FullHD", "While flashrom is working correctly, there might be problems with the firmware images themselves. Please see https://flashrom.org/pipermail/flashrom/2012-July/009600.html for details."),
	B("ASRock",	"ALiveNF6G-DVI",	OK, "http://www.asrock.com/mb/overview.asp?Model=ALiveNF6G-DVI", NULL),
	B("ASRock",	"AM2NF6G-VSTA",		OK, "http://www.asrock.com/mb/overview.asp?Model=AM2NF6G-VSTA", NULL),
	B("ASRock",	"AMCP7AION-HT",		OK, "http://www.asrock.com/nettop/NVIDIA/ION%20330HT/", "Used in ION 330HT(-BD) barebones."),
	B("ASRock",	"ConRoeXFire-eSATA2",	OK, "http://www.asrock.com/mb/overview.asp?model=conroexfire-esata2", NULL),
	B("ASRock",	"E350M1/USB3",		OK, "http://www.asrock.com/mb/overview.asp?model=e350m1/usb3", "Vendor firmware writes to flash at shutdown. This probably corrupts the flash in case you write coreboot while running the vendor firmware. Simply updating the vendor firmware should be fine."),
	B("ASRock",	"Fatal1ty 970 Performance", OK, "http://www.asrock.com/mb/overview.asp?Model=Fatal1ty%20970%20Performance", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASRock",	"Fatal1ty Z77 Performance", BAD, "http://www.asrock.com/mb/overview.asp?Model=Fatal1ty%20Z77%20Performance", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASRock",	"G31M-GS",		OK, "http://www.asrock.com/mb/overview.asp?Model=G31M-GS", NULL),
	B("ASRock",	"G31M-S rev 2.0",	OK, "http://www.asrock.com/mb/overview.asp?model=G31M-S", NULL),
	B("ASRock",	"G41M-VS3",		OK, "http://www.asrock.com/mb/overview.asp?Model=G41M-VS3", NULL),
	B("ASRock",	"H61M-ITX",		BAD, "http://www.asrock.com/mb/overview.asp?Model=H61M-ITX", "Probing works (Macronix MX25L3205, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASRock",	"H67M",			BAD, "http://www.asrock.com/mb/overview.asp?Model=H67M", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASRock",	"IMB-180-H",		OK, "http://www.asrock.com/ipc/overview.asp?Model=IMB-A180-H", NULL),
	B("ASRock",	"K7S41",		OK, "http://www.asrock.com/mb/overview.asp?Model=K7S41", NULL),
	B("ASRock",	"K7S41GX",		OK, "http://www.asrock.com/mb/overview.asp?Model=K7S41GX", NULL),
	B("ASRock",	"K7VT4A+",		BAD, "http://www.asrock.com/mb/overview.asp?Model=K7VT4A%2b", "No chip found, probably due to flash translation. https://flashrom.org/pipermail/flashrom/2009-August/000393.html"),
	B("ASRock",	"K8S8X",		OK, "http://www.asrock.com/mb/overview.asp?Model=K8S8X", NULL),
	B("ASRock",	"M3A790GXH/128M",	OK, "http://www.asrock.com/mb/overview.asp?Model=M3A790GXH/128M", NULL),
	B("ASRock",	"N61P-S",		OK, "http://www.asrock.com/mb/overview.asp?Model=N61P-S", NULL),
	B("ASRock",	"N68C-S UCC",		OK, "http://www.asrock.com/mb/overview.asp?Model=N68C-S%20UCC", NULL),
	B("ASRock",	"P4i65G",		OK, "http://www.asrock.com/mb/overview.asp?Model=P4i65G", NULL),
	B("ASRock",	"P4i65GV",		OK, "http://www.asrock.com/mb/overview.asp?Model=P4i65GV", NULL),
	B("ASRock",	"Z68 Extreme4",		BAD, "http://www.asrock.com/mb/overview.asp?Model=Z68%20Extreme4", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"A7N8X Deluxe",		OK, "https://www.asus.com/Motherboards/AMD_Socket_A/A7N8X_Deluxe/", NULL),
	B("ASUS",	"A7N8X-E Deluxe",	OK, "https://www.asus.com/Motherboards/AMD_Socket_A/A7N8XE_Deluxe/", NULL),
	B("ASUS",	"A7N8X-VM/400",		OK, "https://www.asus.com/Motherboards/AMD_Socket_A/A7N8XVM400/", NULL),
	B("ASUS",	"A7V133",		OK, NULL, NULL),
	B("ASUS",	"A7V333",		OK, NULL, NULL),
	B("ASUS",	"A7V400-MX",		OK, "https://www.asus.com/Motherboards/AMD_Socket_A/A7V400MX/", NULL),
	B("ASUS",	"A7V600-X",		OK, "https://www.asus.com/Motherboards/AMD_Socket_A/A7V600X/", NULL),
	B("ASUS",	"A7V8X",		OK, "https://www.asus.com/Motherboards/AMD_Socket_A/A7V8X/", NULL),
	B("ASUS",	"A7V8X-MX",		OK, "https://www.asus.com/Motherboards/AMD_Socket_A/A7V8XMX/", NULL),
	B("ASUS",	"A7V8X-MX SE",		OK, "https://www.asus.com/Motherboards/AMD_Socket_A/A7V8XMX_SE/", NULL),
	B("ASUS",	"A7V8X-X",		OK, "https://www.asus.com/Motherboards/AMD_Socket_A/A7V8XX/", NULL),
	B("ASUS",	"A8M2N-LA (NodusM3-GL8E)", OK, "http://h10010.www1.hp.com/ewfrf/wc/document?docname=c00757531&cc=us&dlc=en&lc=en", "This is an OEM board from HP, the HP name is NodusM3-GL8E."),
	B("ASUS",	"A8N-E",		OK, "https://www.asus.com/Motherboards/AMD_Socket_939/A8NE/", NULL),
	B("ASUS",	"A8N-LA (Nagami-GL8E)",	OK, "http://h10025.www1.hp.com/ewfrf/wc/document?lc=en&cc=us&docname=c00647121&dlc=en", "This is an OEM board from HP, the HP name is Nagami-GL8E."),
	B("ASUS",	"A8N-SLI",		OK, "https://www.asus.com/Motherboards/AMD_Socket_939/A8NSLI/", NULL),
	B("ASUS",	"A8N-SLI Deluxe",	NT, NULL, "Should work out of the box since r1593."),
	B("ASUS",	"A8N-SLI Premium",	OK, "https://www.asus.com/Motherboards/AMD_Socket_939/A8NSLI_Premium/", NULL),
	B("ASUS",	"A8N-VM",		OK, "https://www.asus.com/Motherboards/AMD_Socket_939/A8NVM/", NULL),
	B("ASUS",	"A8N-VM CSM",		OK, "https://www.asus.com/Motherboards/AMD_Socket_939/A8NVM_CSM/", NULL),
	B("ASUS",	"A8NE-FM/S",		OK, "http://www.hardwareschotte.de/hardware/preise/proid_1266090/preis_ASUS+A8NE-FM", NULL),
	B("ASUS",	"A8V Deluxe",		OK, "https://www.asus.com/Motherboards/AMD_Socket_939/A8V_Deluxe/", NULL),
	B("ASUS",	"A8V-E Deluxe",		OK, "https://www.asus.com/Motherboards/AMD_Socket_939/A8VE_Deluxe/", NULL),
	B("ASUS",	"A8V-E SE",		OK, "https://www.asus.com/Motherboards/AMD_Socket_939/A8VE_SE/", "See http://www.coreboot.org/pipermail/coreboot/2007-October/026496.html"),
	B("ASUS",	"C60M1-I",		OK, "https://www.asus.com/Motherboards/C60M1I/", "The MAC address of the onboard network card is stored in flash."),
	B("ASUS",	"Crosshair II Formula",	OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/Crosshair_II_Formula/", NULL),
	B("ASUS",	"Crosshair IV Extreme",	OK, "https://www.asus.com/Motherboards/AMD_AM3/Crosshair_IV_Extreme/", NULL),
	B("ASUS",	"CUSL2-C",		OK, NULL, "The image provided by ASUS is only 256 kB big and has to be written to the upper 256 kB of the 512 kB chip."),
	B("ASUS",	"DSAN-DX",		NT, "https://www.asus.com/Server_Workstation/Server_Motherboards/DSANDX/", NULL),
	B("ASUS",	"E35M1-I DELUXE",	OK, "https://www.asus.com/Motherboards/AMD_CPU_on_Board/E35M1I_DELUXE/", NULL),
	B("ASUS",	"F1A75-V PRO",		OK, "https://www.asus.com/Motherboard/F1A75V_PRO/", NULL),
	B("ASUS",	"F2A85-M",		DEP, "https://www.asus.com/Motherboards/F2A85M/", "UEFI builds v6404 and above disable access to some parts of the flash, cf. http://www.coreboot.org/ASUS_F2A85-M#UEFI_builds_that_allow_flash_chip_access"),
	B("ASUS",	"K8N",			OK, "https://www.asus.com/Motherboards/AMD_Socket_754/K8N/", NULL),
	B("ASUS",	"K8V",			OK, "https://www.asus.com/Motherboards/AMD_Socket_754/K8V/", NULL),
	B("ASUS",	"K8V SE Deluxe",	OK, "https://www.asus.com/Motherboards/AMD_Socket_754/K8V_SE_Deluxe/", NULL),
	B("ASUS",	"K8V-X",		OK, "https://www.asus.com/Motherboards/AMD_Socket_754/K8VX/", NULL),
	B("ASUS",	"K8V-X SE",		OK, "https://www.asus.com/Motherboards/AMD_Socket_754/K8VX_SE/", NULL),
	B("ASUS",	"KFSN4-DRE/SAS",	OK, "https://www.asus.com/Server_Workstation/Server_Motherboards/KFSN4DRESAS/", NULL),
	B("ASUS",	"M2A-MX",		OK, "https://www.asus.com/Motherboards/AMD_AM2/M2AMX/", NULL),
	B("ASUS",	"M2A-VM (HDMI)",	OK, "https://www.asus.com/Motherboards/AMD_AM2/M2AVM/", NULL),
	B("ASUS",	"M2N32-SLI Deluxe",	OK, "https://www.asus.com/Motherboards/AMD_AM2/M2N32SLI_DeluxeWireless_Edition/", NULL),
	B("ASUS",	"M2N68-VM",		OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M2N68VM/", NULL),
	B("ASUS",	"M2NBP-VM CSM",		OK, "https://www.asus.com/Motherboards/AMD_AM2/M2NBPVM_CSM/", NULL),
	B("ASUS",	"M2N-E",		OK, "https://www.asus.com/Motherboards/AMD_AM2/M2NE/", "If the machine doesn't come up again after flashing, try resetting the NVRAM(CMOS). The MAC address of the onboard network card will change to the value stored in the new image, so backup the old address first. See https://flashrom.org/pipermail/flashrom/2009-November/000879.html"),
	B("ASUS",	"M2N-E SLI",		OK, "https://www.asus.com/Motherboards/AMD_AM2/M2NE_SLI/", NULL),
	B("ASUS",	"M2N-MX SE Plus",	OK, "https://www.asus.com/Motherboards/M2NMX_SE_Plus/", NULL),
	B("ASUS",	"M2NPV-VM",		OK, "https://www.asus.com/Motherboards/AMD_AM2/M2NPVVM/", NULL),
	B("ASUS",	"M2N-SLI Deluxe",	OK, "https://www.asus.com/Motherboards/AMD_AM2/M2NSLI_Deluxe/", NULL),
	B("ASUS",	"M2V",			OK, "https://www.asus.com/Motherboards/AMD_AM2/M2V/", NULL),
	B("ASUS",	"M2V-MX",		OK, "https://www.asus.com/Motherboards/AMD_AM2/M2VMX/", NULL),
	B("ASUS",	"M3A",			OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M3A/", NULL),
	B("ASUS",	"M3A76-CM",		OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M3A76CM/", NULL),
	B("ASUS",	"M3A78-EH",		OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M3A78EH/", NULL),
	B("ASUS",	"M3A78-EM",		OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M3A78EM/", NULL),
	B("ASUS",	"M3N78 PRO",		OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M3N78_PRO/", NULL),
	B("ASUS",	"M3N78-VM",		OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M3N78VM/", NULL),
	B("ASUS",	"M3N-H/HDMI",		OK, "https://www.asus.com/Motherboards/M3NHHDMI//", NULL),
	B("ASUS",	"M4A785TD-M EVO",	OK, "https://www.asus.com/Motherboards/AMD_AM3/M4A785TDM_EVO/", NULL),
	B("ASUS",	"M4A785TD-V EVO",	OK, "https://www.asus.com/Motherboards/AMD_AM3/M4A785TDV_EVO/", NULL),
	B("ASUS",	"M4A785T-M",		OK, "https://www.asus.com/Motherboards/AMD_AM3/M4A785TM/", NULL),
	B("ASUS",	"M4A78-EM",		OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M4A78EM/", NULL),
	B("ASUS",	"M4A78LT-M LE",		OK, "https://www.asus.com/Motherboards/AMD_AM3/M4A78LTM_LE/", NULL),
	B("ASUS",	"M4A79T Deluxe",	OK, "https://www.asus.com/Motherboards/AMD_AM3/M4A79T_Deluxe/", NULL),
	B("ASUS",	"M4A87TD/USB3",		OK, "https://www.asus.com/Motherboards/AMD_AM3/M4A87TDUSB3/", NULL),
	B("ASUS",	"M4A89GTD PRO",		OK, "https://www.asus.com/Motherboards/AMD_AM3/M4A89GTD_PRO/", NULL),
	B("ASUS",	"M4N68T V2",		OK, "https://www.asus.com/Motherboards/AMD_AM3/M4N68T_V2/", NULL),
	B("ASUS",	"M4N78 PRO",		OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M4N78_PRO/", NULL),
	B("ASUS",	"M4N78 SE",		OK, "https://www.asus.com/Motherboards/AMD_AM2Plus/M4N78_SE/", NULL),
	B("ASUS",	"M5A78L-M LX",		OK, "https://www.asus.com/Motherboards/AMD_AM3Plus/M5A78LM_LX/", "The MAC address of the onboard LAN NIC is stored in flash, hence overwritten by flashrom; see https://flashrom.org/pipermail/flashrom/2012-May/009200.html"),
	B("ASUS",	"M5A97 (rev. 1.0)",	OK, "https://www.asus.com/Motherboard/M5A97/", NULL),
	B("ASUS",	"M5A99X EVO",		OK, "https://www.asus.com/Motherboards/AMD_AM3Plus/M5A99X_EVO/", NULL),
	B("ASUS",	"Maximus IV Extreme",	BAD, "https://www.asus.com/Motherboards/Intel_Socket_1155/Maximus_IV_Extreme/", "Probing works (Macronix MX25L3205, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"MEW-AM",		BAD, NULL, "No public report found. Owned by Uwe Hermann <uwe@hermann-uwe.de>. May work now."),
	B("ASUS",	"MEW-VM",		BAD, "http://www.elhvb.com/mboards/OEM/HP/manual/ASUS%20MEW-VM.htm", "No public report found. Owned by Uwe Hermann <uwe@hermann-uwe.de>. May work now."),
	B("ASUS",	"OPLX-M",		NT, NULL, "Untested board enable."),
	B("ASUS",	"P2B",			OK, NULL, NULL),
	B("ASUS",	"P2B-D",		OK, NULL, NULL),
	B("ASUS",	"P2B-DS",		OK, NULL, NULL),
	B("ASUS",	"P2B-F",		OK, NULL, NULL),
	B("ASUS",	"P2B-LS",		OK, NULL, NULL),
	B("ASUS",	"P2B-N",		OK, NULL, NULL),
	B("ASUS",	"P2E-M",		OK, NULL, NULL),
	B("ASUS",	"P2L97-S",		OK, NULL, NULL),
	B("ASUS",	"P3B-F",		OK, NULL, "Owned by Uwe Hermann <uwe@hermann-uwe.de>."),
	B("ASUS",	"P4B266",		OK, NULL, NULL),
	B("ASUS",	"P4B266-LM",		OK, "http://esupport.sony.com/US/perl/swu-list.pl?mdl=PCVRX650", NULL),
	B("ASUS",	"P4B533-E",		OK, NULL, NULL),
	B("ASUS",	"P4C800-E Deluxe",	OK, "https://www.asus.com/Motherboards/Intel_Socket_478/P4C800E_Deluxe/", NULL),
	B("ASUS",	"P4GV-LA (Guppy)",	OK, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?objectID=c00363478", NULL),
	B("ASUS",	"P4P800",		OK, "https://www.asus.com/Motherboards/Intel_Socket_478/P4P800/", NULL),
	B("ASUS",	"P4P800-E Deluxe",	OK, "https://www.asus.com/Motherboards/Intel_Socket_478/P4P800E_Deluxe/", NULL),
	B("ASUS",	"P4P800-VM",		OK, "https://www.asus.com/Motherboards/Intel_Socket_478/P4P800VM/", NULL),
	B("ASUS",	"P4P800-X",		OK, "https://www.asus.com/Motherboards/Intel_Socket_478/P4P800X/", NULL),
	B("ASUS",	"P4P800SE",		OK, "https://www.asus.com/supportonly/P4P800 SE/", NULL),
	B("ASUS",	"P4PE-X/TE",		NT, "https://www.asus.com/999/html/events/mb/socket478/p4pe-x-te/overview.htm", NULL),
	B("ASUS",	"P4S533-X",		OK, NULL, NULL),
	B("ASUS",	"P4S800-MX",		OK, "https://www.asus.com/Motherboards/Intel_Socket_478/P4S800MX/", NULL),
	B("ASUS",	"P4SC-E",		OK, NULL, "Part of ASUS Terminator P4 533 barebone system"),
	B("ASUS",	"P4SD-LA",		OK, "http://h10025.www1.hp.com/ewfrf/wc/document?docname=c00022505", NULL),
	B("ASUS",	"P5A",			OK, NULL, NULL),
	B("ASUS",	"P5B",			OK, NULL, NULL),
	B("ASUS",	"P5B-Deluxe",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5B_Deluxe/", NULL),
	B("ASUS",	"P5B-VM",		OK, NULL, NULL),
	B("ASUS",	"P5BV-M",		BAD, NULL, "Reported by Bernhard M. Wiedemann <bernhard@uml12d.zq1.de> to flashrom@coreboot.org, no public archive. Missing board enable and/or SST49LF008A unlocking. May work now."),
	B("ASUS",	"P5BV-R",		OK, "https://www.asus.com/Server_Workstation/Servers/RS120E5PA2/", "Used in RS120-E5/PA2 servers."),
	B("ASUS",	"P5GC-MX/1333",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5GCMX1333/", NULL),
	B("ASUS",	"P5GD1 Pro",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5GD1_PRO/", NULL),
	B("ASUS",	"P5GD1-VM/S",		OK, NULL, "This is an OEM board from FSC. Although flashrom supports it and can probably not distinguish it from the P5GD1-VM, please note that the P5GD1-VM BIOS does not support the FSC variants completely."),
	B("ASUS",	"P5GD1(-VM)",		NT, NULL, "Untested board enable."),
	B("ASUS",	"P5GD2 Premium",	OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5GD2_Premium/", NULL),
	B("ASUS",	"P5GD2-X",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5GD2X/", NULL),
	B("ASUS",	"P5GDC Deluxe",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5GDC_Deluxe/", NULL),
	B("ASUS",	"P5GDC-V Deluxe",	OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5GDCV_Deluxe/", NULL),
	B("ASUS",	"P5GD2/C variants",	NT, NULL, "Untested board enable."),
	B("ASUS",	"P5K SE",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5K_SE/", NULL),
	B("ASUS",	"P5K-V",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5KV/", NULL),
	B("ASUS",	"P5K-VM",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5KVM/", NULL),
	B("ASUS",	"P5KC",			OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5KC/", NULL),
	B("ASUS",	"P5KPL-AM IN/GB",	OK, "http://support.asus.com/download.aspx?SLanguage=en&m=P5KPL-AM+IN%2fGB&os=29", NULL),
	B("ASUS",	"P5KPL-CM",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5KPLCM/", NULL),
	B("ASUS",	"P5KPL-VM",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5KPLVM/", "Found in V3-P5G31."),
	B("ASUS",	"P5L-MX",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5LMX/", NULL),
	B("ASUS",	"P5L-VM 1394",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5LVM_1394/", NULL),
	B("ASUS",	"P5LD2",		OK, NULL, NULL),
	B("ASUS",	"P5LD2-MQ",		OK, "http://support.asus.com/download.aspx?SLanguage=en&p=8&s=12&m=Vintage-PH2&os=&hashedid=n/a", "Found in ASUS Vintage-PH2 barebones."),
	B("ASUS",	"P5LD2-VM",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5LD2VM/", NULL),
	B("ASUS",	"P5LD2-VM DH",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5LD2VM_DH/", NULL),
	B("ASUS",	"P5LP-LE (Lithium-UL8E)", OK, "http://h10025.www1.hp.com/ewfrf/wc/document?docname=c00379616&tmp_task=prodinfoCategory&cc=us&dlc=en&lc=en&product=1159887", "This is an OEM board from HP."),
	B("ASUS",	"P5LP-LE (Epson OEM)",	OK, NULL, "This is an OEM board from Epson (e.g. Endeavor MT7700)."),
	B("ASUS",	"P5LP-LE",		NT, NULL, "This designation is used for OEM boards from HP, Epson and maybe others. The HP names vary and not all of them have been tested yet. Please report any success or failure, thanks."),
	B("ASUS",	"P5N-D",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5ND/", NULL),
	B("ASUS",	"P5N-E SLI",		NT, "https://www.asus.com/Motherboards/Intel_Socket_775/P5NE_SLI/", "Untested board enable."),
	B("ASUS",	"P5N32-E SLI",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5N32E_SLI/", NULL),
	B("ASUS",	"P5N7A-VM",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5N7AVM/", NULL),
	B("ASUS",	"P5ND2-SLI Deluxe",	OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5ND2SLI_Deluxe/", NULL),
	B("ASUS",	"P5PE-VM",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5PEVM/", NULL),
	B("ASUS",	"P5QPL-AM",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5QPLAM/", NULL),
	B("ASUS",	"P5VD1-X",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5VD1X/", NULL),
	B("ASUS",	"P5VD2-MX",		OK, "https://www.asus.com/Motherboards/Intel_Socket_775/P5VD2MX/", "The MAC address of the onboard LAN NIC is stored in flash, hence overwritten by flashrom; see https://flashrom.org/pipermail/flashrom/2012-March/009014.html"),
	B("ASUS",	"P6T SE",		OK, "https://www.asus.com/Motherboards/Intel_Socket_1366/P6T_SE/", NULL),
	B("ASUS",	"P6T Deluxe",		OK, "https://www.asus.com/Motherboards/Intel_Socket_1366/P6T_Deluxe/", NULL),
	B("ASUS",	"P6T Deluxe V2",	OK, "https://www.asus.com/Motherboards/Intel_Socket_1366/P6T_Deluxe_V2/", NULL),
	B("ASUS",	"P7H57D-V EVO",		OK, "https://www.asus.com/Motherboards/Intel_Socket_1156/P7H57DV_EVO/", NULL),
	B("ASUS",	"P7H55-M LX",		BAD, NULL, "flashrom works correctly, but GbE LAN is nonworking (probably due to a missing/bogus MAC address; see https://flashrom.org/pipermail/flashrom/2011-July/007432.html and http://ubuntuforums.org/showthread.php?t=1534389 for a possible workaround)"),
	B("ASUS",	"P8B-E/4L",		BAD, NULL, "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8B WS",		BAD, NULL, "Probing works (Winbond W25Q32, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8B75-M LE",		BAD, NULL, "Probing works (2x 8192 kB via hwseq), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8H61 PRO",		BAD, NULL, "Probing works (Winbond W25Q32, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8H61-M LE/USB3",	BAD, NULL, "Probing works (Winbond W25Q32, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8H67-M PRO",		BAD, NULL, "Probing works (Macronix MX25L3205, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."), // some firmware versions apparently are not locked, see report by Marek Zakrzewski
	B("ASUS",	"P8H77-I",		OK, "https://www.asus.com/Motherboards/P8H77I/", NULL),
	B("ASUS",	"P8H77-M",		OK, "https://www.asus.com/Motherboards/P8H77M/", NULL),
	B("ASUS",	"P8H77-V LE",		OK, "https://www.asus.com/Motherboards/P8H77V_LE/", NULL),
	B("ASUS",	"P8P67 (rev. 3.1)",	BAD, NULL, "Probing works (Macronix MX25L3205, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8P67 LE (B2)",	OK, NULL, NULL),
	B("ASUS",	"P8P67 LE (B3)",	BAD, NULL, "Probing works (Winbond W25Q32, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8P67 PRO (rev. 3.0)",	OK, "https://www.asus.com/Motherboards/Intel_Socket_1155/P8P67_PRO/", NULL),
	B("ASUS",	"P8P67-M PRO",		BAD, NULL, "Probing works (Macronix MX25L3205, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8Z68-V",		OK, "https://www.asus.com/Motherboards/Intel_Socket_1155/P8Z68V/", "Warning: MAC address of LOM is stored at 0x1000 - 0x1005 of the image."),
	B("ASUS",	"P8Z68-V LE",		BAD, NULL, "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8Z68-V PRO",		BAD, NULL, "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ASUS",	"P8Z68-V PRO/GEN3",	OK, "https://www.asus.com/Motherboards/Intel_Socket_1155/P8Z68V_PROGEN3/", "Warning: MAC address of LOM is stored at 0x1000 - 0x1005 of the image."),
	B("ASUS",	"RAMPAGE III GENE",	OK, "https://www.asus.com/Motherboards/RAMPAGE_III_GENE/", "The MAC address of the onboard network card is stored in flash."),
	B("ASUS",	"SABERTOOTH 990FX",	OK, "https://www.asus.com/Motherboards/AMD_AM3Plus/SABERTOOTH_990FX/", NULL),
	B("ASUS",	"SABERTOOTH 990FX R2.0", OK, "https://www.asus.com/Motherboards/AMD_AM3Plus/SABERTOOTH_990FX_R20/", NULL),
	B("ASUS",	"TUSL2-C",		NT, "http://support.asus.com/download.aspx?SLanguage=en&p=1&s=4&m=TUSL2-C&os=&hashedid=n/a", "Untested board enable."),
	B("ASUS",	"Z8NA-D6C",		OK, "https://www.asus.com/Server_Workstation/Server_Motherboards/Z8NAD6C/", NULL),
	B("ASUS",	"Z8PE-D12",		OK, "https://www.asus.com/Server_Workstation/Server_Motherboards/Z8PED12/", NULL),
	B("Attro",	"G5G100-P",		OK, "http://www.attro.com/motherboard/G5G100-P.htm", NULL),
	B("Bachmann",	"OT200",		OK, "http://www.bachmann.info/produkte/bedien-und-beobachtungsgeraete/operator-terminals/", NULL),
	B("BCOM",	"WinNET100",		OK, "http://www.coreboot.org/BCOM_WINNET100", "Used in the IGEL-316 thin client."),
	B("Bifferos",	"Bifferboard",		OK, "http://bifferos.co.uk/", NULL),
	B("Biostar",	"H61MGC",		BAD, NULL, "Probing works (Eon EN25Q32(A/B), 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Biostar",	"H61MU3",		BAD, NULL, "Probing works (Eon EN25Q32(A/B), 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Biostar",	"M6TBA",		BAD, "ftp://ftp.biostar-usa.com/manuals/M6TBA/", "No public report found. Owned by Uwe Hermann <uwe@hermann-uwe.de>. May work now."),
	B("Biostar",	"M7NCD Pro",		OK, "http://www.biostar.com.tw/app/en/mb/introduction.php?S_ID=260", NULL),
	B("Biostar",	"M7VIQ",		NT, NULL, NULL),
	B("Biostar",	"N61PB-M2S",		OK, NULL, NULL),
	B("Biostar",	"N68S3+",		OK, NULL, NULL),
	B("Biostar",	"P4M80-M4",		OK, "http://www.biostar-usa.com/mbdetails.asp?model=p4m80-m4", NULL),
	B("Biostar",	"TA780G M2+",		OK, "http://www.biostar.com.tw/app/en/mb/introduction.php?S_ID=344", NULL),
	B("Biostar",	"TA790GX A3+",		OK, "http://www.biostar.com.tw/app/en/mb/introduction.php?S_ID=395", NULL),
	B("Boser",	"HS-6637",		BAD, "http://www.boser.com.tw/manual/HS-62376637v3.4.pdf", "Reported by Mark Robinson <mark@zl2tod.net> to flashrom@coreboot.org, no public archive. Missing board enable and/or F29C51002T unlocking. May work now."),
	B("Congatec",	"conga-X852",		OK, "http://www.congatec.com/single_news+M57715f6263d.html?&L=1", NULL),
	B("Dell",	"Inspiron 580",		BAD, "http://support.dell.com/support/edocs/systems/insp580/en/index.htm", "Probing works (Macronix MX25L6405, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME is locked."),
	B("Dell",	"OptiPlex 7010",	BAD, NULL, "Mainboard model is 0KRC95. Probing works (Hardware Sequencing 4 + 8MB), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME is locked."),
	B("Dell",	"OptiPlex GX1",		OK, "http://support.dell.com/support/edocs/systems/ban_gx1/en/index.htm", NULL),
	B("Dell",	"PowerEdge 1850",	OK, "http://support.dell.com/support/edocs/systems/pe1850/en/index.htm", NULL),
	B("Dell",	"PowerEdge C6220",	BAD, NULL, "Mainboard model is 0HYFFG. Probing works (Macronix MX25L6405, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME is locked (and there are even overlapping PRs)."),
	B("Dell",	"Vostro 460",		BAD, "http://support.dell.com/support/edocs/systems/vos460/en/index.htm", "Mainboard model is 0Y2MRG. Probing works (Macronix MX25L3205, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME is locked."),
	B("DFI",	"855GME-MGF",		BAD, "http://www.dfi.com.tw/portal/CM/cmproduct/XX_cmproddetail/XX_WbProdsWindow?action=e&downloadType=&windowstate=normal&mode=view&downloadFlag=false&itemId=433", "Probably needs a board enable. http://www.coreboot.org/pipermail/coreboot/2009-May/048549.html"),
	B("DFI",	"AD77",			NT, NULL, "Untested board enable."),
	B("DFI",	"Blood-Iron P35 T2RL",	OK, "http://lp.lanparty.com.tw/portal/CM/cmproduct/XX_cmproddetail/XX_WbProdsWindow?itemId=516&downloadFlag=false&action=1", NULL),
	B("Elitegroup",	"848P-A7",		OK, NULL, NULL),
	B("Elitegroup",	"GeForce6100PM-M2 (V3.0)", OK, NULL, NULL),
	B("Elitegroup",	"GeForce6100SM-M",	OK, NULL, NULL),
	B("Elitegroup",	"GeForce7050M-M (V2.0)", OK, "http://www.ecs.com.tw/ECSWebSite/Product/Product_Detail.aspx?DetailID=865&MenuID=20&LanID=0", NULL),
	B("Elitegroup",	"GF7050VT-M",		OK, NULL, NULL),
	B("Elitegroup", "GF7100PVT-M3 (V1.0)",	OK, NULL, NULL),
	B("Elitegroup", "GF8200A",		OK, NULL, NULL),
	B("Elitegroup",	"K7S5A",		OK, NULL, NULL),
	B("Elitegroup",	"K7S6A",		OK, NULL, NULL),
	B("Elitegroup", "K7SEM (V1.0A)",	OK, NULL, NULL),
	B("Elitegroup",	"K7VTA3",		OK, NULL, NULL),
	B("Elitegroup",	"P4M800PRO-M (V1.0A, V2.0)", OK, NULL, NULL),
	B("Elitegroup", "P4VXMS (V1.0A)",	OK, NULL, NULL),
	B("Elitegroup",	"P6BAP-A+ (V2.2)",	OK, NULL, NULL),
	B("Elitegroup",	"P6IWP-Fe",		OK, NULL, NULL),
	B("Elitegroup",	"P6VAP-A+",		OK, NULL, NULL),
	B("Elitegroup", "RS485M-M",		OK, NULL, NULL),
	B("Emerson",	"ATCA-7360",		OK, "http://www.emerson.com/sites/Network_Power/en-US/Products/Product_Detail/Product1/Pages/EmbCompATCA-7360.aspx", NULL),
	B("EPoX",	"EP-3PTA",		BAD, NULL, "Missing board enable (W83627HF/F/HG/G), see https://flashrom.org/pipermail/flashrom/2012-April/009043.html"),
	B("EPoX",	"EP-8K5A2",		OK, "http://www.epox.com/product.asp?ID=EP-8K5A2", NULL),
	B("EPoX",	"EP-8NPA7I",		OK, "http://www.epox.com/product.asp?ID=EP-8NPA7I", NULL),
	B("EPoX",	"EP-8RDA3+",		OK, "http://www.epox.com/product.asp?ID=EP-8RDA3plus", NULL),
	B("EPoX",	"EP-9NPA7I",		OK, "http://www.epox.com/product.asp?ID=EP-9NPA7I", NULL),
	B("EPoX",	"EP-BX3",		OK, "http://www.epox.com/product.asp?ID=EP-BX3", NULL),
	B("EVGA",	"122-CK-NF68",		OK, NULL, NULL),
	B("EVGA",	"132-CK-NF78",		OK, "http://www.evga.com/articles/385.asp", NULL),
	B("EVGA",	"270-WS-W555-A2 (Classified SR-2)", OK, "http://www.evga.com/products/moreInfo.asp?pn=270-WS-W555-A2", NULL),
	B("FIC",	"VA-502",		BAD, "ftp://ftp.fic.com.tw/motherboard/manual/socket7/va-502/", "No public report found. Owned by Uwe Hermann <uwe@hermann-uwe.de>. Seems the PCI subsystem IDs are identical with the Tekram P6Pro-A5. May work now."),
	B("Foxconn",	"6150K8MD-8EKRSH",	OK, "http://www.foxconnchannel.com/ProductDetail.aspx?T=Motherboard&U=en-us0000157", NULL),
	B("Foxconn",	"A6VMX",		OK, "http://www.foxconnchannel.com/ProductDetail.aspx?T=Motherboard&U=en-us0000346", NULL),
	B("Foxconn",	"P4M800P7MA-RS2",	OK, "http://www.foxconnchannel.com/ProductDetail.aspx?T=Motherboard&U=en-us0000138", NULL),
	B("Foxconn",	"P55MX",		OK, "http://www.foxconnchannel.com/ProductDetail.aspx?T=motherboard&U=en-us0000474", "Needs the MFG jumper to be set correctly before flashing to enable the Flash Descriptor Override Strap."),
	B("Foxconn",	"Q45M",			BAD, "http://www.foxconnchannel.com/ProductDetail.aspx?T=Motherboard&U=en-us0000587", "Probing works (Hardware sequencing, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME is locked."),
	B("Freetech",	"P6F91i",		OK, "http://web.archive.org/web/20010417035034/http://www.freetech.com/prod/P6F91i.html", NULL),
	B("Fujitsu",	"D2724-A1x",		OK, NULL, "Used in ESPRIMO E5625."),
	B("Fujitsu",	"D3041-A1x",		OK, NULL, "Used in ESPRIMO P2560, contains an Atmel AT26DF081A."),
	B("Fujitsu-Siemens", "CELSIUS W410",	BAD, "ftp://ftp.ts.fujitsu.com/pub/mainboard-oem-sales/Products/Mainboards/Industrial&ExtendedLifetime/D3061&D3062/", "Mainboard model is D3062-A1. Probing works (Macronix MX25L6405, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME is locked."),
	B("Fujitsu-Siemens", "ESPRIMO P5915",	OK, "http://uk.ts.fujitsu.com/rl/servicesupport/techsupport/professionalpc/ESPRIMO/P/EsprimoP5915-6.htm", "Mainboard model is D2312-A2."),
	B("GIGABYTE",	"GA-2761GXDK",		OK, "http://www.computerbase.de/news/hardware/mainboards/amd-systeme/2007/mai/gigabyte_dtx-mainboard/", NULL),
	B("GIGABYTE",	"GA-6BXC",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1445", NULL),
	B("GIGABYTE",	"GA-6BXDU",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1429", NULL),
	B("GIGABYTE",	"GA-6IEM",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1379", NULL),
	B("GIGABYTE",	"GA-6VXE7+",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2410", NULL),
	B("GIGABYTE",	"GA-6ZMA",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1541", NULL),
	B("GIGABYTE",	"GA-770TA-UD3",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3272", NULL),
	B("GIGABYTE",	"GA-7DXR",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1302", NULL),
	B("GIGABYTE",	"GA-7VT600",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1666", NULL),
	B("GIGABYTE",	"GA-7ZM",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1366", "Works fine if you remove jumper JP9 on the board and disable the flash protection BIOS option."),
	B("GIGABYTE",	"GA-880GMA-USB3 (rev. 3.1)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3817", NULL),
	B("GIGABYTE",	"GA-8I945GZME-RH",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2304", NULL),
	B("GIGABYTE",	"GA-8IP775",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1830", NULL),
	B("GIGABYTE",	"GA-8IRML",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1343", NULL),
	B("GIGABYTE",	"GA-8PE667 Ultra 2",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1607", NULL),
	B("GIGABYTE",	"GA-8S648",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1674", NULL),
	B("GIGABYTE",	"GA-8SIMLFS 2.0",	OK, NULL, "This is an OEM board used by Fujitsu."),
	B("GIGABYTE",	"GA-8SIMLH",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1399", NULL),
	B("GIGABYTE",	"GA-945GCM-S2 (rev. 3.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2466", NULL),
	B("GIGABYTE",	"GA-945GM-S2",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2331", NULL),
	B("GIGABYTE",	"GA-945PL-S3P (rev. 6.6)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2541", NULL),
	B("GIGABYTE",	"GA-965GM-S2 (rev. 2.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2617", NULL),
	B("GIGABYTE",	"GA-965P-DS4",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2288", NULL),
	B("GIGABYTE",	"GA-965P-S3 (rev. 1.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2321", NULL),
	B("GIGABYTE",	"GA-970A-D3P (rev. 1.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4642", NULL),
	B("GIGABYTE",	"GA-970A-UD3P (rev. 2.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=5194", "Primary flash chip is a Macronix MX25L3206E."),
	B("GIGABYTE",	"GA-990FXA-UD3 (rev. 4.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4672", NULL),
	B("GIGABYTE",	"GA-A75M-UD2H",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3928", NULL),
	B("GIGABYTE",	"GA-B85M-D3H",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4567", NULL),
	B("GIGABYTE",	"GA-EG43M-S2H",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2878", NULL),
	B("GIGABYTE",	"GA-EP31-DS3L (rev. 1.0, 2.1)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2964", NULL),
	B("GIGABYTE",	"GA-EP35-DS3L",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2778", NULL),
	B("GIGABYTE",	"GA-EX58-UD4P",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2986", NULL),
	B("GIGABYTE",	"GA-G33M-S2",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2557", NULL),
	B("GIGABYTE",	"GA-G33M-S2L",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2692", NULL),
	B("GIGABYTE",	"GA-G41MT-S2PT",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3960", NULL),
	B("GIGABYTE",	"GA-H55M-S2",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3509", "8 MB (ME) + 1 MB (BIOS) flash chips - hardware sequencing required."),
	B("GIGABYTE",	"GA-H61M-D2-B3",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3773", NULL),
	B("GIGABYTE",	"GA-H61M-D2H-USB3",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4004", NULL),
	B("GIGABYTE",	"GA-H77-D3H",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4141", "Does only work with -p internal:ich_spi_mode=hwseq due to an evil twin of MX25L6405 and ICH SPI lockdown."),
	B("GIGABYTE",	"GA-H77-DS3H (rev. 1.1)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4318", NULL),
	B("GIGABYTE",	"GA-H77M-D3H",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4388", NULL),
	B("GIGABYTE",	"GA-J1900N-D3V",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4918", NULL),
	B("GIGABYTE",	"GA-K8N51GMF-9",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1939", NULL),
	B("GIGABYTE",	"GA-K8N51GMF",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1950", NULL),
	B("GIGABYTE",	"GA-K8N-SLI",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1928", NULL),
	B("GIGABYTE",	"GA-K8NS",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=1784", NULL),
	B("GIGABYTE",	"GA-M56S-S3",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2607", NULL),
	B("GIGABYTE",	"GA-M57SLI-S4",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2287", NULL),
	B("GIGABYTE",	"GA-M61P-S3",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2434", NULL),
	B("GIGABYTE",	"GA-M720-US3",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3006", NULL),
	B("GIGABYTE",	"GA-MA69VM-S2",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2500", NULL),
	B("GIGABYTE",	"GA-MA74GM-S2H (rev. 3.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3152", NULL),
	B("GIGABYTE",	"GA-MA770-UD3 (rev. 2.1)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3302", NULL),
	B("GIGABYTE",	"GA-MA770T-UD3P",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3096", NULL),
	B("GIGABYTE",	"GA-MA780G-UD3H",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3004", NULL),
	B("GIGABYTE",	"GA-MA785GMT-UD2H (rev. 1.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3156", NULL),
	B("GIGABYTE",	"GA-MA78G-DS3H (rev. 1.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2800", NULL),
	B("GIGABYTE",	"GA-MA78GM-S2H",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2758", NULL), /* TODO: Rev. 1.BAD, 1.OK, or 2.x? */
	B("GIGABYTE",	"GA-MA78GPM-DS2H",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2859", NULL),
	B("GIGABYTE",	"GA-MA790FX-DQ6",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2690", NULL),
	B("GIGABYTE",	"GA-MA790GP-DS4H",	OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2887", NULL),
	B("GIGABYTE",	"GA-MA790XT-UD4P (rev. 1.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3010", NULL),
	B("GIGABYTE",	"GA-P31-DS3L",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2615", NULL),
	B("GIGABYTE",	"GA-P31-S3G",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=2676", NULL),
	B("GIGABYTE",	"GA-P55-USB3 (rev. 2.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3440", NULL),
	B("GIGABYTE",	"GA-P55A-UD4 (rev. 1.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3436", NULL),
	B("GIGABYTE",	"GA-P55A-UD7"		, OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3324", NULL),
	B("GIGABYTE",	"GA-P67A-UD3P",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3649", NULL),
	B("GIGABYTE",	"GA-X58A-UD3R (rev. 2.0)", OK, NULL, NULL),
	B("GIGABYTE",	"GA-X58A-UD7 (rev. 2.0)", OK, NULL, NULL),
	B("GIGABYTE",	"GA-X79-UD5",		OK, NULL, NULL),
	B("GIGABYTE",	"GA-X79-UD3",		OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4050", "Contains a Macronix MX25L6406E."),
	B("GIGABYTE",	"GA-X79-UP4 (rev. 1.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4288", NULL),
	B("GIGABYTE",	"GA-Z68MA-D2H-B3 (rev. 1.3)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3975", NULL),
	B("GIGABYTE",	"GA-Z68MX-UD2H-B (rev. 1.3)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3854", NULL),
	B("GIGABYTE",	"GA-Z68XP-UD3 (rev. 1.0)", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=3892", NULL),
	B("GIGABYTE",	"GA-Z77MX-D3H",		BAD, "http://www.gigabyte.com/products/product-page.aspx?pid=4145", "Uses MX25L6436E and requires a small patch (but works flawlessly with that)."),
	B("GIGABYTE",	"GA-Z87-HD3", OK, "http://www.gigabyte.com/products/product-page.aspx?pid=4489", NULL),
	B("HP",		"8100 Elite CMT PC (304Bh)", BAD, NULL, "SPI lock down, PR, read-only descriptor, locked ME region."),
	B("HP",		"e-Vectra P2706T",	OK, "http://h20000.www2.hp.com/bizsupport/TechSupport/Home.jsp?lang=en&cc=us&prodSeriesId=77515&prodTypeId=12454", NULL),
	B("HP",		"Evans-GL6 (Pegatron IPMEL-AE)", OK, "http://h10025.www1.hp.com/ewfrf/wc/document?cc=us&lc=en&dlc=en&docname=c01925513", "Found in HP Pavilion Slimline s5220f."),
	B("HP",		"ProLiant DL145 G3",	OK, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?objectID=c00816835&lang=en&cc=us&taskId=101&prodSeriesId=3219755&prodTypeId=15351", NULL),
	B("HP",		"ProLiant DL165 G6",	OK, "http://h10010.www1.hp.com/wwpc/us/en/sm/WF05a/15351-15351-3328412-241644-3328421-3955644.html", NULL),
	B("HP",		"ProLiant N40L",	OK, NULL, NULL),
	B("HP",		"Puffer2-UL8E",		OK, "http://h10025.www1.hp.com/ewfrf/wc/document?docname=c00300023", NULL),
	B("HP",		"dc7800",		BAD, "http://h10010.www1.hp.com/wwpc/us/en/sm/WF06a/12454-12454-64287-321860-3328898-3459241.html?dnr=1", "ICH9DO with SPI lock down, BIOS lock, PR, read-only descriptor, locked ME region."),
	B("HP",		"Vectra VL400",		OK, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?objectID=c00060658&lang=en&cc=us", NULL),
	B("HP",		"Vectra VL420 SFF",	OK, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?objectID=c00060661&lang=en&cc=us", NULL),
	B("HP",		"xw4400 (0A68h)",	BAD, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?objectID=c00775230", "ICH7 with SPI lock down, BIOS lock, flash block detection (SST25VF080B); see http://paste.flashrom.org/view.php?id=686"),
	B("HP",		"xw6400",		BAD, NULL, "No chip found, see https://flashrom.org/pipermail/flashrom/2012-March/009006.html"),
	B("HP",		"xw9300",		BAD, "http://h20000.www2.hp.com/bizsupport/TechSupport/Home.jsp?lang=en&cc=us&prodTypeId=12454&prodSeriesId=459226", "Missing board enable, see https://flashrom.org/pipermail/flashrom/2012-March/008885.html"),
	B("HP",		"xw9400",		OK, "http://h20000.www2.hp.com/bizsupport/TechSupport/Home.jsp?lang=en&cc=us&prodSeriesId=3211286&prodTypeId=12454", "Boot block is write protected unless the solder points next to F2 are shorted."),
	B("HP",		"Z400 Workstation (0AE4h)", BAD, NULL, "ICH10R with BIOS lock enable and a protected range PRBAD, see https://flashrom.org/pipermail/flashrom/2012-June/009350.html"),
	B("IBASE",	"MB899",		OK, "http://www.ibase-i.com.tw/2009/mb899.html", NULL),
	B("IBM",	"x3455",		OK, "http://www-03.ibm.com/systems/x/hardware/rack/x3455/index.html", NULL),
	B("IEI",	"PICOe-9452",		OK, "http://www.ieiworld.com/product_groups/industrial/content.aspx?keyword=WSB&gid=00001000010000000001&cid=08125380291060861658&id=08142308605814597144", NULL),
	B("Intel",	"D201GLY",		OK, "http://www.intel.com/support/motherboards/desktop/d201gly/index.htm", NULL),
	B("Intel",	"D2700MUD",		BAD, "http://www.intel.com/cd/products/services/emea/eng/motherboards/desktop/D2700MUD/", "SMM protection enabled"),
	B("Intel",	"D425KT",		BAD, "http://www.intel.com/content/www/us/en/motherboards/desktop-motherboards/desktop-board-d425kt.html", "NM10 with SPI lock down, BIOS lock, see https://flashrom.org/pipermail/flashrom/2012-January/008600.html"),
	B("Intel",	"D865GLC",		BAD, NULL, "ICH5 with BIOS lock enable, see http://paste.flashrom.org/view.php?id=775"),
	B("Intel",	"D945GCNL",		OK, NULL, NULL),
	B("Intel",	"DG45ID",		BAD, "http://www.intel.com/products/desktop/motherboards/dg45id/dg45id-overview.htm", "Probing works (Winbond W25x32, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME is locked."),
	B("Intel",	"DQ965GF",		BAD, NULL, "Probing enables Hardware Sequencing (behind that hides a SST SST25VF016B, 2048 kB). Parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME is locked (and the platform data region seems to be bogus)."),
	B("Intel",	"DG965OT",		BAD, NULL, "Probing enables Hardware Sequencing (behind that hides a SST SST25VF080B, 1024 kB). Parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME is locked (and the platform data region seems to be bogus)."),
	B("Intel",	"DH61AG ",		BAD, NULL, "H61 with BIOS lock enable and locked ME region, see https://flashrom.org/pipermail/flashrom/2012-June/009417.html"),
	B("Intel",	"DH67CF",		BAD, NULL, "H67 with BIOS lock enable and locked ME region, see https://flashrom.org/pipermail/flashrom/2011-September/007789.html"),
	B("Intel",	"DH67CL",		BAD, NULL, "H67 with BIOS lock enable and locked ME region, see https://flashrom.org/pipermail/flashrom/2012-November/010112.html"),
	B("Intel",	"DN2800MT (Marshalltown)", BAD, NULL, "BIOS locked via BIOS_CNTL."),
	B("Intel",	"DQ45CB",		BAD, NULL, "Probing works (Winbond W25Q32, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Intel",	"DQ77MK",		BAD, NULL, "Q77 with BIOS lock enable and locked ME region, see http://paste.flashrom.org/view.php?id=1603"),
	B("Intel",	"EP80759",		OK, NULL, NULL),
	B("Intel",	"Foxhollow",		OK, NULL, "Intel reference board."),
	B("Intel",	"Greencity",		OK, NULL, "Intel reference board."),
	B("Intel",	"SE440BX-2",		BAD, "http://downloadcenter.intel.com/SearchResult.aspx?lang=eng&ProductFamily=Desktop+Boards&ProductLine=Discontinued+Motherboards&ProductProduct=Intel%C2%AE+SE440BX-2+Motherboard", "Probably won't work, see http://www.coreboot.org/pipermail/flashrom/2010-July/003952.html"),
	B("IWILL",	"DK8-HTX",		OK, "http://web.archive.org/web/20060507170150/http://www.iwill.net/product_2.asp?p_id=98", NULL),
	B("Jetway",	"J-7BXAN",		OK, "http://www.jetway.com.tw/evisn/download/d7BXAS.htm", NULL),
	B("Jetway",	"J7F4K1G5D-PB",		OK, "http://www.jetway.com.tw/jw/ipcboard_view.asp?productid=282&proname=J7F4K1G5D", NULL),
	B("Kontron",	"986LCD-M",		OK, "http://de.kontron.com/products/boards+and+mezzanines/embedded+motherboards/miniitx+motherboards/986lcdmmitx.html", NULL),
	B("Lanner",	"EM-8510C",		OK, NULL, NULL),
	B("Lenovo",	"Tilapia CRB",		OK, NULL, "Used in ThinkCentre M75e."),
	B("Lex",	"CV700A",		OK, "http://www.lex.com.tw/product/CV700A-spec.htm", NULL),
	B("Mitac",	"6513WU",		OK, "http://web.archive.org/web/20050313054828/http://www.mitac.com/micweb/products/tyan/6513wu/6513wu.htm", NULL),
	B("MSC",	"Q7-TCTC",		OK, "http://www.msc-ge.com/en/produkte/com/moduls/overview/5779-www.html", NULL),
	B("MSI",	"MS-6153",		OK, "http://www.msi.com/product/mb/MS-6153.html", NULL),
	B("MSI",	"MS-6156",		OK, "http://uk.ts.fujitsu.com/rl/servicesupport/techsupport/boards/Motherboards/MicroStar/Ms6156/MS6156.htm", NULL),
	B("MSI",	"MS-6163 (MS-6163 Pro)",OK, "http://www.msi.com/product/mb/MS-6163-Pro.html", NULL),
	B("MSI",	"MS-6178",		BAD, "http://www.msi.com/product/mb/MS-6178.html", "Immediately powers off if you try to hot-plug the chip. However, this does '''not''' happen if you use coreboot. Owned by Uwe Hermann <uwe@hermann-uwe.de>."),
	B("MSI",	"MS-6330 (K7T Turbo)",	OK, "http://www.msi.com/product/mb/K7T-Turbo.html", NULL),
	B("MSI",	"MS-6391 (845 Pro4)",	OK, "http://www.msi.com/product/mb/845-Pro4.html", NULL),
	B("MSI",	"MS-6561 (745 Ultra)",	OK, "http://www.msi.com/product/mb/745-Ultra.html", NULL),
	B("MSI",	"MS-6566 (845 Ultra-C)",OK, "http://www.msi.com/product/mb/845-Ultra-C.html", NULL),
	B("MSI",	"MS-6570 (K7N2)",	OK, "http://www.msi.com/product/mb/K7N2.html", NULL),
	B("MSI",	"MS-6577 (Xenon)",	OK, "http://h10025.www1.hp.com/ewfrf/wc/document?product=90390&lc=en&cc=us&dlc=en&docname=bph07843", "This is an OEM board from HP, the HP name is Xenon."),
	B("MSI",	"MS-6590 (KT4 Ultra)",	OK, "http://www.msi.com/product/mb/KT4-Ultra.html", NULL),
	//B("MSI",	"MS-6702E (K8T Neo2-F/FIR)",OK, "http://www.msi.com/product/mb/K8T-Neo2-F--FIR.html", NULL), This was wrongly attributed to the MS-7094 board enable.
	B("MSI",	"MS-6704 (845PE Max2 PCB 1.0)", OK, "http://www.msi.com/product/mb/845PE-Max2.html", "Write protection must be disabled in the BIOS setup."),
	B("MSI",	"MS-6712 (KT4V)",	OK, "http://www.msi.com/product/mb/KT4V---KT4V-L--v1-0-.html", NULL),
	B("MSI",	"MS-6787 (P4MAM-V/P4MAM-L)", OK, "http://www.msi.com/service/search/?kw=6787&type=product", NULL),
	B("MSI",	"MS-7005 (651M-L)",	OK, "http://www.msi.com/product/mb/651M-L.html", NULL),
	B("MSI",	"MS-7025 (K8N Neo2 Platinum)", OK, "http://www.msi.com/product/mb/K8N-Neo2-Platinum.html", NULL),
	B("MSI",	"MS-7030 (K8N Neo Platinum)", OK, "http://www.msi.com/product/mb/K8N-Neo-Platinum.html", NULL),
	B("MSI",	"MS-7046",		OK, "http://www.heimir.de/ms7046/", NULL),
	B("MSI",	"MS-7061 (KM4M-V/KM4AM-V)", OK, "http://www.msi.com/service/search/?kw=7061&type=product", NULL),
	B("MSI",	"MS-7065",		OK, "http://browse.geekbench.ca/geekbench2/view/53114", NULL),
	B("MSI",	"MS-7094 (K8T Neo2-F V2.0)",OK, "http://www.msi.com/product/mb/K8T_Neo2F_V2.0.html", NULL),
	B("MSI",	"MS-7125 (K8N Neo4(-F/-FI/-FX/Platinum))", OK, "http://www.msi.com/product/mb/K8N_Neo4_Platinum_PCB_1.0.html", NULL),
	B("MSI",	"MS-7135 (K8N Neo3)",	OK, "http://www.msi.com/product/mb/K8N-Neo3.html", NULL),
	B("MSI",	"MS-7142 (K8MM-V)",	OK, "http://www.msi.com/product/mb/K8MM-V.html", NULL),
	B("MSI",	"MS-7168 (Orion)",	OK, "http://support.packardbell.co.uk/uk/item/index.php?i=spec_orion&pi=platform_honeymoon_istart", NULL),
	B("MSI",	"MS-7207 (K8NGM2-L)",	OK, "http://www.msi.com/product/mb/K8NGM2-FID--IL--L.html", NULL),
	B("MSI",	"MS-7211 (PM8M3-V)",	OK, "http://www.msi.com/product/mb/PM8M3-V.html", NULL),
	B("MSI",	"MS-7236 (945PL Neo3)",	OK, "http://www.msi.com/product/mb/945PL-Neo3.html", NULL),
	B("MSI",	"MS-7250 (K9N SLI (rev 2.1))", OK, "http://www.msi.com/product/mb/K9N--SLI.html", NULL),
	B("MSI",	"MS-7253 (K9VGM-V)",	OK, "http://www.msi.com/product/mb/K9VGM-V.html", NULL),
	B("MSI",	"MS-7255 (P4M890M)",	OK, "http://www.msi.com/product/mb/P4M890M-L-IL.html", NULL),
	B("MSI",	"MS-7260 (K9N Neo PCB 1.0)", BAD, "http://www.msi.com/product/mb/K9N-Neo--PCB-1-0-.html", "Interestingly flashrom does not work when the vendor BIOS is booted, but it ''does'' work flawlessly when the machine is booted with coreboot. Owned by Uwe Hermann <uwe@hermann-uwe.de>."),
	B("MSI",	"MS-7309 (K9N6SGM-V)", BAD, "http://www.msi.com/product/mb/K9N6SGM-V---K9N6PGM-FI---K9N6PGM-F.html", "Uses Fintek F71882F/F71883F/F71887 SPI-to-LPC translation."),
	B("MSI",	"MS-7309 (K9N6PGM2-V2)", OK, "http://www.msi.com/product/mb/K9N6PGM2-V2.html", NULL),
	B("MSI",	"MS-7312 (K9MM-V)",	OK, "http://www.msi.com/product/mb/K9MM-V.html", NULL),
	B("MSI",	"MS-7336",		OK, NULL, "Some non-essential DMI data (e.g. serial numbers) is overwritten when using flashrom. This is an OEM board used by HP (e.g. dx2300 Microtower)."),
	B("MSI",	"MS-7345 (P35 Neo2-FIR)", OK, "http://www.msi.com/product/mb/P35-Neo2-FR---FIR.html", NULL),
	B("MSI",	"MS-7357 (G33M)",	OK, "http://www.msi.com/product/mb/G33M.html", NULL),
	B("MSI",	"MS-7368 (K9AG Neo2-Digital)", OK, "http://www.msi.com/product/mb/K9AG-Neo2-Digital.html", NULL),
	B("MSI",	"MS-7369 (K9N Neo V2)", OK, "http://www.msi.com/product/mb/K9N-Neo-V2.html", NULL),
	B("MSI",	"MS-7376 (K9A2 Platinum V1)", OK, "http://www.msi.com/product/mb/K9A2-Platinum.html", NULL),
	B("MSI",	"MS-7379 (G31M)",	OK, "http://www.msi.com/product/mb/G31M.html", NULL),
	B("MSI",	"MS-7399 1.1 (Persian)", OK, "http://acersupport.com/acerpanam/desktop/0000/Acer/AspireM5640/AspireM5640sp2.shtml", "This is an OEM board used by Acer in e.g. Aspire M5640/M3640."),
	B("MSI",	"MS-7502",		OK, NULL, "This is an OEM board used by Medion in e.g. Medion MD8833."),
	B("MSI",	"MS-7522 (MSI X58 Pro-E)", OK, "http://www.msi.com/product/mb/X58_ProE.html", NULL),
	B("MSI",	"MS-7529 (G31M3-L(S) V2)", OK, "http://www.msi.com/product/mb/G31M3-L-V2---G31M3-LS-V2.html", NULL),
	B("MSI",	"MS-7529 (G31TM-P21)",	OK, "http://www.msi.com/product/mb/G31TM-P21.html", NULL),
	B("MSI",	"MS-7548 (Aspen-GL8E)", OK, "http://h10025.www1.hp.com/ewfrf/wc/document?docname=c01635688&lc=en&cc=us&dlc=en", NULL),
	B("MSI",	"MS-7551 (KA780G)",	OK, "http://www.msi.com/product/mb/KA780G.html", NULL),
	B("MSI",	"MS-7596 (785GM-E51)",  OK, "http://www.msi.com/product/mb/785GM-E51.html", NULL),
	B("MSI",	"MS-7597 (GF615M-P33)",	BAD, NULL, "Missing board enable/SIO support (Fintek F71889), see https://flashrom.org/pipermail/flashrom/2012-March/008956.html"),
	B("MSI",	"MS-7599 (870-C45)",	OK, "http://www.msi.com/product/mb/870-C45.html", NULL),
	B("MSI",	"MS-7613 (Iona-GL8E)",	BAD, "http://h10025.www1.hp.com/ewfrf/wc/document?docname=c02014355&lc=en&cc=dk&dlc=en&product=4348478", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("MSI",	"MS-7635 (H55M-ED55)",	BAD, "http://www.msi.com/product/mb/H55M-ED55.html", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("MSI",	"MS-7640 (890FXA-GD70)",OK, "http://www.msi.com/product/mb/890FXA-GD70.html", NULL),
	B("MSI",	"MS-7642 (890GXM-G65)",	OK, "http://www.msi.com/product/mb/890GXM-G65.html", NULL),
	B("MSI",	"MS-7676 (H67MA-ED55(B3))", OK, "http://www.msi.com/product/mb/H67MA-ED55--B3-.html", "Seems to work fine basically, but user reported (hopefully unrelated) buggy behavior of the board after a firmware upgrade. See https://flashrom.org/pipermail/flashrom/2012-January/008547.html"),
	B("MSI",	"MS-7676 (Z68MA-G45 (B3))", OK, "http://www.msi.com/product/mb/Z68MA-G45--B3-.html", NULL),
	B("MSI",	"MS-7696 (A75MA-G55)",	OK, "http://www.msi.com/product/mb/A75MA-G55.html", NULL),
	B("MSI",	"MS-7698 (E350IA-E45)",	OK, "http://www.msi.com/product/mb/E350IA-E45.html", NULL),
	B("MSI",	"MS-7740 (H61MA-E35(B3))", OK, "http://www.msi.com/product/mb/H61MA-E35--B3-.html", NULL),
	B("MSI",	"MS-7756 (H77MA-G43)",	OK, "http://www.msi.com/product/mb/H77MA-G43.html", NULL),
	B("MSI",	"MS-7760 (X79A-GD45 (8D))", OK, "http://www.msi.com/product/mb/X79A-GD45-8D.html", NULL),
	B("MSI",	"MS-7808 (B75MA-E33)",	OK, "http://www.msi.com/product/mb/B75MA-E33.html", NULL),
	B("MSI",	"MS-7816 (H87-G43)",	OK, "http://www.msi.com/product/mb/H87-G43.html", NULL),
	B("MSI",	"MS-7817 (H81M-E33)",	OK, "http://www.msi.com/product/mb/H81ME33.html", NULL),
	B("MSI",	"MS-9830 (IM-945GSE-A, A9830IMS)", OK, "http://www.msi.com/product/ipc/IM-945GSE-A.html", NULL),
	B("NEC",	"PowerMate 2000",	OK, "http://support.necam.com/mobilesolutions/hardware/Desktops/pm2000/celeron/", NULL),
	B("Nokia",	"IP530",		OK, NULL, NULL),
	B("Palit",	"N61S",			OK, NULL, NULL),
	B("PCCHIPS ",	"M598LMR (V9.0)",	OK, NULL, NULL),
	B("PCCHIPS ",	"M863G (V5.1A)",	OK, "http://www.pcchips.com.tw/PCCWebSite/Products/ProductsDetail.aspx?CategoryID=1&DetailID=343&DetailName=Feature&MenuID=1&LanID=0", NULL),
	B("PC Engines",	"Alix.1c",		OK, "http://pcengines.ch/alix1c.htm", NULL),
	B("PC Engines",	"Alix.2c2",		OK, "http://pcengines.ch/alix2c2.htm", NULL),
	B("PC Engines",	"Alix.2c3",		OK, "http://pcengines.ch/alix2c3.htm", NULL),
	B("PC Engines",	"Alix.2d3",		OK, "http://pcengines.ch/alix2d3.htm", NULL),
	B("PC Engines",	"Alix.3c3",		OK, "http://pcengines.ch/alix3c3.htm", NULL),
	B("PC Engines",	"Alix.3d3",		OK, "http://pcengines.ch/alix3d3.htm", NULL),
	B("PC Engines",	"Alix.6f2",		OK, "http://pcengines.ch/alix6f2.htm", NULL),
	B("PC Engines",	"APU",			OK, "http://pcengines.ch/apu.htm", NULL),
	B("PC Engines",	"WRAP.2E",		OK, "http://pcengines.ch/wrap2e1.htm", NULL),
	B("PCWARE",	"APM80-D3",		OK, "http://www.pcwarebr.com.br/produtos_mb_apm80-d3.php", "Probably manufactured by ASUS"),
	B("Pegatron",	"IPP7A-CP",		OK, NULL, NULL),
	B("Portwell",	"PEB-4700VLA",		OK, "http://www.portwell.com/products/detail.asp?CUSTCHAR1=PEB-4700VLA", NULL),
	B("RCA",	"RM4100",		OK, "http://www.settoplinux.org/index.php?title=RCA_RM4100", NULL),
	B("Samsung",	"Polaris 32",		OK, NULL, NULL),
	B("SAPPHIRE",	"IPC-E350M1",		OK, "http://www.sapphiretech.com/presentation/product/?pid=1034&lid=1", NULL),
	B("Shuttle",	"AB61",			OK, "http://www.shuttle.eu/_archive/older/de/ab61.htm", NULL),
	B("Shuttle",	"AK31",			OK, "http://www.motherboard.cz/mb/shuttle/AK31.htm", NULL),
	B("Shuttle",	"AK38N",		OK, "http://eu.shuttle.com/en/desktopdefault.aspx/tabid-36/558_read-9889/", NULL),
	B("Shuttle",	"AV11V30",		OK, NULL, NULL),
	B("Shuttle",	"AV18E2",		OK, "http://www.shuttle.eu/_archive/older/de/av18.htm", NULL),
	B("Shuttle",	"FB61",			OK, "http://www.shuttle.eu/_archive/older/en/fb61.htm#mainboardfb6", "Used in SB61G2 systems."),
	B("Shuttle",	"FD37",			OK, "http://www.shuttle.eu/products/discontinued/barebones/sd37p2/", NULL),
	B("Shuttle",	"FH67",			OK, "http://www.shuttle.eu/products/mini-pc/sh67h3/specification/", NULL),
	B("Shuttle",	"FN25",			OK, "http://www.shuttle.eu/products/discontinued/barebones/sn25p/?0=", NULL),
	B("Shuttle",	"FN78S",		OK, "http://www.shuttle.eu/products/discontinued/barebones/sn78sh7/", NULL),
	B("Shuttle",	"X50/X50(B)",		OK, "http://au.shuttle.com/product_detail_spec.jsp?PI=1241", NULL),
	B("Soyo",	"SY-5VD",		BAD, "http://www.soyo.com/content/Downloads/163/&c=80&p=464&l=English", "No public report found. Owned by Uwe Hermann <uwe@hermann-uwe.de>. May work now."),
	B("Soyo",	"SY-6BA+ III",		OK, "http://www.motherboard.cz/mb/soyo/SY-6BA+III.htm", NULL),
	B("Soyo",	"SY-7VCA",		OK, "http://www.tomshardware.com/reviews/12-socket-370-motherboards,196-15.html", NULL),
	B("Sun",	"Blade x6250",		OK, "http://www.sun.com/servers/blades/x6250/", NULL),
	B("Sun",	"Fire x4150",		BAD, "http://www.sun.com/servers/x64/x4150/", "No public report found. May work now."),
	B("Sun",	"Fire x4200",		BAD, "http://www.sun.com/servers/entry/x4200/", "No public report found. May work now."),
	B("Sun",	"Fire x4540",		BAD, "http://www.sun.com/servers/x64/x4540/", "No public report found. May work now."),
	B("Sun",	"Fire x4600",		BAD, "http://www.sun.com/servers/x64/x4600/", "No public report found. May work now."),
	B("Sun",	"Ultra 40 M2",		OK, "http://download.oracle.com/docs/cd/E19127-01/ultra40.ws/820-0123-13/intro.html", NULL),
	B("Supermicro",	"A1SAi-2550F",		OK, "http://www.supermicro.com/products/motherboard/Atom/X10/A1SAi-2550F.cfm", NULL),
	B("Supermicro",	"H8QC8",		OK, "http://www.supermicro.com/Aplus/motherboard/Opteron/nforce/H8QC8.cfm", NULL),
	B("Supermicro",	"H8QME-2",		OK, "http://www.supermicro.com/Aplus/motherboard/Opteron8000/MCP55/H8QME-2.cfm", NULL),
	B("Supermicro",	"X10SLM-F",		BAD, "http://www.supermicro.com/products/motherboard/Xeon/C220/X10SLM-F.cfm", "Probing works (Winbond W25Q128, 16384 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked; SMM protection enabled."),
	B("Supermicro", "X5DP8-G2",		OK, "http://www.supermicro.com/products/motherboard/Xeon/E7501/X5DP8-G2.cfm", NULL),
	B("Supermicro", "X7DBT-INF",		OK, "http://www.supermicro.com/products/motherboard/Xeon1333/5000P/X7DBT-INF.cfm", NULL),
	B("Supermicro", "X7DWT",		OK, "http://www.supermicro.com/products/motherboard/Xeon1333/5400/X7DWT.cfm", "Used in Dell C6100 servers."),
	B("Supermicro", "X7SPA-H(F)",		OK, "http://www.supermicro.com/products/motherboard/ATOM/ICH9/X7SPA.cfm?typ=H", NULL),
	B("Supermicro", "X7SPE-HF-D525",	OK, "http://www.supermicro.com/products/motherboard/ATOM/ICH9/X7SPE-HF-D525.cfm", NULL),
	B("Supermicro", "X8DT3",		OK, "http://www.supermicro.com/products/motherboard/QPI/5500/X8DT3.cfm", NULL),
	B("Supermicro", "X8DT6-F",		OK, "http://www.supermicro.nl/products/motherboard/QPI/5500/X8DT6-F.cfm?IPMI=Y&SAS=Y", NULL),
	B("Supermicro", "X8DTE-F",		OK, "http://www.supermicro.com/products/motherboard/QPI/5500/X8DT6-F.cfm?IPMI=Y&SAS=N", NULL),
	B("Supermicro", "X8DTG-D",		OK, "http://www.supermicro.com/products/motherboard/qpi/5500/x8dtg-df.cfm", NULL),
	B("Supermicro", "X8DTH-6F",		OK, "http://www.supermicro.com/products/motherboard/QPI/5500/X8DTH-6F.cfm", NULL),
	B("Supermicro",	"X8DTT-F",		OK, "http://www.supermicro.com/products/motherboard/QPI/5500/X8DTT-F.cfm", NULL),
	B("Supermicro",	"X8DTT-HIBQF",		OK, "http://www.supermicro.com/products/motherboard/QPI/5500/X8DTT-H.cfm", NULL),
	B("Supermicro",	"X8DTU-6TF+",		BAD, "http://www.supermicro.com/products/motherboard/QPI/5500/X8DTU_.cfm?TYP=SAS&LAN=10", "Probing works (Atmel AT25DF321A, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Supermicro",	"X8DTU-F",		OK, "http://www.supermicro.com/products/motherboard/QPI/5500/X8DTU-F.cfm", NULL),
	B("Supermicro",	"X8SAX",		OK, "http://www.supermicro.com/products/motherboard/xeon3000/x58/x8sax.cfm", NULL),
	B("Supermicro",	"X8SIE(-F)",		BAD, "http://www.supermicro.com/products/motherboard/Xeon3000/3400/X8SIE.cfm?IPMI=N&TYP=LN2", "Requires unlocking the ME although the registers are set up correctly by the descriptor/BIOS already (tested with swseq and hwseq)."),
	B("Supermicro",	"X8SIL-F",		OK, "http://www.supermicro.com/products/motherboard/Xeon3000/3400/X8SIL.cfm", NULL),
	B("Supermicro",	"X8STi",		OK, "http://www.supermicro.com/products/motherboard/Xeon3000/X58/X8STi.cfm", NULL),
	B("Supermicro",	"X9DR3-F",		BAD, "http://www.supermicro.com/products/motherboard/xeon/c600/x9dr3-f.cfm", "Probing works (Numonyx N25Q128 (supported by SFDP only atm), 16384 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Supermicro",	"X9DRD-7LN4F",		BAD, "http://www.supermicro.com/products/motherboard/xeon/c600/x9drd-7ln4f.cfm", "Probing works (Numonyx N25Q128 (supported by SFDP only atm), 16384 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Supermicro",	"X9DRT-HF+",		BAD, NULL, "Probing works (Numonyx N25Q128 (supported by SFDP only atm), 16384 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked; SMM protection enabled."),
	B("Supermicro",	"X9DRW",		BAD, NULL, "Probing works (Numonyx N25Q128 (supported by SFDP only atm), 16384 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Supermicro",	"X9QRi-F+",		BAD, "http://www.supermicro.com/products/motherboard/Xeon/C600/X9QRi-F_.cfm", "Probing works (Macronix MX25L12805, 16384 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked; SMM protection enabled."),
	B("Supermicro",	"X9SCA-F",		BAD, "http://www.supermicro.com/products/motherboard/Xeon/C202_C204/X9SCA-F.cfm", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Supermicro",	"X9SCE-F",		BAD, "http://www.supermicro.com/products/motherboard/Xeon/C202_C204/X9SCE-F.cfm", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Supermicro",	"X9SCL",		BAD, "http://www.supermicro.com/products/motherboard/Xeon/C202_C204/X9SCL.cfm", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Supermicro",	"X9SCM-F",		BAD, "http://www.supermicro.com/products/motherboard/Xeon/C202_C204/X9SCM-F.cfm", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("T-Online",	"S-100",		OK, "http://wiki.freifunk-hannover.de/T-Online_S_100", NULL),
	B("Tekram",	"P6Pro-A5",		OK, "http://www.motherboard.cz/mb/tekram/P6Pro-A5.htm", NULL),
	B("Termtek",	"TK-3370 (Rev:2.5B)",	OK, NULL, NULL),
	B("Thomson",	"IP1000",		OK, "http://www.settoplinux.org/index.php?title=Thomson_IP1000", NULL),
	B("TriGem",	"Anaheim-3",		OK, "http://www.e4allupgraders.info/dir1/motherboards/socket370/anaheim3.shtml", NULL),
	B("TriGem",	"Lomita",		OK, "http://www.e4allupgraders.info/dir1/motherboards/socket370/lomita.shtml", NULL),
	B("Tyan",	"S1846 (Tsunami ATX)",	OK, "http://www.tyan.com/archive/products/html/tsunamiatx.html", NULL),
	B("Tyan",	"S2466 (Tiger MPX)",	OK, "http://www.tyan.com/product_board_detail.aspx?pid=461", NULL),
	B("Tyan",	"S2498 (Tomcat K7M)",	OK, "http://www.tyan.com/archive/products/html/tomcatk7m.html", NULL),
	B("Tyan",	"S2723 (Tiger i7501)",	OK, "http://www.tyan.com/archive/products/html/tigeri7501.html", NULL),
	B("Tyan",	"S2875 (Tiger K8W)",	OK, "http://www.tyan.com/archive/products/html/tigerk8w.html", NULL),
	B("Tyan",	"S2881 (Thunder K8SR)",	OK, "http://www.tyan.com/product_board_detail.aspx?pid=115", NULL),
	B("Tyan",	"S2882-D (Thunder K8SD Pro)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=127", NULL),
	B("Tyan",	"S2882 (Thunder K8S Pro)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=121", NULL),
	B("Tyan",	"S2891 (Thunder K8SRE)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=144", NULL),
	B("Tyan",	"S2892 (Thunder K8SE)",	OK, "http://www.tyan.com/product_board_detail.aspx?pid=145", NULL),
	B("Tyan",	"S2895 (Thunder K8WE)",	OK, "http://www.tyan.com/archive/products/html/thunderk8we.html", NULL),
	B("Tyan",	"S2912 (Thunder n3600R)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=157", NULL),
	B("Tyan",	"S2915-E (Thunder n6650W)", OK, "http://tyan.com/product_SKU_spec.aspx?ProductType=MB&pid=541&SKU=600000041", NULL),
	B("Tyan",	"S2915 (Thunder n6650W)", OK, "http://tyan.com/product_board_detail.aspx?pid=163", NULL),
	B("Tyan",	"S2933 (Thunder n3600S)", OK, "http://tyan.com/product_SKU_spec.aspx?ProductType=MB&pid=478&SKU=600000063", NULL),
	B("Tyan",	"S3095 (Tomcat i945GM)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=181", NULL),
	B("Tyan",	"S3992 (Thunder h2000M)", OK, "http://tyan.com/product_board_detail.aspx?pid=235", NULL),
	B("Tyan",	"S4882 (Thunder K8QS Pro)", OK, "http://www.tyan.com/archive/products/html/thunderk8qspro.html", NULL),
	B("Tyan",	"S5180 (Toledo i965R)",	OK, "http://www.tyan.com/product_board_detail.aspx?pid=456", NULL),
	B("Tyan",	"S5191 (Toledo i3000R)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=343", NULL),
	B("Tyan",	"S5197 (Toledo i3010W)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=349", NULL),
	B("Tyan",	"S5211-1U (Toledo i3200R)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=593", NULL),
	B("Tyan",	"S5211 (Toledo i3210W)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=591", NULL),
	B("Tyan",	"S5220 (Toledo q35T)",	OK, "http://www.tyan.com/product_board_detail.aspx?pid=597", NULL),
	B("Tyan",	"S5375-1U (Tempest i5100X)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=610", NULL),
	B("Tyan",	"S5375 (Tempest i5100X)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=566", NULL),
	B("Tyan",	"S5376 (Tempest i5100W)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=605", "Both S5376G2NR and S5376WAG2NR should work."),
	B("Tyan",	"S5377 (Tempest i5100T)", OK, "http://www.tyan.com/product_SKU_spec.aspx?ProductType=MB&pid=642&SKU=600000017", NULL),
	B("Tyan",	"S5382 (Tempest i5000PW)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=439", NULL),
	B("Tyan",	"S5397 (Tempest i5400PW)", OK, "http://www.tyan.com/product_board_detail.aspx?pid=560", NULL),
	B("Tyan",	"S7066 (S7066WGM3NR)",	BAD, "http://www.tyan.com/product_SKU_spec.aspx?ProductType=MB&pid=790&SKU=600000330", "Probing works (Winbond W25Q64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("VIA",	"EITX-3000",		OK, "http://www.viaembedded.com/en/products/boards/810/1/EITX-3000.html", NULL),
	B("VIA",	"EPIA M/MII/...",	OK, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=202", NULL), /* EPIA-MII link for now */
	B("VIA",	"EPIA SP",		OK, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=261", NULL),
	B("VIA",	"EPIA-CN",		OK, "http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=400", NULL),
	B("VIA",	"EPIA EK",		OK, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?motherboard_id=420", NULL),
	B("VIA",	"EPIA-EX15000G",	OK, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=450", NULL),
	B("VIA",	"EPIA-LN",		OK, "http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=473", NULL),
	B("VIA",	"EPIA-M700",		OK, "http://via.com.tw/servlet/downloadSvl?motherboard_id=670&download_file_id=3700", NULL),
	B("VIA",	"EPIA-N/NL",		OK, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=221", NULL), /* EPIA-N link for now */
	B("VIA",	"EPIA-NX15000G",	OK, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=470", NULL),
	B("VIA",	"NAB74X0",		OK, "http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=590", NULL),
	B("VIA",	"pc2500e",		OK, "http://www.via.com.tw/en/initiatives/empowered/pc2500_mainboard/index.jsp", NULL),
	B("VIA",	"PC3500G",		OK, "http://www.via.com.tw/en/initiatives/empowered/pc3500_mainboard/index.jsp", NULL),
	B("VIA",	"VB700X",		OK, "http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=490", NULL),
	B("ZOTAC",	"Fusion-ITX WiFi (FUSION350-A-E)", OK, NULL, NULL),
	B("ZOTAC",	"GeForce 8200",		OK, NULL, NULL),
	B("ZOTAC",	"H61-ITX WiFi (H61ITX-A-E)", BAD, NULL, "Probing works (Winbond W25Q32, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ZOTAC",	"H67-ITX WiFi (H67ITX-C-E)", BAD, NULL, "Probing works (Winbond W25Q32, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("ZOTAC",	"IONITX-A-E",		OK, NULL, NULL),
	B("ZOTAC",	"IONITX-F-E",		OK, NULL, NULL),
	B("ZOTAC",	"nForce 630i Supreme (N73U-Supreme)", OK, NULL, NULL),
	B("ZOTAC",	"ZBOX AD02 (PLUS)",	OK, NULL, NULL),
	B("ZOTAC",	"ZBOX HD-ID11",		OK, NULL, NULL),
#endif

	{0},
};

/* Please keep this list alphabetically ordered by vendor/board. */
const struct board_info laptops_known[] = {
#if defined(__i386__) || defined(__x86_64__)
	B("Acer",	"Aspire 1520",		OK, "http://support.acer.com/us/en/acerpanam/notebook/0000/Acer/Aspire1520/Aspire1520nv.shtml", NULL),
	B("Acer",	"Aspire One",		BAD, NULL, "http://www.coreboot.org/pipermail/coreboot/2009-May/048041.html"),
	B("ASUS",	"A8Jm",			OK, NULL, NULL),
	B("ASUS",	"Eee PC 701 4G",	BAD, "https://www.asus.com/Eee/Eee_PC/Eee_PC_4G/", "It seems the chip (25X40) is behind some SPI flash translation layer (likely in the EC, the ENE KB3310)."),
	B("ASUS",	"M6Ne",			NT, NULL, "Untested board enable."),
	B("ASUS",	"U38N",			OK, NULL, NULL),
	B("Clevo",	"P150HM",		BAD, "http://www.clevo.com.tw/en/products/prodinfo_2.asp?productid=307", "Probing works (Macronix MX25L3205, 4096 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs), ME region is locked."),
	B("Dell",	"Latitude D630",	OK, NULL, NULL),
	B("Dell",	"Inspiron 1420",	OK, NULL, NULL),
	B("Dell",	"Latitude CPi A366XT",	BAD, "http://www.coreboot.org/Dell_Latitude_CPi_A366XT", "The laptop immediately powers off if you try to hot-swap the chip. It's not yet tested if write/erase would work on this laptop."),
	B("Dell",	"Vostro 3700",		BAD, NULL, "Locked ME, see https://flashrom.org/pipermail/flashrom/2012-May/009197.html."),
	B("Dell",	"Latitude E6520",	BAD, NULL, "Locked ME, see https://flashrom.org/pipermail/flashrom/2012-June/009420.html."),
	B("Elitegroup",	"A928",			OK, NULL, "Bootsector is locked and needs to be skipped with a layout file (writeable address range is 00000000:0003bfff)."),
	B("Fujitsu",	"Amilo Xi 3650",	OK, NULL, NULL),
	B("HP/Compaq",	"EliteBook 8560p",	BAD, NULL, "SPI lock down, SMM protection, PR in BIOS region, read-only descriptor, locked ME region."),
	B("HP/Compaq",	"nx9005",		BAD, "http://h18000.www1.hp.com/products/quickspecs/11602_na/11602_na.HTML", "Shuts down when probing for a chip. https://flashrom.org/pipermail/flashrom/2010-May/003321.html"),
	B("HP/Compaq",	"nx9010",		BAD, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?lang=en&cc=us&objectID=c00348514", "Hangs upon '''flashrom -V''' (needs hard power-cycle then)."),
	B("IBM/Lenovo",	"ThinkPad T40p",	BAD, "http://www.thinkwiki.org/wiki/Category:T40p", NULL),
	B("IBM/Lenovo",	"ThinkPad T410s",	BAD, "http://www.thinkwiki.org/wiki/Category:T410s", "Probing works (Winbond W25X64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs) and ME is locked. Also, a Protected Range is locking the top range of the BIOS region (presumably the boot block)."),
	B("IBM/Lenovo",	"ThinkPad T420",	BAD, "http://www.thinkwiki.org/wiki/Category:T420", "Probing works (Macronix MX25L6405, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs) and ME is locked. Also, a Protected Range is locking the top range of the BIOS region (presumably the boot block)."),
	B("IBM/Lenovo",	"ThinkPad X1",		BAD, "http://www.thinkwiki.org/wiki/Category:X1", "Probing works (ST M25PX64, 8192 kB, SPI), but parts of the flash are problematic: descriptor is r/o (conforming to ICH reqs) and ME is locked. Also, a Protected Range is locking the top range of the BIOS region (presumably the boot block)."),
	B("IBM/Lenovo",	"ThinkPad T530",	DEP, "http://www.thinkwiki.org/wiki/Category:T530", "Works fine but only with coreboot (due to locked regions and additional PR restrictions)."),
	B("IBM/Lenovo",	"ThinkPad 240",		BAD, "http://www.stanford.edu/~bresnan//tp240.html", "Seems to (partially) work at first, but one block/sector cannot be written which then leaves you with a bricked laptop. Maybe this can be investigated and fixed in software later."),
	B("IBM/Lenovo",	"3000 V100 TF05Cxx",	OK, "http://www5.pc.ibm.com/europe/products.nsf/products?openagent&brand=Lenovo3000Notebook&series=Lenovo+3000+V+Series#viewallmodelstop", NULL),
	//B("MSI",	"GT60-2OD",		OK, "http://www.msi.com/product/nb/GT60_2OD.html", NULL), requires layout patches
	B("Teclast",	"X98 Air 3G",		OK, NULL, NULL),
#endif

	{0},
};
#endif
