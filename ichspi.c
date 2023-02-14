/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Stefan Wildemann <stefan.wildemann@kontron.com>
 * Copyright (C) 2008 Claus Gindhart <claus.gindhart@kontron.com>
 * Copyright (C) 2008 Dominik Geyer <dominik.geyer@kontron.com>
 * Copyright (C) 2008 coresystems GmbH <info@coresystems.de>
 * Copyright (C) 2009, 2010 Carl-Daniel Hailfinger
 * Copyright (C) 2011 Stefan Tauner
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

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "spi.h"
#include "ich_descriptors.h"

/* Apollo Lake */
#define APL_REG_FREG12		0xe0	/* 32 Bytes Flash Region 12 */

/* Sunrise Point */

/* Added HSFS Status bits */
#define HSFS_WRSDIS_OFF		11	/* 11: Flash Configuration Lock-Down */
#define HSFS_WRSDIS		(0x1 << HSFS_WRSDIS_OFF)
#define HSFS_PRR34_LOCKDN_OFF	12	/* 12: PRR3 PRR4 Lock-Down */
#define HSFS_PRR34_LOCKDN	(0x1 << HSFS_PRR34_LOCKDN_OFF)
/* HSFS_BERASE vanished */

/*
 * HSFC and HSFS 16-bit registers are combined into the 32-bit
 * BIOS_HSFSTS_CTL register in the Sunrise Point datasheet,
 * however we still treat them separately in order to reuse code.
 */

/*
 * HSFC Control bits
 *
 * FCYCLE is a 2 bit field (HSFC bits 1-2) on ICH9 and 4 bit field
 * (HSFC bits 1-4) on PCH100.
 *
 * ICH9 and PCH100 use the same FCYCLE values for flash operations,
 * however FCYCLE values above 3 are only supported by PCH100.
 *
 * 0: SPI Read
 * 2: SPI Write
 * 3: SPI Erase 4K
 * 4: SPI Erase 64K
 * 6: SPI RDID
 * 7: SPI Write Status
 * 8: SPI Read Status
 */
#define HSFC_FGO_OFF		0
#define HSFC_FGO		(0x1 << HSFC_FGO_OFF)
#define HSFC_FCYCLE_MASK(n)	((n) << HSFC_FCYCLE_OFF)
#define HSFC_FCYCLE_OFF		1
#define HSFC_CYCLE_READ		HSFC_FCYCLE_MASK(0x0)
#define HSFC_CYCLE_WRITE	HSFC_FCYCLE_MASK(0x2)
#define HSFC_CYCLE_BLOCK_ERASE	HSFC_FCYCLE_MASK(0x3)
#define HSFC_CYCLE_RDID		HSFC_FCYCLE_MASK(0x6)
#define HSFC_CYCLE_WR_STATUS	HSFC_FCYCLE_MASK(0x7)
#define HSFC_CYCLE_RD_STATUS	HSFC_FCYCLE_MASK(0x8)

/* PCH100 controller register definition */
#define PCH100_HSFC_FCYCLE_OFF		1
#define PCH100_HSFC_FCYCLE_BIT_WIDTH	0xf
#define PCH100_HSFC_FCYCLE	HSFC_FCYCLE_MASK(PCH100_HSFC_FCYCLE_BIT_WIDTH)
/* New HSFC Control bit */
#define PCH100_HSFC_WET_OFF	(21 - 16)	/* 5: Write Enable Type */
#define PCH100_HSFC_WET		(0x1 << PCH100_HSFC_WET_OFF)

#define PCH100_FADDR_FLA	0x07ffffff

#define PCH100_REG_DLOCK	0x0c	/* 32 Bits Discrete Lock Bits */
#define DLOCK_BMWAG_LOCKDN_OFF	0
#define DLOCK_BMWAG_LOCKDN	(0x1 << DLOCK_BMWAG_LOCKDN_OFF)
#define DLOCK_BMRAG_LOCKDN_OFF	1
#define DLOCK_BMRAG_LOCKDN	(0x1 << DLOCK_BMRAG_LOCKDN_OFF)
#define DLOCK_SBMWAG_LOCKDN_OFF	2
#define DLOCK_SBMWAG_LOCKDN	(0x1 << DLOCK_SBMWAG_LOCKDN_OFF)
#define DLOCK_SBMRAG_LOCKDN_OFF	3
#define DLOCK_SBMRAG_LOCKDN	(0x1 << DLOCK_SBMRAG_LOCKDN_OFF)
#define DLOCK_PR0_LOCKDN_OFF	8
#define DLOCK_PR0_LOCKDN	(0x1 << DLOCK_PR0_LOCKDN_OFF)
#define DLOCK_PR1_LOCKDN_OFF	9
#define DLOCK_PR1_LOCKDN	(0x1 << DLOCK_PR1_LOCKDN_OFF)
#define DLOCK_PR2_LOCKDN_OFF	10
#define DLOCK_PR2_LOCKDN	(0x1 << DLOCK_PR2_LOCKDN_OFF)
#define DLOCK_PR3_LOCKDN_OFF	11
#define DLOCK_PR3_LOCKDN	(0x1 << DLOCK_PR3_LOCKDN_OFF)
#define DLOCK_PR4_LOCKDN_OFF	12
#define DLOCK_PR4_LOCKDN	(0x1 << DLOCK_PR4_LOCKDN_OFF)
#define DLOCK_SSEQ_LOCKDN_OFF	16
#define DLOCK_SSEQ_LOCKDN	(0x1 << DLOCK_SSEQ_LOCKDN_OFF)

#define PCH100_REG_FPR0		0x84	/* 32 Bits Protected Range 0 */
#define PCH100_REG_GPR0		0x98	/* 32 Bits Global Protected Range 0 */

#define PCH100_REG_SSFSC	0xA0	/* 32 Bits Status (8) + Control (24) */
#define PCH100_REG_PREOP	0xA4	/* 16 Bits */
#define PCH100_REG_OPTYPE	0xA6	/* 16 Bits */
#define PCH100_REG_OPMENU	0xA8	/* 64 Bits */

/* ICH9 controller register definition */
#define ICH9_REG_HSFS		0x04	/* 16 Bits Hardware Sequencing Flash Status */
#define HSFS_FDONE_OFF		0	/* 0: Flash Cycle Done */
#define HSFS_FDONE		(0x1 << HSFS_FDONE_OFF)
#define HSFS_FCERR_OFF		1	/* 1: Flash Cycle Error */
#define HSFS_FCERR		(0x1 << HSFS_FCERR_OFF)
#define HSFS_AEL_OFF		2	/* 2: Access Error Log */
#define HSFS_AEL		(0x1 << HSFS_AEL_OFF)
#define HSFS_BERASE_OFF		3	/* 3-4: Block/Sector Erase Size */
#define HSFS_BERASE		(0x3 << HSFS_BERASE_OFF)
#define HSFS_SCIP_OFF		5	/* 5: SPI Cycle In Progress */
#define HSFS_SCIP		(0x1 << HSFS_SCIP_OFF)
					/* 6-12: reserved */
#define HSFS_FDOPSS_OFF		13	/* 13: Flash Descriptor Override Pin-Strap Status */
#define HSFS_FDOPSS		(0x1 << HSFS_FDOPSS_OFF)
#define HSFS_FDV_OFF		14	/* 14: Flash Descriptor Valid */
#define HSFS_FDV		(0x1 << HSFS_FDV_OFF)
#define HSFS_FLOCKDN_OFF	15	/* 15: Flash Configuration Lock-Down */
#define HSFS_FLOCKDN		(0x1 << HSFS_FLOCKDN_OFF)

#define ICH9_REG_HSFC		0x06	/* 16 Bits Hardware Sequencing Flash Control */

					/* 0: Flash Cycle Go */
					/* 1-2: FLASH Cycle */
#define ICH9_HSFC_FCYCLE_OFF		1
#define ICH9_HSFC_FCYCLE_BIT_WIDTH	3
#define ICH9_HSFC_FCYCLE	HSFC_FCYCLE_MASK(ICH9_HSFC_FCYCLE_BIT_WIDTH)
					/* 3-7: reserved */
#define HSFC_FDBC_OFF		8	/* 8-13: Flash Data Byte Count */
#define HSFC_FDBC		(0x3f << HSFC_FDBC_OFF)
#define HSFC_FDBC_VAL(n)	(((n) << HSFC_FDBC_OFF) & HSFC_FDBC)
					/* 14: reserved */
#define HSFC_SME_OFF		15	/* 15: SPI SMI# Enable */
#define HSFC_SME		(0x1 << HSFC_SME_OFF)

#define ICH9_REG_FADDR		0x08	/* 32 Bits */
#define ICH9_FADDR_FLA		0x01ffffff
#define ICH9_REG_FDATA0		0x10	/* 64 Bytes */

#define ICH9_REG_FRAP		0x50	/* 32 Bytes Flash Region Access Permissions */
#define ICH9_REG_FREG0		0x54	/* 32 Bytes Flash Region 0 */

#define ICH9_REG_PR0		0x74	/* 32 Bytes Protected Range 0 */
#define PR_WP_OFF		31	/* 31: write protection enable */
#define PR_RP_OFF		15	/* 15: read protection enable */

#define ICH9_REG_SSFS		0x90	/* 08 Bits */
#define SSFS_SCIP_OFF		0	/* SPI Cycle In Progress */
#define SSFS_SCIP		(0x1 << SSFS_SCIP_OFF)
#define SSFS_FDONE_OFF		2	/* Cycle Done Status */
#define SSFS_FDONE		(0x1 << SSFS_FDONE_OFF)
#define SSFS_FCERR_OFF		3	/* Flash Cycle Error */
#define SSFS_FCERR		(0x1 << SSFS_FCERR_OFF)
#define SSFS_AEL_OFF		4	/* Access Error Log */
#define SSFS_AEL		(0x1 << SSFS_AEL_OFF)
/* The following bits are reserved in SSFS: 1,5-7. */
#define SSFS_RESERVED_MASK	0x000000e2

#define ICH9_REG_SSFC		0x91	/* 24 Bits */
/* We combine SSFS and SSFC to one 32-bit word,
 * therefore SSFC bits are off by 8. */
						/* 0: reserved */
#define SSFC_SCGO_OFF		(1 + 8)		/* 1: SPI Cycle Go */
#define SSFC_SCGO		(0x1 << SSFC_SCGO_OFF)
#define SSFC_ACS_OFF		(2 + 8)		/* 2: Atomic Cycle Sequence */
#define SSFC_ACS		(0x1 << SSFC_ACS_OFF)
#define SSFC_SPOP_OFF		(3 + 8)		/* 3: Sequence Prefix Opcode Pointer */
#define SSFC_SPOP		(0x1 << SSFC_SPOP_OFF)
#define SSFC_COP_OFF		(4 + 8)		/* 4-6: Cycle Opcode Pointer */
#define SSFC_COP		(0x7 << SSFC_COP_OFF)
						/* 7: reserved */
#define SSFC_DBC_OFF		(8 + 8)		/* 8-13: Data Byte Count */
#define SSFC_DBC		(0x3f << SSFC_DBC_OFF)
#define SSFC_DS_OFF		(14 + 8)	/* 14: Data Cycle */
#define SSFC_DS			(0x1 << SSFC_DS_OFF)
#define SSFC_SME_OFF		(15 + 8)	/* 15: SPI SMI# Enable */
#define SSFC_SME		(0x1 << SSFC_SME_OFF)
#define SSFC_SCF_OFF		(16 + 8)	/* 16-18: SPI Cycle Frequency */
#define SSFC_SCF		(0x7 << SSFC_SCF_OFF)
#define SSFC_SCF_20MHZ		0x00000000
#define SSFC_SCF_33MHZ		0x01000000
						/* 19-23: reserved */
#define SSFC_RESERVED_MASK	0xf8008100

#define ICH9_REG_PREOP		0x94	/* 16 Bits */
#define ICH9_REG_OPTYPE		0x96	/* 16 Bits */
#define ICH9_REG_OPMENU		0x98	/* 64 Bits */

#define ICH9_REG_BBAR		0xA0	/* 32 Bits BIOS Base Address Configuration */
#define BBAR_MASK	0x00ffff00		/* 8-23: Bottom of System Flash */

#define ICH8_REG_VSCC		0xC1	/* 32 Bits Vendor Specific Component Capabilities */
#define ICH9_REG_LVSCC		0xC4	/* 32 Bits Host Lower Vendor Specific Component Capabilities */
#define ICH9_REG_UVSCC		0xC8	/* 32 Bits Host Upper Vendor Specific Component Capabilities */
/* The individual fields of the VSCC registers are defined in the file
 * ich_descriptors.h. The reason is that the same layout is also used in the
 * flash descriptor to define the properties of the different flash chips
 * supported. The BIOS (or the ME?) is responsible to populate the ICH registers
 * with the information from the descriptor on startup depending on the actual
 * chip(s) detected. */

#define ICH9_REG_FPB		0xD0	/* 32 Bits Flash Partition Boundary */
#define FPB_FPBA_OFF		0	/* 0-12: Block/Sector Erase Size */
#define FPB_FPBA			(0x1FFF << FPB_FPBA_OFF)

// ICH9R SPI commands
#define SPI_OPCODE_TYPE_READ_NO_ADDRESS		0
#define SPI_OPCODE_TYPE_WRITE_NO_ADDRESS	1
#define SPI_OPCODE_TYPE_READ_WITH_ADDRESS	2
#define SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS	3

// ICH7 registers
#define ICH7_REG_SPIS		0x00	/* 16 Bits */
#define SPIS_SCIP		0x0001
#define SPIS_GRANT		0x0002
#define SPIS_CDS		0x0004
#define SPIS_FCERR		0x0008
#define SPIS_RESERVED_MASK	0x7ff0

/* VIA SPI is compatible with ICH7, but maxdata
   to transfer is 16 bytes.

   DATA byte count on ICH7 is 8:13, on VIA 8:11

   bit 12 is port select CS0 CS1
   bit 13 is FAST READ enable
   bit 7  is used with fast read and one shot controls CS de-assert?
*/

#define ICH7_REG_SPIC		0x02	/* 16 Bits */
#define SPIC_SCGO		0x0002
#define SPIC_ACS		0x0004
#define SPIC_SPOP		0x0008
#define SPIC_DS			0x4000

#define ICH7_REG_SPIA		0x04	/* 32 Bits */
#define ICH7_REG_SPID0		0x08	/* 64 Bytes */
#define ICH7_REG_PREOP		0x54	/* 16 Bits */
#define ICH7_REG_OPTYPE		0x56	/* 16 Bits */
#define ICH7_REG_OPMENU		0x58	/* 64 Bits */

enum ich_access_protection {
	NO_PROT		= 0,
	READ_PROT	= 1,
	WRITE_PROT	= 2,
	LOCKED		= 3,
};

/* ICH SPI configuration lock-down. May be set during chipset enabling. */
static bool ichspi_lock = false;

static enum ich_chipset ich_generation = CHIPSET_ICH_UNKNOWN;
static uint32_t ichspi_bbar;

static void *ich_spibar = NULL;

typedef struct _OPCODE {
	uint8_t opcode;		//This commands spi opcode
	uint8_t spi_type;	//This commands spi type
	uint8_t atomic;		//Use preop: (0: none, 1: preop0, 2: preop1
} OPCODE;

/* Suggested opcode definition:
 * Preop 1: Write Enable
 * Preop 2: Write Status register enable
 *
 * OP 0: Write address
 * OP 1: Read Address
 * OP 2: ERASE block
 * OP 3: Read Status register
 * OP 4: Read ID
 * OP 5: Write Status register
 * OP 6: chip private (read JEDEC id)
 * OP 7: Chip erase
 */
typedef struct _OPCODES {
	uint8_t preop[2];
	OPCODE opcode[8];
} OPCODES;

static OPCODES *curopcodes = NULL;

/* HW access functions */
static uint32_t REGREAD32(int X)
{
	return mmio_readl(ich_spibar + X);
}

static uint16_t REGREAD16(int X)
{
	return mmio_readw(ich_spibar + X);
}

static uint16_t REGREAD8(int X)
{
	return mmio_readb(ich_spibar + X);
}

#define REGWRITE32(off, val) mmio_writel(val, ich_spibar+(off))
#define REGWRITE16(off, val) mmio_writew(val, ich_spibar+(off))
#define REGWRITE8(off, val)  mmio_writeb(val, ich_spibar+(off))

/* Common SPI functions */

static int find_opcode(OPCODES *op, uint8_t opcode)
{
	int a;

	if (op == NULL) {
		msg_perr("\n%s: null OPCODES pointer!\n", __func__);
		return -1;
	}

	for (a = 0; a < 8; a++) {
		if (op->opcode[a].opcode == opcode)
			return a;
	}

	return -1;
}

static int find_preop(OPCODES *op, uint8_t preop)
{
	int a;

	if (op == NULL) {
		msg_perr("\n%s: null OPCODES pointer!\n", __func__);
		return -1;
	}

	for (a = 0; a < 2; a++) {
		if (op->preop[a] == preop)
			return a;
	}

	return -1;
}

/* for pairing opcodes with their required preop */
struct preop_opcode_pair {
	uint8_t preop;
	uint8_t opcode;
};

/* List of opcodes which need preopcodes and matching preopcodes. Unused. */
const struct preop_opcode_pair pops[] = {
	{JEDEC_WREN, JEDEC_BYTE_PROGRAM},
	{JEDEC_WREN, JEDEC_SE}, /* sector erase */
	{JEDEC_WREN, JEDEC_BE_52}, /* block erase */
	{JEDEC_WREN, JEDEC_BE_D8}, /* block erase */
	{JEDEC_WREN, JEDEC_CE_60}, /* chip erase */
	{JEDEC_WREN, JEDEC_CE_C7}, /* chip erase */
	 /* FIXME: WRSR requires either EWSR or WREN depending on chip type. */
	{JEDEC_WREN, JEDEC_WRSR},
	{JEDEC_EWSR, JEDEC_WRSR},
	{0,}
};

/* Reasonable default configuration. Needs ad-hoc modifications if we
 * encounter unlisted opcodes. Fun.
 */
static OPCODES O_ST_M25P = {
	{
	 JEDEC_WREN,
	 JEDEC_EWSR,
	},
	{
	 {JEDEC_BYTE_PROGRAM, SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS, 0},	// Write Byte
	 {JEDEC_READ, SPI_OPCODE_TYPE_READ_WITH_ADDRESS, 0},	// Read Data
	 {JEDEC_SE, SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS, 0},	// Erase Sector
	 {JEDEC_RDSR, SPI_OPCODE_TYPE_READ_NO_ADDRESS, 0},	// Read Device Status Reg
	 {JEDEC_REMS, SPI_OPCODE_TYPE_READ_WITH_ADDRESS, 0},	// Read Electronic Manufacturer Signature
	 {JEDEC_WRSR, SPI_OPCODE_TYPE_WRITE_NO_ADDRESS, 0},	// Write Status Register
	 {JEDEC_RDID, SPI_OPCODE_TYPE_READ_NO_ADDRESS, 0},	// Read JDEC ID
	 {JEDEC_CE_C7, SPI_OPCODE_TYPE_WRITE_NO_ADDRESS, 0},	// Bulk erase
	}
};

/* List of opcodes with their corresponding spi_type
 * It is used to reprogram the chipset OPCODE table on-the-fly if an opcode
 * is needed which is currently not in the chipset OPCODE table
 */
static OPCODE POSSIBLE_OPCODES[] = {
	 {JEDEC_BYTE_PROGRAM, SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS, 0},	// Write Byte
	 {JEDEC_READ, SPI_OPCODE_TYPE_READ_WITH_ADDRESS, 0},	// Read Data
	 {JEDEC_BE_D8, SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS, 0},	// Erase Sector
	 {JEDEC_RDSR, SPI_OPCODE_TYPE_READ_NO_ADDRESS, 0},	// Read Device Status Reg
	 {JEDEC_REMS, SPI_OPCODE_TYPE_READ_WITH_ADDRESS, 0},	// Read Electronic Manufacturer Signature
	 {JEDEC_WRSR, SPI_OPCODE_TYPE_WRITE_NO_ADDRESS, 0},	// Write Status Register
	 {JEDEC_RDID, SPI_OPCODE_TYPE_READ_NO_ADDRESS, 0},	// Read JDEC ID
	 {JEDEC_CE_C7, SPI_OPCODE_TYPE_WRITE_NO_ADDRESS, 0},	// Bulk erase
	 {JEDEC_SE, SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS, 0},	// Sector erase
	 {JEDEC_BE_52, SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS, 0},	// Block erase
	 {JEDEC_AAI_WORD_PROGRAM, SPI_OPCODE_TYPE_WRITE_NO_ADDRESS, 0},	// Auto Address Increment
};

static OPCODES O_EXISTING = {};

/* pretty printing functions */
static void prettyprint_opcodes(OPCODES *ops)
{
	OPCODE oc;
	const char *t;
	const char *a;
	uint8_t i;
	static const char *const spi_type[4] = {
		"read  w/o addr",
		"write w/o addr",
		"read  w/  addr",
		"write w/  addr"
	};
	static const char *const atomic_type[3] = {
		"none",
		" 0  ",
		" 1  "
	};

	if (ops == NULL)
		return;

	msg_pdbg2("        OP        Type      Pre-OP\n");
	for (i = 0; i < 8; i++) {
		oc = ops->opcode[i];
		t = (oc.spi_type > 3) ? "invalid" : spi_type[oc.spi_type];
		a = (oc.atomic > 2) ? "invalid" : atomic_type[oc.atomic];
		msg_pdbg2("op[%d]: 0x%02x, %s, %s\n", i, oc.opcode, t, a);
	}
	msg_pdbg2("Pre-OP 0: 0x%02x, Pre-OP 1: 0x%02x\n", ops->preop[0],
		 ops->preop[1]);
}

#define pprint_reg16(reg, bit, val, sep) msg_pdbg("%s=%"PRId16"" sep, #bit, (val & reg##_##bit) >> reg##_##bit##_OFF)
#define pprint_reg32(reg, bit, val, sep) msg_pdbg("%s=%"PRId32"" sep, #bit, (val & reg##_##bit) >> reg##_##bit##_OFF)

static void prettyprint_ich9_reg_hsfs(uint16_t reg_val, enum ich_chipset ich_gen)
{
	msg_pdbg("HSFS: ");
	pprint_reg16(HSFS, FDONE, reg_val, ", ");
	pprint_reg16(HSFS, FCERR, reg_val, ", ");
	pprint_reg16(HSFS, AEL, reg_val, ", ");
	switch (ich_gen) {
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_ELKHART_LAKE:
		break;
	default:
		pprint_reg16(HSFS, BERASE, reg_val, ", ");
		break;
	}
	pprint_reg16(HSFS, SCIP, reg_val, ", ");
	switch (ich_gen) {
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_ELKHART_LAKE:
		pprint_reg16(HSFS, PRR34_LOCKDN, reg_val, ", ");
		pprint_reg16(HSFS, WRSDIS, reg_val, ", ");
		break;
	default:
		break;
	}
	pprint_reg16(HSFS, FDOPSS, reg_val, ", ");
	pprint_reg16(HSFS, FDV, reg_val, ", ");
	pprint_reg16(HSFS, FLOCKDN, reg_val, "\n");
}

static void prettyprint_ich9_reg_hsfc(uint16_t reg_val, enum ich_chipset ich_gen)
{
	msg_pdbg("HSFC: ");
	pprint_reg16(HSFC, FGO, reg_val, ", ");
	switch (ich_gen) {
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_ELKHART_LAKE:
		pprint_reg16(PCH100_HSFC, FCYCLE, reg_val, ", ");
		pprint_reg16(PCH100_HSFC, WET, reg_val, ", ");
		break;
	default:
		pprint_reg16(ICH9_HSFC, FCYCLE, reg_val, ", ");
		break;
	}
	pprint_reg16(HSFC, FDBC, reg_val, ", ");
	pprint_reg16(HSFC, SME, reg_val, "\n");
}

static void prettyprint_ich9_reg_ssfs(uint32_t reg_val)
{
	msg_pdbg("SSFS: ");
	pprint_reg32(SSFS, SCIP, reg_val, ", ");
	pprint_reg32(SSFS, FDONE, reg_val, ", ");
	pprint_reg32(SSFS, FCERR, reg_val, ", ");
	pprint_reg32(SSFS, AEL, reg_val, "\n");
}

static void prettyprint_ich9_reg_ssfc(uint32_t reg_val)
{
	msg_pdbg("SSFC: ");
	pprint_reg32(SSFC, SCGO, reg_val, ", ");
	pprint_reg32(SSFC, ACS, reg_val, ", ");
	pprint_reg32(SSFC, SPOP, reg_val, ", ");
	pprint_reg32(SSFC, COP, reg_val, ", ");
	pprint_reg32(SSFC, DBC, reg_val, ", ");
	pprint_reg32(SSFC, SME, reg_val, ", ");
	pprint_reg32(SSFC, SCF, reg_val, "\n");
}

static void prettyprint_pch100_reg_dlock(const uint32_t reg_val)
{
	msg_pdbg("DLOCK: ");
	pprint_reg32(DLOCK, BMWAG_LOCKDN, reg_val, ", ");
	pprint_reg32(DLOCK, BMRAG_LOCKDN, reg_val, ", ");
	pprint_reg32(DLOCK, SBMWAG_LOCKDN, reg_val, ", ");
	pprint_reg32(DLOCK, SBMRAG_LOCKDN, reg_val, ",\n       ");
	pprint_reg32(DLOCK, PR0_LOCKDN, reg_val, ", ");
	pprint_reg32(DLOCK, PR1_LOCKDN, reg_val, ", ");
	pprint_reg32(DLOCK, PR2_LOCKDN, reg_val, ", ");
	pprint_reg32(DLOCK, PR3_LOCKDN, reg_val, ", ");
	pprint_reg32(DLOCK, PR4_LOCKDN, reg_val, ",\n       ");
	pprint_reg32(DLOCK, SSEQ_LOCKDN, reg_val, "\n");
}

static struct swseq_data {
	size_t reg_ssfsc;
	size_t reg_preop;
	size_t reg_optype;
	size_t reg_opmenu;
} swseq_data;

static uint8_t lookup_spi_type(uint8_t opcode)
{
	unsigned int a;

	for (a = 0; a < ARRAY_SIZE(POSSIBLE_OPCODES); a++) {
		if (POSSIBLE_OPCODES[a].opcode == opcode)
			return POSSIBLE_OPCODES[a].spi_type;
	}

	return 0xFF;
}

static int program_opcodes(OPCODES *op, int enable_undo, enum ich_chipset ich_gen)
{
	uint8_t a;
	uint16_t preop, optype;
	uint32_t opmenu[2];

	/* Program Prefix Opcodes */
	/* 0:7 Prefix Opcode 1 */
	preop = (op->preop[0]);
	/* 8:16 Prefix Opcode 2 */
	preop |= ((uint16_t) op->preop[1]) << 8;

	/* Program Opcode Types 0 - 7 */
	optype = 0;
	for (a = 0; a < 8; a++) {
		optype |= ((uint16_t) op->opcode[a].spi_type) << (a * 2);
	}

	/* Program Allowable Opcodes 0 - 3 */
	opmenu[0] = 0;
	for (a = 0; a < 4; a++) {
		opmenu[0] |= ((uint32_t) op->opcode[a].opcode) << (a * 8);
	}

	/* Program Allowable Opcodes 4 - 7 */
	opmenu[1] = 0;
	for (a = 4; a < 8; a++) {
		opmenu[1] |= ((uint32_t) op->opcode[a].opcode) << ((a - 4) * 8);
	}

	msg_pdbg2("\n%s: preop=%04x optype=%04x opmenu=%08"PRIx32"%08"PRIx32"\n", __func__, preop, optype, opmenu[0], opmenu[1]);
	switch (ich_gen) {
	case CHIPSET_ICH7:
	case CHIPSET_TUNNEL_CREEK:
	case CHIPSET_CENTERTON:
		/* Register undo only for enable_undo=1, i.e. first call. */
		if (enable_undo) {
			rmmio_valw(ich_spibar + ICH7_REG_PREOP);
			rmmio_valw(ich_spibar + ICH7_REG_OPTYPE);
			rmmio_vall(ich_spibar + ICH7_REG_OPMENU);
			rmmio_vall(ich_spibar + ICH7_REG_OPMENU + 4);
		}
		mmio_writew(preop, ich_spibar + ICH7_REG_PREOP);
		mmio_writew(optype, ich_spibar + ICH7_REG_OPTYPE);
		mmio_writel(opmenu[0], ich_spibar + ICH7_REG_OPMENU);
		mmio_writel(opmenu[1], ich_spibar + ICH7_REG_OPMENU + 4);
		break;
	case CHIPSET_ICH8:
	default:		/* Future version might behave the same */
		/* Register undo only for enable_undo=1, i.e. first call. */
		if (enable_undo) {
			rmmio_valw(ich_spibar + swseq_data.reg_preop);
			rmmio_valw(ich_spibar + swseq_data.reg_optype);
			rmmio_vall(ich_spibar + swseq_data.reg_opmenu);
			rmmio_vall(ich_spibar + swseq_data.reg_opmenu + 4);
		}
		mmio_writew(preop, ich_spibar + swseq_data.reg_preop);
		mmio_writew(optype, ich_spibar + swseq_data.reg_optype);
		mmio_writel(opmenu[0], ich_spibar + swseq_data.reg_opmenu);
		mmio_writel(opmenu[1], ich_spibar + swseq_data.reg_opmenu + 4);
		break;
	}

	return 0;
}

static int reprogram_opcode_on_the_fly(uint8_t opcode, unsigned int writecnt, unsigned int readcnt)
{
	uint8_t spi_type;

	spi_type = lookup_spi_type(opcode);
	if (spi_type > 3) {
		/* Try to guess spi type from read/write sizes.
		 * The following valid writecnt/readcnt combinations exist:
		 * writecnt  = 4, readcnt >= 0
		 * writecnt  = 1, readcnt >= 0
		 * writecnt >= 4, readcnt  = 0
		 * writecnt >= 1, readcnt  = 0
		 * writecnt >= 1 is guaranteed for all commands.
		 */
		if (readcnt == 0)
			/* if readcnt=0 and writecount >= 4, we don't know if it is WRITE_NO_ADDRESS
			 * or WRITE_WITH_ADDRESS. But if we use WRITE_NO_ADDRESS and the first 3 data
			 * bytes are actual the address, they go to the bus anyhow
			 */
			spi_type = SPI_OPCODE_TYPE_WRITE_NO_ADDRESS;
		else if (writecnt == 1) // and readcnt is > 0
			spi_type = SPI_OPCODE_TYPE_READ_NO_ADDRESS;
		else if (writecnt == 4) // and readcnt is > 0
			spi_type = SPI_OPCODE_TYPE_READ_WITH_ADDRESS;
		else // we have an invalid case
			return SPI_INVALID_LENGTH;
	}
	int oppos = 2;	// use original JEDEC_BE_D8 offset
	curopcodes->opcode[oppos].opcode = opcode;
	curopcodes->opcode[oppos].spi_type = spi_type;
	program_opcodes(curopcodes, 0, ich_generation);
	oppos = find_opcode(curopcodes, opcode);
	msg_pdbg2("on-the-fly OPCODE (0x%02X) re-programmed, op-pos=%d\n", opcode, oppos);
	return oppos;
}

/*
 * Returns -1 if at least one mandatory opcode is inaccessible, 0 otherwise.
 * FIXME: this should also check for
 *   - at least one probing opcode (RDID (incl. AT25F variants?), REMS, RES?)
 *   - at least one erasing opcode (lots.)
 *   - at least one program opcode (BYTE_PROGRAM, AAI_WORD_PROGRAM, ...?)
 *   - necessary preops? (EWSR, WREN, ...?)
 */
static int ich_missing_opcodes(void)
{
	uint8_t ops[] = {
		JEDEC_READ,
		JEDEC_RDSR,
		0
	};
	int i = 0;
	while (ops[i] != 0) {
		msg_pspew("checking for opcode 0x%02x\n", ops[i]);
		if (find_opcode(curopcodes, ops[i]) == -1)
			return -1;
		i++;
	}
	return 0;
}

/*
 * Try to set BBAR (BIOS Base Address Register), but read back the value in case
 * it didn't stick.
 */
static void ich_set_bbar(uint32_t min_addr, enum ich_chipset ich_gen)
{
	int bbar_off;
	switch (ich_gen) {
	case CHIPSET_ICH7:
	case CHIPSET_TUNNEL_CREEK:
	case CHIPSET_CENTERTON:
		bbar_off = 0x50;
		break;
	case CHIPSET_ICH8:
	case CHIPSET_BAYTRAIL:
		msg_pdbg("BBAR offset is unknown!\n");
		return;
	case CHIPSET_ICH9:
	default:		/* Future version might behave the same */
		bbar_off = ICH9_REG_BBAR;
		break;
	}

	ichspi_bbar = mmio_readl(ich_spibar + bbar_off) & ~BBAR_MASK;
	if (ichspi_bbar) {
		msg_pdbg("Reserved bits in BBAR not zero: 0x%08"PRIx32"\n",
			 ichspi_bbar);
	}
	min_addr &= BBAR_MASK;
	ichspi_bbar |= min_addr;
	rmmio_writel(ichspi_bbar, ich_spibar + bbar_off);
	ichspi_bbar = mmio_readl(ich_spibar + bbar_off) & BBAR_MASK;

	/* We don't have any option except complaining. And if the write
	 * failed, the restore will fail as well, so no problem there.
	 */
	if (ichspi_bbar != min_addr)
		msg_perr("Setting BBAR to 0x%08"PRIx32" failed! New value: 0x%08"PRIx32".\n",
			 min_addr, ichspi_bbar);
}

/* Create a struct OPCODES based on what we find in the locked down chipset. */
static int generate_opcodes(OPCODES * op, enum ich_chipset ich_gen)
{
	int a;
	uint16_t preop, optype;
	uint32_t opmenu[2];

	if (op == NULL) {
		msg_perr("\n%s: null OPCODES pointer!\n", __func__);
		return -1;
	}

	switch (ich_gen) {
	case CHIPSET_ICH7:
	case CHIPSET_TUNNEL_CREEK:
	case CHIPSET_CENTERTON:
		preop = REGREAD16(ICH7_REG_PREOP);
		optype = REGREAD16(ICH7_REG_OPTYPE);
		opmenu[0] = REGREAD32(ICH7_REG_OPMENU);
		opmenu[1] = REGREAD32(ICH7_REG_OPMENU + 4);
		break;
	case CHIPSET_ICH8:
	default:		/* Future version might behave the same */
		preop = REGREAD16(swseq_data.reg_preop);
		optype = REGREAD16(swseq_data.reg_optype);
		opmenu[0] = REGREAD32(swseq_data.reg_opmenu);
		opmenu[1] = REGREAD32(swseq_data.reg_opmenu + 4);
		break;
	}

	op->preop[0] = (uint8_t) preop;
	op->preop[1] = (uint8_t) (preop >> 8);

	for (a = 0; a < 8; a++) {
		op->opcode[a].spi_type = (uint8_t) (optype & 0x3);
		optype >>= 2;
	}

	for (a = 0; a < 4; a++) {
		op->opcode[a].opcode = (uint8_t) (opmenu[0] & 0xff);
		opmenu[0] >>= 8;
	}

	for (a = 4; a < 8; a++) {
		op->opcode[a].opcode = (uint8_t) (opmenu[1] & 0xff);
		opmenu[1] >>= 8;
	}

	/* No preopcodes used by default. */
	for (a = 0; a < 8; a++)
		op->opcode[a].atomic = 0;

	return 0;
}

/* This function generates OPCODES from or programs OPCODES to ICH according to
 * the chipset's SPI configuration lock.
 *
 * It should be called before ICH sends any spi command.
 */
static int ich_init_opcodes(enum ich_chipset ich_gen)
{
	int rc = 0;
	OPCODES *curopcodes_done;

	if (curopcodes)
		return 0;

	if (ichspi_lock) {
		msg_pdbg("Reading OPCODES... ");
		curopcodes_done = &O_EXISTING;
		rc = generate_opcodes(curopcodes_done, ich_gen);
	} else {
		msg_pdbg("Programming OPCODES... ");
		curopcodes_done = &O_ST_M25P;
		rc = program_opcodes(curopcodes_done, 1, ich_gen);
	}

	if (rc) {
		curopcodes = NULL;
		msg_perr("failed\n");
		return 1;
	} else {
		curopcodes = curopcodes_done;
		msg_pdbg("done\n");
		prettyprint_opcodes(curopcodes);
		return 0;
	}
}

/* Fill len bytes from the data array into the fdata/spid registers.
 *
 * Note that using len > flash->mst->spi.max_data_write will trash the registers
 * following the data registers.
 */
static void ich_fill_data(const uint8_t *data, int len, int reg0_off)
{
	uint32_t temp32 = 0;
	int i;

	if (len <= 0)
		return;

	for (i = 0; i < len; i++) {
		if ((i % 4) == 0)
			temp32 = 0;

		temp32 |= ((uint32_t) data[i]) << ((i % 4) * 8);

		if ((i % 4) == 3) /* 32 bits are full, write them to regs. */
			REGWRITE32(reg0_off + (i - (i % 4)), temp32);
	}
	i--;
	if ((i % 4) != 3) /* Write remaining data to regs. */
		REGWRITE32(reg0_off + (i - (i % 4)), temp32);
}

/* Read len bytes from the fdata/spid register into the data array.
 *
 * Note that using len > flash->mst->spi.max_data_read will return garbage or
 * may even crash.
 */
static void ich_read_data(uint8_t *data, int len, int reg0_off)
{
	int i;
	uint32_t temp32 = 0;

	for (i = 0; i < len; i++) {
		if ((i % 4) == 0)
			temp32 = REGREAD32(reg0_off + i);

		data[i] = (temp32 >> ((i % 4) * 8)) & 0xff;
	}
}

static int ich7_run_opcode(OPCODE op, uint32_t offset,
			   uint8_t datalength, uint8_t * data, int maxdata)
{
	bool write_cmd = false;
	int timeout;
	uint32_t temp32;
	uint16_t temp16;
	uint64_t opmenu;
	int opcode_index;

	/* Is it a write command? */
	if ((op.spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS)
	    || (op.spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS)) {
		write_cmd = true;
	}

	timeout = 100 * 60;	/* 60 ms are 9.6 million cycles at 16 MHz. */
	while ((REGREAD16(ICH7_REG_SPIS) & SPIS_SCIP) && --timeout) {
		default_delay(10);
	}
	if (!timeout) {
		msg_perr("Error: SCIP never cleared!\n");
		return 1;
	}

	/* Program offset in flash into SPIA while preserving reserved bits. */
	temp32 = REGREAD32(ICH7_REG_SPIA) & ~0x00FFFFFF;
	REGWRITE32(ICH7_REG_SPIA, (offset & 0x00FFFFFF) | temp32);

	/* Program data into SPID0 to N */
	if (write_cmd && (datalength != 0))
		ich_fill_data(data, datalength, ICH7_REG_SPID0);

	/* Assemble SPIS */
	temp16 = REGREAD16(ICH7_REG_SPIS);
	/* keep reserved bits */
	temp16 &= SPIS_RESERVED_MASK;
	/* clear error status registers */
	temp16 |= (SPIS_CDS | SPIS_FCERR);
	REGWRITE16(ICH7_REG_SPIS, temp16);

	/* Assemble SPIC */
	temp16 = 0;

	if (datalength != 0) {
		temp16 |= SPIC_DS;
		temp16 |= ((uint32_t) ((datalength - 1) & (maxdata - 1))) << 8;
	}

	/* Select opcode */
	opmenu = REGREAD32(ICH7_REG_OPMENU);
	opmenu |= ((uint64_t)REGREAD32(ICH7_REG_OPMENU + 4)) << 32;

	for (opcode_index = 0; opcode_index < 8; opcode_index++) {
		if ((opmenu & 0xff) == op.opcode) {
			break;
		}
		opmenu >>= 8;
	}
	if (opcode_index == 8) {
		msg_pdbg("Opcode %x not found.\n", op.opcode);
		return 1;
	}
	temp16 |= ((uint16_t) (opcode_index & 0x07)) << 4;

	timeout = 100 * 60;	/* 60 ms are 9.6 million cycles at 16 MHz. */
	/* Handle Atomic. Atomic commands include three steps:
	    - sending the preop (mainly EWSR or WREN)
	    - sending the main command
	    - waiting for the busy bit (WIP) to be cleared
	   This means the timeout must be sufficient for chip erase
	   of slow high-capacity chips.
	 */
	switch (op.atomic) {
	case 2:
		/* Select second preop. */
		temp16 |= SPIC_SPOP;
		/* Fall through. */
	case 1:
		/* Atomic command (preop+op) */
		temp16 |= SPIC_ACS;
		timeout = 100 * 1000 * 60;	/* 60 seconds */
		break;
	}

	/* Start */
	temp16 |= SPIC_SCGO;

	/* write it */
	REGWRITE16(ICH7_REG_SPIC, temp16);

	/* Wait for Cycle Done Status or Flash Cycle Error. */
	while (((REGREAD16(ICH7_REG_SPIS) & (SPIS_CDS | SPIS_FCERR)) == 0) &&
	       --timeout) {
		default_delay(10);
	}
	if (!timeout) {
		msg_perr("timeout, ICH7_REG_SPIS=0x%04x\n", REGREAD16(ICH7_REG_SPIS));
		return 1;
	}

	/* FIXME: make sure we do not needlessly cause transaction errors. */
	temp16 = REGREAD16(ICH7_REG_SPIS);
	if (temp16 & SPIS_FCERR) {
		msg_perr("Transaction error!\n");
		/* keep reserved bits */
		temp16 &= SPIS_RESERVED_MASK;
		REGWRITE16(ICH7_REG_SPIS, temp16 | SPIS_FCERR);
		return 1;
	}

	if ((!write_cmd) && (datalength != 0))
		ich_read_data(data, datalength, ICH7_REG_SPID0);

	return 0;
}

static int ich9_run_opcode(OPCODE op, uint32_t offset,
			   uint8_t datalength, uint8_t * data)
{
	bool write_cmd = false;
	int timeout;
	uint32_t temp32;
	uint64_t opmenu;
	int opcode_index;

	/* Is it a write command? */
	if ((op.spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS)
	    || (op.spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS)) {
		write_cmd = true;
	}

	timeout = 100 * 60;	/* 60 ms are 9.6 million cycles at 16 MHz. */
	while ((REGREAD8(swseq_data.reg_ssfsc) & SSFS_SCIP) && --timeout) {
		default_delay(10);
	}
	if (!timeout) {
		msg_perr("Error: SCIP never cleared!\n");
		return 1;
	}

	/* Program offset in flash into FADDR while preserve the reserved bits
	 * and clearing the 25. address bit which is only usable in hwseq. */
	temp32 = REGREAD32(ICH9_REG_FADDR) & ~0x01FFFFFF;
	REGWRITE32(ICH9_REG_FADDR, (offset & 0x00FFFFFF) | temp32);

	/* Program data into FDATA0 to N */
	if (write_cmd && (datalength != 0))
		ich_fill_data(data, datalength, ICH9_REG_FDATA0);

	/* Assemble SSFS + SSFC */
	temp32 = REGREAD32(swseq_data.reg_ssfsc);
	/* Keep reserved bits only */
	temp32 &= SSFS_RESERVED_MASK | SSFC_RESERVED_MASK;
	/* Clear cycle done and cycle error status registers */
	temp32 |= (SSFS_FDONE | SSFS_FCERR);
	REGWRITE32(swseq_data.reg_ssfsc, temp32);

	/* Use 20 MHz */
	temp32 |= SSFC_SCF_20MHZ;

	/* Set data byte count (DBC) and data cycle bit (DS) */
	if (datalength != 0) {
		uint32_t datatemp;
		temp32 |= SSFC_DS;
		datatemp = ((((uint32_t)datalength - 1) << SSFC_DBC_OFF) & SSFC_DBC);
		temp32 |= datatemp;
	}

	/* Select opcode */
	opmenu = REGREAD32(swseq_data.reg_opmenu);
	opmenu |= ((uint64_t)REGREAD32(swseq_data.reg_opmenu + 4)) << 32;

	for (opcode_index = 0; opcode_index < 8; opcode_index++) {
		if ((opmenu & 0xff) == op.opcode) {
			break;
		}
		opmenu >>= 8;
	}
	if (opcode_index == 8) {
		msg_pdbg("Opcode %x not found.\n", op.opcode);
		return 1;
	}
	temp32 |= ((uint32_t) (opcode_index & 0x07)) << (8 + 4);

	timeout = 100 * 60;	/* 60 ms are 9.6 million cycles at 16 MHz. */
	/* Handle Atomic. Atomic commands include three steps:
	    - sending the preop (mainly EWSR or WREN)
	    - sending the main command
	    - waiting for the busy bit (WIP) to be cleared
	   This means the timeout must be sufficient for chip erase
	   of slow high-capacity chips.
	 */
	switch (op.atomic) {
	case 2:
		/* Select second preop. */
		temp32 |= SSFC_SPOP;
		/* Fall through. */
	case 1:
		/* Atomic command (preop+op) */
		temp32 |= SSFC_ACS;
		timeout = 100 * 1000 * 60;	/* 60 seconds */
		break;
	}

	/* Start */
	temp32 |= SSFC_SCGO;

	/* write it */
	REGWRITE32(swseq_data.reg_ssfsc, temp32);

	/* Wait for Cycle Done Status or Flash Cycle Error. */
	while (((REGREAD32(swseq_data.reg_ssfsc) & (SSFS_FDONE | SSFS_FCERR)) == 0) &&
	       --timeout) {
		default_delay(10);
	}
	if (!timeout) {
		msg_perr("timeout, REG_SSFS=0x%08"PRIx32"\n", REGREAD32(swseq_data.reg_ssfsc));
		return 1;
	}

	/* FIXME make sure we do not needlessly cause transaction errors. */
	temp32 = REGREAD32(swseq_data.reg_ssfsc);
	if (temp32 & SSFS_FCERR) {
		msg_perr("Transaction error!\n");
		prettyprint_ich9_reg_ssfs(temp32);
		prettyprint_ich9_reg_ssfc(temp32);
		/* keep reserved bits */
		temp32 &= SSFS_RESERVED_MASK | SSFC_RESERVED_MASK;
		/* Clear the transaction error. */
		REGWRITE32(swseq_data.reg_ssfsc, temp32 | SSFS_FCERR);
		return 1;
	}

	if ((!write_cmd) && (datalength != 0))
		ich_read_data(data, datalength, ICH9_REG_FDATA0);

	return 0;
}

static int run_opcode(const struct flashctx *flash, OPCODE op, uint32_t offset,
		      uint8_t datalength, uint8_t * data)
{
	/* max_data_read == max_data_write for all Intel/VIA SPI masters */
	uint8_t maxlength = flash->mst->spi.max_data_read;

	if (ich_generation == CHIPSET_ICH_UNKNOWN) {
		msg_perr("%s: unsupported chipset\n", __func__);
		return -1;
	}

	if (datalength > maxlength) {
		msg_perr("%s: Internal command size error for "
			"opcode 0x%02x, got datalength=%i, want <=%i\n",
			__func__, op.opcode, datalength, maxlength);
		return SPI_INVALID_LENGTH;
	}

	switch (ich_generation) {
	case CHIPSET_ICH7:
	case CHIPSET_TUNNEL_CREEK:
	case CHIPSET_CENTERTON:
		return ich7_run_opcode(op, offset, datalength, data, maxlength);
	case CHIPSET_ICH8:
	default:		/* Future version might behave the same */
		return ich9_run_opcode(op, offset, datalength, data);
	}
}

static int ich_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
				unsigned int readcnt,
				const unsigned char *writearr,
				unsigned char *readarr)
{
	int result;
	int opcode_index = -1;
	const unsigned char cmd = *writearr;
	OPCODE *opcode;
	uint32_t addr = 0;
	uint8_t *data;
	int count;

	/* find cmd in opcodes-table */
	opcode_index = find_opcode(curopcodes, cmd);
	if (opcode_index == -1) {
		if (!ichspi_lock)
			opcode_index = reprogram_opcode_on_the_fly(cmd, writecnt, readcnt);
		if (opcode_index == SPI_INVALID_LENGTH) {
			msg_pdbg("OPCODE 0x%02x has unsupported length, will not execute.\n", cmd);
			return SPI_INVALID_LENGTH;
		} else if (opcode_index == -1) {
			msg_pdbg("Invalid OPCODE 0x%02x, will not execute.\n", cmd);
			return SPI_INVALID_OPCODE;
		}
	}

	opcode = &(curopcodes->opcode[opcode_index]);

	/* The following valid writecnt/readcnt combinations exist:
	 * writecnt  = 4, readcnt >= 0
	 * writecnt  = 1, readcnt >= 0
	 * writecnt >= 4, readcnt  = 0
	 * writecnt >= 1, readcnt  = 0
	 * writecnt >= 1 is guaranteed for all commands.
	 */
	if ((opcode->spi_type == SPI_OPCODE_TYPE_READ_WITH_ADDRESS) &&
	    (writecnt != 4)) {
		msg_perr("%s: Internal command size error for opcode "
			"0x%02x, got writecnt=%i, want =4\n", __func__, cmd, writecnt);
		return SPI_INVALID_LENGTH;
	}
	if ((opcode->spi_type == SPI_OPCODE_TYPE_READ_NO_ADDRESS) &&
	    (writecnt != 1)) {
		msg_perr("%s: Internal command size error for opcode "
			"0x%02x, got writecnt=%i, want =1\n", __func__, cmd, writecnt);
		return SPI_INVALID_LENGTH;
	}
	if ((opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) &&
	    (writecnt < 4)) {
		msg_perr("%s: Internal command size error for opcode "
			"0x%02x, got writecnt=%i, want >=4\n", __func__, cmd, writecnt);
		return SPI_INVALID_LENGTH;
	}
	if (((opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) ||
	     (opcode->spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS)) &&
	    (readcnt)) {
		msg_perr("%s: Internal command size error for opcode "
			"0x%02x, got readcnt=%i, want =0\n", __func__, cmd, readcnt);
		return SPI_INVALID_LENGTH;
	}

	/* Translate read/write array/count.
	 * The maximum data length is identical for the maximum read length and
	 * for the maximum write length excluding opcode and address. Opcode and
	 * address are stored in separate registers, not in the data registers
	 * and are thus not counted towards data length. The only exception
	 * applies if the opcode definition (un)intentionally classifies said
	 * opcode incorrectly as non-address opcode or vice versa. */
	if (opcode->spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS) {
		data = (uint8_t *) (writearr + 1);
		count = writecnt - 1;
	} else if (opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) {
		data = (uint8_t *) (writearr + 4);
		count = writecnt - 4;
	} else {
		data = (uint8_t *) readarr;
		count = readcnt;
	}

	/* if opcode-type requires an address */
	if (cmd == JEDEC_REMS || cmd == JEDEC_RES) {
		addr = ichspi_bbar;
	} else if (opcode->spi_type == SPI_OPCODE_TYPE_READ_WITH_ADDRESS ||
	    opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) {
		/* BBAR may cut part of the chip off at the lower end. */
		const uint32_t valid_base = ichspi_bbar & ((flash->chip->total_size * 1024) - 1);
		const uint32_t addr_offset = ichspi_bbar - valid_base;
		/* Highest address we can program is (2^24 - 1). */
		const uint32_t valid_end = (1 << 24) - addr_offset;

		addr = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		const uint32_t addr_end = addr + count;

		if (addr < valid_base ||
		    addr_end < addr || /* integer overflow check */
		    addr_end > valid_end) {
			msg_perr("%s: Addressed region 0x%06"PRIx32"-0x%06"PRIx32" not in allowed range 0x%06"PRIx32"-0x%06"PRIx32"\n",
				 __func__, addr, addr_end - 1, valid_base, valid_end - 1);
			return SPI_INVALID_ADDRESS;
		}
		addr += addr_offset;
	}

	result = run_opcode(flash, *opcode, addr, count, data);
	if (result) {
		msg_pdbg("Running OPCODE 0x%02x failed ", opcode->opcode);
		if ((opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) ||
		    (opcode->spi_type == SPI_OPCODE_TYPE_READ_WITH_ADDRESS)) {
			msg_pdbg("at address 0x%06"PRIx32" ", addr);
		}
		msg_pdbg("(payload length was %d).\n", count);

		/* Print out the data array if it contains data to write.
		 * Errors are detected before the received data is read back into
		 * the array so it won't make sense to print it then. */
		if ((opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) ||
		    (opcode->spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS)) {
			int i;
			msg_pspew("The data was:\n");
			for (i = 0; i < count; i++){
				msg_pspew("%3d: 0x%02"PRIx8"\n", i, data[i]);
			}
		}
	}

	return result;
}

#define MAX_FD_REGIONS 16
struct fd_region {
	const char* name;
	enum ich_access_protection level;
	uint32_t base;
	uint32_t limit;
};

struct hwseq_data {
	uint32_t size_comp0;
	uint32_t size_comp1;
	uint32_t addr_mask;
	bool only_4k;
	uint32_t hsfc_fcycle;

	struct fd_region fd_regions[MAX_FD_REGIONS];
};

static struct hwseq_data *get_hwseq_data_from_context(const struct flashctx *flash)
{
	return flash->mst->opaque.data;
}

/* Sets FLA in FADDR to (addr & hwseq_data->addr_mask) without touching other bits. */
static void ich_hwseq_set_addr(uint32_t addr, uint32_t mask)
{
	uint32_t addr_old = REGREAD32(ICH9_REG_FADDR) & ~mask;
	REGWRITE32(ICH9_REG_FADDR, (addr & mask) | addr_old);
}

/* Sets FADDR.FLA to 'addr' and returns the erase block size in bytes
 * of the block containing this address. May return nonsense if the address is
 * not valid. The erase block size for a specific address depends on the flash
 * partition layout as specified by FPB and the partition properties as defined
 * by UVSCC and LVSCC respectively. An alternative to implement this method
 * would be by querying FPB and the respective VSCC register directly.
 */
static uint32_t ich_hwseq_get_erase_block_size(unsigned int addr, uint32_t addr_mask, bool only_4k)
{
	uint8_t enc_berase;
	static const uint32_t dec_berase[4] = {
		256,
		4 * 1024,
		8 * 1024,
		64 * 1024
	};

	if (only_4k) {
		return 4 * 1024;
	}

	ich_hwseq_set_addr(addr, addr_mask);
	enc_berase = (REGREAD16(ICH9_REG_HSFS) & HSFS_BERASE) >> HSFS_BERASE_OFF;
	return dec_berase[enc_berase];
}

/* Polls for Cycle Done Status, Flash Cycle Error or timeout in 8 us intervals.
   Resets all error flags in HSFS.
   Returns 0 if the cycle completes successfully without errors within
   timeout us, 1 on errors. */
static int ich_hwseq_wait_for_cycle_complete(unsigned int len, enum ich_chipset ich_gen, uint32_t addr_mask)
{
	/*
	 * The SPI bus may be busy due to performing operations from other masters, hence
	 * introduce the long timeout of 30s to cover the worst case scenarios as well.
	 */
	unsigned int timeout_us = 30 * 1000 * 1000;
	uint16_t hsfs;
	uint32_t addr;

	timeout_us /= 8; /* scale timeout duration to counter */
	while ((((hsfs = REGREAD16(ICH9_REG_HSFS)) &
		 (HSFS_FDONE | HSFS_FCERR)) == 0) &&
	       --timeout_us) {
		default_delay(8);
	}
	REGWRITE16(ICH9_REG_HSFS, REGREAD16(ICH9_REG_HSFS));
	if (!timeout_us) {
		addr = REGREAD32(ICH9_REG_FADDR) & addr_mask;
		msg_perr("Timeout error between offset 0x%08"PRIx32" and "
			 "0x%08"PRIx32" (= 0x%08"PRIx32" + %d)!\n",
			 addr, addr + len - 1, addr, len - 1);
		prettyprint_ich9_reg_hsfs(hsfs, ich_gen);
		prettyprint_ich9_reg_hsfc(REGREAD16(ICH9_REG_HSFC), ich_gen);
		return 1;
	}

	if (hsfs & HSFS_FCERR) {
		addr = REGREAD32(ICH9_REG_FADDR) & addr_mask;
		msg_perr("Transaction error between offset 0x%08"PRIx32" and "
			 "0x%08"PRIx32" (= 0x%08"PRIx32" + %d)!\n",
			 addr, addr + len - 1, addr, len - 1);
		prettyprint_ich9_reg_hsfs(hsfs, ich_gen);
		prettyprint_ich9_reg_hsfc(REGREAD16(ICH9_REG_HSFC), ich_gen);
		return 1;
	}
	return 0;
}

/* Fire up a transfer using the hardware sequencer. */
static void ich_start_hwseq_xfer(const struct flashctx *flash,
		uint32_t hsfc_cycle, uint32_t flash_addr, size_t len,
		uint32_t addr_mask)
{
	/* make sure HSFC register is cleared before initiate any operation */
	uint16_t hsfc = 0;

	/* Sets flash_addr in FADDR */
	ich_hwseq_set_addr(flash_addr, addr_mask);

	/* make sure FDONE, FCERR, AEL are cleared by writing 1 to them */
	REGWRITE16(ICH9_REG_HSFS, REGREAD16(ICH9_REG_HSFS));

	/* Set up transaction parameters. */
	hsfc |= hsfc_cycle;
	/*
	 * The number of bytes transferred is the value of `FDBC` plus 1, hence,
	 * subtracted 1 from the length field.
	 * As per Intel EDS, `0b` in the FDBC represents 1 byte while `0x3f`
	 * represents 64-bytes to be transferred.
	 */
	hsfc |= HSFC_FDBC_VAL(len - 1);
	hsfc |= HSFC_FGO; /* start */
	prettyprint_ich9_reg_hsfc(hsfc, ich_generation);
	REGWRITE16(ICH9_REG_HSFC, hsfc);
}

static int ich_wait_for_hwseq_spi_cycle_complete(void)
{
	if (REGREAD8(ICH9_REG_HSFS) & HSFS_SCIP) {
		msg_perr("Error: SCIP bit is unexpectedly set.\n");
		return 1;
	}
	return 0;
}

/* Execute SPI flash transfer */
static int ich_exec_sync_hwseq_xfer(const struct flashctx *flash, uint32_t hsfc_cycle, uint32_t flash_addr,
				size_t len, enum ich_chipset ich_gen, uint32_t addr_mask)
{
	if (ich_wait_for_hwseq_spi_cycle_complete()) {
		msg_perr("SPI Transaction Timeout due to previous operation in process!\n");
		return 1;
	}

	ich_start_hwseq_xfer(flash, hsfc_cycle, flash_addr, len, addr_mask);
	return ich_hwseq_wait_for_cycle_complete(len, ich_gen, addr_mask);
}

static void ich_get_region(const struct flashctx *flash, unsigned int addr, struct flash_region *region)
{
	struct ich_descriptors desc = { 0 };
	const ssize_t nr = ich_number_of_regions(ich_generation, &desc.content);
	const struct hwseq_data *hwseq_data = get_hwseq_data_from_context(flash);
	const struct fd_region *fd_regions = hwseq_data->fd_regions;

	/*
	 * Set default values for the region. If no flash descriptor containing
	 * addr is found, these values will be used instead.
	 *
	 * The region start and end are constrained so that they do not overlap
	 * any flash descriptor regions.
	 */
	const char *name = "";
	region->read_prot  = false;
	region->write_prot = false;
	region->start = 0;
	region->end = flashrom_flash_getsize(flash);

	for (ssize_t i = 0; i < nr; i++) {
		uint32_t base = fd_regions[i].base;
		uint32_t limit = fd_regions[i].limit;
		enum ich_access_protection level = fd_regions[i].level;

		if (addr < base) {
			/*
			 * fd_regions[i] starts after addr, constrain
			 * region->end so that it does not overlap.
			 */
			region->end = min(region->end, base);
		} else if (addr > limit) {
			/*
			 * fd_regions[i] ends before addr, constrain
			 * region->start so that it does not overlap.
			 */
			region->start = max(region->start, limit + 1);
		} else {
			/* fd_regions[i] contains addr, copy to *region. */
			name = fd_regions[i].name;
			region->start = base;
			region->end = limit + 1;
			region->read_prot  = (level == LOCKED) || (level == READ_PROT);
			region->write_prot = (level == LOCKED) || (level == WRITE_PROT);
			break;
		}
	}

	region->name = strdup(name);
}

/* Given RDID info, return pointer to entry in flashchips[] */
static const struct flashchip *flash_id_to_entry(uint32_t mfg_id, uint32_t model_id)
{
	const struct flashchip *chip;

	for (chip = &flashchips[0]; chip->vendor; chip++) {
		if ((chip->manufacture_id == mfg_id) &&
		    (chip->model_id == model_id) &&
		    (chip->probe == PROBE_SPI_RDID) &&
		    ((chip->bustype & BUS_SPI) == BUS_SPI))
			return chip;
	}

	return NULL;
}

static int ich_hwseq_read_status(const struct flashctx *flash, enum flash_reg reg, uint8_t *value)
{
	const int len = 1;
	const struct hwseq_data *hwseq_data = get_hwseq_data_from_context(flash);

	if (reg != STATUS1) {
		msg_pdbg("%s: only supports STATUS1\n", __func__);
		/*
		 * Return SPI_INVALID_OPCODE to be consistent with spi_read_register()
		 * and make error handling simpler even though this isn't a SPI master.
		 */
		return SPI_INVALID_OPCODE;
	}
	msg_pdbg("Reading Status register\n");

	if (ich_exec_sync_hwseq_xfer(flash, HSFC_CYCLE_RD_STATUS, 1, len, ich_generation,
		hwseq_data->addr_mask)) {
		msg_perr("Reading Status register failed\n!!");
		return -1;
	}
	ich_read_data(value, len, ICH9_REG_FDATA0);

	return 0;
}

static int ich_hwseq_write_status(const struct flashctx *flash, enum flash_reg reg, uint8_t value)
{
	const int len = 1;
	const struct hwseq_data *hwseq_data = get_hwseq_data_from_context(flash);

	if (reg != STATUS1) {
		msg_pdbg("%s: only supports STATUS1\n", __func__);
		/*
		 * Return SPI_INVALID_OPCODE to be consistent with spi_write_register()
		 * and make error handling simpler even though this isn't a SPI master.
		 */
		return SPI_INVALID_OPCODE;
	}
	msg_pdbg("Writing status register\n");

	ich_fill_data(&value, len, ICH9_REG_FDATA0);

	if (ich_exec_sync_hwseq_xfer(flash, HSFC_CYCLE_WR_STATUS, 1, len, ich_generation,
		hwseq_data->addr_mask)) {
		msg_perr("Writing Status register failed\n!!");
		return -1;
	}

	return 0;
}

static void ich_hwseq_get_flash_id(struct flashctx *flash, enum ich_chipset ich_gen)
{
	const struct hwseq_data *hwseq_data = get_hwseq_data_from_context(flash);
	if (hwseq_data->size_comp1 != 0) {
		msg_pinfo("Multiple flash components detected, skipping flash identification.\n");
		return;
	}

	/* PCH100 or above is required for RDID, ICH9 does not support it. */
	if (hwseq_data->hsfc_fcycle != PCH100_HSFC_FCYCLE) {
		msg_pinfo("RDID cycle not supported, skipping flash identification.\n");
		return;
	}

	/*
	 * RDID gives 3 byte output:
	 *     Byte 0: Manufacturer ID
	 *     Byte 1: Model ID (MSB)
	 *     Byte 2: Model ID (LSB)
	 */
	const int len = 3;
	uint8_t data[len];

	if (ich_exec_sync_hwseq_xfer(flash, HSFC_CYCLE_RDID, 1, len, ich_gen,
		hwseq_data->addr_mask)) {
		msg_perr("Timed out waiting for RDID to complete.\n");
	}

	ich_read_data(data, len, ICH9_REG_FDATA0);
	uint32_t mfg_id = data[0];
	uint32_t model_id = (data[1] << 8) | data[2];

	const struct flashchip *entry = flash_id_to_entry(mfg_id, model_id);
	if (!entry) {
		msg_pwarn("Unable to identify chip, mfg_id: 0x%02"PRIx32", "
				"model_id: 0x%02"PRIx32"\n", mfg_id, model_id);
	}

	msg_pdbg("Chip identified: %s\n", entry->name);

	/* Update informational flash chip entries only */
	flash->chip->vendor = entry->vendor;
	flash->chip->name = entry->name;
	flash->chip->manufacture_id = entry->manufacture_id;
	flash->chip->model_id = entry->model_id;
	/* total_size read from flash descriptor */
	flash->chip->page_size = entry->page_size;
	flash->chip->feature_bits = entry->feature_bits;
	flash->chip->tested = entry->tested;
	/* Support writeprotect */
	flash->chip->reg_bits = entry->reg_bits;
	flash->chip->decode_range = entry->decode_range;
}

static int ich_hwseq_probe(struct flashctx *flash)
{
	uint32_t total_size, boundary;
	uint32_t erase_size_low, size_low, erase_size_high, size_high;
	struct block_eraser *eraser;
	const struct hwseq_data *hwseq_data = get_hwseq_data_from_context(flash);

	total_size = hwseq_data->size_comp0 + hwseq_data->size_comp1;
	msg_cdbg("Hardware sequencing reports %d attached SPI flash chip",
		 (hwseq_data->size_comp1 != 0) ? 2 : 1);
	if (hwseq_data->size_comp1 != 0)
		msg_cdbg("s with a combined");
	else
		msg_cdbg(" with a");
	msg_cdbg(" density of %"PRId32" kB.\n", total_size / 1024);
	flash->chip->total_size = total_size / 1024;

	eraser = &(flash->chip->block_erasers[0]);
	if (!hwseq_data->only_4k)
		boundary = (REGREAD32(ICH9_REG_FPB) & FPB_FPBA) << 12;
	else
		boundary = 0;
	size_high = total_size - boundary;
	erase_size_high = ich_hwseq_get_erase_block_size(boundary, hwseq_data->addr_mask, hwseq_data->only_4k);

	if (boundary == 0) {
		msg_cdbg2("There is only one partition containing the whole "
			 "address space (0x%06x - 0x%06"PRIx32").\n", 0, size_high-1);
		eraser->eraseblocks[0].size = erase_size_high;
		eraser->eraseblocks[0].count = size_high / erase_size_high;
		msg_cdbg2("There are %"PRId32" erase blocks with %"PRId32" B each.\n",
			 size_high / erase_size_high, erase_size_high);
	} else {
		msg_cdbg2("The flash address space (0x%06x - 0x%06"PRIx32") is divided "
			 "at address 0x%06"PRIx32" in two partitions.\n",
			 0, total_size-1, boundary);
		size_low = total_size - size_high;
		erase_size_low = ich_hwseq_get_erase_block_size(0, hwseq_data->addr_mask, hwseq_data->only_4k);

		eraser->eraseblocks[0].size = erase_size_low;
		eraser->eraseblocks[0].count = size_low / erase_size_low;
		msg_cdbg("The first partition ranges from 0x%06x to 0x%06"PRIx32".\n", 0, size_low-1);
		msg_cdbg("In that range are %"PRId32" erase blocks with %"PRId32" B each.\n",
			 size_low / erase_size_low, erase_size_low);

		eraser->eraseblocks[1].size = erase_size_high;
		eraser->eraseblocks[1].count = size_high / erase_size_high;
		msg_cdbg("The second partition ranges from 0x%06"PRIx32" to 0x%06"PRIx32".\n",
			 boundary, total_size-1);
		msg_cdbg("In that range are %"PRId32" erase blocks with %"PRId32" B each.\n",
			 size_high / erase_size_high, erase_size_high);
	}

	/* May be overwritten by ich_hwseq_get_flash_id(). */
	flash->chip->tested = TEST_OK_PREWB;

	ich_hwseq_get_flash_id(flash, ich_generation);

	return 1;
}

static int ich_hwseq_block_erase(struct flashctx *flash, unsigned int addr,
				 unsigned int len)
{
	uint32_t erase_block;
	const struct hwseq_data *hwseq_data = get_hwseq_data_from_context(flash);

	erase_block = ich_hwseq_get_erase_block_size(addr, hwseq_data->addr_mask, hwseq_data->only_4k);
	if (len != erase_block) {
		msg_cerr("Erase block size for address 0x%06x is %"PRId32" B, "
			 "but requested erase block size is %d B. "
			 "Not erasing anything.\n", addr, erase_block, len);
		return -1;
	}

	/* Although the hardware supports this (it would erase the whole block
	 * containing the address) we play safe here. */
	if (addr % erase_block != 0) {
		msg_cerr("Erase address 0x%06x is not aligned to the erase "
			 "block boundary (any multiple of %"PRId32"). "
			 "Not erasing anything.\n", addr, erase_block);
		return -1;
	}

	if (addr + len > flash->chip->total_size * 1024) {
		msg_perr("Request to erase some inaccessible memory address(es)"
			 " (addr=0x%x, len=%d). Not erasing anything.\n", addr, len);
		return -1;
	}

	msg_pdbg("Erasing %d bytes starting at 0x%06x.\n", len, addr);

	if (ich_exec_sync_hwseq_xfer(flash, HSFC_CYCLE_BLOCK_ERASE, addr, 1, ich_generation,
		hwseq_data->addr_mask))
		return -1;
	return 0;
}

static int ich_hwseq_read(struct flashctx *flash, uint8_t *buf,
			  unsigned int addr, unsigned int len)
{
	uint8_t block_len;
	const struct hwseq_data *hwseq_data = get_hwseq_data_from_context(flash);

	if (addr + len > flash->chip->total_size * 1024) {
		msg_perr("Request to read from an inaccessible memory address "
			 "(addr=0x%x, len=%d).\n", addr, len);
		return -1;
	}

	msg_pdbg("Reading %d bytes starting at 0x%06x.\n", len, addr);
	/* clear FDONE, FCERR, AEL by writing 1 to them (if they are set) */
	REGWRITE16(ICH9_REG_HSFS, REGREAD16(ICH9_REG_HSFS));

	while (len > 0) {
		/* Obey programmer limit... */
		block_len = min(len, flash->mst->opaque.max_data_read);
		/* as well as flash chip page borders as demanded in the Intel datasheets. */
		block_len = min(block_len, 256 - (addr & 0xFF));

		if (ich_exec_sync_hwseq_xfer(flash, HSFC_CYCLE_READ, addr, block_len, ich_generation,
			hwseq_data->addr_mask))
			return 1;
		ich_read_data(buf, block_len, ICH9_REG_FDATA0);
		addr += block_len;
		buf += block_len;
		len -= block_len;
	}
	return 0;
}

static int ich_hwseq_write(struct flashctx *flash, const uint8_t *buf, unsigned int addr, unsigned int len)
{
	uint8_t block_len;
	const struct hwseq_data *hwseq_data = get_hwseq_data_from_context(flash);

	if (addr + len > flash->chip->total_size * 1024) {
		msg_perr("Request to write to an inaccessible memory address "
			 "(addr=0x%x, len=%d).\n", addr, len);
		return -1;
	}

	msg_pdbg("Writing %d bytes starting at 0x%06x.\n", len, addr);
	/* clear FDONE, FCERR, AEL by writing 1 to them (if they are set) */
	REGWRITE16(ICH9_REG_HSFS, REGREAD16(ICH9_REG_HSFS));

	while (len > 0) {
		/* Obey programmer limit... */
		block_len = min(len, flash->mst->opaque.max_data_write);
		/* as well as flash chip page borders as demanded in the Intel datasheets. */
		block_len = min(block_len, 256 - (addr & 0xFF));
		ich_fill_data(buf, block_len, ICH9_REG_FDATA0);

		if (ich_exec_sync_hwseq_xfer(flash, HSFC_CYCLE_WRITE, addr, block_len, ich_generation,
			hwseq_data->addr_mask))
			return -1;
		addr += block_len;
		buf += block_len;
		len -= block_len;
	}
	return 0;
}

static int ich_hwseq_shutdown(void *data)
{
	free(data);
	return 0;
}

static int ich_spi_send_multicommand(const struct flashctx *flash,
				     struct spi_command *cmds)
{
	int ret = 0;
	int i;
	int oppos, preoppos;
	for (; (cmds->writecnt || cmds->readcnt) && !ret; cmds++) {
		if ((cmds + 1)->writecnt || (cmds + 1)->readcnt) {
			/* Next command is valid. */
			preoppos = find_preop(curopcodes, cmds->writearr[0]);
			oppos = find_opcode(curopcodes, (cmds + 1)->writearr[0]);
			if ((oppos == -1) && (preoppos != -1)) {
				/* Current command is listed as preopcode in
				 * ICH struct OPCODES, but next command is not
				 * listed as opcode in that struct.
				 * Check for command sanity, then
				 * try to reprogram the ICH opcode list.
				 */
				if (find_preop(curopcodes,
					       (cmds + 1)->writearr[0]) != -1) {
					msg_perr("%s: Two subsequent "
						"preopcodes 0x%02x and 0x%02x, "
						"ignoring the first.\n",
						__func__, cmds->writearr[0],
						(cmds + 1)->writearr[0]);
					continue;
				}
				/* If the chipset is locked down, we'll fail
				 * during execution of the next command anyway.
				 * No need to bother with fixups.
				 */
				if (!ichspi_lock) {
					oppos = reprogram_opcode_on_the_fly((cmds + 1)->writearr[0], (cmds + 1)->writecnt, (cmds + 1)->readcnt);
					if (oppos == -1)
						continue;
					curopcodes->opcode[oppos].atomic = preoppos + 1;
					continue;
				}
			}
			if ((oppos != -1) && (preoppos != -1)) {
				/* Current command is listed as preopcode in
				 * ICH struct OPCODES and next command is listed
				 * as opcode in that struct. Match them up.
				 */
				curopcodes->opcode[oppos].atomic = preoppos + 1;
				continue;
			}
			/* If none of the above if-statements about oppos or
			 * preoppos matched, this is a normal opcode.
			 */
		}
		ret = ich_spi_send_command(flash, cmds->writecnt, cmds->readcnt,
					   cmds->writearr, cmds->readarr);
		/* Reset the type of all opcodes to non-atomic. */
		for (i = 0; i < 8; i++)
			curopcodes->opcode[i].atomic = 0;
	}
	return ret;
}

static bool ich_spi_probe_opcode(const struct flashctx *flash, uint8_t opcode)
{
	return find_opcode(curopcodes, opcode) >= 0;
}

#define ICH_BMWAG(x) ((x >> 24) & 0xff)
#define ICH_BMRAG(x) ((x >> 16) & 0xff)
#define ICH_BRWA(x)  ((x >>  8) & 0xff)
#define ICH_BRRA(x)  ((x >>  0) & 0xff)

static const enum ich_access_protection access_perms_to_protection[] = {
	LOCKED, WRITE_PROT, READ_PROT, NO_PROT
};
static const char *const access_names[] = {
	"read-write", "write-only", "read-only", "locked"
};

static enum ich_access_protection ich9_handle_frap(struct fd_region *fd_regions,
						   uint32_t frap, unsigned int i)
{
	static const char *const region_names[] = {
		"Flash Descriptor", "BIOS", "Management Engine",
		"Gigabit Ethernet", "Platform Data", "Device Expansion",
		"BIOS2", "unknown", "EC/BMC",
	};
	const char *const region_name = i < ARRAY_SIZE(region_names) ? region_names[i] : "unknown";

	uint32_t base, limit;
	unsigned int rwperms_idx;
	enum ich_access_protection rwperms;
	const int offset = i < 12
		? ICH9_REG_FREG0 + i * 4
		: APL_REG_FREG12 + (i - 12) * 4;
	uint32_t freg = mmio_readl(ich_spibar + offset);

	base  = ICH_FREG_BASE(freg);
	limit = ICH_FREG_LIMIT(freg);
	if (base > limit || (freg == 0 && i > 0)) {
		/* this FREG is disabled */
		msg_pdbg2("0x%02X: 0x%08"PRIx32" FREG%u: %s region is unused.\n",
			  offset, freg, i, region_name);
		return NO_PROT;
	}
	msg_pdbg("0x%02X: 0x%08"PRIx32" ", offset, freg);

	if (i < 8) {
		rwperms_idx = (((ICH_BRWA(frap) >> i) & 1) << 1) |
			      (((ICH_BRRA(frap) >> i) & 1) << 0);
		rwperms = access_perms_to_protection[rwperms_idx];
	} else {
		/* Datasheets don't define any access bits for regions > 7. We
		   can't rely on the actual descriptor settings either as there
		   are several overrides for them (those by other masters are
		   not even readable by us, *shrug*). */
		msg_pdbg("FREG%u: %s region (0x%08"PRIx32"-0x%08"PRIx32") has unknown permissions.\n",
				i, region_name, base, limit);
		return NO_PROT;
	}
	msg_pinfo("FREG%u: %s region (0x%08"PRIx32"-0x%08"PRIx32") is %s.\n", i,
		  region_name, base, limit, access_names[rwperms]);

	/* Save region attributes for use by ich_get_region(). */
	fd_regions[i].base = base;
	fd_regions[i].limit = limit;
	fd_regions[i].level = rwperms;
	fd_regions[i].name = region_name;

	return rwperms;
}

	/* In contrast to FRAP and the master section of the descriptor the bits
	 * in the PR registers have an inverted meaning. The bits in FRAP
	 * indicate read and write access _grant_. Here they indicate read
	 * and write _protection_ respectively. If both bits are 0 the address
	 * bits are ignored.
	 */
#define ICH_PR_PERMS(pr)	(((~((pr) >> PR_RP_OFF) & 1) << 0) | \
				 ((~((pr) >> PR_WP_OFF) & 1) << 1))

static enum ich_access_protection ich9_handle_pr(const size_t reg_pr0, unsigned int i)
{
	uint8_t off = reg_pr0 + (i * 4);
	uint32_t pr = mmio_readl(ich_spibar + off);
	unsigned int rwperms_idx = ICH_PR_PERMS(pr);
	enum ich_access_protection rwperms = access_perms_to_protection[rwperms_idx];

	/* From 5 on we have GPR registers and start from 0 again. */
	const char *const prefix = i >= 5 ? "G" : "";
	if (i >= 5)
		i -= 5;

	if (rwperms == NO_PROT) {
		msg_pdbg2("0x%02"PRIX8": 0x%08"PRIx32" (%sPR%u is unused)\n", off, pr, prefix, i);
		return NO_PROT;
	}

	msg_pdbg("0x%02"PRIX8": 0x%08"PRIx32" ", off, pr);
	msg_pwarn("%sPR%u: Warning: 0x%08"PRIx32"-0x%08"PRIx32" is %s.\n", prefix, i, ICH_FREG_BASE(pr),
		  ICH_FREG_LIMIT(pr), access_names[rwperms]);

	return rwperms;
}

/* Set/Clear the read and write protection enable bits of PR register @i
 * according to @read_prot and @write_prot. */
static void ich9_set_pr(const size_t reg_pr0, int i, int read_prot, int write_prot)
{
	void *addr = ich_spibar + reg_pr0 + (i * 4);
	uint32_t old = mmio_readl(addr);
	uint32_t new;

	msg_gspew("PR%u is 0x%08"PRIx32"", i, old);
	new = old & ~((1 << PR_RP_OFF) | (1 << PR_WP_OFF));
	if (read_prot)
		new |= (1 << PR_RP_OFF);
	if (write_prot)
		new |= (1 << PR_WP_OFF);
	if (old == new) {
		msg_gspew(" already.\n");
		return;
	}
	msg_gspew(", trying to set it to 0x%08"PRIx32" ", new);
	rmmio_writel(new, addr);
	msg_gspew("resulted in 0x%08"PRIx32".\n", mmio_readl(addr));
}

static const struct spi_master spi_master_ich7 = {
	.max_data_read	= 64,
	.max_data_write	= 64,
	.command	= ich_spi_send_command,
	.multicommand	= ich_spi_send_multicommand,
	.map_flash_region	= physmap,
	.unmap_flash_region	= physunmap,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
};

static const struct spi_master spi_master_ich9 = {
	.max_data_read	= 64,
	.max_data_write	= 64,
	.command	= ich_spi_send_command,
	.multicommand	= ich_spi_send_multicommand,
	.map_flash_region	= physmap,
	.unmap_flash_region	= physunmap,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.probe_opcode	= ich_spi_probe_opcode,
};

static const struct opaque_master opaque_master_ich_hwseq = {
	.max_data_read	= 64,
	.max_data_write	= 64,
	.probe		= ich_hwseq_probe,
	.read		= ich_hwseq_read,
	.write		= ich_hwseq_write,
	.erase		= ich_hwseq_block_erase,
	.read_register	= ich_hwseq_read_status,
	.write_register	= ich_hwseq_write_status,
	.get_region	= ich_get_region,
	.shutdown	= ich_hwseq_shutdown,
};

static int init_ich7_spi(void *spibar, enum ich_chipset ich_gen)
{
	unsigned int i;

	msg_pdbg("0x00: 0x%04"PRIx16"     (SPIS)\n",	mmio_readw(spibar + 0));
	msg_pdbg("0x02: 0x%04"PRIx16"     (SPIC)\n",	mmio_readw(spibar + 2));
	msg_pdbg("0x04: 0x%08"PRIx32" (SPIA)\n",	mmio_readl(spibar + 4));

	ichspi_bbar = mmio_readl(spibar + 0x50);

	msg_pdbg("0x50: 0x%08"PRIx32" (BBAR)\n",	ichspi_bbar);
	msg_pdbg("0x54: 0x%04"PRIx16"     (PREOP)\n",	mmio_readw(spibar + 0x54));
	msg_pdbg("0x56: 0x%04"PRIx16"     (OPTYPE)\n",	mmio_readw(spibar + 0x56));
	msg_pdbg("0x58: 0x%08"PRIx32" (OPMENU)\n",	mmio_readl(spibar + 0x58));
	msg_pdbg("0x5c: 0x%08"PRIx32" (OPMENU+4)\n",	mmio_readl(spibar + 0x5c));

	for (i = 0; i < 3; i++) {
		int offs;
		offs = 0x60 + (i * 4);
		msg_pdbg("0x%02x: 0x%08"PRIx32" (PBR%u)\n", offs, mmio_readl(spibar + offs), i);
	}
	if (mmio_readw(spibar) & (1 << 15)) {
		msg_pwarn("WARNING: SPI Configuration Lockdown activated.\n");
		ichspi_lock = true;
	}
	ich_init_opcodes(ich_gen);
	ich_set_bbar(0, ich_gen);
	register_spi_master(&spi_master_ich7, NULL);

	return 0;
}

enum ich_spi_mode {
	ich_auto,
	ich_hwseq,
	ich_swseq
};

static int get_ich_spi_mode_param(const struct programmer_cfg *cfg, enum ich_spi_mode *ich_spi_mode)
{
	char *const arg = extract_programmer_param_str(cfg, "ich_spi_mode");
	if (!arg) {
		return 0;
	} else if (!strcmp(arg, "hwseq")) {
		*ich_spi_mode = ich_hwseq;
		msg_pspew("user selected hwseq\n");
	} else if (!strcmp(arg, "swseq")) {
		*ich_spi_mode = ich_swseq;
		msg_pspew("user selected swseq\n");
	} else if (!strcmp(arg, "auto")) {
		msg_pspew("user selected auto\n");
		*ich_spi_mode = ich_auto;
	} else if (!strlen(arg)) {
		msg_perr("Missing argument for ich_spi_mode.\n");
		free(arg);
		return ERROR_FLASHROM_FATAL;
	} else {
		msg_perr("Unknown argument for ich_spi_mode: %s\n", arg);
		free(arg);
		return ERROR_FLASHROM_FATAL;
	}
	free(arg);

	return 0;
}

static void init_chipset_properties(struct swseq_data *swseq, struct hwseq_data *hwseq,
					size_t *num_freg, size_t *num_pr, size_t *reg_pr0,
					enum ich_chipset ich_gen)
{
	/* Moving registers / bits */
	switch (ich_gen) {
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_GEMINI_LAKE:
	case CHIPSET_JASPER_LAKE:
	case CHIPSET_ELKHART_LAKE:
		*num_pr			= 6;	/* Includes GPR0 */
		*reg_pr0		= PCH100_REG_FPR0;
		swseq->reg_ssfsc	= PCH100_REG_SSFSC;
		swseq->reg_preop	= PCH100_REG_PREOP;
		swseq->reg_optype	= PCH100_REG_OPTYPE;
		swseq->reg_opmenu	= PCH100_REG_OPMENU;
		hwseq->addr_mask	= PCH100_FADDR_FLA;
		hwseq->only_4k		= true;
		hwseq->hsfc_fcycle	= PCH100_HSFC_FCYCLE;
		break;
	default:
		*num_pr			= 5;
		*reg_pr0		= ICH9_REG_PR0;
		swseq->reg_ssfsc	= ICH9_REG_SSFS;
		swseq->reg_preop	= ICH9_REG_PREOP;
		swseq->reg_optype	= ICH9_REG_OPTYPE;
		swseq->reg_opmenu	= ICH9_REG_OPMENU;
		hwseq->addr_mask	= ICH9_FADDR_FLA;
		hwseq->only_4k		= false;
		hwseq->hsfc_fcycle	= ICH9_HSFC_FCYCLE;
		break;
	}

	switch (ich_gen) {
	case CHIPSET_100_SERIES_SUNRISE_POINT:
		*num_freg = 10;
		break;
	case CHIPSET_C620_SERIES_LEWISBURG:
		*num_freg = 12;	/* 12 MMIO regs, but 16 regions in FD spec */
		break;
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_GEMINI_LAKE:
	case CHIPSET_JASPER_LAKE:
	case CHIPSET_ELKHART_LAKE:
		*num_freg = 16;
		break;
	default:
		*num_freg = 5;
		break;
	}
}

static int init_ich_default(const struct programmer_cfg *cfg, void *spibar, enum ich_chipset ich_gen)
{
	unsigned int i;
	uint16_t tmp2;
	uint32_t tmp;
	int ich_spi_rw_restricted = 0;
	bool desc_valid = false;
	struct ich_descriptors desc = { 0 };
	enum ich_spi_mode ich_spi_mode = ich_auto;
	size_t num_freg, num_pr, reg_pr0;
	struct hwseq_data hwseq_data = { 0 };
	init_chipset_properties(&swseq_data, &hwseq_data, &num_freg, &num_pr, &reg_pr0, ich_gen);

	int ret = get_ich_spi_mode_param(cfg, &ich_spi_mode);
	if (ret)
		return ret;

	tmp2 = mmio_readw(spibar + ICH9_REG_HSFS);
	msg_pdbg("0x04: 0x%04x (HSFS)\n", tmp2);
	prettyprint_ich9_reg_hsfs(tmp2, ich_gen);
	if (tmp2 & HSFS_FLOCKDN) {
		msg_pinfo("SPI Configuration is locked down.\n");
		ichspi_lock = true;
	}
	if (tmp2 & HSFS_FDV)
		desc_valid = true;
	if (!(tmp2 & HSFS_FDOPSS) && desc_valid)
		msg_pinfo("The Flash Descriptor Override Strap-Pin is set. Restrictions implied by\n"
			  "the Master Section of the flash descriptor are NOT in effect. Please note\n"
			  "that Protected Range (PR) restrictions still apply.\n");
	ich_init_opcodes(ich_gen);

	if (desc_valid) {
		tmp2 = mmio_readw(spibar + ICH9_REG_HSFC);
		msg_pdbg("0x06: 0x%04"PRIx16" (HSFC)\n", tmp2);
		prettyprint_ich9_reg_hsfc(tmp2, ich_gen);
	}

	tmp = mmio_readl(spibar + ICH9_REG_FADDR);
	msg_pdbg2("0x08: 0x%08"PRIx32" (FADDR)\n", tmp);

	switch (ich_gen) {
	case CHIPSET_100_SERIES_SUNRISE_POINT:
	case CHIPSET_C620_SERIES_LEWISBURG:
	case CHIPSET_300_SERIES_CANNON_POINT:
	case CHIPSET_400_SERIES_COMET_POINT:
	case CHIPSET_500_SERIES_TIGER_POINT:
	case CHIPSET_600_SERIES_ALDER_POINT:
	case CHIPSET_METEOR_LAKE:
	case CHIPSET_APOLLO_LAKE:
	case CHIPSET_GEMINI_LAKE:
	case CHIPSET_JASPER_LAKE:
	case CHIPSET_ELKHART_LAKE:
		tmp = mmio_readl(spibar + PCH100_REG_DLOCK);
		msg_pdbg("0x0c: 0x%08"PRIx32" (DLOCK)\n", tmp);
		prettyprint_pch100_reg_dlock(tmp);
		break;
	default:
		break;
	}

	if (desc_valid) {
		tmp = mmio_readl(spibar + ICH9_REG_FRAP);
		msg_pdbg("0x50: 0x%08"PRIx32" (FRAP)\n", tmp);
		msg_pdbg("BMWAG 0x%02"PRIx32", ", ICH_BMWAG(tmp));
		msg_pdbg("BMRAG 0x%02"PRIx32", ", ICH_BMRAG(tmp));
		msg_pdbg("BRWA 0x%02"PRIx32", ", ICH_BRWA(tmp));
		msg_pdbg("BRRA 0x%02"PRIx32"\n", ICH_BRRA(tmp));

		/* Handle FREGx and FRAP registers */
		for (i = 0; i < num_freg; i++)
			ich_spi_rw_restricted |= ich9_handle_frap(hwseq_data.fd_regions, tmp, i);
		if (ich_spi_rw_restricted)
			msg_pinfo("Not all flash regions are freely accessible by flashrom. This is "
				  "most likely\ndue to an active ME. Please see "
				  "https://flashrom.org/ME for details.\n");
	}

	/* Handle PR registers */
	for (i = 0; i < num_pr; i++) {
		/* if not locked down try to disable PR locks first */
		if (!ichspi_lock)
			ich9_set_pr(reg_pr0, i, 0, 0);
		ich_spi_rw_restricted |= ich9_handle_pr(reg_pr0, i);
	}

	switch (ich_spi_rw_restricted) {
	case WRITE_PROT:
		msg_pwarn("At least some flash regions are write protected. For write operations,\n"
			  "you should use a flash layout and include only writable regions. See\n"
			  "manpage for more details.\n");
		break;
	case READ_PROT:
	case LOCKED:
		msg_pwarn("At least some flash regions are read protected. You have to use a flash\n"
			  "layout and include only accessible regions. For write operations, you'll\n"
			  "additionally need the --noverify-all switch. See manpage for more details.\n");
		break;
	}

	tmp = mmio_readl(spibar + swseq_data.reg_ssfsc);
	msg_pdbg("0x%zx: 0x%02"PRIx32" (SSFS)\n", swseq_data.reg_ssfsc, tmp & 0xff);
	prettyprint_ich9_reg_ssfs(tmp);
	if (tmp & SSFS_FCERR) {
		msg_pdbg("Clearing SSFS.FCERR\n");
		mmio_writeb(SSFS_FCERR, spibar + swseq_data.reg_ssfsc);
	}
	msg_pdbg("0x%zx: 0x%06"PRIx32" (SSFC)\n", swseq_data.reg_ssfsc + 1, tmp >> 8);
	prettyprint_ich9_reg_ssfc(tmp);

	msg_pdbg("0x%zx: 0x%04"PRIx16" (PREOP)\n",
		 swseq_data.reg_preop, mmio_readw(spibar + swseq_data.reg_preop));
	msg_pdbg("0x%zx: 0x%04"PRIx16" (OPTYPE)\n",
		 swseq_data.reg_optype, mmio_readw(spibar + swseq_data.reg_optype));
	msg_pdbg("0x%zx: 0x%08"PRIx32" (OPMENU)\n",
		 swseq_data.reg_opmenu, mmio_readl(spibar + swseq_data.reg_opmenu));
	msg_pdbg("0x%zx: 0x%08"PRIx32" (OPMENU+4)\n",
		 swseq_data.reg_opmenu + 4, mmio_readl(spibar + swseq_data.reg_opmenu + 4));

	if (desc_valid) {
		switch (ich_gen) {
		case CHIPSET_ICH8:
		case CHIPSET_100_SERIES_SUNRISE_POINT:
		case CHIPSET_C620_SERIES_LEWISBURG:
		case CHIPSET_300_SERIES_CANNON_POINT:
		case CHIPSET_400_SERIES_COMET_POINT:
		case CHIPSET_500_SERIES_TIGER_POINT:
		case CHIPSET_600_SERIES_ALDER_POINT:
		case CHIPSET_METEOR_LAKE:
		case CHIPSET_APOLLO_LAKE:
		case CHIPSET_GEMINI_LAKE:
		case CHIPSET_JASPER_LAKE:
		case CHIPSET_BAYTRAIL:
		case CHIPSET_ELKHART_LAKE:
			break;
		default:
			ichspi_bbar = mmio_readl(spibar + ICH9_REG_BBAR);
			msg_pdbg("0x%x: 0x%08"PRIx32" (BBAR)\n", ICH9_REG_BBAR, ichspi_bbar);
			ich_set_bbar(0, ich_gen);
			break;
		}

		if (ich_gen == CHIPSET_ICH8) {
			tmp = mmio_readl(spibar + ICH8_REG_VSCC);
			msg_pdbg("0x%x: 0x%08"PRIx32" (VSCC)\n", ICH8_REG_VSCC, tmp);
			msg_pdbg("VSCC: ");
			prettyprint_ich_reg_vscc(tmp, FLASHROM_MSG_DEBUG, true);
		} else {
			tmp = mmio_readl(spibar + ICH9_REG_LVSCC);
			msg_pdbg("0x%x: 0x%08"PRIx32" (LVSCC)\n", ICH9_REG_LVSCC, tmp);
			msg_pdbg("LVSCC: ");
			prettyprint_ich_reg_vscc(tmp, FLASHROM_MSG_DEBUG, true);

			tmp = mmio_readl(spibar + ICH9_REG_UVSCC);
			msg_pdbg("0x%x: 0x%08"PRIx32" (UVSCC)\n", ICH9_REG_UVSCC, tmp);
			msg_pdbg("UVSCC: ");
			prettyprint_ich_reg_vscc(tmp, FLASHROM_MSG_DEBUG, false);
		}

		switch (ich_gen) {
		case CHIPSET_ICH8:
		case CHIPSET_100_SERIES_SUNRISE_POINT:
		case CHIPSET_C620_SERIES_LEWISBURG:
		case CHIPSET_300_SERIES_CANNON_POINT:
		case CHIPSET_400_SERIES_COMET_POINT:
		case CHIPSET_500_SERIES_TIGER_POINT:
		case CHIPSET_600_SERIES_ALDER_POINT:
		case CHIPSET_METEOR_LAKE:
		case CHIPSET_APOLLO_LAKE:
		case CHIPSET_GEMINI_LAKE:
		case CHIPSET_JASPER_LAKE:
		case CHIPSET_ELKHART_LAKE:
			break;
		default:
			tmp = mmio_readl(spibar + ICH9_REG_FPB);
			msg_pdbg("0x%x: 0x%08"PRIx32" (FPB)\n", ICH9_REG_FPB, tmp);
			break;
		}

		if (read_ich_descriptors_via_fdo(ich_gen, spibar, &desc) == ICH_RET_OK)
			prettyprint_ich_descriptors(ich_gen, &desc);

		/* If the descriptor is valid and indicates multiple
		 * flash devices we need to use hwseq to be able to
		 * access the second flash device.
		 */
		if (ich_spi_mode == ich_auto && desc.content.NC != 0) {
			msg_pinfo("Enabling hardware sequencing due to multiple flash chips detected.\n");
			ich_spi_mode = ich_hwseq;
		}
	}

	if (ich_spi_mode == ich_auto && ichspi_lock &&
	    ich_missing_opcodes()) {
		msg_pinfo("Enabling hardware sequencing because "
			  "some important opcode is locked.\n");
		ich_spi_mode = ich_hwseq;
	}

	if (ich_spi_mode == ich_auto &&
	    (ich_gen == CHIPSET_100_SERIES_SUNRISE_POINT ||
	     ich_gen == CHIPSET_300_SERIES_CANNON_POINT ||
	     ich_gen == CHIPSET_400_SERIES_COMET_POINT ||
	     ich_gen == CHIPSET_500_SERIES_TIGER_POINT ||
	     ich_gen == CHIPSET_600_SERIES_ALDER_POINT)) {
		msg_pdbg("Enabling hardware sequencing by default for 100+ series PCH.\n");
		ich_spi_mode = ich_hwseq;
	}

	if (ich_spi_mode == ich_auto &&
	    (ich_gen == CHIPSET_APOLLO_LAKE ||
	     ich_gen == CHIPSET_GEMINI_LAKE ||
	     ich_gen == CHIPSET_JASPER_LAKE ||
	     ich_gen == CHIPSET_ELKHART_LAKE ||
	     ich_gen == CHIPSET_METEOR_LAKE)) {
		msg_pdbg("Enabling hardware sequencing by default for Apollo/Gemini/Jasper/Elkhart/Meteor Lake.\n");
		ich_spi_mode = ich_hwseq;
	}

	if (ich_spi_mode == ich_hwseq) {
		if (!desc_valid) {
			msg_perr("Hardware sequencing was requested "
				 "but the flash descriptor is not valid. Aborting.\n");
			return ERROR_FLASHROM_FATAL;
		}

		int tmpi = getFCBA_component_density(ich_gen, &desc, 0);
		if (tmpi < 0) {
			msg_perr("Could not determine density of flash component %d.\n", 0);
			return ERROR_FLASHROM_FATAL;
		}
		hwseq_data.size_comp0 = tmpi;

		tmpi = getFCBA_component_density(ich_gen, &desc, 1);
		if (tmpi < 0) {
			msg_perr("Could not determine density of flash component %d.\n", 1);
			return ERROR_FLASHROM_FATAL;
		}
		hwseq_data.size_comp1 = tmpi;

		struct hwseq_data *opaque_hwseq_data = calloc(1, sizeof(struct hwseq_data));
		if (!opaque_hwseq_data)
			return ERROR_FLASHROM_FATAL;
		memcpy(opaque_hwseq_data, &hwseq_data, sizeof(*opaque_hwseq_data));
		register_opaque_master(&opaque_master_ich_hwseq, opaque_hwseq_data);
	} else {
		register_spi_master(&spi_master_ich9, NULL);
	}

	return 0;
}

int ich_init_spi(const struct programmer_cfg *cfg, void *spibar, enum ich_chipset ich_gen)
{
	ich_generation = ich_gen;
	ich_spibar = spibar;

	switch (ich_gen) {
	case CHIPSET_ICH7:
	case CHIPSET_TUNNEL_CREEK:
	case CHIPSET_CENTERTON:
		return init_ich7_spi(spibar, ich_gen);
	case CHIPSET_ICH8:
	default:	/* Future version might behave the same */
		return init_ich_default(cfg, spibar, ich_gen);
	}
}

static const struct spi_master spi_master_via = {
	.max_data_read	= 16,
	.max_data_write	= 16,
	.command	= ich_spi_send_command,
	.multicommand	= ich_spi_send_multicommand,
	.map_flash_region	= physmap,
	.unmap_flash_region	= physunmap,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.probe_opcode	= ich_spi_probe_opcode,
};

int via_init_spi(uint32_t mmio_base)
{
	int i;

	ich_spibar = rphysmap("VIA SPI MMIO registers", mmio_base, 0x70);
	if (ich_spibar == ERROR_PTR)
		return ERROR_FLASHROM_FATAL;
	/* Do we really need no write enable? Like the LPC one at D17F0 0x40 */

	/* Not sure if it speaks all these bus protocols. */
	internal_buses_supported &= BUS_LPC | BUS_FWH;
	ich_generation = CHIPSET_ICH7;
	register_spi_master(&spi_master_via, NULL);

	msg_pdbg("0x00: 0x%04"PRIx16" (SPIS)\n",	mmio_readw(ich_spibar + 0));
	msg_pdbg("0x02: 0x%04"PRIx16" (SPIC)\n",	mmio_readw(ich_spibar + 2));
	msg_pdbg("0x04: 0x%08"PRIx32" (SPIA)\n",	mmio_readl(ich_spibar + 4));
	for (i = 0; i < 2; i++) {
		int offs;
		offs = 8 + (i * 8);
		msg_pdbg("0x%02x: 0x%08"PRIx32" (SPID%d)\n", offs, mmio_readl(ich_spibar + offs), i);
		msg_pdbg("0x%02x: 0x%08"PRIx32" (SPID%d+4)\n", offs + 4,
			 mmio_readl(ich_spibar + offs + 4), i);
	}
	ichspi_bbar = mmio_readl(ich_spibar + 0x50);

	msg_pdbg("0x50: 0x%08"PRIx32" (BBAR)\n",	ichspi_bbar);
	msg_pdbg("0x54: 0x%04"PRIx16" (PREOP)\n",	mmio_readw(ich_spibar + 0x54));
	msg_pdbg("0x56: 0x%04"PRIx16" (OPTYPE)\n",	mmio_readw(ich_spibar + 0x56));
	msg_pdbg("0x58: 0x%08"PRIx32" (OPMENU)\n",	mmio_readl(ich_spibar + 0x58));
	msg_pdbg("0x5c: 0x%08"PRIx32" (OPMENU+4)\n",	mmio_readl(ich_spibar + 0x5c));
	for (i = 0; i < 3; i++) {
		int offs;
		offs = 0x60 + (i * 4);
		msg_pdbg("0x%02x: 0x%08"PRIx32" (PBR%d)\n", offs, mmio_readl(ich_spibar + offs), i);
	}
	msg_pdbg("0x6c: 0x%04x     (CLOCK/DEBUG)\n", mmio_readw(ich_spibar + 0x6c));
	if (mmio_readw(ich_spibar) & (1 << 15)) {
		msg_pwarn("Warning: SPI Configuration Lockdown activated.\n");
		ichspi_lock = true;
	}

	ich_set_bbar(0, ich_generation);
	ich_init_opcodes(ich_generation);

	return 0;
}
