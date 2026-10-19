/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Abdelkader Boudih <coreboot@seuros.com>
 */

#include <include/test.h>
#include <string.h>

#include "tests.h"
#include "chipdrivers.h"
#include "flash.h"
#include "flashchips.h"
#include "programmer.h"
#include "spi.h"

/* How the emulated chip reaches above 16 MiB, as a real chip of that kind behaves. */
enum emu_4ba_method {
	EMU_4BA_NONE,
	EMU_4BA_B7,		/* 0xb7 and 0xe9, no WREN needed */
	EMU_4BA_B7_WREN,	/* 0xb7 and 0xe9, ignored without WREN */
	EMU_4BA_BANK,		/* bank register 0x17/0x16, bit 7 enables 4-Byte mode */
	EMU_4BA_EAR,		/* extended address register 0xc5/0xc8 */
};

struct emu_chip {
	const uint8_t *data;
	size_t len;
	enum emu_4ba_method method;
	uint32_t size;		/* flash array size in bytes */
	bool wel;
	bool in_4ba;
	uint8_t ear;
};

/* Content of the emulated flash array, different in each 16 MiB bank. */
static uint8_t emu_byte(uint32_t addr)
{
	return (addr * 2654435761u) >> 24;
}

static bool is_4ba_mode(const struct emu_chip *emu)
{
	return emu->in_4ba || (emu->method == EMU_4BA_BANK && (emu->ear & 0x80));
}

static void emu_read_sfdp(const struct emu_chip *emu, unsigned int writecnt, unsigned int readcnt,
			  const unsigned char *writearr, unsigned char *readarr)
{
	assert_int_equal(writecnt, JEDEC_SFDP_OUTSIZE - 1);
	/* The dummy byte is read back as the first byte. */
	assert_true(readcnt >= 1);

	const uint32_t addr = (writearr[1] << 16) | (writearr[2] << 8) | writearr[3];
	memset(readarr, 0xff, readcnt);
	for (unsigned int i = 0; i < readcnt - 1; i++) {
		if (addr + i < emu->len)
			readarr[i + 1] = emu->data[addr + i];
	}
}

static void emu_jedec_read(const struct emu_chip *emu, unsigned int writecnt, unsigned int readcnt,
			   const unsigned char *writearr, unsigned char *readarr)
{
	uint32_t addr;

	if (writearr[0] == JEDEC_READ_4BA || is_4ba_mode(emu)) {
		assert_int_equal(writecnt, 5);
		addr = ((uint32_t)writearr[1] << 24) | (writearr[2] << 16) | (writearr[3] << 8) | writearr[4];
	} else {
		assert_int_equal(writecnt, 4);
		const uint8_t high = emu->method == EMU_4BA_BANK ? emu->ear & 0x7f : emu->ear;
		addr = ((uint32_t)high << 24) | (writearr[1] << 16) | (writearr[2] << 8) | writearr[3];
	}
	assert_true(addr + readcnt <= emu->size);

	for (unsigned int i = 0; i < readcnt; i++)
		readarr[i] = emu_byte(addr + i);
}

/* Serves SFDP reads from a byte array, unused SFDP space reads as 0xff, and emulates a flash array. */
static int emu_spi_command(const struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
			   const unsigned char *writearr, unsigned char *readarr)
{
	struct emu_chip *emu = flash->mst->spi.data;
	const bool wel = emu->wel;

	assert_true(writecnt >= 1);
	emu->wel = false;

	switch (writearr[0]) {
	case JEDEC_SFDP:
		emu_read_sfdp(emu, writecnt, readcnt, writearr, readarr);
		break;
	case JEDEC_WREN:
		emu->wel = true;
		break;
	case JEDEC_RDSR:
		readarr[0] = wel ? SPI_SR_WEL : 0;
		emu->wel = wel;
		break;
	case JEDEC_ENTER_4_BYTE_ADDR_MODE:
	case JEDEC_EXIT_4_BYTE_ADDR_MODE:
		assert_true(emu->method == EMU_4BA_B7 || emu->method == EMU_4BA_B7_WREN);
		if (emu->method == EMU_4BA_B7 || wel)
			emu->in_4ba = writearr[0] == JEDEC_ENTER_4_BYTE_ADDR_MODE;
		break;
	case JEDEC_WRITE_EXT_ADDR_REG:
	case ALT_WRITE_EXT_ADDR_REG_17:
		assert_int_equal(writearr[0] == JEDEC_WRITE_EXT_ADDR_REG ? EMU_4BA_EAR : EMU_4BA_BANK,
				 emu->method);
		assert_int_equal(writecnt, 2);
		assert_true(wel);
		emu->ear = writearr[1];
		break;
	case JEDEC_READ:
	case JEDEC_READ_4BA:
		emu_jedec_read(emu, writecnt, readcnt, writearr, readarr);
		break;
	default:
		fail_msg("Unexpected opcode 0x%02x", writearr[0]);
	}
	return 0;
}

/* Probing starts from the generic entry, which provides the fields SFDP does not describe. */
static const struct flashchip *sfdp_generic_chip(void)
{
	for (unsigned int i = 0; i < flashchips_size - 1; i++) {
		if (flashchips[i].manufacture_id == GENERIC_MANUF_ID &&
		    flashchips[i].model_id == SFDP_DEVICE_ID)
			return &flashchips[i];
	}
	fail_msg("SFDP-capable chip entry not found");
	return NULL;
}

static int run_sfdp_probe(struct flashchip *chip, const uint8_t *data, size_t len)
{
	struct emu_chip emu = { .data = data, .len = len };
	struct registered_master mst = {
		.buses_supported = BUS_SPI,
		.spi.command = emu_spi_command,
		.spi.data = &emu,
	};
	struct flashctx flash = {
		.chip = chip,
		.mst = &mst,
	};

	*chip = *sfdp_generic_chip();
	return probe_spi_sfdp(&flash);
}

static void assert_eraser(const struct block_eraser *eraser, enum block_erase_func func,
			  uint32_t size, uint32_t count)
{
	assert_int_equal(eraser->block_erase, func);
	assert_int_equal(eraser->eraseblocks[0].size, size);
	assert_int_equal(eraser->eraseblocks[0].count, count);
}

/* Read from a MX25L6406E: SFDP 1.0, JEDEC table of 9 DWORDs plus Macronix table. */
static const uint8_t sfdp_mx25l6406e[] = {
	0x53, 0x46, 0x44, 0x50, 0x00, 0x01, 0x01, 0xff, /* @0x00: signature, rev 1.0, NPH = 1 */
	0x00, 0x00, 0x01, 0x09, 0x30, 0x00, 0x00, 0xff, /* @0x08: JEDEC table 1.0, 9 DW @0x30 */
	0xc2, 0x00, 0x01, 0x04, 0x60, 0x00, 0x00, 0xff, /* @0x10: Macronix table 1.0, 4 DW @0x60 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xe5, 0x20, 0x81, 0xff, 0xff, 0xff, 0xff, 0x03, /* @0x30: JEDEC table */
	0x00, 0xff, 0x00, 0xff, 0x08, 0x3b, 0x00, 0xff,
	0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff,
	0xff, 0xff, 0x00, 0xff, 0x0c, 0x20, 0x10, 0xd8,
	0x00, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x36, 0x00, 0x27, 0xf6, 0x4f, 0xff, 0xff, /* @0x60: Macronix table */
	0xfe, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

/* MX25L25635F datasheet rev 1.5: SFDP 1.0, 32 MiB, JEDEC table of 9 DWORDs. */
static const uint8_t sfdp_mx25l25635f[] = {
	0x53, 0x46, 0x44, 0x50, 0x00, 0x01, 0x01, 0xff, /* @0x00: signature, rev 1.0, NPH = 1 */
	0x00, 0x00, 0x01, 0x09, 0x30, 0x00, 0x00, 0xff, /* @0x08: JEDEC table 1.0, 9 DW @0x30 */
	0xc2, 0x00, 0x01, 0x04, 0x60, 0x00, 0x00, 0xff, /* @0x10: Macronix table 1.0, 4 DW @0x60 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xe5, 0x20, 0xf3, 0xff, 0xff, 0xff, 0xff, 0x0f, /* @0x30: JEDEC table */
	0x44, 0xeb, 0x08, 0x6b, 0x08, 0x3b, 0x04, 0xbb,
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff,
	0xff, 0xff, 0x44, 0xeb, 0x0c, 0x20, 0x0f, 0x52,
	0x10, 0xd8, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x36, 0x00, 0x27, 0x9d, 0xf9, 0xc0, 0x64, /* @0x60: Macronix table */
	0x85, 0xcb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

/*
 * GD25Q256D datasheet rev 1.5: SFDP 1.6, 32 MiB, JEDEC table of 16 DWORDs,
 * GigaDevice table and 4-byte address instruction table.
 */
static const uint8_t sfdp_gd25q256d[] = {
	0x53, 0x46, 0x44, 0x50, 0x06, 0x01, 0x02, 0xff, /* @0x00: signature, rev 1.6, NPH = 2 */
	0x00, 0x06, 0x01, 0x10, 0x30, 0x00, 0x00, 0xff, /* @0x08: JEDEC table 1.6, 16 DW @0x30 */
	0xc8, 0x00, 0x01, 0x03, 0x90, 0x00, 0x00, 0xff, /* @0x10: GigaDevice table 1.0, 3 DW @0x90 */
	0x84, 0x00, 0x01, 0x02, 0xc0, 0x00, 0x00, 0xff, /* @0x18: 4BA instruction table 1.0, 2 DW @0xc0 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xe5, 0x20, 0xf3, 0xff, 0xff, 0xff, 0xff, 0x0f, /* @0x30: JEDEC table */
	0x44, 0xeb, 0x08, 0x6b, 0x08, 0x3b, 0x42, 0xbb,
	0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff,
	0xff, 0xff, 0x00, 0xff, 0x0c, 0x20, 0x0f, 0x52,
	0x10, 0xd8, 0x00, 0xff, 0x42, 0x62, 0xc9, 0xfe,
	0x82, 0xe9, 0x14, 0x58, 0xec, 0x60, 0x06, 0x33,
	0x7a, 0x75, 0x7a, 0x75, 0x04, 0xbd, 0xd5, 0x5c,
	0x00, 0x06, 0x44, 0x00, 0x08, 0x50, 0x00, 0x01,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x36, 0x00, 0x27, 0x9e, 0xf9, 0x77, 0x64, /* @0x90: GigaDevice table */
	0xfc, 0xeb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x0e, 0xf0, 0xff, 0x21, 0x5c, 0xdc, 0xff, /* @0xc0: 4BA instruction table */
};

void sfdp_mx25l6406e_test_success(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	assert_int_equal(1, run_sfdp_probe(&chip, sfdp_mx25l6406e, sizeof(sfdp_mx25l6406e)));

	assert_int_equal(8 * KiB, chip.total_size);
	assert_int_equal(64, chip.page_size);
	assert_int_equal(SPI_CHIP_WRITE256, chip.write);
	assert_int_equal(SPI_CHIP_READ, chip.read);
	assert_int_equal(FEATURE_WRSR_EWSR, chip.feature_bits);
	assert_eraser(&chip.block_erasers[0], SPI_BLOCK_ERASE_20, 4 * KiB, 2048);
	assert_eraser(&chip.block_erasers[1], SPI_BLOCK_ERASE_D8, 64 * KiB, 128);
	assert_int_equal(NO_BLOCK_ERASE_FUNC, chip.block_erasers[2].block_erase);
}

/* SFDP 1.0 has no DWORD 16, so there is no way to learn how to reach above 16 MiB. */
void sfdp_mx25l25635f_no_4ba_info_rejected(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	assert_int_equal(0, run_sfdp_probe(&chip, sfdp_mx25l25635f, sizeof(sfdp_mx25l25635f)));
}

void sfdp_gd25q256d_test_success(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	assert_int_equal(1, run_sfdp_probe(&chip, sfdp_gd25q256d, sizeof(sfdp_gd25q256d)));

	assert_int_equal(32 * KiB, chip.total_size);
	assert_int_equal(256, chip.page_size);
	assert_int_equal(SPI_CHIP_WRITE256, chip.write);
	assert_int_equal(SPI_CHIP_READ, chip.read);
	assert_int_equal(FEATURE_WRSR_EWSR | FEATURE_4BA_ENTER | FEATURE_4BA_NATIVE, chip.feature_bits);
	assert_eraser(&chip.block_erasers[0], SPI_BLOCK_ERASE_20, 4 * KiB, 8192);
	assert_eraser(&chip.block_erasers[1], SPI_BLOCK_ERASE_52, 32 * KiB, 1024);
	assert_eraser(&chip.block_erasers[2], SPI_BLOCK_ERASE_D8, 64 * KiB, 512);
	assert_int_equal(NO_BLOCK_ERASE_FUNC, chip.block_erasers[3].block_erase);
}

/* Copies the GD25Q256D table with other 4-Byte address enter and exit methods in DWORD 16. */
static void gd25q256d_with_4ba_methods(uint8_t *data, uint8_t enter, uint16_t exit)
{
	const size_t dw16 = 0x30 + 15 * 4;

	memcpy(data, sfdp_gd25q256d, sizeof(sfdp_gd25q256d));
	uint32_t val = data[dw16] | (data[dw16 + 1] << 8) | (data[dw16 + 2] << 16) |
		       ((uint32_t)data[dw16 + 3] << 24);
	val = (val & 0x3fff) | ((uint32_t)(exit & 0x3ff) << 14) | ((uint32_t)enter << 24);
	for (unsigned int i = 0; i < 4; i++)
		data[dw16 + i] = (val >> (8 * i)) & 0xff;
}

static int run_gd25q256d_4ba_methods(struct flashchip *chip, uint8_t enter, uint16_t exit)
{
	uint8_t data[sizeof(sfdp_gd25q256d)];

	gd25q256d_with_4ba_methods(data, enter, exit);
	return run_sfdp_probe(chip, data, sizeof(data));
}

static void assert_emu_read(struct flashctx *flash, uint32_t start, uint32_t len)
{
	uint8_t buf[64];

	assert_true(len <= sizeof(buf));
	assert_int_equal(0, spi_chip_read(flash, buf, start, len));
	for (uint32_t i = 0; i < len; i++)
		assert_int_equal(emu_byte(start + i), buf[i]);
}

/*
 * Detects an emulated 32 MiB chip through its SFDP table, sets up 4-Byte addressing the way
 * prepare_flash_access() does and reads across the 16 MiB boundary and the end of the chip.
 */
static void run_32mib_read(const uint8_t *data, size_t len, enum emu_4ba_method method,
			   uint32_t features)
{
	struct flashchip chip;
	struct emu_chip emu = {
		.data = data,
		.len = len,
		.method = method,
		.size = 32 * MiB,
	};
	struct registered_master mst = {
		.buses_supported = BUS_SPI,
		.spi.features = features,
		.spi.max_data_read = 16,
		.spi.command = emu_spi_command,
		.spi.read = default_spi_read,
		.spi.data = &emu,
	};
	struct flashctx flash = {
		.chip = &chip,
		.mst = &mst,
		.address_high_byte = -1,
	};

	chip = *sfdp_generic_chip();
	assert_int_equal(1, probe_spi_sfdp(&flash));
	assert_int_equal(32 * KiB, chip.total_size);

	if (spi_chip_4ba(&flash) && !spi_master_no_4ba_modes(&flash))
		assert_int_equal(0, spi_enter_4ba(&flash));

	assert_emu_read(&flash, 0, 32);
	assert_emu_read(&flash, 16 * MiB - 24, 48);
	assert_emu_read(&flash, 32 * MiB - 32, 32);
	assert_emu_read(&flash, 0x123456, 16);

	if (spi_chip_4ba(&flash) && !spi_master_no_4ba_modes(&flash)) {
		assert_int_equal(0, spi_exit_4ba(&flash));
		assert_false(is_4ba_mode(&emu));
	}
}

static void run_4ba_read(uint8_t methods, enum emu_4ba_method method)
{
	uint8_t data[sizeof(sfdp_gd25q256d)];

	gd25q256d_with_4ba_methods(data, methods, methods);
	/* Drop the 4-Byte address instruction table, so reads go through the address mode */
	data[6] = 0x01;
	run_32mib_read(data, sizeof(data), method, SPI_MASTER_4BA);
}

void sfdp_4ba_read_b7_test_success(void **state)
{
	(void) state; /* unused */

	run_4ba_read(BIT(0), EMU_4BA_B7);
}

void sfdp_4ba_read_b7_wren_test_success(void **state)
{
	(void) state; /* unused */

	run_4ba_read(BIT(1), EMU_4BA_B7_WREN);
}

void sfdp_4ba_read_bank_register_test_success(void **state)
{
	(void) state; /* unused */

	run_4ba_read(BIT(3), EMU_4BA_BANK);
}

void sfdp_4ba_read_ext_addr_register_test_success(void **state)
{
	(void) state; /* unused */

	run_4ba_read(BIT(2), EMU_4BA_EAR);
}

void sfdp_4ba_enter_wren_test_success(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	assert_int_equal(1, run_gd25q256d_4ba_methods(&chip, BIT(1), BIT(1)));
	assert_int_equal(FEATURE_WRSR_EWSR | FEATURE_4BA_ENTER_WREN | FEATURE_4BA_NATIVE, chip.feature_bits);
}

void sfdp_4ba_bank_register_test_success(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	assert_int_equal(1, run_gd25q256d_4ba_methods(&chip, BIT(3), BIT(3)));
	assert_int_equal(FEATURE_WRSR_EWSR | FEATURE_4BA_ENTER_EAR7 | FEATURE_4BA_EAR_1716 |
			 FEATURE_4BA_NATIVE, chip.feature_bits);
}

void sfdp_4ba_ext_addr_register_test_success(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	assert_int_equal(1, run_gd25q256d_4ba_methods(&chip, BIT(2), BIT(2)));
	assert_int_equal(FEATURE_WRSR_EWSR | FEATURE_4BA_EAR_C5C8 | FEATURE_4BA_NATIVE, chip.feature_bits);
}

/* Entering with 0xb7 is useless when only a hardware reset leaves the mode again. */
void sfdp_4ba_enter_without_exit_rejected(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	assert_int_equal(0, run_gd25q256d_4ba_methods(&chip, BIT(0), BIT(5)));
}

/* Reads with 0x13 and 4-Byte addresses, the chip never leaves 3-Byte address mode. */
void sfdp_4ba_read_native_test_success(void **state)
{
	(void) state; /* unused */

	run_32mib_read(sfdp_gd25q256d, sizeof(sfdp_gd25q256d), EMU_4BA_NONE,
		       SPI_MASTER_4BA | SPI_MASTER_NO_4BA_MODES);
}

/* Probes the GD25Q256D table with one double word replaced. */
static int run_gd25q256d_with_dword(struct flashchip *chip, size_t offset, uint32_t val)
{
	uint8_t data[sizeof(sfdp_gd25q256d)];

	memcpy(data, sfdp_gd25q256d, sizeof(data));
	for (unsigned int i = 0; i < 4; i++)
		data[offset + i] = (val >> (8 * i)) & 0xff;
	return run_sfdp_probe(chip, data, sizeof(data));
}

void sfdp_4ba_instructions_test_success(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	/* Only 0x13 and 0x12 */
	assert_int_equal(1, run_gd25q256d_with_dword(&chip, 0xc0, BIT(0) | BIT(6)));
	assert_int_equal(FEATURE_4BA_READ | FEATURE_4BA_WRITE, chip.feature_bits & FEATURE_4BA_NATIVE);
	/* Only 0x0c */
	assert_int_equal(1, run_gd25q256d_with_dword(&chip, 0xc0, BIT(1)));
	assert_int_equal(FEATURE_4BA_FAST_READ, chip.feature_bits & FEATURE_4BA_NATIVE);
}

void sfdp_4ba_instructions_ignored(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	/* 16 MiB chip */
	assert_int_equal(1, run_gd25q256d_with_dword(&chip, 0x34, 16 * MiB * 8 - 1));
	assert_int_equal(16 * KiB, chip.total_size);
	assert_int_equal(0, chip.feature_bits & FEATURE_4BA_NATIVE);
	/* ID MSB 0x00 instead of 0xff */
	assert_int_equal(1, run_gd25q256d_with_dword(&chip, 0x1c, 0x000000c0));
	assert_int_equal(0, chip.feature_bits & FEATURE_4BA_NATIVE);
	/* Major version 2 */
	assert_int_equal(1, run_gd25q256d_with_dword(&chip, 0x18, 0x02020084));
	assert_int_equal(0, chip.feature_bits & FEATURE_4BA_NATIVE);
}

/* Probes the GD25Q256D table with another page size in DWORD 11. */
static int run_gd25q256d_page_size(struct flashchip *chip, uint8_t n)
{
	const size_t dw11 = 0x30 + 10 * 4;
	uint8_t data[sizeof(sfdp_gd25q256d)];

	memcpy(data, sfdp_gd25q256d, sizeof(data));
	data[dw11] = (data[dw11] & 0x0f) | (n << 4);
	return run_sfdp_probe(chip, data, sizeof(data));
}

void sfdp_page_size_test_success(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	assert_int_equal(1, run_gd25q256d_page_size(&chip, 9));
	assert_int_equal(512, chip.page_size);
	assert_int_equal(1, run_gd25q256d_page_size(&chip, 6));
	assert_int_equal(64, chip.page_size);
}

/* Out of range page sizes leave the 64 B write chunk size from DWORD 1. */
void sfdp_page_size_implausible_use_default(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;

	assert_int_equal(1, run_gd25q256d_page_size(&chip, 5));
	assert_int_equal(64, chip.page_size);
	assert_int_equal(1, run_gd25q256d_page_size(&chip, 15));
	assert_int_equal(64, chip.page_size);
}

void sfdp_no_signature_rejected(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;
	const uint8_t blank[0x40] = { 0 };

	assert_int_equal(0, run_sfdp_probe(&chip, blank, sizeof(blank)));
}

void sfdp_unknown_major_revision_rejected(void **state)
{
	(void) state; /* unused */
	struct flashchip chip;
	uint8_t data[sizeof(sfdp_mx25l6406e)];

	memcpy(data, sfdp_mx25l6406e, sizeof(data));
	data[5] = 0x02;
	assert_int_equal(0, run_sfdp_probe(&chip, data, sizeof(data)));
}
