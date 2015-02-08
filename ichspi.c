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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#if defined(__i386__) || defined(__x86_64__)

#include <string.h>
#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"
#include "spi.h"
#include "ich_descriptors.h"

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
#define HSFC_FGO_OFF		0	/* 0: Flash Cycle Go */
#define HSFC_FGO		(0x1 << HSFC_FGO_OFF)
#define HSFC_FCYCLE_OFF		1	/* 1-2: FLASH Cycle */
#define HSFC_FCYCLE		(0x3 << HSFC_FCYCLE_OFF)
					/* 3-7: reserved */
#define HSFC_FDBC_OFF		8	/* 8-13: Flash Data Byte Count */
#define HSFC_FDBC		(0x3f << HSFC_FDBC_OFF)
					/* 14: reserved */
#define HSFC_SME_OFF		15	/* 15: SPI SMI# Enable */
#define HSFC_SME		(0x1 << HSFC_SME_OFF)

#define ICH9_REG_FADDR		0x08	/* 32 Bits */
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

/* ICH SPI configuration lock-down. May be set during chipset enabling. */
static int ichspi_lock = 0;

static enum ich_chipset ich_generation = CHIPSET_ICH_UNKNOWN;
uint32_t ichspi_bbar = 0;

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
static int find_opcode(OPCODES *op, uint8_t opcode);
static int find_preop(OPCODES *op, uint8_t preop);
static int generate_opcodes(OPCODES * op);
static int program_opcodes(OPCODES *op, int enable_undo);
static int run_opcode(const struct flashctx *flash, OPCODE op, uint32_t offset,
		      uint8_t datalength, uint8_t * data);

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
	 {JEDEC_BE_D8, SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS, 0},	// Erase Sector
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

#define pprint_reg(reg, bit, val, sep) msg_pdbg("%s=%d" sep, #bit, (val & reg##_##bit)>>reg##_##bit##_OFF)

static void prettyprint_ich9_reg_hsfs(uint16_t reg_val)
{
	msg_pdbg("HSFS: ");
	pprint_reg(HSFS, FDONE, reg_val, ", ");
	pprint_reg(HSFS, FCERR, reg_val, ", ");
	pprint_reg(HSFS, AEL, reg_val, ", ");
	pprint_reg(HSFS, BERASE, reg_val, ", ");
	pprint_reg(HSFS, SCIP, reg_val, ", ");
	pprint_reg(HSFS, FDOPSS, reg_val, ", ");
	pprint_reg(HSFS, FDV, reg_val, ", ");
	pprint_reg(HSFS, FLOCKDN, reg_val, "\n");
}

static void prettyprint_ich9_reg_hsfc(uint16_t reg_val)
{
	msg_pdbg("HSFC: ");
	pprint_reg(HSFC, FGO, reg_val, ", ");
	pprint_reg(HSFC, FCYCLE, reg_val, ", ");
	pprint_reg(HSFC, FDBC, reg_val, ", ");
	pprint_reg(HSFC, SME, reg_val, "\n");
}

static void prettyprint_ich9_reg_ssfs(uint32_t reg_val)
{
	msg_pdbg("SSFS: ");
	pprint_reg(SSFS, SCIP, reg_val, ", ");
	pprint_reg(SSFS, FDONE, reg_val, ", ");
	pprint_reg(SSFS, FCERR, reg_val, ", ");
	pprint_reg(SSFS, AEL, reg_val, "\n");
}

static void prettyprint_ich9_reg_ssfc(uint32_t reg_val)
{
	msg_pdbg("SSFC: ");
	pprint_reg(SSFC, SCGO, reg_val, ", ");
	pprint_reg(SSFC, ACS, reg_val, ", ");
	pprint_reg(SSFC, SPOP, reg_val, ", ");
	pprint_reg(SSFC, COP, reg_val, ", ");
	pprint_reg(SSFC, DBC, reg_val, ", ");
	pprint_reg(SSFC, SME, reg_val, ", ");
	pprint_reg(SSFC, SCF, reg_val, "\n");
}

static uint8_t lookup_spi_type(uint8_t opcode)
{
	int a;

	for (a = 0; a < ARRAY_SIZE(POSSIBLE_OPCODES); a++) {
		if (POSSIBLE_OPCODES[a].opcode == opcode)
			return POSSIBLE_OPCODES[a].spi_type;
	}

	return 0xFF;
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
	program_opcodes(curopcodes, 0);
	oppos = find_opcode(curopcodes, opcode);
	msg_pdbg2("on-the-fly OPCODE (0x%02X) re-programmed, op-pos=%d\n", opcode, oppos);
	return oppos;
}

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

/* Create a struct OPCODES based on what we find in the locked down chipset. */
static int generate_opcodes(OPCODES * op)
{
	int a;
	uint16_t preop, optype;
	uint32_t opmenu[2];

	if (op == NULL) {
		msg_perr("\n%s: null OPCODES pointer!\n", __func__);
		return -1;
	}

	switch (ich_generation) {
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
		preop = REGREAD16(ICH9_REG_PREOP);
		optype = REGREAD16(ICH9_REG_OPTYPE);
		opmenu[0] = REGREAD32(ICH9_REG_OPMENU);
		opmenu[1] = REGREAD32(ICH9_REG_OPMENU + 4);
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

static int program_opcodes(OPCODES *op, int enable_undo)
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

	msg_pdbg2("\n%s: preop=%04x optype=%04x opmenu=%08x%08x\n", __func__, preop, optype, opmenu[0], opmenu[1]);
	switch (ich_generation) {
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
			rmmio_valw(ich_spibar + ICH9_REG_PREOP);
			rmmio_valw(ich_spibar + ICH9_REG_OPTYPE);
			rmmio_vall(ich_spibar + ICH9_REG_OPMENU);
			rmmio_vall(ich_spibar + ICH9_REG_OPMENU + 4);
		}
		mmio_writew(preop, ich_spibar + ICH9_REG_PREOP);
		mmio_writew(optype, ich_spibar + ICH9_REG_OPTYPE);
		mmio_writel(opmenu[0], ich_spibar + ICH9_REG_OPMENU);
		mmio_writel(opmenu[1], ich_spibar + ICH9_REG_OPMENU + 4);
		break;
	}

	return 0;
}

/*
 * Returns -1 if at least one mandatory opcode is inaccessible, 0 otherwise.
 * FIXME: this should also check for
 *   - at least one probing opcode (RDID (incl. AT25F variants?), REMS, RES?)
 *   - at least one erasing opcode (lots.)
 *   - at least one program opcode (BYTE_PROGRAM, AAI_WORD_PROGRAM, ...?)
 *   - necessary preops? (EWSR, WREN, ...?)
 */
static int ich_missing_opcodes()
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
static void ich_set_bbar(uint32_t min_addr)
{
	int bbar_off;
	switch (ich_generation) {
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
		msg_pdbg("Reserved bits in BBAR not zero: 0x%08x\n",
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
		msg_perr("Setting BBAR to 0x%08x failed! New value: 0x%08x.\n",
			 min_addr, ichspi_bbar);
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

/* This function generates OPCODES from or programs OPCODES to ICH according to
 * the chipset's SPI configuration lock.
 *
 * It should be called before ICH sends any spi command.
 */
static int ich_init_opcodes(void)
{
	int rc = 0;
	OPCODES *curopcodes_done;

	if (curopcodes)
		return 0;

	if (ichspi_lock) {
		msg_pdbg("Reading OPCODES... ");
		curopcodes_done = &O_EXISTING;
		rc = generate_opcodes(curopcodes_done);
	} else {
		msg_pdbg("Programming OPCODES... ");
		curopcodes_done = &O_ST_M25P;
		rc = program_opcodes(curopcodes_done, 1);
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

static int ich7_run_opcode(OPCODE op, uint32_t offset,
			   uint8_t datalength, uint8_t * data, int maxdata)
{
	int write_cmd = 0;
	int timeout;
	uint32_t temp32;
	uint16_t temp16;
	uint64_t opmenu;
	int opcode_index;

	/* Is it a write command? */
	if ((op.spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS)
	    || (op.spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS)) {
		write_cmd = 1;
	}

	timeout = 100 * 60;	/* 60 ms are 9.6 million cycles at 16 MHz. */
	while ((REGREAD16(ICH7_REG_SPIS) & SPIS_SCIP) && --timeout) {
		programmer_delay(10);
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
		/* And fall through. */
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
		programmer_delay(10);
	}
	if (!timeout) {
		msg_perr("timeout, ICH7_REG_SPIS=0x%04x\n",
			 REGREAD16(ICH7_REG_SPIS));
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
	int write_cmd = 0;
	int timeout;
	uint32_t temp32;
	uint64_t opmenu;
	int opcode_index;

	/* Is it a write command? */
	if ((op.spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS)
	    || (op.spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS)) {
		write_cmd = 1;
	}

	timeout = 100 * 60;	/* 60 ms are 9.6 million cycles at 16 MHz. */
	while ((REGREAD8(ICH9_REG_SSFS) & SSFS_SCIP) && --timeout) {
		programmer_delay(10);
	}
	if (!timeout) {
		msg_perr("Error: SCIP never cleared!\n");
		return 1;
	}

	/* Program offset in flash into FADDR while preserve the reserved bits
	 * and clearing the 25. address bit which is only useable in hwseq. */
	temp32 = REGREAD32(ICH9_REG_FADDR) & ~0x01FFFFFF;
	REGWRITE32(ICH9_REG_FADDR, (offset & 0x00FFFFFF) | temp32);

	/* Program data into FDATA0 to N */
	if (write_cmd && (datalength != 0))
		ich_fill_data(data, datalength, ICH9_REG_FDATA0);

	/* Assemble SSFS + SSFC */
	temp32 = REGREAD32(ICH9_REG_SSFS);
	/* Keep reserved bits only */
	temp32 &= SSFS_RESERVED_MASK | SSFC_RESERVED_MASK;
	/* Clear cycle done and cycle error status registers */
	temp32 |= (SSFS_FDONE | SSFS_FCERR);
	REGWRITE32(ICH9_REG_SSFS, temp32);

	/* Use 20 MHz */
	temp32 |= SSFC_SCF_20MHZ;

	/* Set data byte count (DBC) and data cycle bit (DS) */
	if (datalength != 0) {
		uint32_t datatemp;
		temp32 |= SSFC_DS;
		datatemp = ((((uint32_t)datalength - 1) << SSFC_DBC_OFF) &
			    SSFC_DBC);
		temp32 |= datatemp;
	}

	/* Select opcode */
	opmenu = REGREAD32(ICH9_REG_OPMENU);
	opmenu |= ((uint64_t)REGREAD32(ICH9_REG_OPMENU + 4)) << 32;

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
		/* And fall through. */
	case 1:
		/* Atomic command (preop+op) */
		temp32 |= SSFC_ACS;
		timeout = 100 * 1000 * 60;	/* 60 seconds */
		break;
	}

	/* Start */
	temp32 |= SSFC_SCGO;

	/* write it */
	REGWRITE32(ICH9_REG_SSFS, temp32);

	/* Wait for Cycle Done Status or Flash Cycle Error. */
	while (((REGREAD32(ICH9_REG_SSFS) & (SSFS_FDONE | SSFS_FCERR)) == 0) &&
	       --timeout) {
		programmer_delay(10);
	}
	if (!timeout) {
		msg_perr("timeout, ICH9_REG_SSFS=0x%08x\n",
			 REGREAD32(ICH9_REG_SSFS));
		return 1;
	}

	/* FIXME make sure we do not needlessly cause transaction errors. */
	temp32 = REGREAD32(ICH9_REG_SSFS);
	if (temp32 & SSFS_FCERR) {
		msg_perr("Transaction error!\n");
		prettyprint_ich9_reg_ssfs(temp32);
		prettyprint_ich9_reg_ssfc(temp32);
		/* keep reserved bits */
		temp32 &= SSFS_RESERVED_MASK | SSFC_RESERVED_MASK;
		/* Clear the transaction error. */
		REGWRITE32(ICH9_REG_SSFS, temp32 | SSFS_FCERR);
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

static int ich_spi_send_command(struct flashctx *flash, unsigned int writecnt,
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
			msg_pdbg("Invalid OPCODE 0x%02x, will not execute.\n",
				 cmd);
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
			"0x%02x, got writecnt=%i, want =4\n", __func__, cmd,
			writecnt);
		return SPI_INVALID_LENGTH;
	}
	if ((opcode->spi_type == SPI_OPCODE_TYPE_READ_NO_ADDRESS) &&
	    (writecnt != 1)) {
		msg_perr("%s: Internal command size error for opcode "
			"0x%02x, got writecnt=%i, want =1\n", __func__, cmd,
			writecnt);
		return SPI_INVALID_LENGTH;
	}
	if ((opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) &&
	    (writecnt < 4)) {
		msg_perr("%s: Internal command size error for opcode "
			"0x%02x, got writecnt=%i, want >=4\n", __func__, cmd,
			writecnt);
		return SPI_INVALID_LENGTH;
	}
	if (((opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) ||
	     (opcode->spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS)) &&
	    (readcnt)) {
		msg_perr("%s: Internal command size error for opcode "
			"0x%02x, got readcnt=%i, want =0\n", __func__, cmd,
			readcnt);
		return SPI_INVALID_LENGTH;
	}

	/* if opcode-type requires an address */
	if (opcode->spi_type == SPI_OPCODE_TYPE_READ_WITH_ADDRESS ||
	    opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) {
		addr = (writearr[1] << 16) |
		    (writearr[2] << 8) | (writearr[3] << 0);
		if (addr < ichspi_bbar) {
			msg_perr("%s: Address 0x%06x below allowed "
				 "range 0x%06x-0xffffff\n", __func__,
				 addr, ichspi_bbar);
			return SPI_INVALID_ADDRESS;
		}
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

	result = run_opcode(flash, *opcode, addr, count, data);
	if (result) {
		msg_pdbg("Running OPCODE 0x%02x failed ", opcode->opcode);
		if ((opcode->spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS) ||
		    (opcode->spi_type == SPI_OPCODE_TYPE_READ_WITH_ADDRESS)) {
			msg_pdbg("at address 0x%06x ", addr);
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
				msg_pspew("%3d: 0x%02x\n", i, data[i]);
			}
		}
	}

	return result;
}

static struct hwseq_data {
	uint32_t size_comp0;
	uint32_t size_comp1;
} hwseq_data;

/* Sets FLA in FADDR to (addr & 0x01FFFFFF) without touching other bits. */
static void ich_hwseq_set_addr(uint32_t addr)
{
	uint32_t addr_old = REGREAD32(ICH9_REG_FADDR) & ~0x01FFFFFF;
	REGWRITE32(ICH9_REG_FADDR, (addr & 0x01FFFFFF) | addr_old);
}

/* Sets FADDR.FLA to 'addr' and returns the erase block size in bytes
 * of the block containing this address. May return nonsense if the address is
 * not valid. The erase block size for a specific address depends on the flash
 * partition layout as specified by FPB and the partition properties as defined
 * by UVSCC and LVSCC respectively. An alternative to implement this method
 * would be by querying FPB and the respective VSCC register directly.
 */
static uint32_t ich_hwseq_get_erase_block_size(unsigned int addr)
{
	uint8_t enc_berase;
	static const uint32_t dec_berase[4] = {
		256,
		4 * 1024,
		8 * 1024,
		64 * 1024
	};

	ich_hwseq_set_addr(addr);
	enc_berase = (REGREAD16(ICH9_REG_HSFS) & HSFS_BERASE) >>
		     HSFS_BERASE_OFF;
	return dec_berase[enc_berase];
}

/* Polls for Cycle Done Status, Flash Cycle Error or timeout in 8 us intervals.
   Resets all error flags in HSFS.
   Returns 0 if the cycle completes successfully without errors within
   timeout us, 1 on errors. */
static int ich_hwseq_wait_for_cycle_complete(unsigned int timeout,
					     unsigned int len)
{
	uint16_t hsfs;
	uint32_t addr;

	timeout /= 8; /* scale timeout duration to counter */
	while ((((hsfs = REGREAD16(ICH9_REG_HSFS)) &
		 (HSFS_FDONE | HSFS_FCERR)) == 0) &&
	       --timeout) {
		programmer_delay(8);
	}
	REGWRITE16(ICH9_REG_HSFS, REGREAD16(ICH9_REG_HSFS));
	if (!timeout) {
		addr = REGREAD32(ICH9_REG_FADDR) & 0x01FFFFFF;
		msg_perr("Timeout error between offset 0x%08x and "
			 "0x%08x (= 0x%08x + %d)!\n",
			 addr, addr + len - 1, addr, len - 1);
		prettyprint_ich9_reg_hsfs(hsfs);
		prettyprint_ich9_reg_hsfc(REGREAD16(ICH9_REG_HSFC));
		return 1;
	}

	if (hsfs & HSFS_FCERR) {
		addr = REGREAD32(ICH9_REG_FADDR) & 0x01FFFFFF;
		msg_perr("Transaction error between offset 0x%08x and "
			 "0x%08x (= 0x%08x + %d)!\n",
			 addr, addr + len - 1, addr, len - 1);
		prettyprint_ich9_reg_hsfs(hsfs);
		prettyprint_ich9_reg_hsfc(REGREAD16(ICH9_REG_HSFC));
		return 1;
	}
	return 0;
}

static int ich_hwseq_probe(struct flashctx *flash)
{
	uint32_t total_size, boundary;
	uint32_t erase_size_low, size_low, erase_size_high, size_high;
	struct block_eraser *eraser;

	total_size = hwseq_data.size_comp0 + hwseq_data.size_comp1;
	msg_cdbg("Hardware sequencing reports %d attached SPI flash chip",
		 (hwseq_data.size_comp1 != 0) ? 2 : 1);
	if (hwseq_data.size_comp1 != 0)
		msg_cdbg("s with a combined");
	else
		msg_cdbg(" with a");
	msg_cdbg(" density of %d kB.\n", total_size / 1024);
	flash->chip->total_size = total_size / 1024;

	eraser = &(flash->chip->block_erasers[0]);
	boundary = (REGREAD32(ICH9_REG_FPB) & FPB_FPBA) << 12;
	size_high = total_size - boundary;
	erase_size_high = ich_hwseq_get_erase_block_size(boundary);

	if (boundary == 0) {
		msg_cdbg2("There is only one partition containing the whole "
			 "address space (0x%06x - 0x%06x).\n", 0, size_high-1);
		eraser->eraseblocks[0].size = erase_size_high;
		eraser->eraseblocks[0].count = size_high / erase_size_high;
		msg_cdbg2("There are %d erase blocks with %d B each.\n",
			 size_high / erase_size_high, erase_size_high);
	} else {
		msg_cdbg2("The flash address space (0x%06x - 0x%06x) is divided "
			 "at address 0x%06x in two partitions.\n",
			 0, total_size-1, boundary);
		size_low = total_size - size_high;
		erase_size_low = ich_hwseq_get_erase_block_size(0);

		eraser->eraseblocks[0].size = erase_size_low;
		eraser->eraseblocks[0].count = size_low / erase_size_low;
		msg_cdbg("The first partition ranges from 0x%06x to 0x%06x.\n",
			 0, size_low-1);
		msg_cdbg("In that range are %d erase blocks with %d B each.\n",
			 size_low / erase_size_low, erase_size_low);

		eraser->eraseblocks[1].size = erase_size_high;
		eraser->eraseblocks[1].count = size_high / erase_size_high;
		msg_cdbg("The second partition ranges from 0x%06x to 0x%06x.\n",
			 boundary, total_size-1);
		msg_cdbg("In that range are %d erase blocks with %d B each.\n",
			 size_high / erase_size_high, erase_size_high);
	}
	flash->chip->tested = TEST_OK_PREW;
	return 1;
}

static int ich_hwseq_block_erase(struct flashctx *flash, unsigned int addr,
				 unsigned int len)
{
	uint32_t erase_block;
	uint16_t hsfc;
	uint32_t timeout = 5000 * 1000; /* 5 s for max 64 kB */

	erase_block = ich_hwseq_get_erase_block_size(addr);
	if (len != erase_block) {
		msg_cerr("Erase block size for address 0x%06x is %d B, "
			 "but requested erase block size is %d B. "
			 "Not erasing anything.\n", addr, erase_block, len);
		return -1;
	}

	/* Although the hardware supports this (it would erase the whole block
	 * containing the address) we play safe here. */
	if (addr % erase_block != 0) {
		msg_cerr("Erase address 0x%06x is not aligned to the erase "
			 "block boundary (any multiple of %d). "
			 "Not erasing anything.\n", addr, erase_block);
		return -1;
	}

	if (addr + len > flash->chip->total_size * 1024) {
		msg_perr("Request to erase some inaccessible memory address(es)"
			 " (addr=0x%x, len=%d). "
			 "Not erasing anything.\n", addr, len);
		return -1;
	}

	msg_pdbg("Erasing %d bytes starting at 0x%06x.\n", len, addr);
	ich_hwseq_set_addr(addr);

	/* make sure FDONE, FCERR, AEL are cleared by writing 1 to them */
	REGWRITE16(ICH9_REG_HSFS, REGREAD16(ICH9_REG_HSFS));

	hsfc = REGREAD16(ICH9_REG_HSFC);
	hsfc &= ~HSFC_FCYCLE; /* clear operation */
	hsfc |= (0x3 << HSFC_FCYCLE_OFF); /* set erase operation */
	hsfc |= HSFC_FGO; /* start */
	msg_pdbg("HSFC used for block erasing: ");
	prettyprint_ich9_reg_hsfc(hsfc);
	REGWRITE16(ICH9_REG_HSFC, hsfc);

	if (ich_hwseq_wait_for_cycle_complete(timeout, len))
		return -1;
	return 0;
}

static int ich_hwseq_read(struct flashctx *flash, uint8_t *buf,
			  unsigned int addr, unsigned int len)
{
	uint16_t hsfc;
	uint16_t timeout = 100 * 60;
	uint8_t block_len;

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

		ich_hwseq_set_addr(addr);
		hsfc = REGREAD16(ICH9_REG_HSFC);
		hsfc &= ~HSFC_FCYCLE; /* set read operation */
		hsfc &= ~HSFC_FDBC; /* clear byte count */
		/* set byte count */
		hsfc |= (((block_len - 1) << HSFC_FDBC_OFF) & HSFC_FDBC);
		hsfc |= HSFC_FGO; /* start */
		REGWRITE16(ICH9_REG_HSFC, hsfc);

		if (ich_hwseq_wait_for_cycle_complete(timeout, block_len))
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
	uint16_t hsfc;
	uint16_t timeout = 100 * 60;
	uint8_t block_len;

	if (addr + len > flash->chip->total_size * 1024) {
		msg_perr("Request to write to an inaccessible memory address "
			 "(addr=0x%x, len=%d).\n", addr, len);
		return -1;
	}

	msg_pdbg("Writing %d bytes starting at 0x%06x.\n", len, addr);
	/* clear FDONE, FCERR, AEL by writing 1 to them (if they are set) */
	REGWRITE16(ICH9_REG_HSFS, REGREAD16(ICH9_REG_HSFS));

	while (len > 0) {
		ich_hwseq_set_addr(addr);
		/* Obey programmer limit... */
		block_len = min(len, flash->mst->opaque.max_data_write);
		/* as well as flash chip page borders as demanded in the Intel datasheets. */
		block_len = min(block_len, 256 - (addr & 0xFF));
		ich_fill_data(buf, block_len, ICH9_REG_FDATA0);
		hsfc = REGREAD16(ICH9_REG_HSFC);
		hsfc &= ~HSFC_FCYCLE; /* clear operation */
		hsfc |= (0x2 << HSFC_FCYCLE_OFF); /* set write operation */
		hsfc &= ~HSFC_FDBC; /* clear byte count */
		/* set byte count */
		hsfc |= (((block_len - 1) << HSFC_FDBC_OFF) & HSFC_FDBC);
		hsfc |= HSFC_FGO; /* start */
		REGWRITE16(ICH9_REG_HSFC, hsfc);

		if (ich_hwseq_wait_for_cycle_complete(timeout, block_len))
			return -1;
		addr += block_len;
		buf += block_len;
		len -= block_len;
	}
	return 0;
}

static int ich_spi_send_multicommand(struct flashctx *flash,
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

#define ICH_BMWAG(x) ((x >> 24) & 0xff)
#define ICH_BMRAG(x) ((x >> 16) & 0xff)
#define ICH_BRWA(x)  ((x >>  8) & 0xff)
#define ICH_BRRA(x)  ((x >>  0) & 0xff)

/* returns 0 if region is unused or r/w */
static int ich9_handle_frap(uint32_t frap, int i)
{
	static const char *const access_names[4] = {
		"locked", "read-only", "write-only", "read-write"
	};
	static const char *const region_names[5] = {
		"Flash Descriptor", "BIOS", "Management Engine",
		"Gigabit Ethernet", "Platform Data"
	};
	uint32_t base, limit;
	int rwperms = (((ICH_BRWA(frap) >> i) & 1) << 1) |
		      (((ICH_BRRA(frap) >> i) & 1) << 0);
	int offset = ICH9_REG_FREG0 + i * 4;
	uint32_t freg = mmio_readl(ich_spibar + offset);

	base  = ICH_FREG_BASE(freg);
	limit = ICH_FREG_LIMIT(freg);
	if (base > limit || (freg == 0 && i > 0)) {
		/* this FREG is disabled */
		msg_pdbg2("0x%02X: 0x%08x FREG%i: %s region is unused.\n",
			  offset, freg, i, region_names[i]);
		return 0;
	}
	msg_pdbg("0x%02X: 0x%08x ", offset, freg);
	if (rwperms == 0x3) {
		msg_pdbg("FREG%i: %s region (0x%08x-0x%08x) is %s.\n", i,
			 region_names[i], base, (limit | 0x0fff),
			 access_names[rwperms]);
		return 0;
	}

	msg_pwarn("FREG%i: Warning: %s region (0x%08x-0x%08x) is %s.\n", i,
		  region_names[i], base, (limit | 0x0fff),
		  access_names[rwperms]);
	return 1;
}

	/* In contrast to FRAP and the master section of the descriptor the bits
	 * in the PR registers have an inverted meaning. The bits in FRAP
	 * indicate read and write access _grant_. Here they indicate read
	 * and write _protection_ respectively. If both bits are 0 the address
	 * bits are ignored.
	 */
#define ICH_PR_PERMS(pr)	(((~((pr) >> PR_RP_OFF) & 1) << 0) | \
				 ((~((pr) >> PR_WP_OFF) & 1) << 1))

/* returns 0 if range is unused (i.e. r/w) */
static int ich9_handle_pr(int i)
{
	static const char *const access_names[3] = {
		"locked", "read-only", "write-only"
	};
	uint8_t off = ICH9_REG_PR0 + (i * 4);
	uint32_t pr = mmio_readl(ich_spibar + off);
	unsigned int rwperms = ICH_PR_PERMS(pr);

	if (rwperms == 0x3) {
		msg_pdbg2("0x%02X: 0x%08x (PR%u is unused)\n", off, pr, i);
		return 0;
	}

	msg_pdbg("0x%02X: 0x%08x ", off, pr);
	msg_pwarn("PR%u: Warning: 0x%08x-0x%08x is %s.\n", i, ICH_FREG_BASE(pr),
		  ICH_FREG_LIMIT(pr) | 0x0fff, access_names[rwperms]);
	return 1;
}

/* Set/Clear the read and write protection enable bits of PR register @i
 * according to @read_prot and @write_prot. */
static void ich9_set_pr(int i, int read_prot, int write_prot)
{
	void *addr = ich_spibar + ICH9_REG_PR0 + (i * 4);
	uint32_t old = mmio_readl(addr);
	uint32_t new;

	msg_gspew("PR%u is 0x%08x", i, old);
	new = old & ~((1 << PR_RP_OFF) | (1 << PR_WP_OFF));
	if (read_prot)
		new |= (1 << PR_RP_OFF);
	if (write_prot)
		new |= (1 << PR_WP_OFF);
	if (old == new) {
		msg_gspew(" already.\n");
		return;
	}
	msg_gspew(", trying to set it to 0x%08x ", new);
	rmmio_writel(new, addr);
	msg_gspew("resulted in 0x%08x.\n", mmio_readl(addr));
}

static const struct spi_master spi_master_ich7 = {
	.type = SPI_CONTROLLER_ICH7,
	.max_data_read = 64,
	.max_data_write = 64,
	.command = ich_spi_send_command,
	.multicommand = ich_spi_send_multicommand,
	.read = default_spi_read,
	.write_256 = default_spi_write_256,
	.write_aai = default_spi_write_aai,
};

static const struct spi_master spi_master_ich9 = {
	.type = SPI_CONTROLLER_ICH9,
	.max_data_read = 64,
	.max_data_write = 64,
	.command = ich_spi_send_command,
	.multicommand = ich_spi_send_multicommand,
	.read = default_spi_read,
	.write_256 = default_spi_write_256,
	.write_aai = default_spi_write_aai,
};

static const struct opaque_master opaque_master_ich_hwseq = {
	.max_data_read = 64,
	.max_data_write = 64,
	.probe = ich_hwseq_probe,
	.read = ich_hwseq_read,
	.write = ich_hwseq_write,
	.erase = ich_hwseq_block_erase,
};

int ich_init_spi(struct pci_dev *dev, void *spibar, enum ich_chipset ich_gen)
{
	int i;
	uint16_t tmp2;
	uint32_t tmp;
	char *arg;
	int ich_spi_force = 0;
	int ich_spi_rw_restricted = 0;
	int desc_valid = 0;
	struct ich_descriptors desc = {{ 0 }};
	enum ich_spi_mode {
		ich_auto,
		ich_hwseq,
		ich_swseq
	} ich_spi_mode = ich_auto;

	ich_generation = ich_gen;
	ich_spibar = spibar;

	switch (ich_generation) {
	case CHIPSET_ICH7:
	case CHIPSET_TUNNEL_CREEK:
	case CHIPSET_CENTERTON:
		msg_pdbg("0x00: 0x%04x     (SPIS)\n",
			     mmio_readw(ich_spibar + 0));
		msg_pdbg("0x02: 0x%04x     (SPIC)\n",
			     mmio_readw(ich_spibar + 2));
		msg_pdbg("0x04: 0x%08x (SPIA)\n",
			     mmio_readl(ich_spibar + 4));
		ichspi_bbar = mmio_readl(ich_spibar + 0x50);
		msg_pdbg("0x50: 0x%08x (BBAR)\n",
			     ichspi_bbar);
		msg_pdbg("0x54: 0x%04x     (PREOP)\n",
			     mmio_readw(ich_spibar + 0x54));
		msg_pdbg("0x56: 0x%04x     (OPTYPE)\n",
			     mmio_readw(ich_spibar + 0x56));
		msg_pdbg("0x58: 0x%08x (OPMENU)\n",
			     mmio_readl(ich_spibar + 0x58));
		msg_pdbg("0x5c: 0x%08x (OPMENU+4)\n",
			     mmio_readl(ich_spibar + 0x5c));
		for (i = 0; i < 3; i++) {
			int offs;
			offs = 0x60 + (i * 4);
			msg_pdbg("0x%02x: 0x%08x (PBR%d)\n", offs,
				     mmio_readl(ich_spibar + offs), i);
		}
		if (mmio_readw(ich_spibar) & (1 << 15)) {
			msg_pwarn("WARNING: SPI Configuration Lockdown activated.\n");
			ichspi_lock = 1;
		}
		ich_init_opcodes();
		ich_set_bbar(0);
		register_spi_master(&spi_master_ich7);
		break;
	case CHIPSET_ICH8:
	default:		/* Future version might behave the same */
		arg = extract_programmer_param("ich_spi_mode");
		if (arg && !strcmp(arg, "hwseq")) {
			ich_spi_mode = ich_hwseq;
			msg_pspew("user selected hwseq\n");
		} else if (arg && !strcmp(arg, "swseq")) {
			ich_spi_mode = ich_swseq;
			msg_pspew("user selected swseq\n");
		} else if (arg && !strcmp(arg, "auto")) {
			msg_pspew("user selected auto\n");
			ich_spi_mode = ich_auto;
		} else if (arg && !strlen(arg)) {
			msg_perr("Missing argument for ich_spi_mode.\n");
			free(arg);
			return ERROR_FATAL;
		} else if (arg) {
			msg_perr("Unknown argument for ich_spi_mode: %s\n",
				 arg);
			free(arg);
			return ERROR_FATAL;
		}
		free(arg);

		arg = extract_programmer_param("ich_spi_force");
		if (arg && !strcmp(arg, "yes")) {
			ich_spi_force = 1;
			msg_pspew("ich_spi_force enabled.\n");
		} else if (arg && !strlen(arg)) {
			msg_perr("Missing argument for ich_spi_force.\n");
			free(arg);
			return ERROR_FATAL;
		} else if (arg) {
			msg_perr("Unknown argument for ich_spi_force: \"%s\" "
				 "(not \"yes\").\n", arg);
			free(arg);
			return ERROR_FATAL;
		}
		free(arg);

		tmp2 = mmio_readw(ich_spibar + ICH9_REG_HSFS);
		msg_pdbg("0x04: 0x%04x (HSFS)\n", tmp2);
		prettyprint_ich9_reg_hsfs(tmp2);
		if (tmp2 & HSFS_FLOCKDN) {
			msg_pwarn("Warning: SPI Configuration Lockdown activated.\n");
			ichspi_lock = 1;
		}
		if (tmp2 & HSFS_FDV)
			desc_valid = 1;
		if (!(tmp2 & HSFS_FDOPSS) && desc_valid)
			msg_pinfo("The Flash Descriptor Override Strap-Pin is set. Restrictions implied by\n"
				  "the Master Section of the flash descriptor are NOT in effect. Please note\n"
				  "that Protected Range (PR) restrictions still apply.\n");
		ich_init_opcodes();

		if (desc_valid) {
			tmp2 = mmio_readw(ich_spibar + ICH9_REG_HSFC);
			msg_pdbg("0x06: 0x%04x (HSFC)\n", tmp2);
			prettyprint_ich9_reg_hsfc(tmp2);
		}

		tmp = mmio_readl(ich_spibar + ICH9_REG_FADDR);
		msg_pdbg2("0x08: 0x%08x (FADDR)\n", tmp);

		if (desc_valid) {
			tmp = mmio_readl(ich_spibar + ICH9_REG_FRAP);
			msg_pdbg("0x50: 0x%08x (FRAP)\n", tmp);
			msg_pdbg("BMWAG 0x%02x, ", ICH_BMWAG(tmp));
			msg_pdbg("BMRAG 0x%02x, ", ICH_BMRAG(tmp));
			msg_pdbg("BRWA 0x%02x, ", ICH_BRWA(tmp));
			msg_pdbg("BRRA 0x%02x\n", ICH_BRRA(tmp));

			/* Handle FREGx and FRAP registers */
			for (i = 0; i < 5; i++)
				ich_spi_rw_restricted |= ich9_handle_frap(tmp, i);
			if (ich_spi_rw_restricted)
				msg_pwarn("Not all flash regions are freely accessible by flashrom. This is "
					  "most likely\ndue to an active ME. Please see http://flashrom.org/ME "
					  "for details.\n");
		}

		/* Handle PR registers */
		for (i = 0; i < 5; i++) {
			/* if not locked down try to disable PR locks first */
			if (!ichspi_lock)
				ich9_set_pr(i, 0, 0);
			ich_spi_rw_restricted |= ich9_handle_pr(i);
		}

		if (ich_spi_rw_restricted) {
			if (!ich_spi_force)
				programmer_may_write = 0;
			msg_pinfo("Writes have been disabled for safety reasons. You can enforce write\n"
				  "support with the ich_spi_force programmer option, but you will most likely\n"
				  "harm your hardware! If you force flashrom you will get no support if\n"
				  "something breaks. On a few mainboards it is possible to enable write\n"
				  "access by setting a jumper (see its documentation or the board itself).\n");
			if (ich_spi_force)
				msg_pinfo("Continuing with write support because the user forced us to!\n");
		}

		tmp = mmio_readl(ich_spibar + ICH9_REG_SSFS);
		msg_pdbg("0x90: 0x%02x (SSFS)\n", tmp & 0xff);
		prettyprint_ich9_reg_ssfs(tmp);
		if (tmp & SSFS_FCERR) {
			msg_pdbg("Clearing SSFS.FCERR\n");
			mmio_writeb(SSFS_FCERR, ich_spibar + ICH9_REG_SSFS);
		}
		msg_pdbg("0x91: 0x%06x (SSFC)\n", tmp >> 8);
		prettyprint_ich9_reg_ssfc(tmp);

		msg_pdbg("0x94: 0x%04x     (PREOP)\n",
			     mmio_readw(ich_spibar + ICH9_REG_PREOP));
		msg_pdbg("0x96: 0x%04x     (OPTYPE)\n",
			     mmio_readw(ich_spibar + ICH9_REG_OPTYPE));
		msg_pdbg("0x98: 0x%08x (OPMENU)\n",
			     mmio_readl(ich_spibar + ICH9_REG_OPMENU));
		msg_pdbg("0x9C: 0x%08x (OPMENU+4)\n",
			     mmio_readl(ich_spibar + ICH9_REG_OPMENU + 4));
		if (ich_generation == CHIPSET_ICH8 && desc_valid) {
			tmp = mmio_readl(ich_spibar + ICH8_REG_VSCC);
			msg_pdbg("0xC1: 0x%08x (VSCC)\n", tmp);
			msg_pdbg("VSCC: ");
			prettyprint_ich_reg_vscc(tmp, MSG_DEBUG, true);
		} else {
			if (ich_generation != CHIPSET_BAYTRAIL && desc_valid) {
				ichspi_bbar = mmio_readl(ich_spibar + ICH9_REG_BBAR);
				msg_pdbg("0xA0: 0x%08x (BBAR)\n",
					     ichspi_bbar);
				ich_set_bbar(0);
			}

			if (desc_valid) {
				tmp = mmio_readl(ich_spibar + ICH9_REG_LVSCC);
				msg_pdbg("0xC4: 0x%08x (LVSCC)\n", tmp);
				msg_pdbg("LVSCC: ");
				prettyprint_ich_reg_vscc(tmp, MSG_DEBUG, true);

				tmp = mmio_readl(ich_spibar + ICH9_REG_UVSCC);
				msg_pdbg("0xC8: 0x%08x (UVSCC)\n", tmp);
				msg_pdbg("UVSCC: ");
				prettyprint_ich_reg_vscc(tmp, MSG_DEBUG, false);

				tmp = mmio_readl(ich_spibar + ICH9_REG_FPB);
				msg_pdbg("0xD0: 0x%08x (FPB)\n", tmp);
			}
		}

		if (desc_valid) {
			if (read_ich_descriptors_via_fdo(ich_spibar, &desc) == ICH_RET_OK)
				prettyprint_ich_descriptors(ich_gen, &desc);

			/* If the descriptor is valid and indicates multiple
			 * flash devices we need to use hwseq to be able to
			 * access the second flash device.
			 */
			if (ich_spi_mode == ich_auto && desc.content.NC != 0) {
				msg_pinfo("Enabling hardware sequencing due to "
					  "multiple flash chips detected.\n");
				ich_spi_mode = ich_hwseq;
			}
		}

		if (ich_spi_mode == ich_auto && ichspi_lock &&
		    ich_missing_opcodes()) {
			msg_pinfo("Enabling hardware sequencing because "
				  "some important opcode is locked.\n");
			ich_spi_mode = ich_hwseq;
		}

		if (ich_spi_mode == ich_hwseq) {
			if (!desc_valid) {
				msg_perr("Hardware sequencing was requested "
					 "but the flash descriptor is not "
					 "valid. Aborting.\n");
				return ERROR_FATAL;
			}

			int tmpi = getFCBA_component_density(ich_generation, &desc, 0);
			if (tmpi < 0) {
				msg_perr("Could not determine density of flash component %d.\n", 0);
				return ERROR_FATAL;
			}
			hwseq_data.size_comp0 = tmpi;

			tmpi = getFCBA_component_density(ich_generation, &desc, 1);
			if (tmpi < 0) {
				msg_perr("Could not determine density of flash component %d.\n", 1);
				return ERROR_FATAL;
			}
			hwseq_data.size_comp1 = tmpi;

			register_opaque_master(&opaque_master_ich_hwseq);
		} else {
			register_spi_master(&spi_master_ich9);
		}
		break;
	}

	return 0;
}

static const struct spi_master spi_master_via = {
	.type = SPI_CONTROLLER_VIA,
	.max_data_read = 16,
	.max_data_write = 16,
	.command = ich_spi_send_command,
	.multicommand = ich_spi_send_multicommand,
	.read = default_spi_read,
	.write_256 = default_spi_write_256,
	.write_aai = default_spi_write_aai,
};

int via_init_spi(struct pci_dev *dev, uint32_t mmio_base)
{
	int i;

	ich_spibar = rphysmap("VIA SPI MMIO registers", mmio_base, 0x70);
	if (ich_spibar == ERROR_PTR)
		return ERROR_FATAL;
	/* Do we really need no write enable? Like the LPC one at D17F0 0x40 */

	/* Not sure if it speaks all these bus protocols. */
	internal_buses_supported = BUS_LPC | BUS_FWH;
	ich_generation = CHIPSET_ICH7;
	register_spi_master(&spi_master_via);

	msg_pdbg("0x00: 0x%04x     (SPIS)\n", mmio_readw(ich_spibar + 0));
	msg_pdbg("0x02: 0x%04x     (SPIC)\n", mmio_readw(ich_spibar + 2));
	msg_pdbg("0x04: 0x%08x (SPIA)\n", mmio_readl(ich_spibar + 4));
	for (i = 0; i < 2; i++) {
		int offs;
		offs = 8 + (i * 8);
		msg_pdbg("0x%02x: 0x%08x (SPID%d)\n", offs,
			 mmio_readl(ich_spibar + offs), i);
		msg_pdbg("0x%02x: 0x%08x (SPID%d+4)\n", offs + 4,
			 mmio_readl(ich_spibar + offs + 4), i);
	}
	ichspi_bbar = mmio_readl(ich_spibar + 0x50);
	msg_pdbg("0x50: 0x%08x (BBAR)\n", ichspi_bbar);
	msg_pdbg("0x54: 0x%04x     (PREOP)\n", mmio_readw(ich_spibar + 0x54));
	msg_pdbg("0x56: 0x%04x     (OPTYPE)\n", mmio_readw(ich_spibar + 0x56));
	msg_pdbg("0x58: 0x%08x (OPMENU)\n", mmio_readl(ich_spibar + 0x58));
	msg_pdbg("0x5c: 0x%08x (OPMENU+4)\n", mmio_readl(ich_spibar + 0x5c));
	for (i = 0; i < 3; i++) {
		int offs;
		offs = 0x60 + (i * 4);
		msg_pdbg("0x%02x: 0x%08x (PBR%d)\n", offs,
			 mmio_readl(ich_spibar + offs), i);
	}
	msg_pdbg("0x6c: 0x%04x     (CLOCK/DEBUG)\n",
		 mmio_readw(ich_spibar + 0x6c));
	if (mmio_readw(ich_spibar) & (1 << 15)) {
		msg_pwarn("Warning: SPI Configuration Lockdown activated.\n");
		ichspi_lock = 1;
	}

	ich_set_bbar(0);
	ich_init_opcodes();

	return 0;
}

#endif
