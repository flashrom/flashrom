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
		for (j = 0; j < 23 - strlen(b[i].name); j++)
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
	       "\n\nVendor:                  Board:                   "
	       "Required option:\n\n", boardcount);

	for (i = 0; b[i].vendor_name != NULL; i++) {
		printf("%s", b[i].vendor_name);
		for (j = 0; j < 25 - strlen(b[i].vendor_name); j++)
			printf(" ");
		printf("%s", b[i].board_name);
		for (j = 0; j < 25 - strlen(b[i].board_name); j++)
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

#define CHIPSET_TH "{| border=\"0\" style=\"font-size: smaller\"\n\
|- bgcolor=\"#6699dd\"\n! align=\"left\" | Vendor\n\
! align=\"left\" | Southbridge\n! align=\"left\" | PCI IDs\n\
! align=\"left\" | Status\n\n"

#define BOARD_TH "{| border=\"0\" style=\"font-size: smaller\" valign=\"top\"\
\n|- bgcolor=\"#6699dd\"\n! align=\"left\" | Vendor\n\
! align=\"left\" | Mainboard\n! align=\"left\" | Status\n\n"

#define BOARD_TH2 "{| border=\"0\" style=\"font-size: smaller\" valign=\"top\"\
\n|- bgcolor=\"#6699dd\"\n! align=\"left\" | Vendor\n\
! align=\"left\" | Mainboard\n! align=\"left\" | Required option\n\
! align=\"left\" | Status\n\n"

#define BOARD_INTRO "\
\n== Supported mainboards ==\n\n\
In general, it is very likely that flashrom works out of the box even if your \
mainboard is not listed below.\n\nThis is a list of mainboards where we have \
verified that they either do or do not need any special initialization to \
make flashrom work (given flashrom supports the respective chipset and flash \
chip), or that they do not yet work at all. If they do not work, support may \
or may not be added later.\n\n\
Mainboards which don't appear in the list may or may not work (we don't \
know, someone has to give it a try). Please report any further verified \
mainboards on the [[Mailinglist|mailing list]].\n"

#define CHIP_TH "{| border=\"0\" style=\"font-size: smaller\" valign=\"top\"\n\
|- bgcolor=\"#6699dd\"\n! align=\"left\" | Vendor\n\
! align=\"left\" | Device\n! align=\"left\" | Size / KB\n\
! align=\"left\" | Type\n! align=\"left\" colspan=\"4\" | Status\n\n\
|- bgcolor=\"#6699ff\"\n| colspan=\"4\" | &nbsp;\n\
| Probe\n| Read\n| Write\n| Erase\n\n"

#define PROGRAMMER_SECTION "\n== Supported programmers ==\n\nThis is a list \
of supported PCI devices flashrom can use as programmer:\n\n{| border=\"0\" \
valign=\"top\"\n| valign=\"top\"|\n\n{| border=\"0\" style=\"font-size: \
smaller\" valign=\"top\"\n|- bgcolor=\"#6699dd\"\n! align=\"left\" | Vendor\n\
! align=\"left\" | Device\n! align=\"left\" | PCI IDs\n\
! align=\"left\" | Status\n\n"

const struct board_info_url boards_url[] = {
	/* Verified working boards that don't need write-enables. */
	/* Please keep this list alphabetically ordered by vendor/board. */
	{ "Abit",		"AX8",			"http://www.abit.com.tw/page/en/motherboard/motherboard_detail.php?DEFTITLE=Y&fMTYPE=Socket%20939&pMODEL_NAME=AX8" },
	{ "Advantech",		"PCM-5820", 		"http://taiwan.advantech.com.tw/products/Model_Detail.asp?model_id=1-1TGZL8&BU=ACG&PD=" },
	{ "ASI",		"MB-5BLMP",		"http://www.hojerteknik.com/winnet.htm" },
	{ "ASUS",		"A8N-E",		"http://www.asus.com.tw/products.aspx?l1=3&l2=15&l3=171&l4=0&model=455&modelmenu=2" },
	{ "ASUS",		"A8NE-FM/S",		"http://www.hardwareschotte.de/hardware/preise/proid_1266090/preis_ASUS+A8NE-FM" },
	{ "ASUS",		"A8N-SLI Premium",	"http://www.asus.com.tw/products.aspx?l1=3&l2=15&l3=148&l4=0&model=539&modelmenu=1" },
	{ "ASUS",		"A8V-E Deluxe",		"http://www.asus.com.tw/products.aspx?l1=3&l2=15&l3=143&l4=0&model=376&modelmenu=1" },
	{ "ASUS",		"M2A-VM",		"http://www.asus.com.tw/products.aspx?l1=3&l2=101&l3=496&l4=0&model=1568&modelmenu=1" },
	{ "ASUS",		"M2N-E",		"http://www.asus.com/products.aspx?l1=3&l2=101&l3=308&l4=0&model=1181&modelmenu=1" },
	{ "ASUS",		"P2B",			"http://www.motherboard.cz/mb/asus/P2B.htm" },
	{ "ASUS",		"P2B-F",		"http://www.motherboard.cz/mb/asus/P2B-F.htm" },
	{ "ASUS",		"P2B-D",		"ftp://ftp.asus.com.tw/pub/ASUS/mb/slot1/440bx/p2b-d/" },
	{ "ASUS",		"P2B-DS",		"ftp://ftp.asus.com.tw/pub/ASUS/mb/slot1/440bx/p2b-ds/" },
	{ "ASUS",		"A7V400-MX",		"http://www.asus.com.tw/products.aspx?l1=3&l2=13&l3=63&l4=0&model=228&modelmenu=1" },
	{ "ASUS",		"A7V8X-MX",		"http://www.asus.com.tw/products.aspx?l1=3&l2=13&l3=64&l4=0&model=229&modelmenu=1" },
	{ "ASUS",		"P4B266",		"http://www.ciao.co.uk/ASUS_Intel_845D_Chipset_P4B266__5409807#productdetail" },
	{ "ASUS",		"A8V-E SE",		"http://www.asus.com.tw/products.aspx?l1=3&l2=15&l3=143&l4=0&model=576&modelmenu=1" },
	{ "ASUS",		"P2L97-S",		"http://www.motherboard.cz/mb/asus/P2L97-S.htm" },
	{ "ASUS",		"M2A-MX",		"http://www.asus.com/products.aspx?l1=3&l2=101&l3=583&l4=0&model=1909&modelmenu=1" },
	{ "ASUS",		"P5B-Deluxe",		"ftp://ftp.asus.com.tw/pub/ASUS/mb/socket775/P5B-Deluxe/" },
	{ "ASUS",		"P6T Deluxe V2",	"http://www.asus.com/product.aspx?P_ID=iRlP8RG9han6saZx" },
	{ "A-Trend",		"ATC-6220",		"http://www.motherboard.cz/mb/atrend/atc6220.htm" },
	{ "BCOM",		"WinNET100",		"http://www.coreboot.org/BCOM_WINNET100_Build_Tutorial" },
	{ "GIGABYTE",		"GA-6BXC",		"http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ClassValue=Motherboard&ProductID=1445&ProductName=GA-6BXC" },
	{ "GIGABYTE",		"GA-6BXDU",		"http://www.gigabyte.com.tw/Products/Motherboard/Products_Spec.aspx?ProductID=1429" },
	{ "GIGABYTE",		"GA-6ZMA",		"http://www.gigabyte.de/Support/Motherboard/BIOS_Model.aspx?ProductID=3289" },
	{ "Intel",		"EP80759",		NULL },
	{ "MSI",		"KT4V",			NULL },
	{ "MSI",		"MS-7046",		NULL },
	{ "MSI",		"MS-7065",		NULL },
	{ "MSI",		"MS-7236 (945PL Neo3)",	"http://global.msi.com.tw/index.php?func=prodmbspec&maincat_no=1&cat2_no=&cat3_no=&prod_no=1173#menu" },
	{ "MSI",		"MS-7345 (P35 Neo2-FIR)","http://www.msi.com/index.php?func=prodcpusupport&maincat_no=1&cat2_no=170&cat3_no=&prod_no=1261#menu" },
	{ "MSI",		"MS-7168 (Orion)",	"http://support.packardbell.co.uk/uk/item/index.php?i=spec_orion&pi=platform_honeymoon_istart" },
	{ "NEC",		"PowerMate 2000",	"http://support.necam.com/mobilesolutions/hardware/Desktops/pm2000/celeron/" },
	{ "PC Engines",		"Alix.1c",		"http://pcengines.ch/alix1c.htm" },
	{ "PC Engines",		"Alix.2c2",		"http://pcengines.ch/alix2c2.htm" },
	{ "PC Engines",		"Alix.2c3",		"http://pcengines.ch/alix2c3.htm" },
	{ "PC Engines",		"Alix.3c3",		"http://pcengines.ch/alix3c3.htm" },
	{ "RCA",		"RM4100",		"http://www.settoplinux.org" },
	{ "Supermicro",		"H8QC8",		"http://www.supermicro.com/Aplus/motherboard/Opteron/nforce/H8QC8.cfm" },
	{ "Sun",		"Blade x6250",		"http://www.sun.com/servers/blades/x6250/" },
	{ "Thomson",		"IP1000",		"http://www.settoplinux.org" },
	{ "T-Online",		"S-100",		"http://wiki.freifunk-hannover.de/T-Online_S_100" },
	{ "Tyan",		"S1846",		"http://www.tyan.com/archive/products/html/tsunamiatx.html" },
	// { "Tyan",		"S2498 (Tomcat K7M)",	"http://www.tyan.com/archive/products/html/tomcatk7m.html" },
	{ "Tyan",		"S2881",		"http://www.tyan.com/product_board_detail.aspx?pid=115" },
	{ "Tyan",		"S2882",		"http://www.tyan.com/product_board_detail.aspx?pid=121" },
	{ "Tyan",		"S2882-D",		"http://www.tyan.com/product_board_detail.aspx?pid=127" },
	{ "Tyan",		"S2891",		NULL },
	{ "Tyan",		"S2892",		NULL },
	{ "Tyan",		"S2895",		NULL },
	{ "Tyan",		"S3095",		"http://www.tyan.com/product_board_detail.aspx?pid=181" },
	{ "Tyan",		"S5180",		"http://www.tyan.com/product_board_detail.aspx?pid=456" },
	{ "Tyan",		"S5191",		"http://www.tyan.com/product_board_detail.aspx?pid=343" },
	{ "Tyan",		"S5197",		"http://www.tyan.com/product_board_detail.aspx?pid=349" },
	{ "Tyan",		"S5211",		"http://www.tyan.com/product_board_detail.aspx?pid=591" },
	{ "Tyan",		"S5211-1U",		"http://www.tyan.com/product_board_detail.aspx?pid=593" },
	{ "Tyan",		"S5220",		"http://www.tyan.com/product_board_detail.aspx?pid=597" },
	{ "Tyan",		"S5375",		"http://www.tyan.com/product_board_detail.aspx?pid=566" },
	{ "Tyan",		"iS5375-1U",		"http://www.tyan.com/product_board_detail.aspx?pid=610" },
	{ "Tyan",		"S5376G2NR/S5376WAG2NR","http://www.tyan.com/product_board_detail.aspx?pid=605" },
	{ "Tyan",		"S5377",		"http://www.tyan.com/product_board_detail.aspx?pid=601" },
	{ "Tyan",		"S5397",		"http://www.tyan.com/product_board_detail.aspx?pid=560" },
	{ "VIA",		"EPIA-M",		"http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=81" },
	{ "VIA",		"EPIA-MII",		"http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=202" },
	{ "VIA",		"EPIA-CN",		"http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=400" },
	{ "VIA",		"EPIA-LN",		"http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=473" },
	{ "VIA",		"VB700X",		"http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=490" },
	{ "VIA",		"NAB74X0",		"http://www.via.com.tw/en/products/mainboards/motherboards.jsp?motherboard_id=590" },
	{ "VIA",		"pc2500e",		"http://www.via.com.tw/en/initiatives/empowered/pc2500_mainboard/index.jsp" },

	/* Verified working boards that DO need write-enables. */
	/* Please keep this list alphabetically ordered by vendor/board. */
	{ "Acorp",		"6A815EPD",		NULL },
	/* TODO: Fill in entries/URLs for the remaining boards. */

	/* Verified non-working boards (for now). */
	/* Please keep this list alphabetically ordered by vendor/board. */
	{ "Abit",		"IS-10",		"http://www.abit.com.tw/page/en/motherboard/motherboard_detail.php?pMODEL_NAME=IS-10&fMTYPE=Socket+478" },
	{ "ASUS",		"A7N8X-E Deluxe",	"http://www.asus.com/products.aspx?l1=3&l2=13&l3=56&l4=0&model=217&modelmenu=1" },
	{ "ASUS",		"MEW-AM",		"ftp://ftp.asus.com.tw/pub/ASUS/mb/sock370/810/mew-am/" },
	{ "ASUS",		"MEW-VM",		"http://www.elhvb.com/mboards/OEM/HP/manual/ASUS%20MEW-VM.htm" },
	{ "ASUS",		"P3B-F",		"ftp://ftp.asus.com.tw/pub/ASUS/mb/slot1/440bx/p3b-f/" },
	{ "ASUS",		"P5B",			"ftp://ftp.asus.com.tw/pub/ASUS/mb/socket775/P5B/" },
	{ "ASUS",		"P5BV-M",		"ftp://ftp.asus.com.tw/pub/ASUS/mb/socket775/P5B-VM/" },
	{ "Biostar",		"M6TBA",		"ftp://ftp.biostar-usa.com/manuals/M6TBA/" },
	{ "Boser",		"HS-6637",		"http://www.boser.com.tw/manual/HS-62376637v3.4.pdf" },
	{ "FIC",		"VA-502",		"ftp://ftp.fic.com.tw/motherboard/manual/socket7/va-502/" },
	{ "MSI",		"MS-7260 (K9N Neo)",	"http://global.msi.com.tw/index.php?func=proddesc&prod_no=255&maincat_no=1" },
	{ "PCCHIPS",		"M537DMA33",		"http://motherboards.mbarron.net/models/pcchips/m537dma.htm" },
	{ "Soyo",		"SY-5VD",		"http://www.soyo.com/content/Downloads/163/&c=80&p=464&l=English" },
	{ "Sun",		"Fire x4540",		"http://www.sun.com/servers/x64/x4540/" },
	{ "Sun",		"Fire x4150",		"http://www.sun.com/servers/x64/x4150/" },
	{ "Sun",		"Fire x4200",		"http://www.sun.com/servers/entry/x4200/" },
	{ "Sun",		"Fire x4600",		"http://www.sun.com/servers/x64/x4600/" },
	{ NULL,			NULL,			0 },
};

static int url(const char *vendor, const char *board)
{
	int i;
	const struct board_info_url *b = boards_url;

        for (i = 0; b[i].vendor != NULL; i++) {
		if (!strcmp(vendor, b[i].vendor) && !strcmp(board, b[i].name))
			return i;
	}

	return -1;
}

void print_supported_chipsets_wiki(void)
{
	int i, j, enablescount = 0, color = 1;
	const struct penable *e;

	for (e = chipset_enables; e->vendor_name != NULL; e++)
		enablescount++;

	printf("\n== Supported chipsets ==\n\nTotal amount of supported "
	       "chipsets: '''%d'''\n\n{| border=\"0\" valign=\"top\"\n| "
	       "valign=\"top\"|\n\n%s", enablescount, CHIPSET_TH);

	e = chipset_enables;
	for (i = 0, j = 0; e[i].vendor_name != NULL; i++, j++) {
		/* Alternate colors if the vendor changes. */
		if (i > 0 && strcmp(e[i].vendor_name, e[i - 1].vendor_name))
			color = !color;

		printf("|- bgcolor=\"#%s\" valign=\"top\"\n| %s || %s "
		       "|| %04x:%04x || %s\n", (color) ? "eeeeee" : "dddddd",
		       e[i].vendor_name, e[i].device_name,
		       e[i].vendor_id, e[i].device_id,
		       (e[i].status == OK) ? "{{OK}}" : "?");

		/* Split table in three columns. */
		if (j >= (enablescount / 3 + 1)) {
			printf("\n|}\n\n| valign=\"top\"|\n\n" CHIPSET_TH);
			j = 0;
		}
	}

	printf("\n|}\n\n|}\n");
}

static void wiki_helper(const char *heading, const char *status,
			int cols, const struct board_info boards[])
{
	int i, j, k, boardcount = 0, color = 1;
	const struct board_info *b;
	const struct board_info_url *u = boards_url;

	for (b = boards; b->vendor != NULL; b++)
		boardcount++;

	printf("\n'''%s'''\n\nTotal amount of boards: '''%d'''\n\n"
	       "{| border=\"0\" valign=\"top\"\n| valign=\"top\"|\n\n%s",
	       heading, boardcount, BOARD_TH);

        for (i = 0, j = 0, b = boards; b[i].vendor != NULL; i++, j++) {
		/* Alternate colors if the vendor changes. */
		if (i > 0 && strcmp(b[i].vendor, b[i - 1].vendor))
			color = !color;

		k = url(b[i].vendor, b[i].name);

		printf("|- bgcolor=\"#%s\" valign=\"top\"\n| %s || %s%s %s%s ||"
		       " {{%s}}\n", (color) ? "eeeeee" : "dddddd", b[i].vendor,
		       (k != -1 && u[k].url) ? "[" : "",
		       (k != -1 && u[k].url) ? u[k].url : "",
		       b[i].name, (k != -1 && u[k].url) ? "]" : "", status);

		/* Split table in 'cols' columns. */
		if (j >= (boardcount / cols + 1)) {
			printf("\n|}\n\n| valign=\"top\"|\n\n" BOARD_TH);
			j = 0;
		}
	}

	printf("\n|}\n\n|}\n");
}

static void wiki_helper2(const char *heading, int cols)
{
	int i, j, boardcount = 0, color = 1;
	struct board_pciid_enable *b;

	for (b = board_pciid_enables; b->vendor_name != NULL; b++)
		boardcount++;

	printf("\n'''%s'''\n\nTotal amount of boards: '''%d'''\n\n"
	       "{| border=\"0\" valign=\"top\"\n| valign=\"top\"|\n\n%s",
	       heading, boardcount, BOARD_TH2);

	b = board_pciid_enables;
	for (i = 0, j = 0; b[i].vendor_name != NULL; i++, j++) {
		/* Alternate colors if the vendor changes. */
		if (i > 0 && strcmp(b[i].vendor_name, b[i - 1].vendor_name))
			color = !color;

		printf("|- bgcolor=\"#%s\" valign=\"top\"\n| %s || %s || "
		       "%s%s%s%s || {{OK}}\n", (color) ? "eeeeee" : "dddddd",
		       b[i].vendor_name, b[i].board_name,
		       (b[i].lb_vendor) ? "-m " : "&mdash;",
		       (b[i].lb_vendor) ? b[i].lb_vendor : "",
		       (b[i].lb_vendor) ? ":" : "",
		       (b[i].lb_vendor) ? b[i].lb_part : "");

		/* Split table in three columns. */
		if (j >= (boardcount / cols + 1)) {
			printf("\n|}\n\n| valign=\"top\"|\n\n" BOARD_TH2);
			j = 0;
		}
	}

	printf("\n|}\n\n|}\n");
}

void print_supported_boards_wiki(void)
{
	printf("%s", BOARD_INTRO);
	wiki_helper("Known good (worked out of the box)", "OK", 3, boards_ok);
	wiki_helper2("Known good (with write-enable code in flashrom)", 3);
	wiki_helper("Not supported (yet)", "No", 3, boards_bad);
}

void print_supported_chips_wiki(void)
{
	int i = 0, c = 1, chipcount = 0;
	struct flashchip *f, *old = NULL;

	for (f = flashchips; f->name != NULL; f++)
		chipcount++;

	printf("\n== Supported chips ==\n\nTotal amount of supported "
	       "chips: '''%d'''\n\n{| border=\"0\" valign=\"top\"\n"
		"| valign=\"top\"|\n\n%s", chipcount, CHIP_TH);

	for (f = flashchips; f->name != NULL; f++, i++) {
		if (!strncmp(f->name, "unknown", 7))
			continue;

		/* Alternate colors if the vendor changes. */
		if (old != NULL && strcmp(old->vendor, f->vendor))
			c = !c;

		printf("|- bgcolor=\"#%s\" valign=\"top\"\n| %s || %s || %d "
		       "|| %s || {{%s}} || {{%s}} || {{%s}} || {{%s}}\n",
		       (c == 1) ? "eeeeee" : "dddddd", f->vendor, f->name,
		       f->total_size, flashbuses_to_text(f->bustype),
		       ((f->tested & TEST_OK_PROBE) ? "OK" : (c) ? "?2" : "?"),
		       ((f->tested & TEST_OK_READ)  ? "OK" : (c) ? "?2" : "?"),
		       ((f->tested & TEST_OK_ERASE) ? "OK" : (c) ? "?2" : "?"),
		       ((f->tested & TEST_OK_WRITE) ? "OK" : (c) ? "?2" : "?"));

		/* Split table into three columns. */
		if (i >= (chipcount / 3 + 1)) {
			printf("\n|}\n\n| valign=\"top\"|\n\n" CHIP_TH);
			i = 0;
		}

		old = f;
	}

	printf("\n|}\n\n|}\n");
}

void print_supported_pcidevs_wiki_header(void)
{
	printf("%s", PROGRAMMER_SECTION);
}

void print_supported_pcidevs_wiki_footer(void)
{
	printf("\n|}\n");
}

void print_supported_pcidevs_wiki(struct pcidev_status *devs)
{
	int i = 0;
	static int c = 0;

	/* Alternate colors if the vendor changes. */
	c = !c;

	for (i = 0; devs[i].vendor_name != NULL; i++) {
		printf("|- bgcolor=\"#%s\" valign=\"top\"\n| %s || %s || "
		       "%04x:%04x || {{%s}}\n", (c) ? "eeeeee" : "dddddd",
		       devs[i].vendor_name, devs[i].device_name,
		       devs[i].vendor_id, devs[i].device_id,
		       (devs[i].status == PCI_NT) ? (c) ? "?2" : "?" : "OK");
	}
}
