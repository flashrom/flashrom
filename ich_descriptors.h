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
#ifndef __ICH_DESCRIPTORS_H__
#define __ICH_DESCRIPTORS_H__ 1

#include <stdint.h>

/* FIXME: Replace with generic return codes */
#define ICH_RET_OK	0
#define ICH_RET_ERR	-1
#define ICH_RET_WARN	-2
#define ICH_RET_PARAM	-3
#define ICH_RET_OOB	-4

#define ICH9_REG_FDOC		0xB0	/* 32 Bits Flash Descriptor Observability Control */
					/* 0-1: reserved */
#define FDOC_FDSI_OFF		2	/* 2-11: Flash Descriptor Section Index */
#define FDOC_FDSI		(0x3f << FDOC_FDSI_OFF)
#define FDOC_FDSS_OFF		12	/* 12-14: Flash Descriptor Section Select */
#define FDOC_FDSS		(0x3 << FDOC_FDSS_OFF)
					/* 15-31: reserved */

#define ICH9_REG_FDOD		0xB4	/* 32 Bits Flash Descriptor Observability Data */

/* Field locations and semantics for LVSCC, UVSCC and related words in the flash
 * descriptor are equal therefore they all share the same macros below. */
#define VSCC_BES_OFF		0	/* 0-1: Block/Sector Erase Size */
#define VSCC_BES			(0x3 << VSCC_BES_OFF)
#define VSCC_WG_OFF		2	/* 2: Write Granularity */
#define VSCC_WG				(0x1 << VSCC_WG_OFF)
#define VSCC_WSR_OFF		3	/* 3: Write Status Required */
#define VSCC_WSR			(0x1 << VSCC_WSR_OFF)
#define VSCC_WEWS_OFF		4	/* 4: Write Enable on Write Status */
#define VSCC_WEWS			(0x1 << VSCC_WEWS_OFF)
					/* 5-7: reserved */
#define VSCC_EO_OFF		8	/* 8-15: Erase Opcode */
#define VSCC_EO				(0xff << VSCC_EO_OFF)
					/* 16-22: reserved */
#define VSCC_VCL_OFF		23	/* 23: Vendor Component Lock */
#define VSCC_VCL			(0x1 << VSCC_VCL_OFF)
					/* 24-31: reserved */

#define ICH_FREG_BASE(flreg)  (((flreg) << 12) & 0x01fff000)
#define ICH_FREG_LIMIT(flreg) (((flreg) >>  4) & 0x01fff000)

/* Used to select the right descriptor printing function.
 * Currently only ICH8 and Ibex Peak are supported.
 */
enum ich_chipset {
	CHIPSET_ICH_UNKNOWN,
	CHIPSET_ICH7 = 7,
	CHIPSET_ICH8,
	CHIPSET_ICH9,
	CHIPSET_ICH10,
	CHIPSET_5_SERIES_IBEX_PEAK,
	CHIPSET_6_SERIES_COUGAR_POINT,
	CHIPSET_7_SERIES_PANTHER_POINT
};

void prettyprint_ich_reg_vscc(uint32_t reg_val, int verbosity);

struct ich_desc_content {
	uint32_t FLVALSIG;	/* 0x00 */
	union {			/* 0x04 */
		uint32_t FLMAP0;
		struct {
			uint32_t FCBA	:8, /* Flash Component Base Address */
				 NC	:2, /* Number Of Components */
					:6,
				 FRBA	:8, /* Flash Region Base Address */
				 NR	:3, /* Number Of Regions */
					:5;
		};
	};
	union {			/* 0x08 */
		uint32_t FLMAP1;
		struct {
			uint32_t FMBA	:8, /* Flash Master Base Address */
				 NM	:3, /* Number Of Masters */
					:5,
				 FISBA	:8, /* Flash ICH Strap Base Address */
				 ISL	:8; /* ICH Strap Length */
		};
	};
	union {			/* 0x0c */
		uint32_t FLMAP2;
		struct {
			uint32_t FMSBA	:8, /* Flash (G)MCH Strap Base Addr. */
				 MSL	:8, /* MCH Strap Length */
					:16;
		};
	};
};

struct ich_desc_component {
	union {			/* 0x00 */
		uint32_t FLCOMP; /* Flash Components Register */
		struct {
			uint32_t comp1_density	:3,
				 comp2_density	:3,
						:11,
				 freq_read	:3,
				 fastread	:1,
				 freq_fastread	:3,
				 freq_write	:3,
				 freq_read_id	:3,
						:2;
		};
	};
	union {			/* 0x04 */
		uint32_t FLILL; /* Flash Invalid Instructions Register */
		struct {
			uint32_t invalid_instr0	:8,
				 invalid_instr1	:8,
				 invalid_instr2	:8,
				 invalid_instr3	:8;
		};
	};
	union {			/* 0x08 */
		uint32_t FLPB; /* Flash Partition Boundary Register */
		struct {
			uint32_t FPBA	:13, /* Flash Partition Boundary Addr */
					:19;
		};
	};
};

struct ich_desc_region {
	union {
		uint32_t FLREGs[5];
		struct {
			struct { /* FLREG0 Flash Descriptor */
				uint32_t reg0_base	:13,
							:3,
					 reg0_limit	:13,
							:3;
			};
			struct { /* FLREG1 BIOS */
				uint32_t reg1_base	:13,
							:3,
					 reg1_limit	:13,
							:3;
			};
			struct { /* FLREG2 ME */
				uint32_t reg2_base	:13,
							:3,
					 reg2_limit	:13,
							:3;
			};
			struct { /* FLREG3 GbE */
				uint32_t reg3_base	:13,
							:3,
					 reg3_limit	:13,
							:3;
			};
			struct { /* FLREG4 Platform */
				uint32_t reg4_base	:13,
							:3,
					 reg4_limit	:13,
							:3;
			};
		};
	};
};

struct ich_desc_master {
	union {
		uint32_t FLMSTR1;
		struct {
			uint32_t BIOS_req_ID	:16,
				 BIOS_descr_r	:1,
				 BIOS_BIOS_r	:1,
				 BIOS_ME_r	:1,
				 BIOS_GbE_r	:1,
				 BIOS_plat_r	:1,
						:3,
				 BIOS_descr_w	:1,
				 BIOS_BIOS_w	:1,
				 BIOS_ME_w	:1,
				 BIOS_GbE_w	:1,
				 BIOS_plat_w	:1,
						:3;
		};
	};
	union {
		uint32_t FLMSTR2;
		struct {
			uint32_t ME_req_ID	:16,
				 ME_descr_r	:1,
				 ME_BIOS_r	:1,
				 ME_ME_r	:1,
				 ME_GbE_r	:1,
				 ME_plat_r	:1,
						:3,
				 ME_descr_w	:1,
				 ME_BIOS_w	:1,
				 ME_ME_w	:1,
				 ME_GbE_w	:1,
				 ME_plat_w	:1,
						:3;
		};
	};
	union {
		uint32_t FLMSTR3;
		struct {
			uint32_t GbE_req_ID	:16,
				 GbE_descr_r	:1,
				 GbE_BIOS_r	:1,
				 GbE_ME_r	:1,
				 GbE_GbE_r	:1,
				 GbE_plat_r	:1,
						:3,
				 GbE_descr_w	:1,
				 GbE_BIOS_w	:1,
				 GbE_ME_w	:1,
				 GbE_GbE_w	:1,
				 GbE_plat_w	:1,
						:3;
		};
	};
};

struct ich_descriptors {
	struct ich_desc_content content;
	struct ich_desc_component component;
	struct ich_desc_region region;
	struct ich_desc_master master;
};

void prettyprint_ich_descriptors(enum ich_chipset, const struct ich_descriptors *desc);

void prettyprint_ich_descriptor_content(const struct ich_desc_content *content);
void prettyprint_ich_descriptor_component(const struct ich_descriptors *desc);
void prettyprint_ich_descriptor_region(const struct ich_descriptors *desc);
void prettyprint_ich_descriptor_master(const struct ich_desc_master *master);

int read_ich_descriptors_via_fdo(void *spibar, struct ich_descriptors *desc);
int getFCBA_component_density(const struct ich_descriptors *desc, uint8_t idx);

#endif /* __ICH_DESCRIPTORS_H__ */
#endif /* defined(__i386__) || defined(__x86_64__) */
