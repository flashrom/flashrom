/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Stefan Wildemann <stefan.wildemann@kontron.com>
 * Copyright (C) 2008 Claus Gindhart <claus.gindhart@kontron.com>
 * Copyright (C) 2008 Dominik Geyer <dominik.geyer@kontron.com>
 * Copyright (C) 2008 coresystems GmbH <info@coresystems.de>
 * Copyright (C) 2009, 2010 Carl-Daniel Hailfinger
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

/*
 * This module is designed for supporting the devices
 * ST M25P40
 * ST M25P80
 * ST M25P16
 * ST M25P32 already tested
 * ST M25P64
 * AT 25DF321 already tested
 * ... and many more SPI flash devices
 *
 */

#if defined(__i386__) || defined(__x86_64__)

#include <string.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

/* ICH9 controller register definition */
#define ICH9_REG_FADDR         0x08	/* 32 Bits */
#define ICH9_REG_FDATA0                0x10	/* 64 Bytes */

#define ICH9_REG_SSFS          0x90	/* 08 Bits */
#define SSFS_SCIP		0x00000001
#define SSFS_CDS		0x00000004
#define SSFS_FCERR		0x00000008
#define SSFS_AEL		0x00000010

#define ICH9_REG_SSFC          0x91	/* 24 Bits */
#define SSFC_SCGO		0x00000200
#define SSFC_ACS		0x00000400
#define SSFC_SPOP		0x00000800
#define SSFC_COP		0x00001000
#define SSFC_DBC		0x00010000
#define SSFC_DS			0x00400000
#define SSFC_SME		0x00800000
#define SSFC_SCF		0x01000000
#define SSFC_SCF_20MHZ 0x00000000
#define SSFC_SCF_33MHZ 0x01000000

#define ICH9_REG_PREOP         0x94	/* 16 Bits */
#define ICH9_REG_OPTYPE                0x96	/* 16 Bits */
#define ICH9_REG_OPMENU                0x98	/* 64 Bits */

// ICH9R SPI commands
#define SPI_OPCODE_TYPE_READ_NO_ADDRESS     0
#define SPI_OPCODE_TYPE_WRITE_NO_ADDRESS    1
#define SPI_OPCODE_TYPE_READ_WITH_ADDRESS   2
#define SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS  3

// ICH7 registers
#define ICH7_REG_SPIS          0x00	/* 16 Bits */
#define SPIS_SCIP              0x00000001
#define SPIS_CDS               0x00000004
#define SPIS_FCERR             0x00000008

/* VIA SPI is compatible with ICH7, but maxdata
   to transfer is 16 bytes.

   DATA byte count on ICH7 is 8:13, on VIA 8:11

   bit 12 is port select CS0 CS1
   bit 13 is FAST READ enable
   bit 7  is used with fast read and one shot controls CS de-assert?
*/

#define ICH7_REG_SPIC          0x02	/* 16 Bits */
#define SPIC_SCGO              0x0002
#define SPIC_ACS               0x0004
#define SPIC_SPOP              0x0008
#define SPIC_DS                0x4000

#define ICH7_REG_SPIA          0x04	/* 32 Bits */
#define ICH7_REG_SPID0         0x08	/* 64 Bytes */
#define ICH7_REG_PREOP         0x54	/* 16 Bits */
#define ICH7_REG_OPTYPE                0x56	/* 16 Bits */
#define ICH7_REG_OPMENU                0x58	/* 64 Bits */

/* ICH SPI configuration lock-down. May be set during chipset enabling. */
static int ichspi_lock = 0;

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

#define REGWRITE32(X,Y) mmio_writel(Y, ich_spibar+X)
#define REGWRITE16(X,Y) mmio_writew(Y, ich_spibar+X)
#define REGWRITE8(X,Y)  mmio_writeb(Y, ich_spibar+X)

/* Common SPI functions */
static int find_opcode(OPCODES *op, uint8_t opcode);
static int find_preop(OPCODES *op, uint8_t preop);
static int generate_opcodes(OPCODES * op);
static int program_opcodes(OPCODES * op);
static int run_opcode(OPCODE op, uint32_t offset,
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

static uint8_t lookup_spi_type(uint8_t opcode)
{
	int a;

	for (a = 0; a < sizeof(POSSIBLE_OPCODES)/sizeof(POSSIBLE_OPCODES[0]); a++) {
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
		// else we have an invalid case, will be handled below
	}
	if (spi_type <= 3) {
		int oppos=2;	// use original JEDEC_BE_D8 offset
		curopcodes->opcode[oppos].opcode = opcode;
		curopcodes->opcode[oppos].spi_type = spi_type;
		program_opcodes(curopcodes);
		oppos = find_opcode(curopcodes, opcode);
		msg_pdbg ("on-the-fly OPCODE (0x%02X) re-programmed, op-pos=%d\n", opcode, oppos);
		return oppos;
	}
	return -1;
}

static int find_opcode(OPCODES *op, uint8_t opcode)
{
	int a;

	for (a = 0; a < 8; a++) {
		if (op->opcode[a].opcode == opcode)
			return a;
	}

	return -1;
}

static int find_preop(OPCODES *op, uint8_t preop)
{
	int a;

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

	switch (spi_controller) {
	case SPI_CONTROLLER_ICH7:
	case SPI_CONTROLLER_VIA:
		preop = REGREAD16(ICH7_REG_PREOP);
		optype = REGREAD16(ICH7_REG_OPTYPE);
		opmenu[0] = REGREAD32(ICH7_REG_OPMENU);
		opmenu[1] = REGREAD32(ICH7_REG_OPMENU + 4);
		break;
	case SPI_CONTROLLER_ICH9:
		preop = REGREAD16(ICH9_REG_PREOP);
		optype = REGREAD16(ICH9_REG_OPTYPE);
		opmenu[0] = REGREAD32(ICH9_REG_OPMENU);
		opmenu[1] = REGREAD32(ICH9_REG_OPMENU + 4);
		break;
	default:
		msg_perr("%s: unsupported chipset\n", __func__);
		return -1;
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

int program_opcodes(OPCODES * op)
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

	/*Program Allowable Opcodes 4 - 7 */
	opmenu[1] = 0;
	for (a = 4; a < 8; a++) {
		opmenu[1] |= ((uint32_t) op->opcode[a].opcode) << ((a - 4) * 8);
	}

	msg_pdbg("\n%s: preop=%04x optype=%04x opmenu=%08x%08x\n", __func__, preop, optype, opmenu[0], opmenu[1]);
	switch (spi_controller) {
	case SPI_CONTROLLER_ICH7:
	case SPI_CONTROLLER_VIA:
		REGWRITE16(ICH7_REG_PREOP, preop);
		REGWRITE16(ICH7_REG_OPTYPE, optype);
		REGWRITE32(ICH7_REG_OPMENU, opmenu[0]);
		REGWRITE32(ICH7_REG_OPMENU + 4, opmenu[1]);
		break;
	case SPI_CONTROLLER_ICH9:
		REGWRITE16(ICH9_REG_PREOP, preop);
		REGWRITE16(ICH9_REG_OPTYPE, optype);
		REGWRITE32(ICH9_REG_OPMENU, opmenu[0]);
		REGWRITE32(ICH9_REG_OPMENU + 4, opmenu[1]);
		break;
	default:
		msg_perr("%s: unsupported chipset\n", __func__);
		return -1;
	}

	return 0;
}

/*
 * Try to set BBAR (BIOS Base Address Register), but read back the value in case
 * it didn't stick.
 */
void ich_set_bbar(uint32_t minaddr)
{
	switch (spi_controller) {
	case SPI_CONTROLLER_ICH7:
		mmio_writel(minaddr, ich_spibar + 0x50);
		ichspi_bbar = mmio_readl(ich_spibar + 0x50);
		/* We don't have any option except complaining. */
		if (ichspi_bbar != minaddr)
			msg_perr("Setting BBAR failed!\n");
		break;
	case SPI_CONTROLLER_ICH9:
		mmio_writel(minaddr, ich_spibar + 0xA0);
		ichspi_bbar = mmio_readl(ich_spibar + 0xA0);
		/* We don't have any option except complaining. */
		if (ichspi_bbar != minaddr)
			msg_perr("Setting BBAR failed!\n");
		break;
	default:
		/* Not sure if BBAR actually exists on VIA. */
		msg_pdbg("Setting BBAR is not implemented for VIA yet.\n");
		break;
	}
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
		rc = program_opcodes(curopcodes_done);
		/* Technically not part of opcode init, but it allows opcodes
		 * to run without transaction errors by setting the lowest
		 * allowed address to zero.
		 */
		ich_set_bbar(0);
	}

	if (rc) {
		curopcodes = NULL;
		msg_perr("failed\n");
		return 1;
	} else {
		curopcodes = curopcodes_done;
		msg_pdbg("done\n");
		return 0;
	}
}

static int ich7_run_opcode(OPCODE op, uint32_t offset,
			   uint8_t datalength, uint8_t * data, int maxdata)
{
	int write_cmd = 0;
	int timeout;
	uint32_t temp32 = 0;
	uint16_t temp16;
	uint32_t a;
	uint64_t opmenu;
	int opcode_index;

	/* Is it a write command? */
	if ((op.spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS)
	    || (op.spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS)) {
		write_cmd = 1;
	}

	/* Programm Offset in Flash into FADDR */
	REGWRITE32(ICH7_REG_SPIA, (offset & 0x00FFFFFF));	/* SPI addresses are 24 BIT only */

	/* Program data into FDATA0 to N */
	if (write_cmd && (datalength != 0)) {
		temp32 = 0;
		for (a = 0; a < datalength; a++) {
			if ((a % 4) == 0) {
				temp32 = 0;
			}

			temp32 |= ((uint32_t) data[a]) << ((a % 4) * 8);

			if ((a % 4) == 3) {
				REGWRITE32(ICH7_REG_SPID0 + (a - (a % 4)),
					   temp32);
			}
		}
		if (((a - 1) % 4) != 3) {
			REGWRITE32(ICH7_REG_SPID0 +
				   ((a - 1) - ((a - 1) % 4)), temp32);
		}

	}

	/* Assemble SPIS */
	temp16 = 0;
	/* clear error status registers */
	temp16 |= (SPIS_CDS + SPIS_FCERR);
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

	/* Handle Atomic */
	switch (op.atomic) {
	case 2:
		/* Select second preop. */
		temp16 |= SPIC_SPOP;
		/* And fall through. */
	case 1:
		/* Atomic command (preop+op) */
		temp16 |= SPIC_ACS;
		break;
	}

	/* Start */
	temp16 |= SPIC_SCGO;

	/* write it */
	REGWRITE16(ICH7_REG_SPIC, temp16);

	/* wait for cycle complete */
	timeout = 100 * 1000 * 60;	// 60s is a looong timeout.
	while (((REGREAD16(ICH7_REG_SPIS) & SPIS_CDS) == 0) && --timeout) {
		programmer_delay(10);
	}
	if (!timeout) {
		msg_perr("timeout\n");
	}

	/* FIXME: make sure we do not needlessly cause transaction errors. */
	if ((REGREAD16(ICH7_REG_SPIS) & SPIS_FCERR) != 0) {
		msg_pdbg("Transaction error!\n");
		return 1;
	}

	if ((!write_cmd) && (datalength != 0)) {
		for (a = 0; a < datalength; a++) {
			if ((a % 4) == 0) {
				temp32 = REGREAD32(ICH7_REG_SPID0 + (a));
			}

			data[a] =
			    (temp32 & (((uint32_t) 0xff) << ((a % 4) * 8)))
			    >> ((a % 4) * 8);
		}
	}

	return 0;
}

static int ich9_run_opcode(OPCODE op, uint32_t offset,
			   uint8_t datalength, uint8_t * data)
{
	int write_cmd = 0;
	int timeout;
	uint32_t temp32;
	uint32_t a;
	uint64_t opmenu;
	int opcode_index;

	/* Is it a write command? */
	if ((op.spi_type == SPI_OPCODE_TYPE_WRITE_NO_ADDRESS)
	    || (op.spi_type == SPI_OPCODE_TYPE_WRITE_WITH_ADDRESS)) {
		write_cmd = 1;
	}

	/* Programm Offset in Flash into FADDR */
	REGWRITE32(ICH9_REG_FADDR, (offset & 0x00FFFFFF));	/* SPI addresses are 24 BIT only */

	/* Program data into FDATA0 to N */
	if (write_cmd && (datalength != 0)) {
		temp32 = 0;
		for (a = 0; a < datalength; a++) {
			if ((a % 4) == 0) {
				temp32 = 0;
			}

			temp32 |= ((uint32_t) data[a]) << ((a % 4) * 8);

			if ((a % 4) == 3) {
				REGWRITE32(ICH9_REG_FDATA0 + (a - (a % 4)),
					   temp32);
			}
		}
		if (((a - 1) % 4) != 3) {
			REGWRITE32(ICH9_REG_FDATA0 +
				   ((a - 1) - ((a - 1) % 4)), temp32);
		}
	}

	/* Assemble SSFS + SSFC */
	/* keep reserved bits (23-19,7,0) */
	temp32 = REGREAD32(ICH9_REG_SSFS);
	temp32 &= 0xF8008100;

	/* clear error status registers */
	temp32 |= (SSFS_CDS + SSFS_FCERR);
	/* Use 20 MHz */
	temp32 |= SSFC_SCF_20MHZ;

	if (datalength != 0) {
		uint32_t datatemp;
		temp32 |= SSFC_DS;
		datatemp = ((uint32_t) ((datalength - 1) & 0x3f)) << (8 + 8);
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

	/* Handle Atomic */
	switch (op.atomic) {
	case 2:
		/* Select second preop. */
		temp32 |= SSFC_SPOP;
		/* And fall through. */
	case 1:
		/* Atomic command (preop+op) */
		temp32 |= SSFC_ACS;
		break;
	}

	/* Start */
	temp32 |= SSFC_SCGO;

	/* write it */
	REGWRITE32(ICH9_REG_SSFS, temp32);

	/*wait for cycle complete */
	timeout = 100 * 1000 * 60;	// 60s is a looong timeout.
	while (((REGREAD32(ICH9_REG_SSFS) & SSFS_CDS) == 0) && --timeout) {
		programmer_delay(10);
	}
	if (!timeout) {
		msg_perr("timeout\n");
	}

	/* FIXME make sure we do not needlessly cause transaction errors. */
	if ((REGREAD32(ICH9_REG_SSFS) & SSFS_FCERR) != 0) {
		msg_pdbg("Transaction error!\n");
		return 1;
	}

	if ((!write_cmd) && (datalength != 0)) {
		for (a = 0; a < datalength; a++) {
			if ((a % 4) == 0) {
				temp32 = REGREAD32(ICH9_REG_FDATA0 + (a));
			}

			data[a] =
			    (temp32 & (((uint32_t) 0xff) << ((a % 4) * 8)))
			    >> ((a % 4) * 8);
		}
	}

	return 0;
}

static int run_opcode(OPCODE op, uint32_t offset,
		      uint8_t datalength, uint8_t * data)
{
	switch (spi_controller) {
	case SPI_CONTROLLER_VIA:
		if (datalength > 16) {
			msg_perr("%s: Internal command size error for "
				"opcode 0x%02x, got datalength=%i, want <=16\n",
				__func__, op.opcode, datalength);
			return SPI_INVALID_LENGTH;
		}
		return ich7_run_opcode(op, offset, datalength, data, 16);
	case SPI_CONTROLLER_ICH7:
		if (datalength > 64) {
			msg_perr("%s: Internal command size error for "
				"opcode 0x%02x, got datalength=%i, want <=16\n",
				__func__, op.opcode, datalength);
			return SPI_INVALID_LENGTH;
		}
		return ich7_run_opcode(op, offset, datalength, data, 64);
	case SPI_CONTROLLER_ICH9:
		if (datalength > 64) {
			msg_perr("%s: Internal command size error for "
				"opcode 0x%02x, got datalength=%i, want <=16\n",
				__func__, op.opcode, datalength);
			return SPI_INVALID_LENGTH;
		}
		return ich9_run_opcode(op, offset, datalength, data);
	default:
		msg_perr("%s: unsupported chipset\n", __func__);
	}

	/* If we ever get here, something really weird happened */
	return -1;
}

int ich_spi_read(struct flashchip *flash, uint8_t * buf, int start, int len)
{
	int maxdata = 64;

	if (spi_controller == SPI_CONTROLLER_VIA)
		maxdata = 16;

	return spi_read_chunked(flash, buf, start, len, maxdata);
}

int ich_spi_write_256(struct flashchip *flash, uint8_t * buf, int start, int len)
{
	int maxdata = 64;

	if (spi_controller == SPI_CONTROLLER_VIA)
		maxdata = 16;

	return spi_write_chunked(flash, buf, start, len, maxdata);
}

int ich_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		    const unsigned char *writearr, unsigned char *readarr)
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
		if (opcode_index == -1) {
			msg_pdbg("Invalid OPCODE 0x%02x\n", cmd);
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
		switch (spi_controller) {
		case SPI_CONTROLLER_ICH7:
		case SPI_CONTROLLER_ICH9:
			if (addr < ichspi_bbar) {
				msg_perr("%s: Address 0x%06x below allowed "
					 "range 0x%06x-0xffffff\n", __func__,
					 addr, ichspi_bbar);
				return SPI_INVALID_ADDRESS;
			}
			break;
		default:
			break;
		}
	}

	/* translate read/write array/count */
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

	result = run_opcode(*opcode, addr, count, data);
	if (result) {
		msg_pdbg("run OPCODE 0x%02x failed\n", opcode->opcode);
	}

	return result;
}

int ich_spi_send_multicommand(struct spi_command *cmds)
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
		ret = ich_spi_send_command(cmds->writecnt, cmds->readcnt,
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

#define ICH_FREG_BASE(x)  ((x >>  0) & 0x1fff)
#define ICH_FREG_LIMIT(x) ((x >> 16) & 0x1fff)

static void do_ich9_spi_frap(uint32_t frap, int i)
{
	const char *access_names[4] = {
		"locked", "read-only", "write-only", "read-write"
	};
	const char *region_names[5] = {
		"Flash Descriptor", "BIOS", "Management Engine",
		"Gigabit Ethernet", "Platform Data"
	};
	uint32_t base, limit;
	int rwperms = (((ICH_BRWA(frap) >> i) & 1) << 1) |
		      (((ICH_BRRA(frap) >> i) & 1) << 0);
	int offset = 0x54 + i * 4;
	uint32_t freg = mmio_readl(ich_spibar + offset);

	msg_pdbg("0x%02X: 0x%08x (FREG%i: %s)\n",
		     offset, freg, i, region_names[i]);

	base  = ICH_FREG_BASE(freg);
	limit = ICH_FREG_LIMIT(freg);
	if (base == 0x1fff && limit == 0) {
		/* this FREG is disabled */
		msg_pdbg("%s region is unused.\n", region_names[i]);
		return;
	}

	msg_pdbg("0x%08x-0x%08x is %s\n",
		    (base << 12), (limit << 12) | 0x0fff,
		    access_names[rwperms]);
}

int ich_init_spi(struct pci_dev *dev, uint32_t base, void *rcrb,
			int ich_generation)
{
	int i;
	uint8_t old, new;
	uint16_t spibar_offset, tmp2;
	uint32_t tmp;

	buses_supported |= CHIP_BUSTYPE_SPI;
	switch (ich_generation) {
	case 7:
		spi_controller = SPI_CONTROLLER_ICH7;
		spibar_offset = 0x3020;
		break;
	case 8:
		spi_controller = SPI_CONTROLLER_ICH9;
		spibar_offset = 0x3020;
		break;
	case 9:
	case 10:
	default:		/* Future version might behave the same */
		spi_controller = SPI_CONTROLLER_ICH9;
		spibar_offset = 0x3800;
		break;
	}

	/* SPIBAR is at RCRB+0x3020 for ICH[78] and RCRB+0x3800 for ICH9. */
	msg_pdbg("SPIBAR = 0x%x + 0x%04x\n", base, spibar_offset);

	/* Assign Virtual Address */
	ich_spibar = rcrb + spibar_offset;

	switch (spi_controller) {
	case SPI_CONTROLLER_ICH7:
		msg_pdbg("0x00: 0x%04x     (SPIS)\n",
			     mmio_readw(ich_spibar + 0));
		msg_pdbg("0x02: 0x%04x     (SPIC)\n",
			     mmio_readw(ich_spibar + 2));
		msg_pdbg("0x04: 0x%08x (SPIA)\n",
			     mmio_readl(ich_spibar + 4));
		for (i = 0; i < 8; i++) {
			int offs;
			offs = 8 + (i * 8);
			msg_pdbg("0x%02x: 0x%08x (SPID%d)\n", offs,
				     mmio_readl(ich_spibar + offs), i);
			msg_pdbg("0x%02x: 0x%08x (SPID%d+4)\n", offs + 4,
				     mmio_readl(ich_spibar + offs + 4), i);
		}
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
		for (i = 0; i < 4; i++) {
			int offs;
			offs = 0x60 + (i * 4);
			msg_pdbg("0x%02x: 0x%08x (PBR%d)\n", offs,
				     mmio_readl(ich_spibar + offs), i);
		}
		msg_pdbg("\n");
		if (mmio_readw(ich_spibar) & (1 << 15)) {
			msg_pinfo("WARNING: SPI Configuration Lockdown activated.\n");
			ichspi_lock = 1;
		}
		ich_init_opcodes();
		break;
	case SPI_CONTROLLER_ICH9:
		tmp2 = mmio_readw(ich_spibar + 4);
		msg_pdbg("0x04: 0x%04x (HSFS)\n", tmp2);
		msg_pdbg("FLOCKDN %i, ", (tmp2 >> 15 & 1));
		msg_pdbg("FDV %i, ", (tmp2 >> 14) & 1);
		msg_pdbg("FDOPSS %i, ", (tmp2 >> 13) & 1);
		msg_pdbg("SCIP %i, ", (tmp2 >> 5) & 1);
		msg_pdbg("BERASE %i, ", (tmp2 >> 3) & 3);
		msg_pdbg("AEL %i, ", (tmp2 >> 2) & 1);
		msg_pdbg("FCERR %i, ", (tmp2 >> 1) & 1);
		msg_pdbg("FDONE %i\n", (tmp2 >> 0) & 1);

		tmp = mmio_readl(ich_spibar + 0x50);
		msg_pdbg("0x50: 0x%08x (FRAP)\n", tmp);
		msg_pdbg("BMWAG 0x%02x, ", ICH_BMWAG(tmp));
		msg_pdbg("BMRAG 0x%02x, ", ICH_BMRAG(tmp));
		msg_pdbg("BRWA 0x%02x, ", ICH_BRWA(tmp));
		msg_pdbg("BRRA 0x%02x\n", ICH_BRRA(tmp));

		/* print out the FREGx registers along with FRAP access bits */
		for(i = 0; i < 5; i++)
			do_ich9_spi_frap(tmp, i);

		msg_pdbg("0x74: 0x%08x (PR0)\n",
			     mmio_readl(ich_spibar + 0x74));
		msg_pdbg("0x78: 0x%08x (PR1)\n",
			     mmio_readl(ich_spibar + 0x78));
		msg_pdbg("0x7C: 0x%08x (PR2)\n",
			     mmio_readl(ich_spibar + 0x7C));
		msg_pdbg("0x80: 0x%08x (PR3)\n",
			     mmio_readl(ich_spibar + 0x80));
		msg_pdbg("0x84: 0x%08x (PR4)\n",
			     mmio_readl(ich_spibar + 0x84));
		msg_pdbg("0x90: 0x%08x (SSFS, SSFC)\n",
			     mmio_readl(ich_spibar + 0x90));
		msg_pdbg("0x94: 0x%04x     (PREOP)\n",
			     mmio_readw(ich_spibar + 0x94));
		msg_pdbg("0x96: 0x%04x     (OPTYPE)\n",
			     mmio_readw(ich_spibar + 0x96));
		msg_pdbg("0x98: 0x%08x (OPMENU)\n",
			     mmio_readl(ich_spibar + 0x98));
		msg_pdbg("0x9C: 0x%08x (OPMENU+4)\n",
			     mmio_readl(ich_spibar + 0x9C));
		ichspi_bbar = mmio_readl(ich_spibar + 0xA0);
		msg_pdbg("0xA0: 0x%08x (BBAR)\n",
			     ichspi_bbar);
		msg_pdbg("0xB0: 0x%08x (FDOC)\n",
			     mmio_readl(ich_spibar + 0xB0));
		if (tmp2 & (1 << 15)) {
			msg_pinfo("WARNING: SPI Configuration Lockdown activated.\n");
			ichspi_lock = 1;
		}
		ich_init_opcodes();
		break;
	default:
		/* Nothing */
		break;
	}

	old = pci_read_byte(dev, 0xdc);
	msg_pdbg("SPI Read Configuration: ");
	new = (old >> 2) & 0x3;
	switch (new) {
	case 0:
	case 1:
	case 2:
		msg_pdbg("prefetching %sabled, caching %sabled, ",
			     (new & 0x2) ? "en" : "dis",
			     (new & 0x1) ? "dis" : "en");
		break;
	default:
		msg_pdbg("invalid prefetching/caching settings, ");
		break;
	}
	return 0;
}

int via_init_spi(struct pci_dev *dev)
{
	uint32_t mmio_base;

	mmio_base = (pci_read_long(dev, 0xbc)) << 8;
	msg_pdbg("MMIO base at = 0x%x\n", mmio_base);
	ich_spibar = physmap("VT8237S MMIO registers", mmio_base, 0x70);

	msg_pdbg("0x6c: 0x%04x     (CLOCK/DEBUG)\n",
		     mmio_readw(ich_spibar + 0x6c));

	/* Not sure if it speaks all these bus protocols. */
	buses_supported = CHIP_BUSTYPE_LPC | CHIP_BUSTYPE_FWH | CHIP_BUSTYPE_SPI;
	spi_controller = SPI_CONTROLLER_VIA;
	ich_init_opcodes();

	return 0;
}

#endif
