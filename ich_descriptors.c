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
#include "flash.h" /* for msg_* */
#include "programmer.h"

void prettyprint_ich_reg_vscc(uint32_t reg_val, int verbosity)
{
	print(verbosity, "BES=0x%x, ",	(reg_val & VSCC_BES)  >> VSCC_BES_OFF);
	print(verbosity, "WG=%d, ",	(reg_val & VSCC_WG)   >> VSCC_WG_OFF);
	print(verbosity, "WSR=%d, ",	(reg_val & VSCC_WSR)  >> VSCC_WSR_OFF);
	print(verbosity, "WEWS=%d, ",	(reg_val & VSCC_WEWS) >> VSCC_WEWS_OFF);
	print(verbosity, "EO=0x%x, ",	(reg_val & VSCC_EO)   >> VSCC_EO_OFF);
	print(verbosity, "VCL=%d\n",	(reg_val & VSCC_VCL)  >> VSCC_VCL_OFF);
}

#define getFCBA(cont)	(((cont)->FLMAP0 <<  4) & 0x00000ff0)
#define getFRBA(cont)	(((cont)->FLMAP0 >> 12) & 0x00000ff0)
#define getFMBA(cont)	(((cont)->FLMAP1 <<  4) & 0x00000ff0)
#define getFISBA(cont)	(((cont)->FLMAP1 >> 12) & 0x00000ff0)
#define getFMSBA(cont)	(((cont)->FLMAP2 <<  4) & 0x00000ff0)

void prettyprint_ich_descriptors(enum ich_chipset cs, const struct ich_descriptors *desc)
{
	prettyprint_ich_descriptor_content(&desc->content);
	prettyprint_ich_descriptor_component(desc);
	prettyprint_ich_descriptor_region(desc);
	prettyprint_ich_descriptor_master(&desc->master);
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
	msg_pdbg2("NR          (Number of Regions):                 %5d\n",
		  cont->NR + 1);
	msg_pdbg2("FRBA        (Flash Region Base Address):         0x%03x\n",
		  getFRBA(cont));
	msg_pdbg2("NC          (Number of Components):              %5d\n",
		  cont->NC + 1);
	msg_pdbg2("FCBA        (Flash Component Base Address):      0x%03x\n",
		  getFCBA(cont));
	msg_pdbg2("ISL         (ICH/PCH Strap Length):              %5d\n",
		  cont->ISL);
	msg_pdbg2("FISBA/FPSBA (Flash ICH/PCH Strap Base Address):  0x%03x\n",
		  getFISBA(cont));
	msg_pdbg2("NM          (Number of Masters):                 %5d\n",
		  cont->NM + 1);
	msg_pdbg2("FMBA        (Flash Master Base Address):         0x%03x\n",
		  getFMBA(cont));
	msg_pdbg2("MSL/PSL     (MCH/PROC Strap Length):             %5d\n",
		  cont->MSL);
	msg_pdbg2("FMSBA       (Flash MCH/PROC Strap Base Address): 0x%03x\n",
		  getFMSBA(cont));
	msg_pdbg2("\n");
}

void prettyprint_ich_descriptor_component(const struct ich_descriptors *desc)
{
	static const char * const freq_str[8] = {
		"20 MHz",	/* 000 */
		"33 MHz",	/* 001 */
		"reserved",	/* 010 */
		"reserved",	/* 011 */
		"50 MHz",	/* 100 */
		"reserved",	/* 101 */
		"reserved",	/* 110 */
		"reserved"	/* 111 */
	};
	static const char * const size_str[8] = {
		"512 kB",	/* 000 */
		"  1 MB",	/* 001 */
		"  2 MB",	/* 010 */
		"  4 MB",	/* 011 */
		"  8 MB",	/* 100 */
		" 16 MB",	/* 101 */
		"reserved",	/* 110 */
		"reserved",	/* 111 */
	};

	msg_pdbg2("=== Component Section ===\n");
	msg_pdbg2("FLCOMP   0x%08x\n", desc->component.FLCOMP);
	msg_pdbg2("FLILL    0x%08x\n", desc->component.FLILL );
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	msg_pdbg2("Component 1 density:           %s\n",
		  size_str[desc->component.comp1_density]);
	if (desc->content.NC)
		msg_pdbg2("Component 2 density:           %s\n",
			  size_str[desc->component.comp2_density]);
	else
		msg_pdbg2("Component 2 is not used.\n");
	msg_pdbg2("Read Clock Frequency:           %s\n",
		  freq_str[desc->component.freq_read]);
	msg_pdbg2("Read ID and Status Clock Freq.: %s\n",
		  freq_str[desc->component.freq_read_id]);
	msg_pdbg2("Write and Erase Clock Freq.:    %s\n",
		  freq_str[desc->component.freq_write]);
	msg_pdbg2("Fast Read is %ssupported.\n",
		  desc->component.fastread ? "" : "not ");
	if (desc->component.fastread)
		msg_pdbg2("Fast Read Clock Frequency:      %s\n",
			  freq_str[desc->component.freq_fastread]);
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
	if (nr >= 5) {
		msg_pdbg2("%s: number of regions too high (%d).\n", __func__,
			 nr);
		return;
	}
	for (i = 0; i <= nr; i++)
		msg_pdbg2("FLREG%d   0x%08x\n", i, desc->region.FLREGs[i]);
	msg_pdbg2("\n");

	msg_pdbg2("--- Details ---\n");
	for (i = 0; i <= nr; i++)
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

/** Returns the integer representation of the component density with index
idx in bytes or 0 if a correct size can not be determined. */
int getFCBA_component_density(const struct ich_descriptors *desc, uint8_t idx)
{
	uint8_t size_enc;
	
	switch(idx) {
	case 0:
		size_enc = desc->component.comp1_density;
		break;
	case 1:
		if (desc->content.NC == 0)
			return 0;
		size_enc = desc->component.comp2_density;
		break;
	default:
		msg_perr("Only ICH SPI component index 0 or 1 are supported "
			 "yet.\n");
		return 0;
	}
	if (size_enc > 5) {
		msg_perr("Density of ICH SPI component with index %d is "
			 "invalid. Encoded density is 0x%x.\n", idx, size_enc);
		return 0;
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

	msg_pdbg2("Reading flash descriptors "
		 "mapped by the chipset via FDOC/FDOD...");
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
	if (nr >= 5) {
		msg_pdbg2("%s: number of regions too high (%d) - failed\n",
			  __func__, nr);
		return ICH_RET_ERR;
	}
	for (i = 0; i <= nr; i++)
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
#endif /* defined(__i386__) || defined(__x86_64__) */
