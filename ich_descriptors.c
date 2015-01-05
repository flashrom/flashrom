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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#if defined(__i386__) || defined(__x86_64__)

#include "ich_descriptors.h"

#ifdef ICH_DESCRIPTORS_FROM_DUMP

#include <stdio.h>
#define print(t, ...) printf(__VA_ARGS__)
#define DESCRIPTOR_MODE_SIGNATURE 0x0ff0a55a
/* The upper map is located in the word before the 256B-long OEM section at the
 * end of the 4kB-long flash descriptor.
 */
#define UPPER_MAP_OFFSET (4096 - 256 - 4)
#define getVTBA(flumap)	(((flumap)->FLUMAP1 << 4) & 0x00000ff0)

#else /* ICH_DESCRIPTORS_FROM_DUMP */

#include "flash.h" /* for msg_* */
#include "programmer.h"

#endif /* ICH_DESCRIPTORS_FROM_DUMP */

#ifndef min
#define min(a, b) (a < b) ? a : b
#endif

void prettyprint_ich_reg_vscc(uint32_t reg_val, int verbosity, bool print_vcl)
{
	print(verbosity, "BES=0x%x, ",	(reg_val & VSCC_BES)  >> VSCC_BES_OFF);
	print(verbosity, "WG=%d, ",	(reg_val & VSCC_WG)   >> VSCC_WG_OFF);
	print(verbosity, "WSR=%d, ",	(reg_val & VSCC_WSR)  >> VSCC_WSR_OFF);
	print(verbosity, "WEWS=%d, ",	(reg_val & VSCC_WEWS) >> VSCC_WEWS_OFF);
	print(verbosity, "EO=0x%x",	(reg_val & VSCC_EO)   >> VSCC_EO_OFF);
	if (print_vcl)
		print(verbosity, ", VCL=%d", (reg_val & VSCC_VCL)  >> VSCC_VCL_OFF);
	print(verbosity, "\n");
}

#define getFCBA(cont)	(((cont)->FLMAP0 <<  4) & 0x00000ff0)
#define getFRBA(cont)	(((cont)->FLMAP0 >> 12) & 0x00000ff0)
#define getFMBA(cont)	(((cont)->FLMAP1 <<  4) & 0x00000ff0)
#define getFISBA(cont)	(((cont)->FLMAP1 >> 12) & 0x00000ff0)
#define getFMSBA(cont)	(((cont)->FLMAP2 <<  4) & 0x00000ff0)

void prettyprint_ich_descriptors(enum ich_chipset cs, const struct ich_descriptors *desc)
{
	prettyprint_ich_descriptor_content(&desc->content);
	prettyprint_ich_descriptor_component(cs, desc);
	prettyprint_ich_descriptor_region(desc);
	prettyprint_ich_descriptor_master(&desc->master);
#ifdef ICH_DESCRIPTORS_FROM_DUMP
	if (cs >= CHIPSET_ICH8) {
		prettyprint_ich_descriptor_upper_map(&desc->upper);
		prettyprint_ich_descriptor_straps(cs, desc);
	}
#endif /* ICH_DESCRIPTORS_FROM_DUMP */
}

void prettyprint_ich_descriptor_content(const struct ich_desc_content *cont)
{
	msg_pdbg2("=== Content Section ===\n");
	msg_pdbg2("FLVALSIG 0x%08x\n", cont->FLVALSIG);
	msg_pdbg2("FLMAP0   0x%08x\n", cont->FLMAP0);
	msg_pdbg2("FLMAP1   0x%08x\n", cont->FLMAP1);
	msg_pdbg2("FLMAP2   0x%08x\n", cont->FLMAP2);
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	msg_pdbg2("NR          (Number of Regions):                 %5d\n",	cont->NR + 1);
	msg_pdbg2("FRBA        (Flash Region Base Address):         0x%03x\n",	getFRBA(cont));
	msg_pdbg2("NC          (Number of Components):              %5d\n",	cont->NC + 1);
	msg_pdbg2("FCBA        (Flash Component Base Address):      0x%03x\n",	getFCBA(cont));
	msg_pdbg2("ISL         (ICH/PCH Strap Length):              %5d\n",	cont->ISL);
	msg_pdbg2("FISBA/FPSBA (Flash ICH/PCH Strap Base Address):  0x%03x\n",	getFISBA(cont));
	msg_pdbg2("NM          (Number of Masters):                 %5d\n",	cont->NM + 1);
	msg_pdbg2("FMBA        (Flash Master Base Address):         0x%03x\n",	getFMBA(cont));
	msg_pdbg2("MSL/PSL     (MCH/PROC Strap Length):             %5d\n",	cont->MSL);
	msg_pdbg2("FMSBA       (Flash MCH/PROC Strap Base Address): 0x%03x\n",	getFMSBA(cont));
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
	case CHIPSET_9_SERIES_WILDCAT_POINT: {
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
	static const char * const freq_str[8] = {
		"20 MHz",	/* 000 */
		"33 MHz",	/* 001 */
		"reserved",	/* 010 */
		"reserved",	/* 011 */
		"50 MHz",	/* 100 */ /* New since Ibex Peak */
		"reserved",	/* 101 */
		"reserved",	/* 110 */
		"reserved"	/* 111 */
	};

	switch (cs) {
	case CHIPSET_ICH8:
	case CHIPSET_ICH9:
	case CHIPSET_ICH10:
		if (value > 1)
			return "reserved";
	case CHIPSET_5_SERIES_IBEX_PEAK:
	case CHIPSET_6_SERIES_COUGAR_POINT:
	case CHIPSET_7_SERIES_PANTHER_POINT:
	case CHIPSET_8_SERIES_LYNX_POINT:
	case CHIPSET_BAYTRAIL:
	case CHIPSET_8_SERIES_LYNX_POINT_LP:
	case CHIPSET_8_SERIES_WELLSBURG:
	case CHIPSET_9_SERIES_WILDCAT_POINT:
		return freq_str[value];
	case CHIPSET_ICH_UNKNOWN:
	default:
		return "unknown";
	}
}

void prettyprint_ich_descriptor_component(enum ich_chipset cs, const struct ich_descriptors *desc)
{

	msg_pdbg2("=== Component Section ===\n");
	msg_pdbg2("FLCOMP   0x%08x\n", desc->component.FLCOMP);
	msg_pdbg2("FLILL    0x%08x\n", desc->component.FLILL );
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	msg_pdbg2("Component 1 density:            %s\n", pprint_density(cs, desc, 0));
	if (desc->content.NC)
		msg_pdbg2("Component 2 density:            %s\n", pprint_density(cs, desc, 1));
	else
		msg_pdbg2("Component 2 is not used.\n");
	msg_pdbg2("Read Clock Frequency:           %s\n", pprint_freq(cs, desc->component.modes.freq_read));
	msg_pdbg2("Read ID and Status Clock Freq.: %s\n", pprint_freq(cs, desc->component.modes.freq_read_id));
	msg_pdbg2("Write and Erase Clock Freq.:    %s\n", pprint_freq(cs, desc->component.modes.freq_write));
	msg_pdbg2("Fast Read is %ssupported.\n", desc->component.modes.fastread ? "" : "not ");
	if (desc->component.modes.fastread)
		msg_pdbg2("Fast Read Clock Frequency:      %s\n",
			  pprint_freq(cs, desc->component.modes.freq_fastread));
	if (cs > CHIPSET_6_SERIES_COUGAR_POINT)
		msg_pdbg2("Dual Output Fast Read Support:  %sabled\n",
			  desc->component.modes.dual_output ? "dis" : "en");
	if (desc->component.FLILL == 0)
		msg_pdbg2("No forbidden opcodes.\n");
	else {
		msg_pdbg2("Invalid instruction 0:          0x%02x\n",
			  desc->component.invalid_instr0);
		msg_pdbg2("Invalid instruction 1:          0x%02x\n",
			  desc->component.invalid_instr1);
		msg_pdbg2("Invalid instruction 2:          0x%02x\n",
			  desc->component.invalid_instr2);
		msg_pdbg2("Invalid instruction 3:          0x%02x\n",
			  desc->component.invalid_instr3);
	}
	msg_pdbg2("\n");
}

static void pprint_freg(const struct ich_desc_region *reg, uint32_t i)
{
	static const char *const region_names[5] = {
		"Descr.", "BIOS", "ME", "GbE", "Platf."
	};
	if (i >= 5) {
		msg_pdbg2("%s: region index too high.\n", __func__);
		return;
	}
	uint32_t base = ICH_FREG_BASE(reg->FLREGs[i]);
	uint32_t limit = ICH_FREG_LIMIT(reg->FLREGs[i]);
	msg_pdbg2("Region %d (%-6s) ", i, region_names[i]);
	if (base > limit)
		msg_pdbg2("is unused.\n");
	else
		msg_pdbg2("0x%08x - 0x%08x\n", base, limit | 0x0fff);
}

void prettyprint_ich_descriptor_region(const struct ich_descriptors *desc)
{
	uint8_t i;
	uint8_t nr = desc->content.NR + 1;
	msg_pdbg2("=== Region Section ===\n");
	if (nr > 5) {
		msg_pdbg2("%s: number of regions too high (%d).\n", __func__,
			 nr);
		return;
	}
	for (i = 0; i < 5; i++)
		msg_pdbg2("FLREG%d   0x%08x\n", i, desc->region.FLREGs[i]);
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	for (i = 0; i < 5; i++)
		pprint_freg(&desc->region, i);
	msg_pdbg2("\n");
}

void prettyprint_ich_descriptor_master(const struct ich_desc_master *mstr)
{
	msg_pdbg2("=== Master Section ===\n");
	msg_pdbg2("FLMSTR1  0x%08x\n", mstr->FLMSTR1);
	msg_pdbg2("FLMSTR2  0x%08x\n", mstr->FLMSTR2);
	msg_pdbg2("FLMSTR3  0x%08x\n", mstr->FLMSTR3);
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
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
	msg_pdbg2("\n");
}

#ifdef ICH_DESCRIPTORS_FROM_DUMP

void prettyprint_ich_descriptor_straps_ich8(const struct ich_descriptors *desc)
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

void prettyprint_ich_descriptor_pchstraps45678_56(const struct ich_desc_south_strap *s)
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

void prettyprint_ich_descriptor_pchstraps111213_56(const struct ich_desc_south_strap *s)
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

void prettyprint_ich_descriptor_straps_ibex(const struct ich_desc_south_strap *s)
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

void prettyprint_ich_descriptor_straps_cougar(const struct ich_desc_south_strap *s)
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

	if (sizeof(desc->north.STRPs) / 4 + 1 < desc->content.MSL) {
		max_count = sizeof(desc->north.STRPs) / 4 + 1;
		msg_pdbg2("MSL (%u) is greater than the current maximum of %u entries.\n",
			  desc->content.MSL, max_count + 1);
		msg_pdbg2("Only the first %u entries will be printed.\n", max_count);
	} else
		max_count = desc->content.MSL;

	msg_pdbg2("--- North/MCH/PROC (%d entries) ---\n", max_count);
	for (i = 0; i < max_count; i++)
		msg_pdbg2("STRP%-2d = 0x%08x\n", i, desc->north.STRPs[i]);
	msg_pdbg2("\n");

	if (sizeof(desc->south.STRPs) / 4 < desc->content.ISL) {
		max_count = sizeof(desc->south.STRPs) / 4;
		msg_pdbg2("ISL (%u) is greater than the current maximum of %u entries.\n",
			  desc->content.ISL, max_count);
		msg_pdbg2("Only the first %u entries will be printed.\n", max_count);
	} else
		max_count = desc->content.ISL;

	msg_pdbg2("--- South/ICH/PCH (%d entries) ---\n", max_count);
	for (i = 0; i < max_count; i++)
		msg_pdbg2("STRP%-2d = 0x%08x\n", i, desc->south.STRPs[i]);
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

void prettyprint_rdid(uint32_t reg_val)
{
	uint8_t mid = reg_val & 0xFF;
	uint16_t did = ((reg_val >> 16) & 0xFF) | (reg_val & 0xFF00);
	msg_pdbg2("Manufacturer ID 0x%02x, Device ID 0x%04x\n", mid, did);
}

void prettyprint_ich_descriptor_upper_map(const struct ich_desc_upper_map *umap)
{
	int i;
	msg_pdbg2("=== Upper Map Section ===\n");
	msg_pdbg2("FLUMAP1  0x%08x\n", umap->FLUMAP1);
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	msg_pdbg2("VTL (length in DWORDS) = %d\n", umap->VTL);
	msg_pdbg2("VTBA (base address)    = 0x%6.6x\n", getVTBA(umap));
	msg_pdbg2("\n");

	msg_pdbg2("VSCC Table: %d entries\n", umap->VTL/2);
	for (i = 0; i < umap->VTL/2; i++) {
		uint32_t jid = umap->vscc_table[i].JID;
		uint32_t vscc = umap->vscc_table[i].VSCC;
		msg_pdbg2("  JID%d  = 0x%08x\n", i, jid);
		msg_pdbg2("  VSCC%d = 0x%08x\n", i, vscc);
		msg_pdbg2("    "); /* indention */
		prettyprint_rdid(jid);
		msg_pdbg2("    "); /* indention */
		prettyprint_ich_reg_vscc(vscc, 0, false);
	}
	msg_pdbg2("\n");
}

/* len is the length of dump in bytes */
int read_ich_descriptors_from_dump(const uint32_t *dump, unsigned int len, struct ich_descriptors *desc)
{
	unsigned int i, max_count;
	uint8_t pch_bug_offset = 0;

	if (dump == NULL || desc == NULL)
		return ICH_RET_PARAM;

	if (dump[0] != DESCRIPTOR_MODE_SIGNATURE) {
		if (dump[4] == DESCRIPTOR_MODE_SIGNATURE)
			pch_bug_offset = 4;
		else
			return ICH_RET_ERR;
	}

	/* map */
	if (len < (4 + pch_bug_offset) * 4 - 1)
		return ICH_RET_OOB;
	desc->content.FLVALSIG	= dump[0 + pch_bug_offset];
	desc->content.FLMAP0	= dump[1 + pch_bug_offset];
	desc->content.FLMAP1	= dump[2 + pch_bug_offset];
	desc->content.FLMAP2	= dump[3 + pch_bug_offset];

	/* component */
	if (len < (getFCBA(&desc->content) + 3 * 4 - 1))
		return ICH_RET_OOB;
	desc->component.FLCOMP	= dump[(getFCBA(&desc->content) >> 2) + 0];
	desc->component.FLILL	= dump[(getFCBA(&desc->content) >> 2) + 1];
	desc->component.FLPB	= dump[(getFCBA(&desc->content) >> 2) + 2];

	/* region */
	if (len < (getFRBA(&desc->content) + 5 * 4 - 1))
		return ICH_RET_OOB;
	desc->region.FLREGs[0] = dump[(getFRBA(&desc->content) >> 2) + 0];
	desc->region.FLREGs[1] = dump[(getFRBA(&desc->content) >> 2) + 1];
	desc->region.FLREGs[2] = dump[(getFRBA(&desc->content) >> 2) + 2];
	desc->region.FLREGs[3] = dump[(getFRBA(&desc->content) >> 2) + 3];
	desc->region.FLREGs[4] = dump[(getFRBA(&desc->content) >> 2) + 4];

	/* master */
	if (len < (getFMBA(&desc->content) + 3 * 4 - 1))
		return ICH_RET_OOB;
	desc->master.FLMSTR1 = dump[(getFMBA(&desc->content) >> 2) + 0];
	desc->master.FLMSTR2 = dump[(getFMBA(&desc->content) >> 2) + 1];
	desc->master.FLMSTR3 = dump[(getFMBA(&desc->content) >> 2) + 2];

	/* upper map */
	desc->upper.FLUMAP1 = dump[(UPPER_MAP_OFFSET >> 2) + 0];

	/* VTL is 8 bits long. Quote from the Ibex Peak SPI programming guide:
	 * "Identifies the 1s based number of DWORDS contained in the VSCC
	 * Table. Each SPI component entry in the table is 2 DWORDS long." So
	 * the maximum of 255 gives us 127.5 SPI components(!?) 8 bytes each. A
	 * check ensures that the maximum offset actually accessed is available.
	 */
	if (len < (getVTBA(&desc->upper) + (desc->upper.VTL / 2 * 8) - 1))
		return ICH_RET_OOB;

	for (i = 0; i < desc->upper.VTL/2; i++) {
		desc->upper.vscc_table[i].JID  = dump[(getVTBA(&desc->upper) >> 2) + i * 2 + 0];
		desc->upper.vscc_table[i].VSCC = dump[(getVTBA(&desc->upper) >> 2) + i * 2 + 1];
	}

	/* MCH/PROC (aka. North) straps */
	if (len < getFMSBA(&desc->content) + desc->content.MSL * 4)
		return ICH_RET_OOB;

	/* limit the range to be written */
	max_count = min(sizeof(desc->north.STRPs) / 4, desc->content.MSL);
	for (i = 0; i < max_count; i++)
		desc->north.STRPs[i] = dump[(getFMSBA(&desc->content) >> 2) + i];

	/* ICH/PCH (aka. South) straps */
	if (len < getFISBA(&desc->content) + desc->content.ISL * 4)
		return ICH_RET_OOB;

	/* limit the range to be written */
	max_count = min(sizeof(desc->south.STRPs) / 4, desc->content.ISL);
	for (i = 0; i < max_count; i++)
		desc->south.STRPs[i] = dump[(getFISBA(&desc->content) >> 2) + i];

	return ICH_RET_OK;
}

#else /* ICH_DESCRIPTORS_FROM_DUMP */

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

static uint32_t read_descriptor_reg(uint8_t section, uint16_t offset, void *spibar)
{
	uint32_t control = 0;
	control |= (section << FDOC_FDSS_OFF) & FDOC_FDSS;
	control |= (offset << FDOC_FDSI_OFF) & FDOC_FDSI;
	mmio_le_writel(control, spibar + ICH9_REG_FDOC);
	return mmio_le_readl(spibar + ICH9_REG_FDOD);
}

int read_ich_descriptors_via_fdo(void *spibar, struct ich_descriptors *desc)
{
	uint8_t i;
	uint8_t nr;
	struct ich_desc_region *r = &desc->region;

	/* Test if bit-fields are working as expected.
	 * FIXME: Replace this with dynamic bitfield fixup
	 */
	for (i = 0; i < 4; i++)
		desc->region.FLREGs[i] = 0x5A << (i * 8);
	if (r->reg0_base != 0x005A || r->reg0_limit != 0x0000 ||
	    r->reg1_base != 0x1A00 || r->reg1_limit != 0x0000 ||
	    r->reg2_base != 0x0000 || r->reg2_limit != 0x005A ||
	    r->reg3_base != 0x0000 || r->reg3_limit != 0x1A00) {
		msg_pdbg("The combination of compiler and CPU architecture used"
			 "does not lay out bit-fields as expected, sorry.\n");
		msg_pspew("r->reg0_base  = 0x%04X (0x005A)\n", r->reg0_base);
		msg_pspew("r->reg0_limit = 0x%04X (0x0000)\n", r->reg0_limit);
		msg_pspew("r->reg1_base  = 0x%04X (0x1A00)\n", r->reg1_base);
		msg_pspew("r->reg1_limit = 0x%04X (0x0000)\n", r->reg1_limit);
		msg_pspew("r->reg2_base  = 0x%04X (0x0000)\n", r->reg2_base);
		msg_pspew("r->reg2_limit = 0x%04X (0x005A)\n", r->reg2_limit);
		msg_pspew("r->reg3_base  = 0x%04X (0x0000)\n", r->reg3_base);
		msg_pspew("r->reg3_limit = 0x%04X (0x1A00)\n", r->reg3_limit);
		return ICH_RET_ERR;
	}

	msg_pdbg2("Reading flash descriptors mapped by the chipset via FDOC/FDOD...");
	/* content section */
	desc->content.FLVALSIG	= read_descriptor_reg(0, 0, spibar);
	desc->content.FLMAP0	= read_descriptor_reg(0, 1, spibar);
	desc->content.FLMAP1	= read_descriptor_reg(0, 2, spibar);
	desc->content.FLMAP2	= read_descriptor_reg(0, 3, spibar);

	/* component section */
	desc->component.FLCOMP	= read_descriptor_reg(1, 0, spibar);
	desc->component.FLILL	= read_descriptor_reg(1, 1, spibar);
	desc->component.FLPB	= read_descriptor_reg(1, 2, spibar);

	/* region section */
	nr = desc->content.NR + 1;
	if (nr > 5) {
		msg_pdbg2("%s: number of regions too high (%d) - failed\n",
			  __func__, nr);
		return ICH_RET_ERR;
	}
	for (i = 0; i < 5; i++)
		desc->region.FLREGs[i] = read_descriptor_reg(2, i, spibar);

	/* master section */
	desc->master.FLMSTR1 = read_descriptor_reg(3, 0, spibar);
	desc->master.FLMSTR2 = read_descriptor_reg(3, 1, spibar);
	desc->master.FLMSTR3 = read_descriptor_reg(3, 2, spibar);

	/* Accessing the strap section via FDOC/D is only possible on ICH8 and
	 * reading the upper map is impossible on all chipsets, so don't bother.
	 */

	msg_pdbg2(" done.\n");
	return ICH_RET_OK;
}
#endif /* ICH_DESCRIPTORS_FROM_DUMP */
#endif /* defined(__i386__) || defined(__x86_64__) */
