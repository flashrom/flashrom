/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
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
#include "flashchips.h"

/*
 * Return a string corresponding to the bustype parameter.
 * Memory is obtained with malloc() and can be freed with free().
 */
char *flashbuses_to_text(enum chipbustype bustype)
{
	char *ret = calloc(1, 1);
	if (bustype == CHIP_BUSTYPE_UNKNOWN) {
		ret = strcat_realloc(ret, "Unknown,");
	/*
	 * FIXME: Once all chipsets and flash chips have been updated, NONSPI
	 * will cease to exist and should be eliminated here as well.
	 */
	} else if (bustype == CHIP_BUSTYPE_NONSPI) {
		ret = strcat_realloc(ret, "Non-SPI,");
	} else {
		if (bustype & CHIP_BUSTYPE_PARALLEL)
			ret = strcat_realloc(ret, "Parallel,");
		if (bustype & CHIP_BUSTYPE_LPC)
			ret = strcat_realloc(ret, "LPC,");
		if (bustype & CHIP_BUSTYPE_FWH)
			ret = strcat_realloc(ret, "FWH,");
		if (bustype & CHIP_BUSTYPE_SPI)
			ret = strcat_realloc(ret, "SPI,");
		if (bustype == CHIP_BUSTYPE_NONE)
			ret = strcat_realloc(ret, "None,");
	}
	/* Kill last comma. */
	ret[strlen(ret) - 1] = '\0';
	ret = realloc(ret, strlen(ret) + 1);
	return ret;
}

#define POS_PRINT(x) do { pos += strlen(x); printf(x); } while (0)

static int digits(int n)
{
	int i;

	if (!n)
		return 1;

	for (i = 0; n; ++i)
		n /= 10;

	return i;
}

void print_supported_chips(void)
{
	int okcol = 0, pos = 0, i, chipcount = 0;
	struct flashchip *f;

	for (f = flashchips; f->name != NULL; f++) {
		if (GENERIC_DEVICE_ID == f->model_id)
			continue;
		okcol = max(okcol, strlen(f->vendor) + 1 + strlen(f->name));
	}
	okcol = (okcol + 7) & ~7;

	for (f = flashchips; f->name != NULL; f++)
		chipcount++;

	printf("\nSupported flash chips (total: %d):\n\n", chipcount);
	POS_PRINT("Vendor:   Device:");
	while (pos < okcol) {
		printf("\t");
		pos += 8 - (pos % 8);
	}

	printf("Tested OK:\tKnown BAD:  Size/KB:  Type:\n\n");
	printf("(P = PROBE, R = READ, E = ERASE, W = WRITE)\n\n");

	for (f = flashchips; f->name != NULL; f++) {
		/* Don't print "unknown XXXX SPI chip" entries. */
		if (!strncmp(f->name, "unknown", 7))
			continue;

		printf("%s", f->vendor);
		for (i = 0; i < 10 - strlen(f->vendor); i++)
			printf(" ");
		printf("%s", f->name);

		pos = 10 + strlen(f->name);
		while (pos < okcol) {
			printf("\t");
			pos += 8 - (pos % 8);
		}
		if ((f->tested & TEST_OK_MASK)) {
			if ((f->tested & TEST_OK_PROBE))
				POS_PRINT("P ");
			if ((f->tested & TEST_OK_READ))
				POS_PRINT("R ");
			if ((f->tested & TEST_OK_ERASE))
				POS_PRINT("E ");
			if ((f->tested & TEST_OK_WRITE))
				POS_PRINT("W ");
		}
		while (pos < okcol + 9) {
			printf("\t");
			pos += 8 - (pos % 8);
		}
		if ((f->tested & TEST_BAD_MASK)) {
			if ((f->tested & TEST_BAD_PROBE))
				printf("P ");
			if ((f->tested & TEST_BAD_READ))
				printf("R ");
			if ((f->tested & TEST_BAD_ERASE))
				printf("E ");
			if ((f->tested & TEST_BAD_WRITE))
				printf("W ");
		}

		printf("\t    %d", f->total_size);
		for (i = 0; i < 10 - digits(f->total_size); i++)
			printf(" ");
		printf("%s\n", flashbuses_to_text(f->bustype));
	}
}

#if INTERNAL_SUPPORT == 1
void print_supported_chipsets(void)
{
	int i, j, chipsetcount = 0;
	const struct penable *c = chipset_enables;

	for (i = 0; c[i].vendor_name != NULL; i++)
		chipsetcount++;

	printf("\nSupported chipsets (total: %d):\n\nVendor:                  "
	       "Chipset:                 PCI IDs:\n\n", chipsetcount);

	for (i = 0; c[i].vendor_name != NULL; i++) {
		printf("%s", c[i].vendor_name);
		for (j = 0; j < 25 - strlen(c[i].vendor_name); j++)
			printf(" ");
		printf("%s", c[i].device_name);
		for (j = 0; j < 25 - strlen(c[i].device_name); j++)
			printf(" ");
		printf("%04x:%04x%s\n", c[i].vendor_id, c[i].device_id,
		       (c[i].status == OK) ? "" : " (untested)");
	}
}

void print_supported_boards_helper(const struct board_info *b, const char *msg)
{
	int i, j, boardcount = 0;

	for (i = 0; b[i].vendor != NULL; i++)
		boardcount++;

	printf("\n%s (total: %d):\n\n", msg, boardcount);

	for (i = 0; b[i].vendor != NULL; i++) {
		printf("%s", b[i].vendor);
		for (j = 0; j < 25 - strlen(b[i].vendor); j++)
			printf(" ");
		printf("%s", b[i].name);
		for (j = 0; j < 28 - strlen(b[i].name); j++)
			printf(" ");
		printf("\n");
	}
}

void print_supported_boards(void)
{
	int i, j, boardcount = 0;
	struct board_pciid_enable *b = board_pciid_enables;

	for (i = 0; b[i].vendor_name != NULL; i++)
		boardcount++;

	printf("\nSupported boards which need write-enable code (total: %d):"
	       "\n\nVendor:                  Board:                        "
	       "Required option:\n\n", boardcount);

	for (i = 0; b[i].vendor_name != NULL; i++) {
		printf("%s", b[i].vendor_name);
		for (j = 0; j < 25 - strlen(b[i].vendor_name); j++)
			printf(" ");
		printf("%s", b[i].board_name);
		for (j = 0; j < 30 - strlen(b[i].board_name); j++)
			printf(" ");
		if (b[i].lb_vendor != NULL)
			printf("-m %s:%s\n", b[i].lb_vendor, b[i].lb_part);
		else
			printf("(none, board is autodetected)\n");
	}

	print_supported_boards_helper(boards_ok,
		"Supported boards which don't need write-enable code");
	print_supported_boards_helper(boards_bad,
		"Boards which have been verified to NOT work yet");
	print_supported_boards_helper(laptops_ok,
		"Laptops which have been verified to work");
	print_supported_boards_helper(laptops_bad,
		"Laptops which have been verified to NOT work yet");
}
#endif

void print_supported(void)
{
		print_supported_chips();
#if INTERNAL_SUPPORT == 1
		print_supported_chipsets();
		print_supported_boards();
#endif
#if (NIC3COM_SUPPORT == 1) || (GFXNVIDIA_SUPPORT == 1) || (DRKAISER_SUPPORT == 1) || (SATASII_SUPPORT == 1)
		printf("\nSupported PCI devices flashrom can use "
		       "as programmer:\n\n");
#endif
#if NIC3COM_SUPPORT == 1
		print_supported_pcidevs(nics_3com);
#endif
#if GFXNVIDIA_SUPPORT == 1
		print_supported_pcidevs(gfx_nvidia);
#endif
#if DRKAISER_SUPPORT == 1
		print_supported_pcidevs(drkaiser_pcidev);
#endif
#if SATASII_SUPPORT == 1
		print_supported_pcidevs(satas_sii);
#endif
}


#if INTERNAL_SUPPORT == 1
/* Please keep this list alphabetically ordered by vendor/board. */
const struct board_info boards_ok[] = {
	/* Verified working boards that don't need write-enables. */
	{ "Abit",		"AX8", },
	{ "Abit",		"Fatal1ty F-I90HD", },
	{ "Advantech",		"PCM-5820", },
	{ "ASI",		"MB-5BLMP", },
	{ "ASRock",		"A770CrossFire", },
	{ "ASRock",		"K8S8X", },
	{ "ASRock",		"M3A790GXH/128M" },
	{ "ASUS",		"A7N8X Deluxe", },
	{ "ASUS",		"A7N8X-E Deluxe", },
	{ "ASUS",		"A7V400-MX", },
	{ "ASUS",		"A7V8X-MX", },
	{ "ASUS",		"A8N-E", },
	{ "ASUS",		"A8NE-FM/S", },
	{ "ASUS",		"A8N-SLI", },
	{ "ASUS",		"A8N-SLI Premium", },
	{ "ASUS",		"A8V Deluxe", },
	{ "ASUS",		"A8V-E Deluxe", },
	{ "ASUS",		"A8V-E SE", },
	{ "ASUS",		"K8V", },
	{ "ASUS",		"K8V SE Deluxe", },
	{ "ASUS",		"K8V-X SE", },
	{ "ASUS",		"M2A-MX", },
	{ "ASUS",		"M2A-VM", },
	{ "ASUS",		"M2N-E", },
	{ "ASUS",		"M2V", },
	{ "ASUS",		"M3A78-EM", },
	{ "ASUS",		"P2B", },
	{ "ASUS",		"P2B-D", },
	{ "ASUS",		"P2B-DS", },
	{ "ASUS",		"P2B-F", },
	{ "ASUS",		"P2L97-S", },
	{ "ASUS",		"P5B-Deluxe", },
	{ "ASUS",		"P5KC", },
	{ "ASUS",		"P5L-MX", },
	{ "ASUS",		"P6T Deluxe V2", },
	{ "A-Trend",		"ATC-6220", },
	{ "BCOM",		"WinNET100", },
	{ "DFI",		"Blood-Iron P35 T2RL", },
	{ "Elitegroup",		"K7S5A", },
	{ "Elitegroup",		"P6VAP-A+", },
	{ "GIGABYTE",		"GA-6BXC", },
	{ "GIGABYTE",		"GA-6BXDU", },
	{ "GIGABYTE",		"GA-6ZMA", },
	{ "GIGABYTE",		"GA-7ZM", },
	{ "GIGABYTE",		"GA-EP35-DS3L", },
	{ "GIGABYTE",		"GA-EX58-UD4P", },
	{ "GIGABYTE",		"GA-MA78GPM-DS2H", },
	{ "GIGABYTE",		"GA-MA790GP-DS4H", },
	{ "GIGABYTE",		"GA-MA770T-UD3P", },
	{ "Intel",		"EP80759", },
	{ "Jetway",		"J7F4K1G5D-PB", },
	{ "MSI",                "MS-6153", },
	{ "MSI",                "MS-6156", },
	{ "MSI",		"MS-6570 (K7N2)", },
	{ "MSI",		"MS-7065", },
	{ "MSI",		"MS-7168 (Orion)", },
	{ "MSI",		"MS-7236 (945PL Neo3)", },
	{ "MSI",		"MS-7255 (P4M890M)", },
	{ "MSI",		"MS-7345 (P35 Neo2-FIR)", },
	{ "MSI",		"MS-7368 (K9AG Neo2-Digital)", },
	{ "NEC",		"PowerMate 2000", },
	{ "PC Engines",		"Alix.1c", },
	{ "PC Engines",		"Alix.2c2", },
	{ "PC Engines",		"Alix.2c3", },
	{ "PC Engines",		"Alix.3c3", },
	{ "PC Engines",		"Alix.3d3", },
	{ "RCA",		"RM4100", },
	{ "Sun",		"Blade x6250", },
	{ "Supermicro",		"H8QC8", },
	{ "Thomson",		"IP1000", },
	{ "TriGem",		"Lomita", },
	{ "T-Online",		"S-100", },
	{ "Tyan",		"iS5375-1U", },
	{ "Tyan",		"S1846", },
	{ "Tyan",		"S2466", },
	{ "Tyan",		"S2881", },
	{ "Tyan",		"S2882", },
	{ "Tyan",		"S2882-D", },
	{ "Tyan",		"S2891", },
	{ "Tyan",		"S2892", },
	{ "Tyan",		"S2895", },
	{ "Tyan",		"S3095", },
	{ "Tyan",		"S5180", },
	{ "Tyan",		"S5191", },
	{ "Tyan",		"S5197", },
	{ "Tyan",		"S5211", },
	{ "Tyan",		"S5211-1U", },
	{ "Tyan",		"S5220", },
	{ "Tyan",		"S5375", },
	{ "Tyan",		"S5376G2NR/S5376WAG2NR", },
	{ "Tyan",		"S5377", },
	{ "Tyan",		"S5397", },
	{ "VIA",		"EPIA-CN", },
	{ "VIA",		"EPIA-EX15000G", },
	{ "VIA",		"EPIA-LN", },
	{ "VIA",		"EPIA-M700", },
	{ "VIA",		"EPIA-NX15000G", },
	{ "VIA",		"EPIA-SP", },
	{ "VIA",		"NAB74X0", },
	{ "VIA",		"pc2500e", },
	{ "VIA",		"VB700X", },

	{},
};

/* Please keep this list alphabetically ordered by vendor/board. */
const struct board_info boards_bad[] = {
	/* Verified non-working boards (for now). */
	{ "Abit",		"IS-10", },
	{ "ASRock",		"K7VT4A+", },
	{ "ASUS",		"MEW-AM", },
	{ "ASUS",		"MEW-VM", },
	{ "ASUS",		"P3B-F", },
	{ "ASUS",		"P5B", },
	{ "ASUS",		"P5BV-M", },
	{ "Biostar",		"M6TBA", },
	{ "Boser",		"HS-6637", },
	{ "DFI",		"855GME-MGF", },
	{ "FIC",		"VA-502", },
	{ "MSI",		"MS-6178", },
	{ "MSI",		"MS-7260 (K9N Neo)", },
	{ "Soyo",		"SY-5VD", },
	{ "Sun",		"Fire x4150", },
	{ "Sun",		"Fire x4200", },
	{ "Sun",		"Fire x4540", },
	{ "Sun",		"Fire x4600", },

	{},
};

/* Please keep this list alphabetically ordered by vendor/board. */
const struct board_info laptops_ok[] = {
	/* Verified working laptops. */
	{ "Lenovo",		"3000 V100 TF05Cxx", },

	{},
};

/* Please keep this list alphabetically ordered by vendor/board. */
const struct board_info laptops_bad[] = {
	/* Verified non-working laptops (for now). */
	{ "Acer",		"Aspire One", },
	{ "ASUS",		"Eee PC 701 4G", },
	{ "Dell",		"Latitude CPi A366XT", },
	{ "HP/Compaq",		"nx9010", },
	{ "IBM/Lenovo",		"Thinkpad T40p", },
	{ "IBM/Lenovo",		"240", },

	{},
};
#endif

