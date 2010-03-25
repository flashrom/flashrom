/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2002 Linux Networx
 * (Written by Eric Biederman <ebiederman@lnxi.com> for Linux Networx)
 * Copyright (C) 2005-2007 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 */

#ifndef COREBOOT_TABLES_H
#define COREBOOT_TABLES_H

#include <stdint.h>

/* The coreboot table information is for conveying information
 * from the firmware to the loaded OS image.  Primarily this
 * is expected to be information that cannot be discovered by
 * other means, such as querying the hardware directly.
 *
 * All of the information should be Position Independent Data.  
 * That is it should be safe to relocated any of the information
 * without it's meaning/correctness changing.   For table that
 * can reasonably be used on multiple architectures the data
 * size should be fixed.  This should ease the transition between
 * 32 bit and 64 bit architectures etc.
 *
 * The completeness test for the information in this table is:
 * - Can all of the hardware be detected?
 * - Are the per motherboard constants available?
 * - Is there enough to allow a kernel to run that was written before
 *   a particular motherboard is constructed? (Assuming the kernel
 *   has drivers for all of the hardware but it does not have
 *   assumptions on how the hardware is connected together).
 *
 * With this test it should be straight forward to determine if a
 * table entry is required or not.  This should remove much of the
 * long term compatibility burden as table entries which are
 * irrelevant or have been replaced by better alternatives may be
 * dropped.  Of course it is polite and expedite to include extra
 * table entries and be backwards compatible, but it is not required.
 */

/* Since coreboot is usually compiled 32bit, gcc will align 64bit 
 * types to 32bit boundaries. If the coreboot table is dumped on a 
 * 64bit system, a uint64_t would be aligned to 64bit boundaries, 
 * breaking the table format.
 *
 * lb_uint64 will keep 64bit coreboot table values aligned to 32bit
 * to ensure compatibility. They can be accessed with the two functions
 * below: unpack_lb64() and pack_lb64()
 *
 * See also: util/lbtdump/lbtdump.c
 */

struct lb_uint64 {
	uint32_t lo;
	uint32_t hi;
};

struct lb_header {
	uint8_t signature[4];	/* LBIO */
	uint32_t header_bytes;
	uint32_t header_checksum;
	uint32_t table_bytes;
	uint32_t table_checksum;
	uint32_t table_entries;
};

/* Every entry in the boot environment list will correspond to a boot
 * info record.  Encoding both type and size.  The type is obviously
 * so you can tell what it is.  The size allows you to skip that
 * boot environment record if you don't know what it easy.  This allows
 * forward compatibility with records not yet defined.
 */
struct lb_record {
	uint32_t tag;		/* tag ID */
	uint32_t size;		/* size of record (in bytes) */
};

#define LB_TAG_UNUSED	0x0000

#define LB_TAG_MEMORY	0x0001

struct lb_memory_range {
	struct lb_uint64 start;
	struct lb_uint64 size;
	uint32_t type;
#define LB_MEM_RAM       1	/* Memory anyone can use */
#define LB_MEM_RESERVED  2	/* Don't use this memory region */
#define LB_MEM_TABLE     16	/* Ram configuration tables are kept in */
};

struct lb_memory {
	uint32_t tag;
	uint32_t size;
	struct lb_memory_range map[0];
};

#define LB_TAG_HWRPB	0x0002
struct lb_hwrpb {
	uint32_t tag;
	uint32_t size;
	uint64_t hwrpb;
};

#define LB_TAG_MAINBOARD	0x0003
struct lb_mainboard {
	uint32_t tag;
	uint32_t size;
	uint8_t vendor_idx;
	uint8_t part_number_idx;
	uint8_t strings[0];
};

#define LB_TAG_VERSION		0x0004
#define LB_TAG_EXTRA_VERSION	0x0005
#define LB_TAG_BUILD		0x0006
#define LB_TAG_COMPILE_TIME	0x0007
#define LB_TAG_COMPILE_BY	0x0008
#define LB_TAG_COMPILE_HOST	0x0009
#define LB_TAG_COMPILE_DOMAIN	0x000a
#define LB_TAG_COMPILER		0x000b
#define LB_TAG_LINKER		0x000c
#define LB_TAG_ASSEMBLER	0x000d
struct lb_string {
	uint32_t tag;
	uint32_t size;
	uint8_t string[0];
};

#define LB_TAG_FORWARD		0x0011
struct lb_forward {
	uint32_t tag;
	uint32_t size;
	uint64_t forward;
};

#endif				/* COREBOOT_TABLES_H */
