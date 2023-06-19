/*
 * This file is part of the flashrom project.
 *
 * Copyright (c) 2010  Matthias Wenzel <bios at mazzoo dot de>
 * Copyright (c) 2011  Stefan Tauner
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

#include "hwaccess_physmap.h"
#include "ich_descriptors.h"

#ifdef ICH_DESCRIPTORS_FROM_DUMP_ONLY
#include <stdio.h>
#include <string.h>
#define print(t, ...) printf(__VA_ARGS__)
#endif

#define DESCRIPTOR_MODE_SIGNATURE 0x0ff0a55a
/* The upper map is located in the word before the 256B-long OEM section at the
 * end of the 4kB-long flash descriptor.
 */
#define UPPER_MAP_OFFSET (4096 - 256 - 4)
#define getVTBA(flumap)	(((flumap)->FLUMAP1 << 4) & 0x00000ff0)

#include <stdbool.h>
#include <sys/types.h>
#include <string.h>
#include "flash.h" /* for msg_* */
#include "programmer.h"

ssize_t ich_number_of_regions(const enum ich_chipset cs, const struct ich_desc_content *const cont)
{
	switch (cs) {
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_GEMINI_LAKE:
		return 6;
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_C740_SERIES_EMMITSBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_700_SERIES_RAPTOR_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_PANTHER_LAKE:
	case CHIPSET_ELKHART_LAKE:
	case CHIPSET_JASPER_LAKE:
		return 16;
	case CHIPSET_100_SERIES_SUNRISE_POINT:
		return 10;
	case CHIPSET_9_SERIES_WILDCAT_POINT_LP:
	case CHIPSET_9_SERIES_WILDCAT_POINT:
	case CHIPSET_8_SERIES_LYNX_POINT_LP:
	case CHIPSET_8_SERIES_LYNX_POINT:
	case CHIPSET_8_SERIES_WELLSBURG:
		if (cont->NR <= 6)
			return cont->NR + 1;
		else
			return -1;
	default:
		if (cont->NR <= 4)
			return cont->NR + 1;
		else
			return -1;
	}
}

ssize_t ich_number_of_masters(const enum ich_chipset cs, const struct ich_desc_content *const cont)
{
	switch (cs) {
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_C740_SERIES_EMMITSBURG:
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_700_SERIES_RAPTOR_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_PANTHER_LAKE:
	case CHIPSET_GEMINI_LAKE:
	case CHIPSET_JASPER_LAKE:
	case CHIPSET_ELKHART_LAKE:
		if (cont->NM <= MAX_NUM_MASTERS)
			return cont->NM;
		break;
	default:
		if (cont->NM < MAX_NUM_MASTERS)
			return cont->NM + 1;
	}

	return -1;
}

void prettyprint_ich_reg_vscc(uint32_t reg_val, int verbosity, bool print_vcl)
{
	print(verbosity, "BES=0x%"PRIx32", ",	(reg_val & VSCC_BES)  >> VSCC_BES_OFF);
	print(verbosity, "WG=%"PRId32", ",	(reg_val & VSCC_WG)   >> VSCC_WG_OFF);
	print(verbosity, "WSR=%"PRId32", ",	(reg_val & VSCC_WSR)  >> VSCC_WSR_OFF);
	print(verbosity, "WEWS=%"PRId32", ",	(reg_val & VSCC_WEWS) >> VSCC_WEWS_OFF);
	print(verbosity, "EO=0x%"PRIx32"",	(reg_val & VSCC_EO)   >> VSCC_EO_OFF);
	if (print_vcl)
		print(verbosity, ", VCL=%"PRId32"", (reg_val & VSCC_VCL)  >> VSCC_VCL_OFF);
	print(verbosity, "\n");
}

#define getFCBA(cont)	(((cont)->FLMAP0 <<  4) & 0x00000ff0)
#define getFRBA(cont)	(((cont)->FLMAP0 >> 12) & 0x00000ff0)
#define getFMBA(cont)	(((cont)->FLMAP1 <<  4) & 0x00000ff0)
#define getFISBA(cont)	(((cont)->FLMAP1 >> 12) & 0x00000ff0)
#define getFMSBA(cont)	(((cont)->FLMAP2 <<  4) & 0x00000ff0)

void prettyprint_ich_chipset(enum ich_chipset cs)
{
	static const char *const chipset_names[] = {
		"Unknown ICH", "ICH8", "ICH9", "ICH10",
		"5 series Ibex Peak", "6 series Cougar Point", "7 series Panther Point",
		"8 series Lynx Point", "Baytrail", "8 series Lynx Point LP", "8 series Wellsburg",
		"9 series Wildcat Point", "9 series Wildcat Point LP", "100 series Sunrise Point",
		"C620 series Lewisburg", "C740 series Emmitsburg", "300 series Cannon Point",
		"400 series Comet Point", "500 series Tiger Point", "600 series Alder Point",
		"Apollo Lake", "Gemini Lake", "Jasper Lake", "Elkhart Lake",
		"Meteor Lake", "Panther Lake",
	};
	if (cs < CHIPSET_ICH8 || cs - CHIPSET_ICH8 + 1 >= ARRAY_SIZE(chipset_names))
		cs = 0;
	else
		cs = cs - CHIPSET_ICH8 + 1;
	msg_pdbg2("Assuming chipset '%s'.\n", chipset_names[cs]);
}

void prettyprint_ich_descriptors(enum ich_chipset cs, const struct ich_descriptors *desc)
{
	prettyprint_ich_descriptor_content(cs, &desc->content);
	prettyprint_ich_descriptor_component(cs, desc);
	prettyprint_ich_descriptor_region(cs, desc);
	prettyprint_ich_descriptor_master(cs, desc);
#ifdef ICH_DESCRIPTORS_FROM_DUMP_ONLY
	if (cs >= CHIPSET_ICH8) {
		prettyprint_ich_descriptor_upper_map(&desc->upper);
		prettyprint_ich_descriptor_straps(cs, desc);
	}
#endif /* ICH_DESCRIPTORS_FROM_DUMP_ONLY */
}

void prettyprint_ich_descriptor_content(enum ich_chipset cs, const struct ich_desc_content *cont)
{
	msg_pdbg2("=== Content Section ===\n");
	msg_pdbg2("FLVALSIG 0x%08"PRIx32"\n", cont->FLVALSIG);
	msg_pdbg2("FLMAP0   0x%08"PRIx32"\n", cont->FLMAP0);
	msg_pdbg2("FLMAP1   0x%08"PRIx32"\n", cont->FLMAP1);
	msg_pdbg2("FLMAP2   0x%08"PRIx32"\n", cont->FLMAP2);
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	msg_pdbg2("NR          (Number of Regions):                 %5zd\n",   ich_number_of_regions(cs, cont));
	msg_pdbg2("FRBA        (Flash Region Base Address):         0x%03"PRIx32"\n", getFRBA(cont));
	msg_pdbg2("NC          (Number of Components):              %5d\n",    cont->NC + 1);
	msg_pdbg2("FCBA        (Flash Component Base Address):      0x%03"PRIx32"\n", getFCBA(cont));
	msg_pdbg2("ISL         (ICH/PCH/SoC Strap Length):          %5d\n",    cont->ISL);
	msg_pdbg2("FISBA/FPSBA (Flash ICH/PCH/SoC Strap Base Addr): 0x%03"PRIx32"\n", getFISBA(cont));
	msg_pdbg2("NM          (Number of Masters):                 %5zd\n",   ich_number_of_masters(cs, cont));
	msg_pdbg2("FMBA        (Flash Master Base Address):         0x%03"PRIx32"\n", getFMBA(cont));
	msg_pdbg2("MSL/PSL     (MCH/PROC Strap Length):             %5d\n",    cont->MSL);
	msg_pdbg2("FMSBA       (Flash MCH/PROC Strap Base Address): 0x%03"PRIx32"\n", getFMSBA(cont));
	msg_pdbg2("\n");
}

static const char *pprint_density(enum ich_chipset cs, const struct ich_descriptors *desc, uint8_t idx)
{
	if (idx > 1) {
		msg_perr("Only ICH SPI component index 0 or 1 are supported yet.\n");
		return NULL;
	}

	if (desc->content.NC == 0 && idx > 0)
		return "unused";

	static const char * const size_str[] = {
		"512 kB",	/* 0000 */
		"1 MB",		/* 0001 */
		"2 MB",		/* 0010 */
		"4 MB",		/* 0011 */
		"8 MB",		/* 0100 */
		"16 MB",	/* 0101 */ /* Maximum up to Lynx Point (excl.) */
		"32 MB",	/* 0110 */
		"64 MB",	/* 0111 */
	};

	switch (cs) {
	case CHIPSET_ICH8:
	case CHIPSET_ICH9:
	case CHIPSET_ICH10:
	case CHIPSET_5_SERIES_IBEX_PEAK:
	case CHIPSET_6_SERIES_COUGAR_POINT:
	case CHIPSET_7_SERIES_PANTHER_POINT:
	case CHIPSET_BAYTRAIL: {
		uint8_t size_enc;
		if (idx == 0) {
			size_enc = desc->component.dens_old.comp1_density;
		} else {
			size_enc = desc->component.dens_old.comp2_density;
		}
		if (size_enc > 5)
			return "reserved";
		return size_str[size_enc];
	}
	case CHIPSET_8_SERIES_LYNX_POINT:
	case CHIPSET_8_SERIES_LYNX_POINT_LP:
	case CHIPSET_8_SERIES_WELLSBURG:
	case CHIPSET_9_SERIES_WILDCAT_POINT:
	case CHIPSET_9_SERIES_WILDCAT_POINT_LP:
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_C740_SERIES_EMMITSBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_700_SERIES_RAPTOR_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_PANTHER_LAKE:
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_GEMINI_LAKE:
	case CHIPSET_JASPER_LAKE:
	case CHIPSET_ELKHART_LAKE: {
		uint8_t size_enc;
		if (idx == 0) {
			size_enc = desc->component.dens_new.comp1_density;
		} else {
			size_enc = desc->component.dens_new.comp2_density;
		}
		if (size_enc > 7)
			return "reserved";
		return size_str[size_enc];
	}
	case CHIPSET_ICH_UNKNOWN:
	default:
		return "unknown";
	}
}

static const char *pprint_freq(enum ich_chipset cs, uint8_t value)
{
	static const char *const freq_str[5][8] = { {
		"20 MHz",
		"33 MHz",
		"reserved",
		"reserved",
		"50 MHz",	/* New since Ibex Peak */
		"reserved",
		"reserved",
		"reserved"
	}, {
		"reserved",
		"reserved",
		"48 MHz",
		"reserved",
		"30 MHz",
		"reserved",
		"17 MHz",
		"reserved"
	}, {
		"reserved",
		"50 MHz",
		"40 MHz",
		"reserved",
		"25 MHz",
		"reserved",
		"14 MHz / 17 MHz",
		"reserved"
	}, {
		"100 MHz",
		"50 MHz",
		"reserved",
		"33 MHz",
		"25 MHz",
		"reserved",
		"14 MHz",
		"reserved"
	}, {
		"reserved",
		"50 MHz",
		"reserved",
		"reserved",
		"33 MHz",
		"20 MHz",
		"reserved",
		"reserved",
	}};

	switch (cs) {
	case CHIPSET_ICH8:
	case CHIPSET_ICH9:
	case CHIPSET_ICH10:
		if (value > 1)
			return "reserved";
		/* Fall through. */
	case CHIPSET_5_SERIES_IBEX_PEAK:
	case CHIPSET_6_SERIES_COUGAR_POINT:
	case CHIPSET_7_SERIES_PANTHER_POINT:
	case CHIPSET_8_SERIES_LYNX_POINT:
	case CHIPSET_BAYTRAIL:
	case CHIPSET_8_SERIES_LYNX_POINT_LP:
	case CHIPSET_8_SERIES_WELLSBURG:
	case CHIPSET_9_SERIES_WILDCAT_POINT:
	case CHIPSET_9_SERIES_WILDCAT_POINT_LP:
		return freq_str[0][value];
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_JASPER_LAKE:
		return freq_str[1][value];
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_GEMINI_LAKE:
		return freq_str[2][value];
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_700_SERIES_RAPTOR_POINT:
	case CHIPSET_C740_SERIES_EMMITSBURG:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_PANTHER_LAKE:
		return freq_str[3][value];
	case CHIPSET_ELKHART_LAKE:
		return freq_str[4][value];
	case CHIPSET_ICH_UNKNOWN:
	default:
		return "unknown";
	}
}

static void pprint_read_freq(enum ich_chipset cs, uint8_t value)
{
	static const char *const freq_str[1][8] = { {
		"20 MHz",
		"24 MHz",
		"30 MHz",
		"48 MHz",
		"60 MHz",
		"reserved",
		"reserved",
		"reserved"
	}};

	switch (cs) {
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
		msg_pdbg2("eSPI/EC Bus Clock Frequency:    %s\n", freq_str[0][value]);
		return;
	case CHIPSET_500_SERIES_TIGER_POINT:
		msg_pdbg2("Read Clock Frequency:           %s\n", "reserved");
		return;
	default:
		msg_pdbg2("Read Clock Frequency:           %s\n", pprint_freq(cs, value));
		return;
	}
}

void prettyprint_ich_descriptor_component(enum ich_chipset cs, const struct ich_descriptors *desc)
{
	bool has_flill1;

	switch (cs) {
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_C740_SERIES_EMMITSBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_700_SERIES_RAPTOR_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_PANTHER_LAKE:
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_GEMINI_LAKE:
	case CHIPSET_JASPER_LAKE:
	case CHIPSET_ELKHART_LAKE:
		has_flill1 = true;
		break;
	default:
		has_flill1 = false;
		break;
	}

	msg_pdbg2("=== Component Section ===\n");
	msg_pdbg2("FLCOMP   0x%08"PRIx32"\n", desc->component.FLCOMP);
	msg_pdbg2("FLILL    0x%08"PRIx32"\n", desc->component.FLILL );
	if (has_flill1)
		msg_pdbg2("FLILL1   0x%08"PRIx32"\n", desc->component.FLILL1);
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	msg_pdbg2("Component 1 density:            %s\n", pprint_density(cs, desc, 0));
	if (desc->content.NC)
		msg_pdbg2("Component 2 density:            %s\n", pprint_density(cs, desc, 1));
	else
		msg_pdbg2("Component 2 is not used.\n");

	pprint_read_freq(cs, desc->component.modes.freq_read);

	msg_pdbg2("Read ID and Status Clock Freq.: %s\n", pprint_freq(cs, desc->component.modes.freq_read_id));
	msg_pdbg2("Write and Erase Clock Freq.:    %s\n", pprint_freq(cs, desc->component.modes.freq_write));
	msg_pdbg2("Fast Read is %ssupported.\n", desc->component.modes.fastread ? "" : "not ");
	if (desc->component.modes.fastread)
		msg_pdbg2("Fast Read Clock Frequency:      %s\n",
			  pprint_freq(cs, desc->component.modes.freq_fastread));
	if (cs > CHIPSET_6_SERIES_COUGAR_POINT)
		msg_pdbg2("Dual Output Fast Read Support:  %sabled\n",
			  desc->component.modes.dual_output ? "en" : "dis");

	bool has_forbidden_opcode = false;
	if (desc->component.FLILL != 0) {
		has_forbidden_opcode = true;
		msg_pdbg2("Invalid instruction 0:          0x%02x\n",
			  desc->component.invalid_instr0);
		msg_pdbg2("Invalid instruction 1:          0x%02x\n",
			  desc->component.invalid_instr1);
		msg_pdbg2("Invalid instruction 2:          0x%02x\n",
			  desc->component.invalid_instr2);
		msg_pdbg2("Invalid instruction 3:          0x%02x\n",
			  desc->component.invalid_instr3);
	}
	if (has_flill1) {
		if (desc->component.FLILL1 != 0) {
			has_forbidden_opcode = true;
			msg_pdbg2("Invalid instruction 4:          0x%02x\n",
				  desc->component.invalid_instr4);
			msg_pdbg2("Invalid instruction 5:          0x%02x\n",
				  desc->component.invalid_instr5);
			msg_pdbg2("Invalid instruction 6:          0x%02x\n",
				  desc->component.invalid_instr6);
			msg_pdbg2("Invalid instruction 7:          0x%02x\n",
				  desc->component.invalid_instr7);
		}
	}
	if (!has_forbidden_opcode)
		msg_pdbg2("No forbidden opcodes.\n");

	msg_pdbg2("\n");
}

static void pprint_freg(const struct ich_desc_region *reg, uint32_t i)
{
	static const char *const region_names[] = {
		"Descr.", "BIOS", "ME", "GbE", "Platf.", "DevExp", "BIOS2", "unknown",
		"EC/BMC", "unknown", "IE", "10GbE0", "10GbE1", "unknown", "unknown", "PTT"
	};
	if (i >= ARRAY_SIZE(region_names)) {
		msg_pdbg2("%s: region index too high.\n", __func__);
		return;
	}
	uint32_t base = ICH_FREG_BASE(reg->FLREGs[i]);
	uint32_t limit = ICH_FREG_LIMIT(reg->FLREGs[i]);
	msg_pdbg2("Region %"PRId32" (%-7s) ", i, region_names[i]);
	if (base > limit)
		msg_pdbg2("is unused.\n");
	else
		msg_pdbg2("0x%08"PRIx32" - 0x%08"PRIx32"\n", base, limit);
}

void prettyprint_ich_descriptor_region(const enum ich_chipset cs, const struct ich_descriptors *const desc)
{
	ssize_t i;
	const ssize_t nr = ich_number_of_regions(cs, &desc->content);
	msg_pdbg2("=== Region Section ===\n");
	if (nr < 0) {
		msg_pdbg2("%s: number of regions too high (%d).\n", __func__,
			  desc->content.NR + 1);
		return;
	}
	for (i = 0; i < nr; i++)
		msg_pdbg2("FLREG%zd   0x%08"PRIx32"\n", i, desc->region.FLREGs[i]);
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	for (i = 0; i < nr; i++)
		pprint_freg(&desc->region, (uint32_t)i);
	msg_pdbg2("\n");
}

void prettyprint_ich_descriptor_master(const enum ich_chipset cs, const struct ich_descriptors *const desc)
{
	ssize_t i;
	ssize_t nm = ich_number_of_masters(cs, &desc->content);
	msg_pdbg2("=== Master Section ===\n");
	if (nm < 0) {
		msg_pdbg2("%s: number of masters too high (%d).\n", __func__,
			  desc->content.NM + 1);
		return;
	}
	if (cs == CHIPSET_C740_SERIES_EMMITSBURG) {
		/*
		 * The SPI programming guide says there are 6 masters (thus NM=6), but it
		 * can only name 5. If there's a 6th then it's undocumented.
		 * However the first 5 are matching '500 Series PCH' and since C740 is a
		 * 500 Series clone, this field probably was not updated when writing the
		 * document.
		 * Hardcode to 5 to be compatible with '500 Series PCH' below.
		 */
		nm = 5;
	}
	for (i = 0; i < nm; i++)
		msg_pdbg2("FLMSTR%zd  0x%08"PRIx32"\n", i + 1, desc->master.FLMSTRs[i]);

	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	if (cs == CHIPSET_100_SERIES_SUNRISE_POINT ||
	    cs == CHIPSET_300_SERIES_CANNON_POINT ||
	    cs == CHIPSET_400_SERIES_COMET_POINT ||
	    cs == CHIPSET_500_SERIES_TIGER_POINT ||
	    cs == CHIPSET_600_SERIES_ALDER_POINT ||
	    cs == CHIPSET_700_SERIES_RAPTOR_POINT ||
	    cs == CHIPSET_C740_SERIES_EMMITSBURG ||
	    cs == CHIPSET_JASPER_LAKE ||
	    cs == CHIPSET_METEOR_LAKE ||
	    cs == CHIPSET_PANTHER_LAKE) {
		const char *const master_names[] = {
			"BIOS", "ME", "GbE", "DevE", "EC",
		};

		if (nm > (ssize_t)ARRAY_SIZE(master_names)) {
			msg_pdbg2("%s: number of masters too high (%zd).\n", __func__, nm);
			return;
		}

		size_t num_regions;
		msg_pdbg2("       FD    BIOS    ME    GbE    Pltf    DE   BIOS2   Reg7    EC    DE2  ");
		if (cs == CHIPSET_100_SERIES_SUNRISE_POINT) {
			num_regions = 10;
			msg_pdbg2("\n");
		} else {
			num_regions = 16;
			msg_pdbg2("  IE   10GbE0 10GbE1  RegD   RegE   PTT  \n");
		}
		for (i = 0; i < nm; i++) {
			const unsigned int ext_region_start = 12;
			size_t j;
			msg_pdbg2("%-4s", master_names[i]);
			for (j = 0; j < (size_t)min(num_regions, ext_region_start); j++)
				msg_pdbg2("   %c%c  ",
					  desc->master.mstr[i].read & (1 << j) ? 'r' : ' ',
					  desc->master.mstr[i].write & (1 << j) ? 'w' : ' ');
			for (j = ext_region_start; j < num_regions; j++)
				msg_pdbg2("   %c%c  ",
					  desc->master.mstr[i].ext_read & (1 << (j - ext_region_start)) ? 'r' : ' ',
					  desc->master.mstr[i].ext_write & (1 << (j - ext_region_start)) ? 'w' : ' ');
			msg_pdbg2("\n");
		}
	} else if (cs == CHIPSET_C620_SERIES_LEWISBURG) {
		const char *const master_names[] = {
			"BIOS", "ME", "GbE", "DE", "BMC", "IE",
		};
		/* NM starts at 1 instead of 0 for LBG */
		if (nm > (ssize_t)ARRAY_SIZE(master_names)) {
			msg_pdbg2("%s: number of masters too high (%d).\n", __func__,
				  desc->content.NM);
			return;
		}

		msg_pdbg2("%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
				"    ", /* width of master name (4 chars minimum) */
				" FD  ", " BIOS", " ME  ", " GbE ", " Pltf",
				" DE  ", "BIOS2", " Reg7", " BMC ", " DE2 ",
				" IE  ", "10GbE", "OpROM", "Reg13", "Reg14",
				"Reg15");
		for (i = 0; i < nm; i++) {
			size_t j;
			msg_pdbg2("%-4s", master_names[i]);
			for (j = 0; j < 16; j++)
				msg_pdbg2("  %c%c  ",
					  desc->master.mstr[i].read & (1 << j) ? 'r' : ' ',
					  desc->master.mstr[i].write & (1 << j) ? 'w' : ' ');
			msg_pdbg2("\n");
		}
	} else if (cs == CHIPSET_APOLLO_LAKE || cs == CHIPSET_GEMINI_LAKE || cs == CHIPSET_ELKHART_LAKE) {
		const char *const master_names[] = { "BIOS", "TXE", };
		if (nm > (ssize_t)ARRAY_SIZE(master_names)) {
			msg_pdbg2("%s: number of masters too high (%d).\n", __func__, desc->content.NM);
			return;
		}

		msg_pdbg2("       FD   IFWI  TXE   n/a  Platf DevExp\n");
		for (i = 0; i < nm; i++) {
			ssize_t j;
			msg_pdbg2("%-4s", master_names[i]);
			for (j = 0; j < ich_number_of_regions(cs, &desc->content); j++)
				msg_pdbg2("   %c%c ",
					  desc->master.mstr[i].read & (1 << j) ? 'r' : ' ',
					  desc->master.mstr[i].write & (1 << j) ? 'w' : ' ');
			msg_pdbg2("\n");
		}
	} else {
		const struct ich_desc_master *const mstr = &desc->master;
		msg_pdbg2("      Descr. BIOS ME GbE Platf.\n");
		msg_pdbg2("BIOS    %c%c    %c%c  %c%c  %c%c   %c%c\n",
		(mstr->BIOS_descr_r)	?'r':' ', (mstr->BIOS_descr_w)	?'w':' ',
		(mstr->BIOS_BIOS_r)	?'r':' ', (mstr->BIOS_BIOS_w)	?'w':' ',
		(mstr->BIOS_ME_r)	?'r':' ', (mstr->BIOS_ME_w)	?'w':' ',
		(mstr->BIOS_GbE_r)	?'r':' ', (mstr->BIOS_GbE_w)	?'w':' ',
		(mstr->BIOS_plat_r)	?'r':' ', (mstr->BIOS_plat_w)	?'w':' ');
		msg_pdbg2("ME      %c%c    %c%c  %c%c  %c%c   %c%c\n",
		(mstr->ME_descr_r)	?'r':' ', (mstr->ME_descr_w)	?'w':' ',
		(mstr->ME_BIOS_r)	?'r':' ', (mstr->ME_BIOS_w)	?'w':' ',
		(mstr->ME_ME_r)		?'r':' ', (mstr->ME_ME_w)	?'w':' ',
		(mstr->ME_GbE_r)	?'r':' ', (mstr->ME_GbE_w)	?'w':' ',
		(mstr->ME_plat_r)	?'r':' ', (mstr->ME_plat_w)	?'w':' ');
		msg_pdbg2("GbE     %c%c    %c%c  %c%c  %c%c   %c%c\n",
		(mstr->GbE_descr_r)	?'r':' ', (mstr->GbE_descr_w)	?'w':' ',
		(mstr->GbE_BIOS_r)	?'r':' ', (mstr->GbE_BIOS_w)	?'w':' ',
		(mstr->GbE_ME_r)	?'r':' ', (mstr->GbE_ME_w)	?'w':' ',
		(mstr->GbE_GbE_r)	?'r':' ', (mstr->GbE_GbE_w)	?'w':' ',
		(mstr->GbE_plat_r)	?'r':' ', (mstr->GbE_plat_w)	?'w':' ');
	}
	msg_pdbg2("\n");
}

static void prettyprint_ich_descriptor_straps_ich8(const struct ich_descriptors *desc)
{
	static const char * const str_GPIO12[4] = {
		"GPIO12",
		"LAN PHY Power Control Function (Native Output)",
		"GLAN_DOCK# (Native Input)",
		"invalid configuration",
	};

	msg_pdbg2("--- MCH details ---\n");
	msg_pdbg2("ME B is %sabled.\n", desc->north.ich8.MDB ? "dis" : "en");
	msg_pdbg2("\n");

	msg_pdbg2("--- ICH details ---\n");
	msg_pdbg2("ME SMBus Address 1: 0x%02x\n", desc->south.ich8.ASD);
	msg_pdbg2("ME SMBus Address 2: 0x%02x\n", desc->south.ich8.ASD2);
	msg_pdbg2("ME SMBus Controller is connected to the %s.\n",
		  desc->south.ich8.MESM2SEL ? "SMLink pins" : "SMBus pins");
	msg_pdbg2("SPI CS1 is used for %s.\n",
		  desc->south.ich8.SPICS1_LANPHYPC_SEL ?
		  "LAN PHY Power Control Function" :
		  "SPI Chip Select");
	msg_pdbg2("GPIO12 is used as %s.\n",
		  str_GPIO12[desc->south.ich8.GPIO12_SEL]);
	msg_pdbg2("PCIe Port 6 is used for %s.\n",
	     desc->south.ich8.GLAN_PCIE_SEL ? "integrated LAN" : "PCI Express");
	msg_pdbg2("%sn BMC Mode: "
		  "Intel AMT SMBus Controller 1 is connected to %s.\n",
		  desc->south.ich8.BMCMODE ? "I" : "Not i",
		  desc->south.ich8.BMCMODE ? "SMLink" : "SMBus");
	msg_pdbg2("TCO is in %s Mode.\n",
	       desc->south.ich8.TCOMODE ? "Advanced TCO" : "Legacy/Compatible");
	msg_pdbg2("ME A is %sabled.\n",
		  desc->south.ich8.ME_DISABLE ? "dis" : "en");
	msg_pdbg2("\n");
}

static void prettyprint_ich_descriptor_straps_56_pciecs(uint8_t conf, uint8_t off)
{
	msg_pdbg2("PCI Express Port Configuration Strap %d: ", off+1);

	off *= 4;
	switch (conf){
	case 0:
		msg_pdbg2("4x1 Ports %d-%d (x1)", 1+off, 4+off);
		break;
	case 1:
		msg_pdbg2("1x2, 2x1 Port %d (x2), Port %d (disabled), "
			  "Ports %d, %d (x1)", 1+off, 2+off, 3+off, 4+off);
		break;
	case 2:
		msg_pdbg2("2x2 Port %d (x2), Port %d (x2), Ports "
			  "%d, %d (disabled)", 1+off, 3+off, 2+off, 4+off);
		break;
	case 3:
		msg_pdbg2("1x4 Port %d (x4), Ports %d-%d (disabled)",
			  1+off, 2+off, 4+off);
		break;
	}
	msg_pdbg2("\n");
}

static void prettyprint_ich_descriptor_pchstraps45678_56(const struct ich_desc_south_strap *s)
{
	/* PCHSTRP4 */
	msg_pdbg2("Intel PHY is %s.\n",
		  (s->ibex.PHYCON == 2) ? "connected" :
			  (s->ibex.PHYCON == 0) ? "disconnected" : "reserved");
	msg_pdbg2("GbE MAC SMBus address is %sabled.\n",
		  s->ibex.GBEMAC_SMBUS_ADDR_EN ? "en" : "dis");
	msg_pdbg2("GbE MAC SMBus address: 0x%02x\n",
		  s->ibex.GBEMAC_SMBUS_ADDR);
	msg_pdbg2("GbE PHY SMBus address: 0x%02x\n",
		  s->ibex.GBEPHY_SMBUS_ADDR);

	/* PCHSTRP5 */
	/* PCHSTRP6 */
	/* PCHSTRP7 */
	msg_pdbg2("Intel ME SMBus Subsystem Vendor ID: 0x%04x\n",
		  s->ibex.MESMA2UDID_VENDOR);
	msg_pdbg2("Intel ME SMBus Subsystem Device ID: 0x%04x\n",
		  s->ibex.MESMA2UDID_VENDOR);

	/* PCHSTRP8 */
}

static void prettyprint_ich_descriptor_pchstraps111213_56(const struct ich_desc_south_strap *s)
{
	/* PCHSTRP11 */
	msg_pdbg2("SMLink1 GP Address is %sabled.\n",
		  s->ibex.SML1GPAEN ? "en" : "dis");
	msg_pdbg2("SMLink1 controller General Purpose Target address: 0x%02x\n",
		  s->ibex.SML1GPA);
	msg_pdbg2("SMLink1 I2C Target address is %sabled.\n",
		  s->ibex.SML1I2CAEN ? "en" : "dis");
	msg_pdbg2("SMLink1 I2C Target address: 0x%02x\n",
		  s->ibex.SML1I2CA);

	/* PCHSTRP12 */
	/* PCHSTRP13 */
}

static void prettyprint_ich_descriptor_straps_ibex(const struct ich_desc_south_strap *s)
{
	static const uint8_t dec_t209min[4] = {
		100,
		50,
		5,
		1
	};

	msg_pdbg2("--- PCH ---\n");

	/* PCHSTRP0 */
	msg_pdbg2("Chipset configuration Softstrap 2: %d\n", s->ibex.cs_ss2);
	msg_pdbg2("Intel ME SMBus Select is %sabled.\n",
		  s->ibex.SMB_EN ? "en" : "dis");
	msg_pdbg2("SMLink0 segment is %sabled.\n",
		  s->ibex.SML0_EN ? "en" : "dis");
	msg_pdbg2("SMLink1 segment is %sabled.\n",
		  s->ibex.SML1_EN ? "en" : "dis");
	msg_pdbg2("SMLink1 Frequency: %s\n",
		  (s->ibex.SML1FRQ == 1) ? "100 kHz" : "reserved");
	msg_pdbg2("Intel ME SMBus Frequency: %s\n",
		  (s->ibex.SMB0FRQ == 1) ? "100 kHz" : "reserved");
	msg_pdbg2("SMLink0 Frequency: %s\n",
		  (s->ibex.SML0FRQ == 1) ? "100 kHz" : "reserved");
	msg_pdbg2("GPIO12 is used as %s.\n", s->ibex.LANPHYPC_GP12_SEL ?
		  "LAN_PHY_PWR_CTRL" : "general purpose output");
	msg_pdbg2("Chipset configuration Softstrap 1: %d\n", s->ibex.cs_ss1);
	msg_pdbg2("DMI RequesterID Checks are %sabled.\n",
		  s->ibex.DMI_REQID_DIS ? "en" : "dis");
	msg_pdbg2("BIOS Boot-Block size (BBBS): %d kB.\n",
		  1 << (6 + s->ibex.BBBS));

	/* PCHSTRP1 */
	msg_pdbg2("Chipset configuration Softstrap 3: 0x%x\n", s->ibex.cs_ss3);

	/* PCHSTRP2 */
	msg_pdbg2("ME SMBus ASD address is %sabled.\n",
		  s->ibex.MESMASDEN ? "en" : "dis");
	msg_pdbg2("ME SMBus Controller ASD Target address: 0x%02x\n",
		  s->ibex.MESMASDA);
	msg_pdbg2("ME SMBus I2C address is %sabled.\n",
		  s->ibex.MESMI2CEN ? "en" : "dis");
	msg_pdbg2("ME SMBus I2C target address: 0x%02x\n",
		  s->ibex.MESMI2CA);

	/* PCHSTRP3 */
	prettyprint_ich_descriptor_pchstraps45678_56(s);
	/* PCHSTRP9 */
	prettyprint_ich_descriptor_straps_56_pciecs(s->ibex.PCIEPCS1, 0);
	prettyprint_ich_descriptor_straps_56_pciecs(s->ibex.PCIEPCS1, 1);
	msg_pdbg2("PCIe Lane Reversal 1: PCIe Lanes 0-3 are %sreserved.\n",
		  s->ibex.PCIELR1 ? "" : "not ");
	msg_pdbg2("PCIe Lane Reversal 2: PCIe Lanes 4-7 are %sreserved.\n",
		  s->ibex.PCIELR2 ? "" : "not ");
	msg_pdbg2("DMI Lane Reversal: DMI Lanes 0-3 are %sreserved.\n",
		  s->ibex.DMILR ? "" : "not ");
	msg_pdbg2("Default PHY PCIe Port is %d.\n", s->ibex.PHY_PCIEPORTSEL+1);
	msg_pdbg2("Integrated MAC/PHY communication over PCIe is %sabled.\n",
		  s->ibex.PHY_PCIE_EN ? "en" : "dis");

	/* PCHSTRP10 */
	msg_pdbg2("Management Engine will boot from %sflash.\n",
		  s->ibex.ME_BOOT_FLASH ? "" : "ROM, then ");
	msg_pdbg2("Chipset configuration Softstrap 5: %d\n", s->ibex.cs_ss5);
	msg_pdbg2("Virtualization Engine Enable 1 is %sabled.\n",
		  s->ibex.VE_EN ? "en" : "dis");
	msg_pdbg2("ME Memory-attached Debug Display Device is %sabled.\n",
		  s->ibex.MMDDE ? "en" : "dis");
	msg_pdbg2("ME Memory-attached Debug Display Device address: 0x%02x\n",
		  s->ibex.MMADDR);
	msg_pdbg2("Chipset configuration Softstrap 7: %d\n", s->ibex.cs_ss7);
	msg_pdbg2("Integrated Clocking Configuration is %d.\n",
		  (s->ibex.ICC_SEL == 7) ? 0 : s->ibex.ICC_SEL);
	msg_pdbg2("PCH Signal CL_RST1# does %sassert when Intel ME performs a "
		  "reset.\n", s->ibex.MER_CL1 ? "" : "not ");

	prettyprint_ich_descriptor_pchstraps111213_56(s);

	/* PCHSTRP14 */
	msg_pdbg2("Virtualization Engine Enable 2 is %sabled.\n",
		  s->ibex.VE_EN2 ? "en" : "dis");
	msg_pdbg2("Virtualization Engine will boot from %sflash.\n",
		  s->ibex.VE_BOOT_FLASH ? "" : "ROM, then ");
	msg_pdbg2("Braidwood SSD functionality is %sabled.\n",
		  s->ibex.BW_SSD ? "en" : "dis");
	msg_pdbg2("Braidwood NVMHCI functionality is %sabled.\n",
		  s->ibex.NVMHCI_EN ? "en" : "dis");

	/* PCHSTRP15 */
	msg_pdbg2("Chipset configuration Softstrap 6: %d\n", s->ibex.cs_ss6);
	msg_pdbg2("Integrated wired LAN Solution is %sabled.\n",
		  s->ibex.IWL_EN ? "en" : "dis");
	msg_pdbg2("t209 min Timing: %d ms\n",
		  dec_t209min[s->ibex.t209min]);
	msg_pdbg2("\n");
}

static void prettyprint_ich_descriptor_straps_cougar(const struct ich_desc_south_strap *s)
{
	msg_pdbg2("--- PCH ---\n");

	/* PCHSTRP0 */
	msg_pdbg2("Chipset configuration Softstrap 1: %d\n", s->cougar.cs_ss1);
	msg_pdbg2("Intel ME SMBus Select is %sabled.\n",
		  s->ibex.SMB_EN ? "en" : "dis");
	msg_pdbg2("SMLink0 segment is %sabled.\n",
		  s->ibex.SML0_EN ? "en" : "dis");
	msg_pdbg2("SMLink1 segment is %sabled.\n",
		  s->ibex.SML1_EN ? "en" : "dis");
	msg_pdbg2("SMLink1 Frequency: %s\n",
		  (s->ibex.SML1FRQ == 1) ? "100 kHz" : "reserved");
	msg_pdbg2("Intel ME SMBus Frequency: %s\n",
		  (s->ibex.SMB0FRQ == 1) ? "100 kHz" : "reserved");
	msg_pdbg2("SMLink0 Frequency: %s\n",
		  (s->ibex.SML0FRQ == 1) ? "100 kHz" : "reserved");
	msg_pdbg2("GPIO12 is used as %s.\n", s->ibex.LANPHYPC_GP12_SEL ?
		  "LAN_PHY_PWR_CTRL" : "general purpose output");
	msg_pdbg2("LinkSec is %sabled.\n",
		  s->cougar.LINKSEC_DIS ? "en" : "dis");
	msg_pdbg2("DMI RequesterID Checks are %sabled.\n",
		  s->ibex.DMI_REQID_DIS ? "en" : "dis");
	msg_pdbg2("BIOS Boot-Block size (BBBS): %d kB.\n",
		  1 << (6 + s->ibex.BBBS));

	/* PCHSTRP1 */
	msg_pdbg2("Chipset configuration Softstrap 3: 0x%x\n", s->ibex.cs_ss3);
	msg_pdbg2("Chipset configuration Softstrap 2: 0x%x\n", s->ibex.cs_ss2);

	/* PCHSTRP2 */
	msg_pdbg2("ME SMBus ASD address is %sabled.\n",
		  s->ibex.MESMASDEN ? "en" : "dis");
	msg_pdbg2("ME SMBus Controller ASD Target address: 0x%02x\n",
		  s->ibex.MESMASDA);
	msg_pdbg2("ME SMBus MCTP Address is %sabled.\n",
		  s->cougar.MESMMCTPAEN ? "en" : "dis");
	msg_pdbg2("ME SMBus MCTP target address: 0x%02x\n",
		  s->cougar.MESMMCTPA);
	msg_pdbg2("ME SMBus I2C address is %sabled.\n",
		  s->ibex.MESMI2CEN ? "en" : "dis");
	msg_pdbg2("ME SMBus I2C target address: 0x%02x\n",
		  s->ibex.MESMI2CA);

	/* PCHSTRP3 */
	prettyprint_ich_descriptor_pchstraps45678_56(s);
	/* PCHSTRP9 */
	prettyprint_ich_descriptor_straps_56_pciecs(s->ibex.PCIEPCS1, 0);
	prettyprint_ich_descriptor_straps_56_pciecs(s->ibex.PCIEPCS1, 1);
	msg_pdbg2("PCIe Lane Reversal 1: PCIe Lanes 0-3 are %sreserved.\n",
		  s->ibex.PCIELR1 ? "" : "not ");
	msg_pdbg2("PCIe Lane Reversal 2: PCIe Lanes 4-7 are %sreserved.\n",
		  s->ibex.PCIELR2 ? "" : "not ");
	msg_pdbg2("DMI Lane Reversal: DMI Lanes 0-3 are %sreserved.\n",
		  s->ibex.DMILR ? "" : "not ");
	msg_pdbg2("ME Debug status writes over SMBUS are %sabled.\n",
		  s->cougar.MDSMBE_EN ? "en" : "dis");
	msg_pdbg2("ME Debug SMBus Emergency Mode address: 0x%02x (raw)\n",
		  s->cougar.MDSMBE_ADD);
	msg_pdbg2("Default PHY PCIe Port is %d.\n", s->ibex.PHY_PCIEPORTSEL+1);
	msg_pdbg2("Integrated MAC/PHY communication over PCIe is %sabled.\n",
		  s->ibex.PHY_PCIE_EN ? "en" : "dis");
	msg_pdbg2("PCIe ports Subtractive Decode Agent is %sabled.\n",
		  s->cougar.SUB_DECODE_EN ? "en" : "dis");
	msg_pdbg2("GPIO74 is used as %s.\n", s->cougar.PCHHOT_SML1ALERT_SEL ?
		  "PCHHOT#" : "SML1ALERT#");

	/* PCHSTRP10 */
	msg_pdbg2("Management Engine will boot from %sflash.\n",
		  s->ibex.ME_BOOT_FLASH ? "" : "ROM, then ");

	msg_pdbg2("ME Debug SMBus Emergency Mode is %sabled.\n",
		  s->cougar.MDSMBE_EN ? "en" : "dis");
	msg_pdbg2("ME Debug SMBus Emergency Mode Address: 0x%02x\n",
		  s->cougar.MDSMBE_ADD);

	msg_pdbg2("Integrated Clocking Configuration used: %d\n",
		  s->cougar.ICC_SEL);
	msg_pdbg2("PCH Signal CL_RST1# does %sassert when Intel ME performs a reset.\n",
		  s->ibex.MER_CL1 ? "" : "not ");
	msg_pdbg2("ICC Profile is selected by %s.\n",
		  s->cougar.ICC_PRO_SEL ? "Softstraps" : "BIOS");
	msg_pdbg2("Deep SX is %ssupported on the platform.\n",
		  s->cougar.Deep_SX_EN ? "not " : "");
	msg_pdbg2("ME Debug LAN Emergency Mode is %sabled.\n",
		  s->cougar.ME_DBG_LAN ? "en" : "dis");

	prettyprint_ich_descriptor_pchstraps111213_56(s);

	/* PCHSTRP14 */
	/* PCHSTRP15 */
	msg_pdbg2("Chipset configuration Softstrap 6: %d\n", s->cougar.cs_ss6);
	msg_pdbg2("Integrated wired LAN is %sabled.\n",
		  s->cougar.IWL_EN ? "en" : "dis");
	msg_pdbg2("Chipset configuration Softstrap 5: %d\n", s->cougar.cs_ss5);
	msg_pdbg2("SMLink1 provides temperature from %s.\n",
		  s->cougar.SMLINK1_THERM_SEL ? "PCH only" : "the CPU, PCH and DIMMs");
	msg_pdbg2("GPIO29 is used as %s.\n", s->cougar.SLP_LAN_GP29_SEL ?
		  "general purpose output" : "SLP_LAN#");

	/* PCHSTRP16 */
	/* PCHSTRP17 */
	msg_pdbg2("Integrated Clock: %s Clock Mode\n",
		  s->cougar.ICML ? "Buffered Through" : "Full Integrated");
	msg_pdbg2("\n");
}

void prettyprint_ich_descriptor_straps(enum ich_chipset cs, const struct ich_descriptors *desc)
{
	unsigned int i, max_count;
	msg_pdbg2("=== Softstraps ===\n");

	max_count = MIN(ARRAY_SIZE(desc->north.STRPs), desc->content.MSL);
	if (max_count < desc->content.MSL) {
		msg_pdbg2("MSL (%u) is greater than the current maximum of %u entries.\n",
			  desc->content.MSL, max_count);
		msg_pdbg2("Only the first %u entries will be printed.\n", max_count);
	}

	msg_pdbg2("--- North/MCH/PROC (%d entries) ---\n", max_count);
	for (i = 0; i < max_count; i++)
		msg_pdbg2("STRP%-2d = 0x%08"PRIx32"\n", i, desc->north.STRPs[i]);
	msg_pdbg2("\n");

	max_count = MIN(ARRAY_SIZE(desc->south.STRPs), desc->content.ISL);
	if (max_count < desc->content.ISL) {
		msg_pdbg2("ISL (%u) is greater than the current maximum of %u entries.\n",
			  desc->content.ISL, max_count);
		msg_pdbg2("Only the first %u entries will be printed.\n", max_count);
	}

	msg_pdbg2("--- South/ICH/PCH (%d entries) ---\n", max_count);
	for (i = 0; i < max_count; i++)
		msg_pdbg2("STRP%-2d = 0x%08"PRIx32"\n", i, desc->south.STRPs[i]);
	msg_pdbg2("\n");

	switch (cs) {
	case CHIPSET_ICH8:
		if (sizeof(desc->north.ich8) / 4 != desc->content.MSL)
			msg_pdbg2("Detailed North/MCH/PROC information is "
				  "probably not reliable, printing anyway.\n");
		if (sizeof(desc->south.ich8) / 4 != desc->content.ISL)
			msg_pdbg2("Detailed South/ICH/PCH information is "
				  "probably not reliable, printing anyway.\n");
		prettyprint_ich_descriptor_straps_ich8(desc);
		break;
	case CHIPSET_5_SERIES_IBEX_PEAK:
		/* PCH straps only. PROCSTRPs are unknown. */
		if (sizeof(desc->south.ibex) / 4 != desc->content.ISL)
			msg_pdbg2("Detailed South/ICH/PCH information is "
				  "probably not reliable, printing anyway.\n");
		prettyprint_ich_descriptor_straps_ibex(&desc->south);
		break;
	case CHIPSET_6_SERIES_COUGAR_POINT:
		/* PCH straps only. PROCSTRP0 is "reserved". */
		if (sizeof(desc->south.cougar) / 4 != desc->content.ISL)
			msg_pdbg2("Detailed South/ICH/PCH information is "
				  "probably not reliable, printing anyway.\n");
		prettyprint_ich_descriptor_straps_cougar(&desc->south);
		break;
	case CHIPSET_ICH_UNKNOWN:
		break;
	default:
		msg_pdbg2("The meaning of the descriptor straps are unknown yet.\n\n");
		break;
	}
}

static void prettyprint_rdid(uint32_t reg_val)
{
	uint8_t mid = reg_val & 0xFF;
	uint16_t did = ((reg_val >> 16) & 0xFF) | (reg_val & 0xFF00);
	msg_pdbg2("Manufacturer ID 0x%02x, Device ID 0x%04x\n", mid, did);
}

void prettyprint_ich_descriptor_upper_map(const struct ich_desc_upper_map *umap)
{
	int i;
	msg_pdbg2("=== Upper Map Section ===\n");
	msg_pdbg2("FLUMAP1  0x%08"PRIx32"\n", umap->FLUMAP1);
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	msg_pdbg2("VTL (length in DWORDS) = %d\n", umap->VTL);
	msg_pdbg2("VTBA (base address)    = 0x%6.6"PRIx32"\n", getVTBA(umap));
	msg_pdbg2("\n");

	msg_pdbg2("VSCC Table: %d entries\n", umap->VTL/2);
	for (i = 0; i < umap->VTL/2; i++) {
		uint32_t jid = umap->vscc_table[i].JID;
		uint32_t vscc = umap->vscc_table[i].VSCC;
		msg_pdbg2("  JID%d  = 0x%08"PRIx32"\n", i, jid);
		msg_pdbg2("  VSCC%d = 0x%08"PRIx32"\n", i, vscc);
		msg_pdbg2("    "); /* indentation */
		prettyprint_rdid(jid);
		msg_pdbg2("    "); /* indentation */
		prettyprint_ich_reg_vscc(vscc, 0, false);
	}
	msg_pdbg2("\n");
}

static inline void warn_peculiar_desc(const char *const name)
{
	msg_pwarn("Peculiar flash descriptor, assuming %s compatibility.\n", name);
}

/*
 * Guesses a minimum chipset version based on the maximum number of
 * soft straps per generation and presence of the MIP base (MDTBA).
 */
static enum ich_chipset guess_ich_chipset_from_content(const struct ich_desc_content *const content,
						       const struct ich_desc_upper_map *const upper)
{
	if (content->ICCRIBA == 0x00) {
		if (content->MSL == 0 && content->ISL <= 2)
			return CHIPSET_ICH8;
		if (content->ISL <= 2)
			return CHIPSET_ICH9;
		if (content->ISL <= 10)
			return CHIPSET_ICH10;
		if (content->ISL <= 16)
			return CHIPSET_5_SERIES_IBEX_PEAK;
		if (content->FLMAP2 == 0) {
			if (content->ISL == 19)
				return CHIPSET_APOLLO_LAKE;
			if (content->ISL == 23)
				return CHIPSET_GEMINI_LAKE;
			warn_peculiar_desc("Gemini Lake");
			return CHIPSET_GEMINI_LAKE;
		}
		if (content->ISL == 0x50)
			return CHIPSET_C740_SERIES_EMMITSBURG;
		warn_peculiar_desc("Ibex Peak");
		return CHIPSET_5_SERIES_IBEX_PEAK;
	} else if (upper->MDTBA == 0x00) {
		if (content->ICCRIBA < 0x31 && content->FMSBA < 0x30) {
			if (content->MSL == 0 && content->ISL <= 17)
				return CHIPSET_BAYTRAIL;
			if (content->MSL <= 1 && content->ISL <= 18)
				return CHIPSET_6_SERIES_COUGAR_POINT;
			if (content->MSL <= 1 && content->ISL <= 21)
				return CHIPSET_8_SERIES_LYNX_POINT;
			warn_peculiar_desc("Lynx Point");
			return CHIPSET_8_SERIES_LYNX_POINT;
		}
		if (content->NM == 6) {
			if (content->ICCRIBA <= 0x34)
				return CHIPSET_C620_SERIES_LEWISBURG;
			warn_peculiar_desc("C620 series");
			return CHIPSET_C620_SERIES_LEWISBURG;
		}
		if (content->ICCRIBA == 0x31)
			return CHIPSET_100_SERIES_SUNRISE_POINT;
		warn_peculiar_desc("100 series");
		return CHIPSET_100_SERIES_SUNRISE_POINT;
	} else {
		if (content->ICCRIBA == 0x34)
			return CHIPSET_300_SERIES_CANNON_POINT;
		if (content->CSSL == 0x11) {
			if (content->CSSO == 0x68)
				return CHIPSET_500_SERIES_TIGER_POINT;
			else if (content->CSSO == 0x5c)
				return CHIPSET_600_SERIES_ALDER_POINT;
		}
		if (content->CSSL == 0x14)
			return CHIPSET_600_SERIES_ALDER_POINT;
		if (content->CSSL == 0x03) {
			if (content->CSSO == 0x58)
				return CHIPSET_ELKHART_LAKE;
			else if (content->CSSO == 0x6c)
				return CHIPSET_JASPER_LAKE;
			else if (content->CSSO == 0x70)
				return CHIPSET_METEOR_LAKE;
			else if (content->CSSO == 0x60)
				return CHIPSET_PANTHER_LAKE;
		}
		msg_pwarn("Unknown flash descriptor, assuming 500 series compatibility.\n");
		return CHIPSET_500_SERIES_TIGER_POINT;
	}
}

/*
 * As an additional measure, we check the read frequency like `ifdtool`.
 * The frequency value 6 (17MHz) was reserved before Skylake and is the
 * only valid value since. Skylake is currently the most important dis-
 * tinction because of the dropped number of regions field (NR).
 */
static enum ich_chipset guess_ich_chipset(const struct ich_desc_content *const content,
					  const struct ich_desc_component *const component,
					  const struct ich_desc_upper_map *const upper)
{
	const enum ich_chipset guess = guess_ich_chipset_from_content(content, upper);

	switch (guess) {
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_700_SERIES_RAPTOR_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_PANTHER_LAKE:
	case CHIPSET_GEMINI_LAKE:
	case CHIPSET_JASPER_LAKE:
	case CHIPSET_ELKHART_LAKE:
		/* `freq_read` was repurposed, so can't check on it any more. */
		break;
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_C740_SERIES_EMMITSBURG:
	case CHIPSET_APOLLO_LAKE:
		if (component->modes.freq_read != 6) {
			msg_pwarn("\nThe flash descriptor looks like a Skylake/Sunrise Point descriptor.\n"
				  "However, the read frequency isn't set to 17MHz (the only valid value).\n"
				  "Please report this message, the output of `ich_descriptors_tool` for\n"
				  "your descriptor and the output of `lspci -nn` to flashrom@flashrom.org\n\n");
		}
		break;
	default:
		if (component->modes.freq_read == 6) {
			msg_pwarn("\nThe flash descriptor has the read frequency set to 17MHz. However,\n"
				  "it doesn't look like a Skylake/Sunrise Point compatible descriptor.\n"
				  "Please report this message, the output of `ich_descriptors_tool` for\n"
				  "your descriptor and the output of `lspci -nn` to flashrom@flashrom.org\n\n");
		}
	}
	return guess;
}

/* len is the length of dump in bytes */
int read_ich_descriptors_from_dump(const uint32_t *const dump, const size_t len,
				   enum ich_chipset *const cs, struct ich_descriptors *const desc)
{
	ssize_t i, max_count;
	size_t pch_bug_offset = 0;

	if (dump == NULL || desc == NULL)
		return ICH_RET_PARAM;

	if (dump[0] != DESCRIPTOR_MODE_SIGNATURE) {
		if (dump[4] == DESCRIPTOR_MODE_SIGNATURE)
			pch_bug_offset = 4;
		else
			return ICH_RET_ERR;
	}

	/* map */
	if (len < (4 + pch_bug_offset) * 4)
		return ICH_RET_OOB;
	desc->content.FLVALSIG	= dump[0 + pch_bug_offset];
	desc->content.FLMAP0	= dump[1 + pch_bug_offset];
	desc->content.FLMAP1	= dump[2 + pch_bug_offset];
	desc->content.FLMAP2	= dump[3 + pch_bug_offset];

	/* component */
	if (len < getFCBA(&desc->content) + 3 * 4)
		return ICH_RET_OOB;
	desc->component.FLCOMP	= dump[(getFCBA(&desc->content) >> 2) + 0];
	desc->component.FLILL	= dump[(getFCBA(&desc->content) >> 2) + 1];
	desc->component.FLPB	= dump[(getFCBA(&desc->content) >> 2) + 2];

	/* upper map */
	desc->upper.FLUMAP1 = dump[(UPPER_MAP_OFFSET >> 2) + 0];

	/* VTL is 8 bits long. Quote from the Ibex Peak SPI programming guide:
	 * "Identifies the 1s based number of DWORDS contained in the VSCC
	 * Table. Each SPI component entry in the table is 2 DWORDS long." So
	 * the maximum of 255 gives us 127.5 SPI components(!?) 8 bytes each. A
	 * check ensures that the maximum offset actually accessed is available.
	 */
	if (len < getVTBA(&desc->upper) + (desc->upper.VTL / 2 * 8))
		return ICH_RET_OOB;

	for (i = 0; i < desc->upper.VTL/2; i++) {
		desc->upper.vscc_table[i].JID  = dump[(getVTBA(&desc->upper) >> 2) + i * 2 + 0];
		desc->upper.vscc_table[i].VSCC = dump[(getVTBA(&desc->upper) >> 2) + i * 2 + 1];
	}

	if (*cs == CHIPSET_ICH_UNKNOWN) {
		*cs = guess_ich_chipset(&desc->content, &desc->component, &desc->upper);
		prettyprint_ich_chipset(*cs);
	}

	/* region */
	const ssize_t nr = ich_number_of_regions(*cs, &desc->content);
	if (nr < 0 || len < getFRBA(&desc->content) + (size_t)nr * 4)
		return ICH_RET_OOB;
	for (i = 0; i < nr; i++)
		desc->region.FLREGs[i] = dump[(getFRBA(&desc->content) >> 2) + i];

	/* master */
	const ssize_t nm = ich_number_of_masters(*cs, &desc->content);
	if (nm < 0 || len < getFMBA(&desc->content) + (size_t)nm * 4)
		return ICH_RET_OOB;
	for (i = 0; i < nm; i++)
		desc->master.FLMSTRs[i] = dump[(getFMBA(&desc->content) >> 2) + i];

	/* MCH/PROC (aka. North) straps */
	if (len < getFMSBA(&desc->content) + desc->content.MSL * 4)
		return ICH_RET_OOB;

	/* limit the range to be written */
	max_count = MIN(sizeof(desc->north.STRPs) / 4, desc->content.MSL);
	for (i = 0; i < max_count; i++)
		desc->north.STRPs[i] = dump[(getFMSBA(&desc->content) >> 2) + i];

	/* ICH/PCH (aka. South) straps */
	if (len < getFISBA(&desc->content) + desc->content.ISL * 4)
		return ICH_RET_OOB;

	/* limit the range to be written */
	max_count = MIN(sizeof(desc->south.STRPs) / 4, desc->content.ISL);
	for (i = 0; i < max_count; i++)
		desc->south.STRPs[i] = dump[(getFISBA(&desc->content) >> 2) + i];

	return ICH_RET_OK;
}

#ifndef ICH_DESCRIPTORS_FROM_DUMP_ONLY

/** Returns the integer representation of the component density with index
\em idx in bytes or -1 if the correct size can not be determined. */
int getFCBA_component_density(enum ich_chipset cs, const struct ich_descriptors *desc, uint8_t idx)
{
	if (idx > 1) {
		msg_perr("Only ICH SPI component index 0 or 1 are supported yet.\n");
		return -1;
	}

	if (desc->content.NC == 0 && idx > 0)
		return 0;

	uint8_t size_enc;
	uint8_t size_max;

	switch (cs) {
	case CHIPSET_ICH8:
	case CHIPSET_ICH9:
	case CHIPSET_ICH10:
	case CHIPSET_5_SERIES_IBEX_PEAK:
	case CHIPSET_6_SERIES_COUGAR_POINT:
	case CHIPSET_7_SERIES_PANTHER_POINT:
	case CHIPSET_BAYTRAIL:
		if (idx == 0) {
			size_enc = desc->component.dens_old.comp1_density;
		} else {
			size_enc = desc->component.dens_old.comp2_density;
		}
		size_max = 5;
		break;
	case CHIPSET_8_SERIES_LYNX_POINT:
	case CHIPSET_8_SERIES_LYNX_POINT_LP:
	case CHIPSET_8_SERIES_WELLSBURG:
	case CHIPSET_9_SERIES_WILDCAT_POINT:
	case CHIPSET_9_SERIES_WILDCAT_POINT_LP:
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_C740_SERIES_EMMITSBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_700_SERIES_RAPTOR_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_PANTHER_LAKE:
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_GEMINI_LAKE:
	case CHIPSET_JASPER_LAKE:
	case CHIPSET_ELKHART_LAKE:
		if (idx == 0) {
			size_enc = desc->component.dens_new.comp1_density;
		} else {
			size_enc = desc->component.dens_new.comp2_density;
		}
		size_max = 7;
		break;
	case CHIPSET_ICH_UNKNOWN:
	default:
		msg_pwarn("Density encoding is unknown on this chipset.\n");
		return -1;
	}

	if (size_enc > size_max) {
		msg_perr("Density of ICH SPI component with index %d is invalid.\n"
			 "Encoded density is 0x%x while maximum allowed is 0x%x.\n",
			 idx, size_enc, size_max);
		return -1;
	}

	return (1 << (19 + size_enc));
}

/* Only used by ichspi.c */
#if CONFIG_INTERNAL == 1 && (defined(__i386__) || defined(__x86_64__))
static uint32_t read_descriptor_reg(enum ich_chipset cs, uint8_t section, uint16_t offset, void *spibar)
{
	uint32_t control = 0;
	uint32_t woffset, roffset;

	control |= (section << FDOC_FDSS_OFF) & FDOC_FDSS;
	control |= (offset << FDOC_FDSI_OFF) & FDOC_FDSI;

	switch (cs) {
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_C740_SERIES_EMMITSBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_700_SERIES_RAPTOR_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_PANTHER_LAKE:
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_GEMINI_LAKE:
	case CHIPSET_JASPER_LAKE:
	case CHIPSET_ELKHART_LAKE:
		woffset = PCH100_REG_FDOC;
		roffset = PCH100_REG_FDOD;
		break;
	default:
		woffset = ICH9_REG_FDOC;
		roffset = ICH9_REG_FDOD;
	}

	mmio_le_writel(control, spibar + woffset);
	return mmio_le_readl(spibar + roffset);
}

int read_ich_descriptors_via_fdo(enum ich_chipset cs, void *spibar, struct ich_descriptors *desc)
{
	ssize_t i;
	struct ich_desc_region *r = &desc->region;

	/* Test if bit-fields are working as expected.
	 * FIXME: Replace this with dynamic bitfield fixup
	 */
	for (i = 0; i < 4; i++)
		desc->region.FLREGs[i] = 0x5A << (i * 8);
	if (r->old_reg[0].base != 0x005A || r->old_reg[0].limit != 0x0000 ||
	    r->old_reg[1].base != 0x1A00 || r->old_reg[1].limit != 0x0000 ||
	    r->old_reg[2].base != 0x0000 || r->old_reg[2].limit != 0x005A ||
	    r->old_reg[3].base != 0x0000 || r->old_reg[3].limit != 0x1A00) {
		msg_pdbg("The combination of compiler and CPU architecture used"
			 "does not lay out bit-fields as expected, sorry.\n");
		msg_pspew("r->old_reg[0].base  = 0x%04X (0x005A)\n", r->old_reg[0].base);
		msg_pspew("r->old_reg[0].limit = 0x%04X (0x0000)\n", r->old_reg[0].limit);
		msg_pspew("r->old_reg[1].base  = 0x%04X (0x1A00)\n", r->old_reg[1].base);
		msg_pspew("r->old_reg[1].limit = 0x%04X (0x0000)\n", r->old_reg[1].limit);
		msg_pspew("r->old_reg[2].base  = 0x%04X (0x0000)\n", r->old_reg[2].base);
		msg_pspew("r->old_reg[2].limit = 0x%04X (0x005A)\n", r->old_reg[2].limit);
		msg_pspew("r->old_reg[3].base  = 0x%04X (0x0000)\n", r->old_reg[3].base);
		msg_pspew("r->old_reg[3].limit = 0x%04X (0x1A00)\n", r->old_reg[3].limit);
		return ICH_RET_ERR;
	}

	msg_pdbg2("Reading flash descriptors mapped by the chipset via FDOC/FDOD...");
	/* content section */
	desc->content.FLVALSIG	= read_descriptor_reg(cs, 0, 0, spibar);
	desc->content.FLMAP0	= read_descriptor_reg(cs, 0, 1, spibar);
	desc->content.FLMAP1	= read_descriptor_reg(cs, 0, 2, spibar);
	desc->content.FLMAP2	= read_descriptor_reg(cs, 0, 3, spibar);

	/* component section */
	desc->component.FLCOMP	= read_descriptor_reg(cs, 1, 0, spibar);
	desc->component.FLILL	= read_descriptor_reg(cs, 1, 1, spibar);
	desc->component.FLPB	= read_descriptor_reg(cs, 1, 2, spibar);

	/* region section */
	const ssize_t nr = ich_number_of_regions(cs, &desc->content);
	if (nr < 0) {
		msg_pdbg2("%s: number of regions too high (%d) - failed\n",
			  __func__, desc->content.NR + 1);
		return ICH_RET_ERR;
	}
	for (i = 0; i < nr; i++)
		desc->region.FLREGs[i] = read_descriptor_reg(cs, 2, i, spibar);

	/* master section */
	const ssize_t nm = ich_number_of_masters(cs, &desc->content);
	if (nm < 0) {
		msg_pdbg2("%s: number of masters too high (%d) - failed\n",
			  __func__, desc->content.NM + 1);
		return ICH_RET_ERR;
	}
	for (i = 0; i < nm; i++)
		desc->master.FLMSTRs[i] = read_descriptor_reg(cs, 3, i, spibar);

	/* Accessing the strap section via FDOC/D is only possible on ICH8 and
	 * reading the upper map is impossible on all chipsets, so don't bother.
	 */

	msg_pdbg2(" done.\n");
	return ICH_RET_OK;
}
#endif

/**
 * @brief Read a layout from the dump of an Intel ICH descriptor.
 *
 * @param layout Pointer where to store the layout.
 * @param dump   The descriptor dump to read from.
 * @param len    The length of the descriptor dump.
 *
 * @return 0 on success,
 *	   1 if the descriptor couldn't be parsed,
 *	   2 when out of memory.
 */
int layout_from_ich_descriptors(
		struct flashrom_layout **const layout,
		const void *const dump, const size_t len)
{
	static const char *const regions[] = {
		"fd", "bios", "me", "gbe", "pd", "reg5", "bios2", "reg7", "ec", "reg9", "ie",
		"10gbe", "reg12", "reg13", "reg14", "reg15"
	};

	struct ich_descriptors desc;
	enum ich_chipset cs = CHIPSET_ICH_UNKNOWN;
	int ret = read_ich_descriptors_from_dump(dump, len, &cs, &desc);
	if (ret) {
		msg_pdbg("%s():%d, returned with value %d.\n",
			__func__, __LINE__, ret);
		return 1;
	}

	if (flashrom_layout_new(layout))
		return 2;

	ssize_t i;
	const ssize_t nr = MIN(ich_number_of_regions(cs, &desc.content), (ssize_t)ARRAY_SIZE(regions));
	for (i = 0; i < nr; ++i) {
		const chipoff_t base = ICH_FREG_BASE(desc.region.FLREGs[i]);
		const chipoff_t limit = ICH_FREG_LIMIT(desc.region.FLREGs[i]);
		if (limit <= base)
			continue;
		if (flashrom_layout_add_region(*layout, base, limit, regions[i])) {
			flashrom_layout_release(*layout);
			*layout = NULL;
			return 2;
		}
	}
	return 0;
}

#endif /* ICH_DESCRIPTORS_FROM_DUMP_ONLY */
