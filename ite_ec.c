/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2021, TUXEDO Computers GmbH
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

/*
 * Contains programmer implementation for ITE EC used for example on TUXEDO
 * laptops.
 */

#if defined(__i386__) || defined(__x86_64__)

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "chipdrivers.h"
#include "acpi_ec.h"
#include "flashchips.h"
#include "hwaccess.h"
#include "programmer.h"
#include "ite_ec.h"

#define EC_CMD_ERASE_ALL    0x01
#define EC_CMD_WRITE_BLOCK  0x02
#define EC_CMD_READ_BLOCK   0x03
#define EC_CMD_GET_FLASH_ID 0x04
#define EC_CMD_ERASE_KBYTE  0x05
#define EC_CMD_WRITE_KBYTE  0x06
#define EC_CMD_READ_PRJ     0x92
#define EC_CMD_READ_VER     0x93
#define EC_CMD_WDG_RESET    0xfe

#define BYTES_PER_BLOCK     (64 * 1024)
#define BYTES_PER_CHUNK     256
#define KBYTES_PER_BLOCK    64
#define CHUNKS_PER_KBYTE    4
#define CHUNKS_PER_BLOCK    256

#define INFO_BUFFER_SIZE    16

/* Autoload parameter controlls the flash mirroring */
enum autoloadaction {
	AUTOLOAD_NO_ACTION,
	AUTOLOAD_DISABLE,
	AUTOLOAD_SETON,
	AUTOLOAD_SETOFF
};

struct ite_ec_data {
	unsigned int rom_size_in_blocks;

	unsigned int ite_string_offset;
	enum autoloadaction autoload_action;

	uint8_t first_kbyte[CHUNKS_PER_KBYTE * BYTES_PER_CHUNK];
	bool support_ite5570;
	uint8_t write_mode;

	bool ac_adapter_plugged;
};

/*
 * This is a 16byte signature which should occupy the image offset 0x40
 * of the EC code, also known as ITEString.
 */
struct ite_string {
	uint8_t _a5_bytes[6]; // constant, 6 bytes of 0xa5
	uint8_t _a4_a5_byte; // varies, either 0xa5 or 0xa4
	uint8_t ec_signature_flag;
	uint8_t signature[2]; // constant? 0x85 0x12
	uint8_t _5a_bytes[2];  // constant 0x5a 0x5a
	uint8_t _aa_byte; // constant 0xaa
	uint8_t ec_mirror_size; // EC flash size to mirror in KiB
	uint8_t _55_bytes; // constant 0x55 0x55
} __attribute__((packed));

static int force = 0;

static int ite_ec_shutdown(void *data)
{
	int ret = 0;

	if (!ec_write_cmd(EC_CMD_WDG_RESET, EC_MAX_STATUS_CHECKS)) {
		msg_perr("Failed to shutdown ite_ec\n");
		ret = 1;
	}

	free(data);

	return ret;
}

static void ite_ec_read_project(struct ite_ec_data *ctx_data)
{
	uint8_t ec_project[INFO_BUFFER_SIZE];
	uint8_t i;

	if (!ec_write_cmd(EC_CMD_READ_PRJ, EC_MAX_STATUS_CHECKS)) {
		msg_perr("Failed to write cmd...\n");
		return;
	}

	for (i = 0; i < INFO_BUFFER_SIZE - 1; i++) {
		if (!ec_read_byte(&ec_project[i], EC_MAX_STATUS_CHECKS)) {
			msg_perr("Failed to read EC project\n");
			return;
		}
		if (ec_project[i] == '$') {
			ec_project[i] = '\0';
			break;
		}
	}

	msg_pinfo("Mainboard EC Project: %s\n", (char *)ec_project);
}

static void ite_ec_read_version(struct ite_ec_data *ctx_data)
{
	uint8_t ec_version[INFO_BUFFER_SIZE];
	uint8_t i;

	if (!ec_write_cmd(EC_CMD_READ_VER, EC_MAX_STATUS_CHECKS)) {
		msg_perr("Failed to write cmd...\n");
		return;
	}

	for (i = 0; i < INFO_BUFFER_SIZE - 1; i++) {
		if (!ec_read_byte(&ec_version[i], EC_MAX_STATUS_CHECKS)) {
			msg_perr("Failed to read EC version\n");
			return;
		}
		if (ec_version[i] == '$') {
			ec_version[i] = '\0';
			break;
		}
	}

	msg_pinfo("Mainboard EC Version: 1.%s\n", (char *)ec_version);
}

static bool ite_ec_init_ctx(struct ite_ec_data *ctx_data)
{
	uint8_t reg_value;

	if (!ec_read_reg(0xf9, &reg_value, EC_MAX_STATUS_CHECKS)) {
		msg_perr("Failed to query flash ROM size.\n");
		return false;
	}

	msg_pdbg("%s: ROM size register value %02x\n", __func__, reg_value);

	if (reg_value == 0xff) {
		msg_pwarn("Querying EC ROM size returned unexpected result.\n"
			  "Probably the EC has just been flashed and the EC RAM has been reset.\n"
			  "You may need to pass the flash size via the programmer parameters or simply try again in a while.\n");
		return false;
	}

	switch (reg_value & 0xf0) {
	case 0x40:
		ctx_data->rom_size_in_blocks = 3;
		break;
	case 0x80:
		ctx_data->rom_size_in_blocks = 4;
		break;
	default:
		ctx_data->rom_size_in_blocks = 2;
		break;
	}

	/* flush the EC registers */
	INB(0x66);
	INB(0x62);

	msg_pdbg("%s: Querying AC adapter state...\n", __func__);
	if (!ec_read_reg(0x10, &reg_value, EC_MAX_STATUS_CHECKS)) {
		msg_perr("Failed to query AC adapter state.\n");
		return false;
	}

	msg_pdbg("%s: AC adapter state %02x\n", __func__, reg_value);

	ctx_data->ac_adapter_plugged = reg_value & 0x01;

	ite_ec_read_project(ctx_data);
	ite_ec_read_version(ctx_data);

	return true;
}

static int ite_ec_read(struct flashctx *flash, uint8_t *buf,
		      unsigned int start, unsigned int len)
{
	unsigned int offset, block, block_start, block_end;
	uint8_t rom_byte;
	struct ite_ec_data *ctx_data = (struct ite_ec_data *)flash->mst->opaque.data;

	/* This EC can read only a whole block. */
	if (!len || len % BYTES_PER_BLOCK != 0) {
		msg_perr("Incorrect read length %x\n", len);
		return 1;
	}

	if (start % BYTES_PER_BLOCK != 0) {
		msg_perr("Incorrect read region start: %x\n", len);
		return 1;
	}

	block_start = start / BYTES_PER_BLOCK;
	block_end = (start + len) / BYTES_PER_BLOCK;

	if (block_end > ctx_data->rom_size_in_blocks) {
		msg_perr("Requested to read block outside of chip boundaries\n");
		return 1;
	}

	for (block = block_start; block < block_end; block++) {
		if (!ec_write_cmd(EC_CMD_READ_BLOCK, EC_MAX_STATUS_CHECKS) ||
		    !ec_write_cmd(block, EC_MAX_STATUS_CHECKS)) {
			msg_perr("Failed to select block to read %d\n", block);
			return 1;
		}
		for (offset = 0; offset < BYTES_PER_BLOCK; ++offset) {

			if (!ec_read_byte(&rom_byte, EC_MAX_STATUS_CHECKS)) {
				msg_perr("Flash read failed @ 0x%x\n", offset);
				return 1;
			}

			*buf = rom_byte;
			++buf;
		}
	}

	return 0;
}

static void ite_ec_patch_autoload(struct ite_ec_data *ctx_data, uint8_t *data)
{
	struct ite_string *itestring;
	const bool blocks_1_2 = ctx_data->rom_size_in_blocks == 1 ||
				ctx_data->rom_size_in_blocks == 2;

	if (ctx_data->ite_string_offset == 0)
		return;

	itestring = (struct ite_string *)&data[ctx_data->ite_string_offset];

	switch (ctx_data->autoload_action) {
	case AUTOLOAD_NO_ACTION:
		break;
	case AUTOLOAD_DISABLE:
		itestring->ec_signature_flag = (blocks_1_2 ? 0x94 : 0x85);
		itestring->ec_mirror_size = 0x00;
		break;
	case AUTOLOAD_SETON:
		itestring->ec_signature_flag = (blocks_1_2 ? 0x94 : 0x85);
		itestring->ec_mirror_size = (blocks_1_2 ? 0x7f : 0xbe);
		break;
	case AUTOLOAD_SETOFF:
		itestring->ec_signature_flag =  (blocks_1_2 ? 0xa5 : 0xb5);
		itestring->ec_mirror_size = 0xaa;
		break;
	}
}

static bool ite_ec_write_block(struct ite_ec_data *ctx_data, const uint8_t *buf,
			      unsigned int block)
{
	const uint8_t param = ctx_data->support_ite5570 ? 0x00 : 0x02;
	uint8_t third_param = param;
	unsigned int i;
	bool skip_first_kbyte = false;

	/* Required: stash first kilobyte and write it after the last block. */
	if (ctx_data->write_mode != 0 && block == 0) {
		memcpy(ctx_data->first_kbyte, buf, sizeof(ctx_data->first_kbyte));
		third_param = 0x04;
		skip_first_kbyte = true;
	}

	if (!ec_write_cmd(EC_CMD_WRITE_BLOCK, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_cmd(param, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_cmd(block, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_cmd(third_param, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_cmd(param, EC_MAX_STATUS_CHECKS)) {
		msg_perr("Unable to send block write command.\n");
		return 1;
	}

	for (i = 0; i < BYTES_PER_BLOCK; i++) {
		if (skip_first_kbyte && (i < CHUNKS_PER_KBYTE * BYTES_PER_CHUNK))
			continue;
		if (!ec_write_byte(buf[i], EC_MAX_STATUS_CHECKS)){
			msg_perr("Unable to write byte @ 0x%x\n", block * BYTES_PER_BLOCK + i);
			return false;
		}
	}

	/* If we're done, write the first kilobyte separately. */
	if (ctx_data->write_mode != 0 && block == ctx_data->rom_size_in_blocks - 1) {
		if (!ec_write_cmd(0x06, EC_MAX_STATUS_CHECKS)) {
			msg_perr("Unable to send kbyte write command.\n");
			return 1;
		}
		for (i = 0; i < CHUNKS_PER_KBYTE * BYTES_PER_CHUNK; i++) {
			if (!ec_write_byte(ctx_data->first_kbyte[i], EC_MAX_STATUS_CHECKS)) {
				msg_perr("Unable to write byte @ 0x%04x\n", i);
				return false;
			}
		}

		memset(ctx_data->first_kbyte, 0, sizeof(ctx_data->first_kbyte));
	}

	return true;
}

static bool offset_is_ite_string(const uint8_t *buf)
{
	struct ite_string *itestring = (struct ite_string *)buf;
	uint8_t _a5_array[6] = { 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5 };
	uint8_t	_5a_array[2] = { 0x5a, 0x5a };

	if (!memcmp(itestring->_a5_bytes, _a5_array, 6) &&
	    !memcmp(itestring->_5a_bytes, _5a_array, 2) &&
	    (itestring->_a4_a5_byte == 0xa4 || itestring->_a4_a5_byte == 0xa5))
		return true;

	return false;
}

static bool ite_ec_get_ite_string_offset(struct ite_ec_data *ctx_data,
					  const uint8_t *buf, unsigned int len)
{
	unsigned int i;

	if (ctx_data->autoload_action == AUTOLOAD_NO_ACTION)
		return true;

	/* Check standard offset 0x40 first */
	if (offset_is_ite_string(&buf[0x40])) {
		ctx_data->ite_string_offset = 0x40;
		return true;
	}

	for (i = 0; i < len / 16; i += sizeof(struct ite_string)) {
		if (offset_is_ite_string(&buf[i])) {
			ctx_data->ite_string_offset = i;
			return true;
		}
	}

	if (ctx_data->ite_string_offset == 0)
		return false;

	return true;
}

static int ite_ec_write(struct flashctx *flash, const uint8_t *buf,
				unsigned int start, unsigned int len)
{
	struct ite_ec_data *ctx_data = (struct ite_ec_data *)flash->mst->opaque.data;
	unsigned int block_start;
	unsigned int block_end;
	unsigned int block;

	/* This EC can write only a whole block. */
	 if (len == 0 || len % BYTES_PER_BLOCK != 0) {
		msg_perr("Incorrect write length %x\n", len);
		return 1;
	}

	if (start % BYTES_PER_BLOCK != 0) {
		msg_perr("Incorrect write region start: %x\n", len);
		return 1;
	}

	block_start = start / BYTES_PER_BLOCK;
	block_end = (start + len) / BYTES_PER_BLOCK;

	if (block_end > ctx_data->rom_size_in_blocks) {
		msg_perr("Requested to write block outside of chip boundaries\n");
		return 1;
	}

	if (ctx_data->autoload_action != AUTOLOAD_NO_ACTION) {
		if (!ite_ec_get_ite_string_offset(ctx_data, buf, len))
			msg_pwarn("Warning: Failed to find autoload section\n"
				  "Autoload parameter will not be updated in flash.\n");

		if (ctx_data->ite_string_offset >= start &&
		    ctx_data->ite_string_offset <= (start + len - sizeof(struct ite_string)))
			ite_ec_patch_autoload(ctx_data, (uint8_t *)buf);
	}

	for (block = block_start; block < block_end; block++) {
		if (!ite_ec_write_block(ctx_data,
				       buf + (BYTES_PER_BLOCK * block),
				       block)) {
			msg_perr("Unable to write full block.\n");
			return 1;
		}
	}

	return 0;
}

static int ite_ec_full_erase(struct ite_ec_data *ctx_data)
{
	unsigned int i;
	uint8_t data;

	if (!ec_write_cmd(EC_CMD_ERASE_ALL, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_cmd(0x00, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_cmd(0x00, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_cmd(0x00, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_cmd(0x00, EC_MAX_STATUS_CHECKS))
		return 1;

	if (ctx_data->rom_size_in_blocks < 3) {
		internal_sleep(15000 * 64);
		return 0;
	}

	for (i = 0; i < 4; i++) {
		if (!ec_read_byte(&data, EC_MAX_STATUS_CHECKS * 3))
			return 1;

		if (data == 0xf8)
			return 0;
	}

	internal_sleep(100000);

	return 1;
}

static int ite_ec_chunkwise_erase(struct ite_ec_data *ctx_data,
				 unsigned int start, unsigned int len)
{
	const unsigned int from_chunk = start / BYTES_PER_CHUNK;
	const unsigned int end = start + len;
	unsigned int to_chunk = end / BYTES_PER_CHUNK;
	unsigned int i;

	if (to_chunk / CHUNKS_PER_BLOCK > ctx_data->rom_size_in_blocks) {
		msg_perr("Requested to erase block outside of chip boundaries\n");
		return 1;
	}

	for (i = from_chunk; i < to_chunk; i += CHUNKS_PER_KBYTE) {
		if (!ec_write_cmd(EC_CMD_ERASE_KBYTE, EC_MAX_STATUS_CHECKS) ||
		    !ec_write_cmd(i / CHUNKS_PER_BLOCK, EC_MAX_STATUS_CHECKS) ||
		    !ec_write_cmd(i % CHUNKS_PER_BLOCK, EC_MAX_STATUS_CHECKS) ||
		    !ec_write_cmd(0x00, EC_MAX_STATUS_CHECKS)) {
			msg_perr("Failed to erase chunk %d\n", i);
			return 1;
		}

		internal_sleep(1000);
	}

	internal_sleep(100000);
	return 0;
}

static int ite_ec_erase(struct flashctx *flash, unsigned int blockaddr,
		       unsigned int blocklen)
{
	struct ite_ec_data *ctx_data = (struct ite_ec_data *)flash->mst->opaque.data;

	/* This EC can erase only a whole block */
	if (!blocklen || blocklen % BYTES_PER_BLOCK != 0) {
		msg_perr("Incorrect erase length %x\n", blocklen);
		return 1;
	}

	if (ctx_data->support_ite5570)
		return ite_ec_chunkwise_erase(ctx_data, blockaddr, blocklen);

	return ite_ec_full_erase(ctx_data);
}


static int ite_ec_probe(struct flashctx *flash)
{
	struct ite_ec_data *ctx_data = (struct ite_ec_data *)flash->mst->opaque.data;

	flash->chip->tested = TEST_OK_PREW;
	flash->chip->page_size = BYTES_PER_BLOCK;
	flash->chip->total_size = ctx_data->rom_size_in_blocks * KBYTES_PER_BLOCK;
	/* This EC supports only write granularity of 64KiB */
	flash->chip->gran = write_gran_64kbytes;
	/*
	 * Erase operation must be done in one sway.
	 * So report an eraser for the whole chip size.
	 */
	flash->chip->block_erasers[0].eraseblocks[0].size =
		ctx_data->rom_size_in_blocks * BYTES_PER_BLOCK;
	flash->chip->block_erasers[0].eraseblocks[0].count = 1;

	force = flash->flags.force;

	return 1;
}

static struct opaque_master programmer_opaque_ite_ec = {
	.max_data_read  = BYTES_PER_BLOCK,
	.max_data_write = BYTES_PER_BLOCK,
	.probe		= ite_ec_probe,
	.read		= ite_ec_read,
	.write		= ite_ec_write,
	.erase		= ite_ec_erase,
};

static bool ite_ec_board_mismatch_enabled(void)
{
	char *p = extract_programmer_param("boardmismatch");
	if (p && strcmp(p, "force") == 0) {
		free(p);
		return true;
	}
	free(p);
	return false;
}

static bool ite_ec_check_params(struct ite_ec_data *ctx_data)
{
	char *p;
	bool ret = true;

	msg_pdbg("%s()\n", __func__);

	p = extract_programmer_param("noaccheck");
	if (p && strcmp(p, "yes") == 0) {
		/* Just mark it as present. */
		ctx_data->ac_adapter_plugged = true;
	}
	free(p);

	p = extract_programmer_param("ite5570");
	if (p && strcmp(p, "yes") == 0) {
		ctx_data->support_ite5570 = true;
	}
	free(p);

	p = extract_programmer_param("autoload");
	if (p) {
		if (!strcmp(p, "none")) {
			ctx_data->autoload_action = AUTOLOAD_NO_ACTION;
		} else if (!strcmp(p, "disable")) {
			ctx_data->autoload_action = AUTOLOAD_DISABLE;
		} else if (!strcmp(p, "on")) {
			ctx_data->autoload_action = AUTOLOAD_SETON;
		} else if (!strcmp(p, "off")) {
			ctx_data->autoload_action = AUTOLOAD_SETOFF;
		} else {
			msg_pdbg("%s(): incorrect autoload param value: %s\n",
				__func__, p);
			ret = false;
		}
	}
	free(p);

	p = extract_programmer_param("romsize");
	if (p) {
		if (!strcmp(p, "64K")) {
			ctx_data->rom_size_in_blocks = 1;
		} else if (!strcmp(p, "128K")) {
			ctx_data->rom_size_in_blocks = 2;
		} else if (!strcmp(p, "192K")) {
			ctx_data->rom_size_in_blocks = 3;
		} else if (!strcmp(p, "256K")) {
			ctx_data->rom_size_in_blocks = 4;
		} else {
			msg_pdbg("%s(): incorrect romsize param value: %s\n",
				__func__, p);
			ret = false;
		}
	}
	free(p);

	return ret;
}

static void get_flash_part_from_id(struct ite_ec_data *ctx_data, uint32_t manuf_id,
				    uint32_t model_id)
{
	const struct flashchip *chip;
	const char *vendor = NULL;
	const char *device = NULL;
	for (chip = flashchips; chip && chip->name; chip++) {
		if (chip->manufacture_id == manuf_id) {
			vendor = chip->vendor;
			if (chip->model_id == model_id) {
				device = chip->name;
				break;
			}
		}
	}

	if (vendor && device)
		msg_pinfo("Found %s flash chip \"%s\" (%d kB).\n",
			  chip->vendor, chip->name, chip->total_size);
	else if (vendor)
		msg_pinfo("Found unknown %s flash chip\n", vendor);
}

static void ite_ec_read_flash_id(struct ite_ec_data *ctx_data)
{

	uint8_t rom_data[4] = { 0 };
	unsigned int id_length, i;
	uint32_t model_id;

	ec_write_cmd(EC_CMD_GET_FLASH_ID, EC_MAX_STATUS_CHECKS);

	id_length = 3;
	if (ctx_data->rom_size_in_blocks == 3 ||
	    ctx_data->rom_size_in_blocks == 4)
		id_length = 4;

	for (i = 0; i < id_length; i++)
		ec_read_byte(&rom_data[i], EC_MAX_STATUS_CHECKS);

	msg_pinfo("Flash Part ID: ");
	for (i = 0; i < id_length; i++)
		msg_pinfo("%02x ", rom_data[i]);

	msg_pinfo("\n");

	model_id = rom_data[1] | ((uint32_t)rom_data[2] << 8);
	model_id |= ((uint32_t)rom_data[3] << 16);

	get_flash_part_from_id(ctx_data, rom_data[0], model_id);
}

struct pci_match_vendor_entry {
	const char *vendor;
	const char *model;
	uint16_t pci_vid;
	uint16_t pci_devid;
	uint16_t ss_venid;
	uint16_t ss_devid;
};

static const struct pci_match_vendor_entry ite_ec_supported_boards[] = {
	{"CLEVO", "L140MU/L141MU", 0x8086, 0x9a14, 0x1558, 0x14a1 },
	{"CLEVO", "NS50MU/NS51MU", 0x8086, 0x9a14, 0x1558, 0x51a1 },
};

static bool is_board_supported(void)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(ite_ec_supported_boards); i++) {
		const struct pci_match_vendor_entry *entry = &ite_ec_supported_boards[i];

		if (pci_card_find(entry->pci_vid, entry->pci_devid,
				  entry->ss_venid, entry->ss_devid)) {
			msg_perr("Found PCI subsystem match for device %s %s\n",
				 entry->vendor, entry->model);
			return true;
		}
	}

	msg_perr("ITE EC programmer not (yet) supported on this device\n");
	return false;
}

static void get_sio_config(struct ite_ec_data *ctx_data, struct superio sio)
{
	switch (sio.model) {
	case 0x5570:
		ctx_data->support_ite5570 = true;
		break;
	default:
		break;
	}
}

static void probe_ite_superio_support(struct ite_ec_data *ctx_data)
{
	int i;

	probe_superio_ite();

	if (superio_count == 0)
		return;

	for (i = 0; i < superio_count; i++) {
		if (superios[i].vendor == SUPERIO_VENDOR_ITE)
			get_sio_config(ctx_data, superios[i]);
	}
}

static int ite_ec_init(void)
{
	bool read_success = false;
	struct ite_ec_data *ctx_data;

	if (rget_io_perms())
		return 1;

	if (pci_init_common() != 0) {
		msg_perr("Failed to initialize PCI\n");
		return 1;
	}

	if (!is_board_supported()) {
		if (ite_ec_board_mismatch_enabled()) {
			msg_pinfo("Proceeding anyway because user forced us to.\n");
		} else {
			msg_pwarn("Probing on unsupported laptop may irritate your EC and cause fan failure, "
			 "backlight failure and sudden poweroff.\n"
			 "You can force probing with \"-p ite_ec:boardmismatch=force\".\n"
			 "Aborting...\n");
			return 1;
		}
	}

	if (!ec_write_reg(0xf9, 0x20, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_reg(0xfa, 0x02, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_reg(0xfb, 0x00, EC_MAX_STATUS_CHECKS) ||
	    !ec_write_reg(0xf8, 0xb1, EC_MAX_STATUS_CHECKS)) {
		msg_perr("Unable to initialize controller.\n");
		return 1;
	}

	ctx_data = calloc(1, sizeof(*ctx_data));
	if (!ctx_data) {
		msg_perr("Unable to allocate space for context data.\n");
		return 1;
	}

	if (!ite_ec_init_ctx(ctx_data))
		goto ite_ec_init_exit;

	if (!ite_ec_check_params(ctx_data))
		goto ite_ec_init_exit;

	if (!ec_write_cmd(0xde, EC_MAX_STATUS_CHECKS) ||
		!ec_write_cmd(0xdc, EC_MAX_STATUS_CHECKS)) {
		msg_perr("%s(): failed to prepare controller\n", __func__);
		goto ite_ec_init_exit;
	}

	if (!ec_write_cmd(0xf0, EC_MAX_STATUS_CHECKS)) {
		msg_perr("Failed to write identification commands.\n");
		goto ite_ec_init_exit;
	}

	probe_ite_superio_support(ctx_data);

	read_success = ec_read_byte(&ctx_data->write_mode, EC_MAX_STATUS_CHECKS);
	msg_pdbg("%s(): write mode %02x\n", __func__, ctx_data->write_mode);
	if (read_success && ctx_data->write_mode != 0x00 &&
	    ctx_data->write_mode != 0xff) {
		if (!ctx_data->support_ite5570) {
			msg_pdbg("%s(): selecting ITE5570 support\n", __func__);
			ctx_data->support_ite5570 = true;
		}
	} else {
		ctx_data->write_mode = 0;
	}

	ite_ec_read_flash_id(ctx_data);

	if (!ctx_data->ac_adapter_plugged) {
		msg_perr("AC adapter is not plugged.\n");
		goto ite_ec_init_exit;
	}

	programmer_opaque_ite_ec.data = ctx_data;

	if (register_shutdown(ite_ec_shutdown, ctx_data))
		goto ite_ec_init_exit;

	return register_opaque_master(&programmer_opaque_ite_ec, ctx_data);

ite_ec_init_exit:
	ite_ec_shutdown(ctx_data);
	return 1;
}

static void copy_version_string(char *buf, uint8_t *const contents)
{
	size_t i;

	for (i = 0; i < INFO_BUFFER_SIZE - 1; i++) {
		buf[i] = contents[i];
		if (buf[i] == '$') {
			buf[i] = '\0';
			break;
		}
	}
}

/**
 * @brief Checks the current and file's EC project and version.
 *
 * Retrieves the EC project and version from the file and current flash
 * contents. Compares both values and returns the action to take by flashrom.
 * If any of the EC project or EC version is not found, return one (failure).
 * If the EC project from the flash dump and the firmware file does not match
 * return one (failure). Otherwise return zero to tell flashrom to proceed with
 * write.
 *
 * @param newcontents The contents of the file to be flashed.
 * @param curcontents Contents dumped from the EC flash.
 * @return 0 on success, 1 on failure
 */
int ite_ec_verify_file_project(uint8_t *const newcontents,
				uint8_t *const curcontents,
				const size_t flash_size)
{
	char current_ec_project[INFO_BUFFER_SIZE];
	char new_ec_project[INFO_BUFFER_SIZE];
	char current_ec_version[INFO_BUFFER_SIZE];
	char new_ec_version[INFO_BUFFER_SIZE];
	size_t new_prj_offset = 0;
	size_t cur_prj_offset = 0;
	size_t new_ver_offset = 0;
	size_t cur_ver_offset = 0;
	size_t i;

	for (i = 0; i < flash_size - 4; i++) {
		if (!strncmp((char *)&newcontents[i], "PRJ:", 4))
			new_prj_offset = i + 4;
		if (!strncmp((char *)&curcontents[i], "PRJ:", 4))
			cur_prj_offset = i + 4;
		if (!strncmp((char *)&newcontents[i], "VER:", 4))
			new_ver_offset = i + 4;
		if (!strncmp((char *)&curcontents[i], "VER:", 4))
			cur_ver_offset = i + 4;
	}

	if (new_prj_offset == 0)  {
		msg_perr("EC project not found in the file");
		goto ite_ec_verify_file_exit;
	}

	if (cur_prj_offset == 0)  {
		msg_perr("EC project not found in the flash content");
		goto ite_ec_verify_file_exit;
	}

	if (new_ver_offset == 0)  {
		msg_perr("EC version not found in the file");
		goto ite_ec_verify_file_exit;
	}

	if (cur_ver_offset == 0)  {
		msg_perr("EC version not found in the flash content");
		goto ite_ec_verify_file_exit;
	}

	copy_version_string(new_ec_project, &newcontents[new_prj_offset]);
	copy_version_string(new_ec_version, &newcontents[new_ver_offset]);
	copy_version_string(current_ec_project, &curcontents[cur_prj_offset]);
	copy_version_string(current_ec_version, &curcontents[cur_ver_offset]);

	msg_pdbg("Current EC project: %s, EC version: %s\n", current_ec_project,
		 current_ec_version);
	msg_pdbg("New EC project: %s, EC version: %s\n", new_ec_project,
		 new_ec_version);

	if (strcmp(current_ec_project, new_ec_project)) {
		msg_perr("Wrong EC project. This file can't be used on this machine\n");
		goto ite_ec_verify_file_exit;
	}

	return 0;

ite_ec_verify_file_exit:
	if (force) {
		msg_pwarn("Proceeding anyway because user forced us to.\n");
		return 0;
	}

	return 1;
}

const struct programmer_entry programmer_ite_ec = {
	.name			= "ite_ec",
	.type			= OTHER,
	.devs.note		= "Programmer for ITE Embedded Controllers\n",
	.init			= ite_ec_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif
