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

#include <stdio.h>
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

#if CONFIG_INTERNAL == 1
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

void print_supported_boards_helper(const struct board_info *boards,
				   const char *devicetype)
{
	int i, j, boardcount_good = 0, boardcount_bad = 0;
	struct board_pciid_enable *b = board_pciid_enables;

	for (i = 0; boards[i].vendor != NULL; i++) {
		if (boards[i].working)
			boardcount_good++;
		else
			boardcount_bad++;
	}

	printf("\nKnown %s (good: %d, bad: %d):"
	       "\n\nVendor:                  Board:                      "
	       "Status: Required option:"
	       "\n\n", devicetype, boardcount_good, boardcount_bad);

	for (i = 0; boards[i].vendor != NULL; i++) {
		printf("%s", boards[i].vendor);
		for (j = 0; j < 25 - strlen(boards[i].vendor); j++)
			printf(" ");
		printf("%s", boards[i].name);
		for (j = 0; j < 28 - strlen(boards[i].name); j++)
			printf(" ");
		printf((boards[i].working) ? "OK      " : "BAD     ");

		for (j = 0; b[j].vendor_name != NULL; j++) {
			if (strcmp(b[j].vendor_name, boards[i].vendor)
			    || strcmp(b[j].board_name, boards[i].name))
				continue;
			if (b[j].lb_vendor == NULL)
				printf("(autodetected)");
			else
				printf("-m %s:%s", b[j].lb_vendor,
						   b[j].lb_part);
		}
		printf("\n");
	}
}
#endif

void print_supported(void)
{
		print_supported_chips();
#if CONFIG_INTERNAL == 1
		print_supported_chipsets();
		print_supported_boards_helper(boards_known, "boards");
		print_supported_boards_helper(laptops_known, "laptops");
#endif
#if CONFIG_NIC3COM+CONFIG_NICREALTEK+CONFIG_GFXNVIDIA+CONFIG_DRKAISER+CONFIG_SATASII+CONFIG_ATAHPT >= 1
		printf("\nSupported PCI devices flashrom can use "
		       "as programmer:\n\n");
#endif
#if CONFIG_NIC3COM == 1
		print_supported_pcidevs(nics_3com);
#endif
#if CONFIG_NICREALTEK == 1
		print_supported_pcidevs(nics_realtek);
		print_supported_pcidevs(nics_realteksmc1211);
#endif
#if CONFIG_GFXNVIDIA == 1
		print_supported_pcidevs(gfx_nvidia);
#endif
#if CONFIG_DRKAISER == 1
		print_supported_pcidevs(drkaiser_pcidev);
#endif
#if CONFIG_SATASII == 1
		print_supported_pcidevs(satas_sii);
#endif
#if CONFIG_ATAHPT == 1
		print_supported_pcidevs(ata_hpt);
#endif
}

#if CONFIG_INTERNAL == 1
/* Please keep this list alphabetically ordered by vendor/board. */
const struct board_info boards_known[] = {
#if defined(__i386__) || defined(__x86_64__)
	B("A-Trend",	"ATC-6220",		1, "http://www.motherboard.cz/mb/atrend/atc6220.htm", NULL),
	B("Abit",	"AX8",			1, "http://www.abit.com.tw/page/en/motherboard/motherboard_detail.php?DEFTITLE=Y&fMTYPE=Socket%20939&pMODEL_NAME=AX8", NULL),
	B("Abit",	"Fatal1ty F-I90HD",	1, "http://www.abit.com.tw/page/de/motherboard/motherboard_detail.php?pMODEL_NAME=Fatal1ty+F-I90HD&fMTYPE=LGA775", NULL),
	B("Abit",	"IP35",			1, "http://www.abit.com.tw/page/en/motherboard/motherboard_detail.php?fMTYPE=LGA775&pMODEL_NAME=IP35", NULL),
	B("Abit",	"IP35 Pro",		1, "http://www.abit.com.tw/page/de/motherboard/motherboard_detail.php?fMTYPE=LGA775&pMODEL_NAME=IP35%20Pro", NULL),
	B("Abit",	"IS-10",		0, "http://www.abit.com.tw/page/en/motherboard/motherboard_detail.php?pMODEL_NAME=IS-10&fMTYPE=Socket+478", NULL),
	B("Abit",	"NF7-S",		1, "http://www.abit.com.tw/page/en/motherboard/motherboard_detail.php?fMTYPE=Socket%20A&pMODEL_NAME=NF7-S", NULL),
	B("Abit",	"VT6X4",		1, "http://www.abit.com.tw/page/en/motherboard/motherboard_detail.php?fMTYPE=Slot%201&pMODEL_NAME=VT6X4", NULL),
	B("Acorp",	"6A815EPD",		1, "http://web.archive.org/web/20021206163652/www.acorp.com.tw/English/default.asp", NULL),
	B("Advantech",	"PCM-5820",		1, "http://www.emacinc.com/sbc_pc_compatible/pcm_5820.htm", NULL),
	B("agami",	"Aruma",		1, "http://web.archive.org/web/20080212111524/http://www.agami.com/site/ais-6000-series", NULL),
	B("Albatron",	"PM266A Pro",		1, "http://www.albatron.com.tw/English/Product/MB/pro_detail.asp?rlink=Overview&no=56", NULL), /* FIXME */
	B("AOpen",	"vKM400Am-S",		1, "http://usa.aopen.com/products_detail.aspx?Auno=824", NULL),
	B("Artec Group","DBE61",		1, "http://wiki.thincan.org/DBE61", NULL),
	B("Artec Group","DBE62",		1, "http://wiki.thincan.org/DBE62", NULL),
	B("ASI",	"MB-5BLMP",		1, "http://www.hojerteknik.com/winnet.htm", "Used in the IGEL WinNET III thin client."),
	B("ASRock",	"A770CrossFire",	1, "http://www.asrock.com/mb/overview.asp?Model=A770CrossFire&s=AM2\%2b", NULL),
	B("ASRock",	"K7VT4A+",		0, "http://www.asrock.com/mb/overview.asp?Model=K7VT4A%%2b&s=", NULL),
	B("ASRock",	"K8S8X",		1, "http://www.asrock.com/mb/overview.asp?Model=K8S8X", "The Super I/O isn't found on this board. See http://www.flashrom.org/pipermail/flashrom/2009-November/000937.html."),
	B("ASRock",	"P4i65GV",		1, NULL, NULL),
	B("ASRock",	"M3A790GXH/128M",	1, "http://www.asrock.com/MB/overview.asp?Model=M3A790GXH/128M", NULL),
	B("ASUS",	"A7N8X Deluxe",		1, "http://www.asus.com/product.aspx?P_ID=wAsRYm41KTp78MFC", NULL),
	B("ASUS",	"A7N8X-E Deluxe",	1, "http://www.asus.com/product.aspx?P_ID=TmQtPJv4jIxmL9C2", NULL),
	B("ASUS",	"A7V133",		1, "ftp://ftp.asus.com.tw/pub/ASUS/mb/socka/kt133a/a7v133/", NULL),
	B("ASUS",	"A7V400-MX",		1, "http://www.asus.com/product.aspx?P_ID=hORgEHRBDLMfwAwx", NULL),
	B("ASUS",	"A7V600-X",		1, "http://www.asus.com/product.aspx?P_ID=L2XYS0rmtCjeOr4k", NULL),
	B("ASUS",	"A7V8X",		1, "http://www.asus.com/product.aspx?P_ID=qfpaGrAy2kLVo0f2", NULL),
	B("ASUS",	"A7V8X-MX",		1, "http://www.asus.com/product.aspx?P_ID=SEJOOYqfuQPitx2H", NULL),
	B("ASUS",	"A7V8X-MX SE",		1, "http://www.asus.com/product.aspx?P_ID=1guVBT1qV5oqhHyZ", NULL),
	B("ASUS",	"A7V8X-X",		1, "http://www.asus.com/product.aspx?P_ID=YcXfRrWHZ9RKoVmw", NULL),
	B("ASUS",	"A8N-E",		1, "http://www.asus.com/product.aspx?P_ID=DzbA8hgqchMBOVRz", NULL),
	B("ASUS",	"A8N-SLI",		1, "http://www.asus.com/product.aspx?P_ID=J9FKa8z2xVId3pDK", NULL),
	B("ASUS",	"A8N-SLI Premium",	1, "http://www.asus.com/product.aspx?P_ID=nbulqxniNmzf0mH1", NULL),
	B("ASUS",	"A8NE-FM/S",		1, "http://www.hardwareschotte.de/hardware/preise/proid_1266090/preis_ASUS+A8NE-FM", NULL),
	B("ASUS",	"A8V Deluxe",		1, "http://www.asus.com/product.aspx?P_ID=tvpdgPNCPaABZRVU", NULL),
	B("ASUS",	"A8V-E Deluxe",		1, "http://www.asus.com/product.aspx?P_ID=hQBPIJWEZnnGAZEh", NULL),
	B("ASUS",	"A8V-E SE",		1, "http://www.asus.com/product.aspx?P_ID=VMfiJJRYTHM4gXIi", "See http://www.coreboot.org/pipermail/coreboot/2007-October/026496.html."),
	B("ASUS",	"K8V",			1, "http://www.asus.com/product.aspx?P_ID=fG2KZOWF7v6MRFRm", NULL),
	B("ASUS",	"K8V SE Deluxe",	1, "http://www.asus.com/product.aspx?P_ID=65HeDI8XM1u6Uy6o", NULL),
	B("ASUS",	"K8V-X SE",		1, "http://www.asus.com/product.aspx?P_ID=lzDXlbBVHkdckHVr", NULL),
	B("ASUS",	"M2A-MX",		1, "http://www.asus.com/product.aspx?P_ID=BmaOnPewi1JgltOZ", NULL),
	B("ASUS",	"M2A-VM",		1, "http://www.asus.com/product.aspx?P_ID=St3pWpym8xXpROQS", "See http://www.coreboot.org/pipermail/coreboot/2007-September/025281.html."),
	B("ASUS",	"M2N-E",		1, "http://www.asus.com/product.aspx?P_ID=NFlvt10av3F7ayQ9", "If the machine doesn't come up again after flashing, try resetting the NVRAM(CMOS). The MAC address of the onboard network card will change to the value stored in the new image, so backup the old address first. See http://www.flashrom.org/pipermail/flashrom/2009-November/000879.html"),
	B("ASUS",	"M2NBP-VM CSM",		1, "http://www.asus.com/product.aspx?P_ID=MnOfzTGd2KkwG0rF", NULL),
	B("ASUS",	"M2V",			1, "http://www.asus.com/product.aspx?P_ID=OqYlEDFfF6ZqZGvp", NULL),
	B("ASUS",	"M2V-MX",		1, "http://www.asus.com/product.aspx?P_ID=7grf8Ci4yxnqzt3z", NULL),
	B("ASUS",	"M3A78-EM",		1, "http://www.asus.com/product.aspx?P_ID=KjpYqzmAd9vsTM2D", NULL),
	B("ASUS",	"MEW-AM",		0, "ftp://ftp.asus.com.tw/pub/ASUS/mb/sock370/810/mew-am/", NULL),
	B("ASUS",	"MEW-VM",		0, "http://www.elhvb.com/mboards/OEM/HP/manual/ASUS%20MEW-VM.htm", NULL),
	B("ASUS",	"P2B",			1, "ftp://ftp.asus.com.tw/pub/ASUS/mb/slot1/440bx/p2b/", NULL),
	B("ASUS",	"P2B-D",		1, "ftp://ftp.asus.com.tw/pub/ASUS/mb/slot1/440bx/p2b-d/", NULL),
	B("ASUS",	"P2B-DS",		1, "ftp://ftp.asus.com.tw/pub/ASUS/mb/slot1/440bx/p2b-ds/", NULL),
	B("ASUS",	"P2B-F",		1, "ftp://ftp.asus.com.tw/pub/ASUS/mb/slot1/440bx/p2b-d/", NULL),
	B("ASUS",	"P2L97-S",		1, "ftp://ftp.asus.com.tw/pub/ASUS/mb/slot1/440lx/p2l97-s/", NULL),
	B("ASUS",	"P3B-F",		0, "ftp://ftp.asus.com.tw/pub/ASUS/mb/slot1/440bx/p3b-f/", NULL),
	B("ASUS",	"P4B266",		1, "ftp://ftp.asus.com.tw/pub/ASUS/mb/sock478/p4b266/", NULL),
	B("ASUS",	"P4B266-LM",		1, "http://esupport.sony.com/US/perl/swu-list.pl?mdl=PCVRX650", NULL),
	B("ASUS",	"P4C800-E Deluxe",	1, "http://www.asus.com/product.aspx?P_ID=cFuVCr9bXXCckmcK", NULL),
	B("ASUS",	"P4P800-E Deluxe",	1, "http://www.asus.com/product.aspx?P_ID=INIJUvLlif7LHp3g", NULL),
	B("ASUS",	"P5A",			1, "ftp://ftp.asus.com.tw/pub/ASUS/mb/sock7/ali/p5a/", NULL),
	B("ASUS",	"P5B",			1, "ftp://ftp.asus.com.tw/pub/ASUS/mb/socket775/P5B/", NULL),
	B("ASUS",	"P5B-Deluxe",		1, "http://www.asus.com/product.aspx?P_ID=bswT66IBSb2rEWNa", NULL),
	B("ASUS",	"P5BV-M",		0, "ftp://ftp.asus.com.tw/pub/ASUS/mb/socket775/P5B-VM/", NULL),
	B("ASUS",	"P5KC",			1, "http://www.asus.com/product.aspx?P_ID=fFZ8oUIGmLpwNMjj", NULL),
	B("ASUS",	"P5L-MX",		1, "http://www.asus.com/product.aspx?P_ID=X70d3NCzH2DE9vWH", NULL),
	B("ASUS",	"P5ND2-SLI Deluxe",	1, "http://www.asus.com/product.aspx?P_ID=WY7XroDuUImVbgp5", NULL),
	B("ASUS",	"P6T Deluxe",		1, "http://www.asus.com/product.aspx?P_ID=vXixf82co6Q5v0BZ", NULL),
	B("ASUS",	"P6T Deluxe V2",	1, "http://www.asus.com/product.aspx?P_ID=iRlP8RG9han6saZx", NULL),
	B("BCOM",	"WinNET100",		1, "http://www.coreboot.org/BCOM_WINNET100", "Used in the IGEL-316 thin client."),
	B("Biostar",	"M6TBA",		0, "ftp://ftp.biostar-usa.com/manuals/M6TBA/", NULL),
	B("Biostar",	"P4M80-M4",		1, "http://www.biostar-usa.com/mbdetails.asp?model=p4m80-m4", NULL),
	B("Boser",	"HS-6637",		0, "http://www.boser.com.tw/manual/HS-62376637v3.4.pdf", NULL),
	B("Dell",	"PowerEdge 1850",	1, "http://support.dell.com/support/edocs/systems/pe1850/en/index.htm", NULL),
	B("DFI",	"855GME-MGF",		0, "http://www.dfi.com.tw/portal/CM/cmproduct/XX_cmproddetail/XX_WbProdsWindow?action=e&downloadType=&windowstate=normal&mode=view&downloadFlag=false&itemId=433", NULL),
	B("DFI",	"Blood-Iron P35 T2RL",	1, "http://lp.lanparty.com.tw/portal/CM/cmproduct/XX_cmproddetail/XX_WbProdsWindow?itemId=516&downloadFlag=false&action=1", NULL),
	B("Elitegroup",	"K7S5A",		1, "http://www.ecs.com.tw/ECSWebSite/Products/ProductsDetail.aspx?detailid=279&CategoryID=1&DetailName=Specification&MenuID=1&LanID=0", NULL),
	B("Elitegroup",	"K7S6A",		1, "http://www.ecs.com.tw/ECSWebSite/Products/ProductsDetail.aspx?detailid=77&CategoryID=1&DetailName=Specification&MenuID=52&LanID=0", NULL),
	B("Elitegroup",	"K7VTA3",		1, "http://www.ecs.com.tw/ECSWebSite/Products/ProductsDetail.aspx?detailid=264&CategoryID=1&DetailName=Specification&MenuID=52&LanID=0", NULL),
	B("Elitegroup",	"P6VAP-A+",		1, "http://www.ecs.com.tw/ECSWebSite/Products/ProductsDetail.aspx?detailid=117&CategoryID=1&DetailName=Specification&MenuID=1&LanID=0", NULL),
	B("EPoX",	"EP-8K5A2",		1, "http://www.epox.com/product.asp?ID=EP-8K5A2", NULL),
	B("EPoX",	"EP-8RDA3+",		1, "http://www.epox.com/product.asp?ID=EP-8RDA3plus", NULL),
	B("EPoX",	"EP-BX3",		1, "http://www.epox.com/product.asp?ID=EP-BX3", NULL),
	B("FIC",	"VA-502",		0, "ftp://ftp.fic.com.tw/motherboard/manual/socket7/va-502/", NULL),
	B("GIGABYTE",	"GA-2761GXDK",		1, "http://www.computerbase.de/news/hardware/mainboards/amd-systeme/2007/mai/gigabyte_dtx-mainboard/", NULL),
	B("GIGABYTE",	"GA-6BXC",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=1445", NULL),
	B("GIGABYTE",	"GA-6BXDU",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=1429", NULL),
	B("GIGABYTE",	"GA-6ZMA",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=1541", NULL),
	B("GIGABYTE",	"GA-7VT600",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=1666", NULL),
	B("GIGABYTE",	"GA-7ZM",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=1366", "Works fine if you remove jumper JP9 on the board and disable the flash protection BIOS option."),
	B("GIGABYTE",	"GA-965P-DS4",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2288", NULL),
	B("GIGABYTE",	"GA-EP35-DS3L",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2778", NULL),
	B("GIGABYTE",	"GA-EX58-UD4P",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2986", NULL),
	B("GIGABYTE",	"GA-K8N-SLI",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=1928", NULL),
	B("GIGABYTE",	"GA-M57SLI-S4",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2287", NULL),
	B("GIGABYTE",	"GA-M61P-S3",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2434", NULL),
	B("GIGABYTE",	"GA-MA69VM-S2",		1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2500", NULL),
	B("GIGABYTE",	"GA-MA770T-UD3P",	1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=3096", NULL),
	B("GIGABYTE",	"GA-MA78G-DS3H",	1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2800", NULL), /* TODO: Rev 1.x or 2.x? */
	B("GIGABYTE",	"GA-MA78GM-S2H",	1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2758", NULL), /* TODO: Rev. 1.0, 1.1, or 2.x? */
	B("GIGABYTE",	"GA-MA78GPM-DS2H",	1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2859", NULL),
	B("GIGABYTE",	"GA-MA790FX-DQ6",	1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2690", NULL),
	B("GIGABYTE",	"GA-MA790GP-DS4H",	1, "http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=2887", NULL),
	B("HP",		"DL145 G3",		1, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?objectID=c00816835&lang=en&cc=us&taskId=101&prodSeriesId=3219755&prodTypeId=15351", NULL),
	B("HP",		"Vectra VL400 PC",	1, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?objectID=c00060658&lang=en&cc=us", NULL),
	B("HP",		"Vectra VL420 SFF PC",	1, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?objectID=c00060661&lang=en&cc=us", NULL),
	B("HP",		"xw9400",		1, "http://h20000.www2.hp.com/bizsupport/TechSupport/Home.jsp?lang=en&cc=us&prodSeriesId=3211286&prodTypeId=12454", "Boot block is write protected unless the solder points next to F2 are shorted." ),
	B("IBM",	"x3455",		1, "http://www-03.ibm.com/systems/x/hardware/rack/x3455/index.html", NULL),
	B("Intel",	"D201GLY",		1, "http://www.intel.com/support/motherboards/desktop/d201gly/index.htm", NULL),
	B("Intel",	"EP80759",		1, NULL, NULL),
	B("IWILL",	"DK8-HTX",		1, "http://web.archive.org/web/20060507170150/http://www.iwill.net/product_2.asp?p_id=98", NULL),
	B("Jetway",	"J7F4K1G5D-PB",		1, "http://www.jetway.com.tw/jetway/system/productshow2.asp?id=389&proname=J7F4K1G5D-P", NULL),
	B("Kontron",	"986LCD-M",		1, "http://de.kontron.com/products/boards+and+mezzanines/embedded+motherboards/miniitx+motherboards/986lcdmmitx.html", NULL),
	B("Mitac",	"6513WU",		1, "http://web.archive.org/web/20050313054828/http://www.mitac.com/micweb/products/tyan/6513wu/6513wu.htm", NULL),
	B("MSI",	"MS-6153",		1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=336", NULL),
	B("MSI",	"MS-6156",		1, "http://uk.ts.fujitsu.com/rl/servicesupport/techsupport/boards/Motherboards/MicroStar/Ms6156/MS6156.htm", NULL),
	B("MSI",	"MS-6178",		0, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=343", "Immediately powers off if you try to hot-plug the chip. However, this does '''not''' happen if you use coreboot."),
	B("MSI",	"MS-6330 (K7T Turbo)",	1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=327", NULL),
	B("MSI",	"MS-6570 (K7N2)",	1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=519", NULL),
	B("MSI",	"MS-6590 (KT4 Ultra)",	1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=502", NULL),
	B("MSI",	"MS-6702E (K8T Neo2-F)",1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=588", NULL),
	B("MSI",	"MS-6712 (KT4V)",	1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=505", NULL),
	B("MSI",	"MS-7005 (651M-L)",	1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=559", NULL),
	B("MSI",	"MS-7046",		1, "http://www.heimir.de/ms7046/", NULL),
	B("MSI",	"MS-7065",		1, "http://browse.geekbench.ca/geekbench2/view/53114", NULL),
	B("MSI",	"MS-7135 (K8N Neo3)",	1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=170", NULL),
	B("MSI",	"MS-7168 (Orion)",	1, "http://support.packardbell.co.uk/uk/item/index.php?i=spec_orion&pi=platform_honeymoon_istart", NULL),
	B("MSI",	"MS-7236 (945PL Neo3)",	1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=1173", NULL),
	B("MSI",	"MS-7255 (P4M890M)",	1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=1082", NULL),
	B("MSI",	"MS-7260, (K9N Neo)",	0, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=255", "Interestingly flashrom does not work when the vendor BIOS is booted, but it ''does'' work flawlessly when the machine is booted with coreboot."),
	B("MSI",	"MS-7312 (K9MM-V)",	1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=1104", NULL),
	B("MSI",	"MS-7345 (P35 Neo2-FIR)",1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=1261", NULL),
	B("MSI",	"MS-7368 (K9AG Neo2-Digital)",1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=1241", NULL),
	B("MSI",	"MS-7376 (K9A2 Platinum)",1, "http://www.msi.com/index.php?func=proddesc&maincat_no=1&prod_no=1332", NULL),
	B("NEC",	"PowerMate 2000",	1, "http://support.necam.com/mobilesolutions/hardware/Desktops/pm2000/celeron/", NULL),
	B("Nokia",	"IP530",		1, NULL, NULL),
	B("PC Engines",	"Alix.1c",		1, "http://pcengines.ch/alix1c.htm", NULL),
	B("PC Engines",	"Alix.2c2",		1, "http://pcengines.ch/alix2c2.htm", NULL),
	B("PC Engines",	"Alix.2c3",		1, "http://pcengines.ch/alix2c3.htm", NULL),
	B("PC Engines",	"Alix.3c3",		1, "http://pcengines.ch/alix3c3.htm", NULL),
	B("PC Engines",	"Alix.3d3",		1, "http://pcengines.ch/alix3d3.htm", NULL),
	B("PC Engines",	"WRAP.2E",		1, "http://pcengines.ch/wrap2e1.htm", NULL),
	B("RCA",	"RM4100",		1, "http://www.settoplinux.org/index.php?title=RCA_RM4100", NULL),
	B("Shuttle",	"AK31",			1, "http://www.motherboard.cz/mb/shuttle/AK31.htm", NULL),
	B("Shuttle",	"AK38N",		1, "http://eu.shuttle.com/en/desktopdefault.aspx/tabid-36/558_read-9889/", NULL),
	B("Shuttle",	"FD37",			1, "http://www.shuttle.eu/products/discontinued/barebones/sd37p2/", NULL),
	B("Shuttle",	"FN25",			1, "http://www.shuttle.eu/products/discontinued/barebones/sn25p/?0=", NULL),
	B("Soyo",	"SY-5VD",		0, "http://www.soyo.com/content/Downloads/163/&c=80&p=464&l=English", NULL),
	B("Soyo",	"SY-7VCA",		1, "http://www.tomshardware.com/reviews/12-socket-370-motherboards,196-15.html", NULL),
	B("Sun",	"Blade x6250",		1, "http://www.sun.com/servers/blades/x6250/", NULL),
	B("Sun",	"Fire x4150",		0, "http://www.sun.com/servers/x64/x4150/", NULL),
	B("Sun",	"Fire x4200",		0, "http://www.sun.com/servers/entry/x4200/", NULL),
	B("Sun",	"Fire x4540",		0, "http://www.sun.com/servers/x64/x4540/", NULL),
	B("Sun",	"Fire x4600",		0, "http://www.sun.com/servers/x64/x4600/", NULL),
	B("Supermicro",	"H8QC8",		1, "http://www.supermicro.com/Aplus/motherboard/Opteron/nforce/H8QC8.cfm", NULL),
	B("Supermicro",	"X8DTT-F",		1, "http://www.supermicro.com/products/motherboard/QPI/5500/X8DTT-F.cfm", NULL),
	B("T-Online",	"S-100",		1, "http://wiki.freifunk-hannover.de/T-Online_S_100", NULL),
	B("Tekram",	"P6Pro-A5",		1, "http://www.motherboard.cz/mb/tekram/P6Pro-A5.htm", NULL),
	B("Termtek",	"TK-3370 (Rev:2.5B)",	1, NULL, NULL),
	B("Thomson",	"IP1000",		1, "http://www.settoplinux.org/index.php?title=Thomson_IP1000", NULL),
	B("TriGem",	"Lomita",		1, "http://www.e4allupgraders.info/dir1/motherboards/socket370/lomita.shtml", NULL),
	B("Tyan",	"iS5375-1U",		1, "http://www.tyan.com/product_board_detail.aspx?pid=610", NULL),
	B("Tyan",	"S1846",		1, "http://www.tyan.com/archive/products/html/tsunamiatx.html", NULL),
	B("Tyan",	"S2466",		1, "http://www.tyan.com/product_board_detail.aspx?pid=461", NULL),
	B("Tyan",	"S2498 (Tomcat K7M)",	1, "http://www.tyan.com/archive/products/html/tomcatk7m.html", NULL),
	B("Tyan",	"S2881",		1, "http://www.tyan.com/product_board_detail.aspx?pid=115", NULL),
	B("Tyan",	"S2882",		1, "http://www.tyan.com/product_board_detail.aspx?pid=121", NULL),
	B("Tyan",	"S2882-D",		1, "http://www.tyan.com/product_board_detail.aspx?pid=127", NULL),
	B("Tyan",	"S2891",		1, "http://www.tyan.com/product_board_detail.aspx?pid=144", NULL),
	B("Tyan",	"S2892",		1, "http://www.tyan.com/product_board_detail.aspx?pid=145", NULL),
	B("Tyan",	"S2895",		1, "http://www.tyan.com/archive/products/html/thunderk8we.html", NULL),
	B("Tyan",	"S3095",		1, "http://www.tyan.com/product_board_detail.aspx?pid=181", NULL),
	B("Tyan",	"S5180",		1, "http://www.tyan.com/product_board_detail.aspx?pid=456", NULL),
	B("Tyan",	"S5191",		1, "http://www.tyan.com/product_board_detail.aspx?pid=343", NULL),
	B("Tyan",	"S5197",		1, "http://www.tyan.com/product_board_detail.aspx?pid=349", NULL),
	B("Tyan",	"S5211",		1, "http://www.tyan.com/product_board_detail.aspx?pid=591", NULL),
	B("Tyan",	"S5211-1U",		1, "http://www.tyan.com/product_board_detail.aspx?pid=593", NULL),
	B("Tyan",	"S5220",		1, "http://www.tyan.com/product_board_detail.aspx?pid=597", NULL),
	B("Tyan",	"S5375",		1, "http://www.tyan.com/product_board_detail.aspx?pid=566", NULL),
	B("Tyan",	"S5376G2NR/S5376WAG2NR",1, "http://www.tyan.com/product_board_detail.aspx?pid=605", NULL),
	B("Tyan",	"S5377",		1, "http://www.tyan.com/product_SKU_spec.aspx?ProductType=MB&pid=642&SKU=600000017", NULL),
	B("Tyan",	"S5382 (Tempest i5000PW)",1, "http://www.tyan.com/product_board_detail.aspx?pid=439", NULL),
	B("Tyan",	"S5397",		1, "http://www.tyan.com/product_board_detail.aspx?pid=560", NULL),
	B("VIA",	"EPIA M/MII/...",	1, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=202", NULL), /* EPIA-MII link for now */
	B("VIA",	"EPIA SP",		1, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=261", NULL),
	B("VIA",	"EPIA-CN",		1, "http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=400", NULL),
	B("VIA",	"EPIA-EX15000G",	1, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=450", NULL),
	B("VIA",	"EPIA-LN",		1, "http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=473", NULL),
	B("VIA",	"EPIA-M700",		1, "http://via.com.tw/servlet/downloadSvl?motherboard_id=670&download_file_id=3700", NULL),
	B("VIA",	"EPIA-N/NL",		1, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=221", NULL), /* EPIA-N link for now */
	B("VIA",	"EPIA-NX15000G",	1, "http://www.via.com.tw/en/products/embedded/ProductDetail.jsp?productLine=1&motherboard_id=470", NULL),
	B("VIA",	"NAB74X0",		1, "http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=590", NULL),
	B("VIA",	"pc2500e",		1, "http://www.via.com.tw/en/initiatives/empowered/pc2500_mainboard/index.jsp", NULL),
	B("VIA",	"PC3500G",		1, "http://www.via.com.tw/en/initiatives/empowered/pc3500_mainboard/index.jsp", NULL),
	B("VIA",	"VB700X",		1, "http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=490", NULL),
#endif

	{},
};

/* Please keep this list alphabetically ordered by vendor/board. */
const struct board_info laptops_known[] = {
#if defined(__i386__) || defined(__x86_64__)
	B("Acer",	"Aspire 1520",	1, "http://support.acer.com/us/en/acerpanam/notebook/0000/Acer/Aspire1520/Aspire1520nv.shtml", NULL),
	B("Acer",	"Aspire One",	0, NULL, "http://www.coreboot.org/pipermail/coreboot/2009-May/048041.html"),
	B("ASUS",	"Eee PC 701 4G",0, "http://www.asus.com/product.aspx?P_ID=h6SPd3tEzLEsrEiS", "It seems the chip (25X40VSIG) is behind some SPI flash translation layer (likely in the EC, the ENE KB3310)."),
	B("Dell",	"Latitude CPi A366XT",0, "http://www.coreboot.org/Dell_Latitude_CPi_A366XT", "The laptop immediately powers off if you try to hot-swap the chip. It's not yet tested if write/erase would work on this laptop."),
	B("HP/Compaq",	"nx9010",	0, "http://h20000.www2.hp.com/bizsupport/TechSupport/Document.jsp?lang=en&cc=us&objectID=c00348514", "Hangs upon '''flashrom -V''' (needs hard power-cycle then)."),
	B("IBM/Lenovo",	"Thinkpad T40p",0, "http://www.thinkwiki.org/wiki/Category:T40p", NULL),
	B("IBM/Lenovo",	"240",		0, "http://www.stanford.edu/~bresnan//tp240.html", "Seems to (partially) work at first, but one block/sector cannot be written which then leaves you with a bricked laptop. Maybe this can be investigated and fixed in software later."),
	B("Lenovo",	"3000 V100 TF05Cxx",1, "http://www5.pc.ibm.com/europe/products.nsf/products?openagent&brand=Lenovo3000Notebook&series=Lenovo+3000+V+Series#viewallmodelstop", NULL),
#endif

	{},
};
#endif
