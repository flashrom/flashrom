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
#include "programmer.h" /* for enum ich_chipset */

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

void prettyprint_ich_reg_vscc(uint32_t reg_val, int verbosity, bool print_vcl);

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
		/* FLCOMP encoding on various generations:
		 *
		 * Chipset/Generation	max_speed	dual_output	density
		 * 			[MHz]		bits		max.	bits
		 * ICH8:		33		N/A		5	0:2, 3:5
		 * ICH9:		33		N/A		5	0:2, 3:5
		 * ICH10:		33		N/A		5	0:2, 3:5
		 * Ibex Peak/5:		50		N/A		5	0:2, 3:5
		 * Cougar Point/6:	50		30		5	0:2, 3:5
		 * Patsburg:		50		30		5	0:2, 3:5
		 * Panther Point/7	50		30		5	0:2, 3:5
		 * Lynx Point/8:	50		30		7	0:3, 4:7
		 * Wildcat Point/9:	50		?? (multi I/O)	?	?:?, ?:?
		 */
		struct {
			uint32_t		:17,
				 freq_read	:3,
				 fastread	:1,
				 freq_fastread	:3,
				 freq_write	:3,
				 freq_read_id	:3,
				 dual_output	:1, /* new since Cougar Point/6 */
						:1;
		} modes;
		struct {
			uint32_t comp1_density	:3,
				 comp2_density	:3,
						:26;
		} dens_old;
		struct {
			uint32_t comp1_density	:4, /* new since Lynx Point/8 */
				 comp2_density	:4,
						:24;
		} dens_new;
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

#ifdef ICH_DESCRIPTORS_FROM_DUMP
struct ich_desc_north_strap {
	union {
		uint32_t STRPs[1]; /* current maximum: ich8 */
		struct { /* ich8 */
			struct { /* STRP2 (in the datasheet) */
				uint32_t MDB			:1,
								:31;
			};
		} ich8;
	};
};

struct ich_desc_south_strap {
	union {
		uint32_t STRPs[18]; /* current maximum: cougar point */
		struct { /* ich8 */
			struct { /* STRP1 */
				uint32_t ME_DISABLE		:1,
								:6,
					 TCOMODE		:1,
					 ASD			:7,
					 BMCMODE		:1,
								:3,
					 GLAN_PCIE_SEL		:1,
					 GPIO12_SEL		:2,
					 SPICS1_LANPHYPC_SEL	:1,
					 MESM2SEL		:1,
								:1,
					 ASD2			:7;
			};
		} ich8;
		struct { /* ibex peak */
			struct { /* STRP0 */
				uint32_t			:1,
					 cs_ss2			:1,
								:5,
					 SMB_EN			:1,
					 SML0_EN		:1,
					 SML1_EN		:1,
					 SML1FRQ		:2,
					 SMB0FRQ		:2,
					 SML0FRQ		:2,
								:4,
					 LANPHYPC_GP12_SEL	:1,
					 cs_ss1			:1,
								:2,
					 DMI_REQID_DIS		:1,
								:4,
					 BBBS			:2,
								:1;
			};
			struct { /* STRP1 */
				uint32_t cs_ss3			:4,
								:28;
			};
			struct { /* STRP2 */
				uint32_t			:8,
					 MESMASDEN		:1,
					 MESMASDA		:7,
								:8,
					 MESMI2CEN		:1,
					 MESMI2CA		:7;
			};
			struct { /* STRP3 */
				uint32_t			:32;
			};
			struct { /* STRP4 */
				uint32_t PHYCON			:2,
								:6,
					 GBEMAC_SMBUS_ADDR_EN	:1,
					 GBEMAC_SMBUS_ADDR	:7,
								:1,
					 GBEPHY_SMBUS_ADDR	:7,
								:8;
			};
			struct { /* STRP5 */
				uint32_t			:32;
			};
			struct { /* STRP6 */
				uint32_t			:32;
			};
			struct { /* STRP7 */
				uint32_t MESMA2UDID_VENDOR	:16,
					 MESMA2UDID_DEVICE	:16;
			};
			struct { /* STRP8 */
				uint32_t			:32;
			};
			struct { /* STRP9 */
				uint32_t PCIEPCS1		:2,
					 PCIEPCS2		:2,
					 PCIELR1		:1,
					 PCIELR2		:1,
					 DMILR			:1,
								:1,
					 PHY_PCIEPORTSEL	:3,
					 PHY_PCIE_EN		:1,
								:20;
			};
			struct { /* STRP10 */
				uint32_t			:1,
					 ME_BOOT_FLASH		:1,
					 cs_ss5			:1,
					 VE_EN			:1,
								:4,
					 MMDDE			:1,
					 MMADDR			:7,
					 cs_ss7			:1,
								:1,
					 ICC_SEL		:3,
					 MER_CL1		:1,
								:10;
			};
			struct { /* STRP11 */
				uint32_t SML1GPAEN		:1,
					 SML1GPA		:7,
								:16,
					 SML1I2CAEN		:1,
					 SML1I2CA		:7;
			};
			struct { /* STRP12 */
				uint32_t			:32;
			};
			struct { /* STRP13 */
				uint32_t			:32;
			};
			struct { /* STRP14 */
				uint32_t			:8,
					 VE_EN2			:1,
								:5,
					 VE_BOOT_FLASH		:1,
								:1,
					 BW_SSD			:1,
					 NVMHCI_EN		:1,
								:14;
			};
			struct { /* STRP15 */
				uint32_t			:3,
					 cs_ss6			:2,
								:1,
					 IWL_EN			:1,
								:1,
					 t209min		:2,
								:22;
			};
		} ibex;
		struct { /* cougar point */
			struct { /* STRP0 */
				uint32_t			:1,
					 cs_ss1			:1,
								:5,
					 SMB_EN			:1,
					 SML0_EN		:1,
					 SML1_EN		:1,
					 SML1FRQ		:2,
					 SMB0FRQ		:2,
					 SML0FRQ		:2,
								:4,
					 LANPHYPC_GP12_SEL	:1,
					 LINKSEC_DIS		:1,
								:2,
					 DMI_REQID_DIS		:1,
								:4,
					 BBBS			:2,
								:1;
			};
			struct { /* STRP1 */
				uint32_t cs_ss3			:4,
								:4,
					 cs_ss2			:1,
								:28;
			};
			struct { /* STRP2 */
				uint32_t			:8,
					 MESMASDEN		:1,
					 MESMASDA		:7,
					 MESMMCTPAEN		:1,
					 MESMMCTPA		:7,
					 MESMI2CEN		:1,
					 MESMI2CA		:7;
			};
			struct { /* STRP3 */
				uint32_t			:32;
			};
			struct { /* STRP4 */
				uint32_t PHYCON			:2,
								:6,
					 GBEMAC_SMBUS_ADDR_EN	:1,
					 GBEMAC_SMBUS_ADDR	:7,
								:1,
					 GBEPHY_SMBUS_ADDR	:7,
								:8;
			};
			struct { /* STRP5 */
				uint32_t			:32;
			};
			struct { /* STRP6 */
				uint32_t			:32;
			};
			struct { /* STRP7 */
				uint32_t MESMA2UDID_VENDOR	:16,
					 MESMA2UDID_DEVICE	:16;
			};
			struct { /* STRP8 */
				uint32_t			:32;
			};
			struct { /* STRP9 */
				uint32_t PCIEPCS1		:2,
					 PCIEPCS2		:2,
					 PCIELR1		:1,
					 PCIELR2		:1,
					 DMILR			:1,
					 cs_ss4			:1,
					 PHY_PCIEPORTSEL	:3,
					 PHY_PCIE_EN		:1,
								:2,
					 SUB_DECODE_EN		:1,
								:7,
					 PCHHOT_SML1ALERT_SEL	:1,
								:9;
			};
			struct { /* STRP10 */
				uint32_t			:1,
					 ME_BOOT_FLASH		:1,
								:6,
					 MDSMBE_EN		:1,
					 MDSMBE_ADD		:7,
								:2,
					 ICC_SEL		:3,
					 MER_CL1		:1,
					 ICC_PRO_SEL		:1,
					 Deep_SX_EN		:1,
					 ME_DBG_LAN		:1,
								:7;
			};
			struct { /* STRP11 */
				uint32_t SML1GPAEN		:1,
					 SML1GPA		:7,
								:16,
					 SML1I2CAEN		:1,
					 SML1I2CA		:7;
			};
			struct { /* STRP12 */
				uint32_t			:32;
			};
			struct { /* STRP13 */
				uint32_t			:32;
			};
			struct { /* STRP14 */
				uint32_t			:32;
			};
			struct { /* STRP15 */
				uint32_t cs_ss6			:6,
					 IWL_EN			:1,
					 cs_ss5			:2,
								:4,
					 SMLINK1_THERM_SEL	:1,
					 SLP_LAN_GP29_SEL	:1,
								:16;
			};
			struct { /* STRP16 */
				uint32_t			:32;
			};
			struct { /* STRP17 */
				uint32_t ICML			:1,
					 cs_ss7			:1,
								:30;
			};
		} cougar;
	};
};

struct ich_desc_upper_map {
	union {
		uint32_t FLUMAP1;		/* Flash Upper Map 1 */
		struct {
			uint32_t VTBA	:8,	/* ME VSCC Table Base Address */
				 VTL	:8,	/* ME VSCC Table Length */
					:16;
		};
	};
	struct {
		union {		/* JEDEC-ID Register */
			uint32_t JID;
			struct {
				uint32_t vid	:8, /* Vendor ID */
					 cid0	:8, /* Component ID 0 */
					 cid1	:8, /* Component ID 1 */
						:8;
			};
		};
		union {		/* Vendor Specific Component Capabilities */
			uint32_t VSCC;
			struct {
				uint32_t ubes	:2, /* Upper Block/Sector Erase Size */
					 uwg	:1, /* Upper Write Granularity */
					 uwsr	:1, /* Upper Write Status Required */
					 uwews	:1, /* Upper Write Enable on Write Status */
						:3,
					 ueo	:8, /* Upper Erase Opcode */
					 lbes	:2, /* Lower Block/Sector Erase Size */
					 lwg	:1, /* Lower Write Granularity */
					 lwsr	:1, /* Lower Write Status Required */
					 lwews	:1, /* Lower Write Enable on Write Status */
						:3,
					 leo	:16; /* Lower Erase Opcode */
			};
		};
	} vscc_table[128];
};
#endif /* ICH_DESCRIPTORS_FROM_DUMP */

struct ich_descriptors {
	struct ich_desc_content content;
	struct ich_desc_component component;
	struct ich_desc_region region;
	struct ich_desc_master master;
#ifdef ICH_DESCRIPTORS_FROM_DUMP
	struct ich_desc_north_strap north;
	struct ich_desc_south_strap south;
	struct ich_desc_upper_map upper;
#endif /* ICH_DESCRIPTORS_FROM_DUMP */
};

void prettyprint_ich_descriptors(enum ich_chipset cs, const struct ich_descriptors *desc);

void prettyprint_ich_descriptor_content(const struct ich_desc_content *content);
void prettyprint_ich_descriptor_component(enum ich_chipset cs, const struct ich_descriptors *desc);
void prettyprint_ich_descriptor_region(const struct ich_descriptors *desc);
void prettyprint_ich_descriptor_master(const struct ich_desc_master *master);

#ifdef ICH_DESCRIPTORS_FROM_DUMP

void prettyprint_ich_descriptor_upper_map(const struct ich_desc_upper_map *umap);
void prettyprint_ich_descriptor_straps(enum ich_chipset cs, const struct ich_descriptors *desc);
int read_ich_descriptors_from_dump(const uint32_t *dump, unsigned int len, struct ich_descriptors *desc);

#else /* ICH_DESCRIPTORS_FROM_DUMP */

int read_ich_descriptors_via_fdo(void *spibar, struct ich_descriptors *desc);
int getFCBA_component_density(enum ich_chipset cs, const struct ich_descriptors *desc, uint8_t idx);

#endif /* ICH_DESCRIPTORS_FROM_DUMP */
#endif /* __ICH_DESCRIPTORS_H__ */
#endif /* defined(__i386__) || defined(__x86_64__) */
