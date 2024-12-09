/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2024 Matti Finder
 * (written by Matti Finder <matti.finder@gmail.com>)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "rpmc.h"
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>
#include "spi.h"

// OP1 commands
#define RPMC_WRITE_ROOT_KEY_MSG_LENGTH (RPMC_OP1_MSG_HEADER_LENGTH + RPMC_HMAC_KEY_LENGTH + RPMC_TRUNCATED_SIG_LENGTH)
#define RPMC_UPDATE_HMAC_KEY_MSG_LENGTH (RPMC_OP1_MSG_HEADER_LENGTH + RPMC_KEY_DATA_LENGTH + RPMC_SIGNATURE_LENGTH)
#define RPMC_INCREMENT_MONOTONIC_COUNTER_MSG_LENGTH (RPMC_OP1_MSG_HEADER_LENGTH + RPMC_COUNTER_LENGTH + RPMC_SIGNATURE_LENGTH)
#define RPMC_GET_MONOTONIC_COUNTER_MSG_LENGTH (RPMC_OP1_MSG_HEADER_LENGTH + RPMC_TAG_LENGTH + RPMC_SIGNATURE_LENGTH)

// OP2 commands
#define RPMC_READ_DATA_MSG_LENGTH 2
#define RPMC_READ_DATA_ANSWER_LENGTH (1 + RPMC_TAG_LENGTH + RPMC_COUNTER_LENGTH + RPMC_SIGNATURE_LENGTH)

static enum rpmc_result rpmc_get_extended_status(struct flashrom_flashctx *flash, uint8_t *status)
{
	const unsigned char extended_status_msg[RPMC_READ_DATA_MSG_LENGTH] = {
		flash->chip->rpmc_ctx.op2_opcode,
		0 // dummy
	};

	if (spi_send_command(flash, RPMC_READ_DATA_MSG_LENGTH, 1, extended_status_msg, status)) {
		msg_gerr("Reading extended status failed\n");
		return RPMC_ERROR_SPI_TRANSMISSION;
	}

	return RPMC_SUCCESS;
}

static enum rpmc_result rpmc_get_extended_status_long(struct flashrom_flashctx *flash,
						      struct rpmc_status_register *status,
						      // optional to check values tag and signature against
						      const unsigned char *const tag,
						      const unsigned char *const key)
{
	const unsigned int tag_offset = 1;
	const unsigned int counter_data_offset = tag_offset + RPMC_TAG_LENGTH;
	const unsigned int signature_offset = counter_data_offset + RPMC_COUNTER_LENGTH;
	const unsigned char cmd[RPMC_READ_DATA_MSG_LENGTH] = {
		flash->chip->rpmc_ctx.op2_opcode,
		0 // dummy
	};
	unsigned char answer[RPMC_READ_DATA_ANSWER_LENGTH];

	if (spi_send_command(flash, RPMC_READ_DATA_MSG_LENGTH, RPMC_READ_DATA_ANSWER_LENGTH, cmd, answer)) {
		msg_gerr("reading extended status failed\n");
		return RPMC_ERROR_SPI_TRANSMISSION;
	}

	status->status = answer[0];

	memcpy(status->tag, answer + tag_offset, RPMC_TAG_LENGTH);

	status->counter_data = answer[counter_data_offset];
	status->counter_data = (status->counter_data << 8) | answer[counter_data_offset + 1];
	status->counter_data = (status->counter_data << 8) | answer[counter_data_offset + 2];
	status->counter_data = (status->counter_data << 8) | answer[counter_data_offset + 3];

	memcpy(status->signature, answer + signature_offset, RPMC_SIGNATURE_LENGTH);

	if (tag != NULL) {
		if (memcmp(tag, status->tag, RPMC_TAG_LENGTH) != 0) {
			msg_gwarn("Tag doesn't match counter might be false\n");
			return RPMC_ERROR_TAG_MISMATCH;
		}
	}

	if (key != NULL) {
		unsigned char signature_buffer[RPMC_SIGNATURE_LENGTH];
		if (HMAC(EVP_sha256(),
			 key, RPMC_HMAC_KEY_LENGTH,
			 answer + tag_offset, RPMC_TAG_LENGTH + RPMC_COUNTER_LENGTH,
			 signature_buffer, NULL) == NULL) {
			msg_gerr("Could not generate signature\n");
			return RPMC_ERROR_OPENSSL;
		}

		if (memcmp(signature_buffer, status->signature, RPMC_SIGNATURE_LENGTH) != 0) {
			msg_gwarn("Signature doesn't match, counter might be false\n");
			return RPMC_ERROR_SIGNATURE_MISMATCH;
		}
	}

	return RPMC_SUCCESS;
}

static enum rpmc_result rpmc_poll_until_finished(struct flashrom_flashctx *flash)
{
	unsigned char poll_response;

	do {
		const unsigned char status_poll_msg = 0x05;

		// since we aren't really a time critical application we just sleep for the longest time
		programmer_delay(flash, flash->chip->rpmc_ctx.polling_long_delay_write_counter_us);

		switch (flash->chip->rpmc_ctx.busy_polling_method) {
			case POLL_READ_STATUS:
				if (spi_send_command(flash, 1, 1, &status_poll_msg, &poll_response)) {
					msg_gerr("Polling Status-Register-1 failed\n");
					return RPMC_ERROR_SPI_TRANSMISSION;
				}
				break;
			case POLL_OP2_EXTENDED_STATUS:
			{
				enum rpmc_result res = rpmc_get_extended_status(flash, &poll_response);
				if (res != RPMC_SUCCESS)
					return res;
				break;
			}
			default:
				msg_gerr("Unknown busy polling method found, this should not happen. Exiting...\n");
				return RPMC_ERROR_INTERNAL;
		}
	} while ((poll_response & 1) != 0);

	return RPMC_SUCCESS;
}

static enum rpmc_result rpmc_calculate_hmac_key_register(const char *const keyfile,
							 const uint32_t key_data,
							 unsigned char *const hmac_key_register)
{
	unsigned char key[RPMC_HMAC_KEY_LENGTH];
	unsigned char key_data_buf[RPMC_KEY_DATA_LENGTH] = {
		(key_data >> 24) & 0xff,
		(key_data >> 16) & 0xff,
		(key_data >> 8) & 0xff,
		key_data & 0xff
	};

	if (keyfile == NULL || read_buf_from_file(key, RPMC_HMAC_KEY_LENGTH, keyfile) != 0)
		return RPMC_ERROR_KEY_READ;

	if (HMAC(EVP_sha256(),
		 key, RPMC_HMAC_KEY_LENGTH,
		 key_data_buf, RPMC_KEY_DATA_LENGTH,
		 hmac_key_register, NULL) == NULL) {
		msg_gerr("Could not calculate HMAC signature for hmac storage\n");
		return RPMC_ERROR_OPENSSL;
	}

	return RPMC_SUCCESS;
}

static enum rpmc_result rpmc_basic_checks(struct flashrom_flashctx *flash, const unsigned int counter_address)
{
	if ((flash->chip->feature_bits & FEATURE_FLASH_HARDENING) == 0) {
		msg_gerr("Flash hardening is not supported on this chip, aborting.\n");
		return RPMC_ERROR_HARDENING_UNSUPPORTED;
	}

	if (counter_address >= flash->chip->rpmc_ctx.num_counters) {
		msg_gerr("Counter address is not in range, should be between 0 and %d.\n",
			 flash->chip->rpmc_ctx.num_counters - 1);
		return RPMC_ERROR_COUNTER_OUT_OF_RANGE;
	}

	return RPMC_SUCCESS;
}

static enum rpmc_result rpmc_send_and_wait(struct flashrom_flashctx *flash,
					   const unsigned char *const msg,
					   const size_t length)
{
	if (spi_send_command(flash, length, 0, msg, NULL))
		return RPMC_ERROR_SPI_TRANSMISSION;

	// check operation status
	return rpmc_poll_until_finished(flash);
}

static enum rpmc_result rpmc_sign_send_wait_check(struct flashrom_flashctx *flash,
						  unsigned char *const msg,
						  const size_t msg_length,
						  const size_t signature_offset,
						  const char *const keyfile,
						  const uint32_t key_data,
						  uint8_t *return_status)
{
	unsigned char hmac_key_register[RPMC_HMAC_KEY_LENGTH];

	enum rpmc_result ret = rpmc_calculate_hmac_key_register(keyfile, key_data, hmac_key_register);
	if (ret != RPMC_SUCCESS)
		return ret;

	if (HMAC(EVP_sha256(),
		 hmac_key_register, RPMC_HMAC_KEY_LENGTH,
		 msg, signature_offset,
		 msg + signature_offset, NULL) == NULL) {
		msg_gerr("Could not generate HMAC signature\n");
		return RPMC_ERROR_OPENSSL;
	}

	ret = rpmc_send_and_wait(flash, msg, msg_length);
	if (ret != RPMC_SUCCESS)
		return ret;

	ret = rpmc_get_extended_status(flash, return_status);
	if (ret != RPMC_SUCCESS)
		return ret;

	return RPMC_SUCCESS;
}

enum rpmc_result rpmc_write_root_key(struct flashrom_flashctx *flash,
				     const char *const keyfile,
				     const unsigned int counter_address)
{
	const unsigned int key_offset = RPMC_OP1_MSG_HEADER_LENGTH;
	const unsigned int signature_offset = key_offset + RPMC_HMAC_KEY_LENGTH;
	const unsigned int signature_cutoff = RPMC_SIGNATURE_LENGTH - RPMC_TRUNCATED_SIG_LENGTH;

	unsigned char msg[RPMC_WRITE_ROOT_KEY_MSG_LENGTH] = {
		flash->chip->rpmc_ctx.op1_opcode, // Opcode
		0x00, // CmdType
		counter_address, // CounterAddr
		0 //Reserved
	};

	enum rpmc_result ret = rpmc_basic_checks(flash, counter_address);
	if (ret != RPMC_SUCCESS)
		return ret;

	if (keyfile == NULL || read_buf_from_file(msg + key_offset, RPMC_HMAC_KEY_LENGTH, keyfile) != 0)
		return RPMC_ERROR_KEY_READ;

	unsigned char signature_buffer[RPMC_SIGNATURE_LENGTH];
	if (HMAC(EVP_sha256(),
		 msg + key_offset, RPMC_HMAC_KEY_LENGTH,
		 msg, RPMC_OP1_MSG_HEADER_LENGTH,
		 signature_buffer, NULL) == NULL) {
		msg_gerr("Could not calculate HMAC signature for message\n");
		return RPMC_ERROR_OPENSSL;
	}

	// need to truncate the signature a bit
	memcpy(msg + signature_offset, signature_buffer + signature_cutoff, RPMC_TRUNCATED_SIG_LENGTH);

	ret = rpmc_send_and_wait(flash, msg, RPMC_WRITE_ROOT_KEY_MSG_LENGTH);
	if (ret != RPMC_SUCCESS)
		return ret;

	uint8_t status;
	ret = rpmc_get_extended_status(flash, &status);
	if (ret != RPMC_SUCCESS)
		return ret;

	if (status & (1 << 1)) {
		return RPMC_ERROR_ROOT_KEY_OVERWRITE;
	} else if (status != 0x80) {
		// Incorrect payload size received or we have an unexpected bit set
		// This should not happen, if we wrote the code correctly
		return RPMC_ERROR_INTERNAL;
	}

	return RPMC_SUCCESS;
}

enum rpmc_result rpmc_update_hmac_key(struct flashrom_flashctx *flash,
				      const char *const keyfile,
				      const uint32_t key_data,
				      const unsigned int counter_address)
{
	const unsigned int signature_offset = RPMC_OP1_MSG_HEADER_LENGTH + RPMC_KEY_DATA_LENGTH;
	unsigned char msg[RPMC_UPDATE_HMAC_KEY_MSG_LENGTH] = {
		flash->chip->rpmc_ctx.op1_opcode, // Opcode
		0x01, // CmdType
		counter_address, // CounterAddr
		0, // Reserved
		(key_data >> 24) & 0xff,
		(key_data >> 16) & 0xff,
		(key_data >> 8) & 0xff,
		key_data & 0xff
	};

	enum rpmc_result ret = rpmc_basic_checks(flash, counter_address);
	if (ret != RPMC_SUCCESS)
		return ret;

	uint8_t status;
	ret = rpmc_sign_send_wait_check(flash,
					msg,
					RPMC_UPDATE_HMAC_KEY_MSG_LENGTH,
					signature_offset,
					keyfile,
					key_data,
					&status);
	if (ret != RPMC_SUCCESS)
		return ret;

	if (status & (1 << 1)) {
		return RPMC_ERROR_COUNTER_UNINITIALIZED;
	} else if (status & (1 << 2)) {
		// Counter address out of range or incorrect payload size received
		// also possible but we check those in the code
		return RPMC_ERROR_WRONG_SIGNATURE;
	} else if (status != 0x80) {
		// Unexpected bit is set
		// This should not happen, if we wrote the code correctly
		return RPMC_ERROR_INTERNAL;
	}

	return RPMC_SUCCESS;
}

enum rpmc_result rpmc_increment_counter(struct flashrom_flashctx *flash,
					const char *const keyfile,
					const uint32_t key_data,
					const unsigned int counter_address,
					const uint32_t previous_value)
{
	const unsigned int signature_offset = RPMC_OP1_MSG_HEADER_LENGTH + RPMC_COUNTER_LENGTH;
	unsigned char msg[RPMC_INCREMENT_MONOTONIC_COUNTER_MSG_LENGTH] = {
		flash->chip->rpmc_ctx.op1_opcode, // Opcode
		0x02, // CmdType
		counter_address, // CounterAddr
		0, // Reserved
		(previous_value >> 24) & 0xff,
		(previous_value >> 16) & 0xff,
		(previous_value >> 8) & 0xff,
		previous_value & 0xff
	};

	enum rpmc_result ret = rpmc_basic_checks(flash, counter_address);
	if (ret != RPMC_SUCCESS)
		return ret;

	uint8_t status;
	ret = rpmc_sign_send_wait_check(flash,
					msg,
					RPMC_INCREMENT_MONOTONIC_COUNTER_MSG_LENGTH,
					signature_offset,
					keyfile,
					key_data,
					&status);
	if (ret != RPMC_SUCCESS)
		return ret;

	if (status & (1 << 4)) {
		return RPMC_ERROR_COUNTER_DATA_MISMATCH;
	} else if (status & (1 << 3)) {
		return RPMC_ERROR_HMAC_KEY_REGISTER_UNINITIALIZED;
	} else if (status & (1 << 2)) {
		// Counter address out of range or incorrect payload size received
		// also possible but we check those in the code
		return RPMC_ERROR_WRONG_SIGNATURE;
	} else if (status != 0x80) {
		// Unexpected bit is set
		// This should not happen, if we wrote the code correctly
		return RPMC_ERROR_INTERNAL;
	}

	return RPMC_SUCCESS;
}

enum rpmc_result rpmc_get_monotonic_counter(struct flashrom_flashctx *flash,
					    const char *const keyfile,
					    const uint32_t key_data,
					    const unsigned int counter_address,
					    uint32_t *const counter_value)
{
	unsigned char hmac_key_register[RPMC_HMAC_KEY_LENGTH];
	const unsigned int tag_offset = RPMC_OP1_MSG_HEADER_LENGTH;
	const unsigned int signature_offset = tag_offset + RPMC_TAG_LENGTH;
	unsigned char msg[RPMC_GET_MONOTONIC_COUNTER_MSG_LENGTH] = {
		flash->chip->rpmc_ctx.op1_opcode, // Opcode
		0x03, // CmdType
		counter_address, // CounterAddr
		0, // Reserved
	};

	enum rpmc_result ret = rpmc_basic_checks(flash, counter_address);
	if (ret != RPMC_SUCCESS)
		return ret;

	if (RAND_bytes(msg + tag_offset, RPMC_TAG_LENGTH) != 1) {
		msg_gerr("Could not generate random tag.\n");
		return RPMC_ERROR_OPENSSL;
	}

	msg_gdbg("Random tag is:");
	for (size_t i = 0; i < RPMC_TAG_LENGTH; i++) {
		msg_gdbg(" 0x%02x", msg[tag_offset + i]);
	}
	msg_gdbg("\n");

	ret = rpmc_calculate_hmac_key_register(keyfile, key_data, hmac_key_register);
	if (ret != RPMC_SUCCESS)
		return ret;

	if (HMAC(EVP_sha256(),
		 hmac_key_register, RPMC_HMAC_KEY_LENGTH,
		 msg, signature_offset,
		 msg + signature_offset, NULL) == NULL) {
		msg_gerr("Could not generate HMAC signature\n");
		return RPMC_ERROR_OPENSSL;
	}

	ret = rpmc_send_and_wait(flash, msg, RPMC_GET_MONOTONIC_COUNTER_MSG_LENGTH);
	if (ret != RPMC_SUCCESS)
		return ret;

	struct rpmc_status_register status;
	ret = rpmc_get_extended_status_long(flash, &status, msg + tag_offset, hmac_key_register);
	if (!(ret == RPMC_ERROR_TAG_MISMATCH || ret == RPMC_ERROR_SIGNATURE_MISMATCH || ret == RPMC_SUCCESS))
		return ret;

	if (status.status & (1 << 3)) {
		return RPMC_ERROR_HMAC_KEY_REGISTER_UNINITIALIZED;
	} else if (status.status & (1 << 2)) {
		// Counter address out of range or incorrect payload size received
		// also possible but we check those in the code
		return RPMC_ERROR_WRONG_SIGNATURE;
	} else if (status.status != 0x80) {
		// Unexpected bit is set
		// This should not happen, if we wrote the code correctly
		return RPMC_ERROR_INTERNAL;
	}

	*counter_value = status.counter_data;
	return ret;
}

enum rpmc_result rpmc_read_data(struct flashrom_flashctx *flash, struct rpmc_status_register *status)
{
	// hack around not having a counter address
	enum rpmc_result ret = rpmc_basic_checks(flash, 0);
	if (ret != RPMC_SUCCESS)
		return ret;

	ret = rpmc_get_extended_status_long(flash, status, NULL, NULL);
	if (ret != RPMC_SUCCESS)
		return ret;

	return RPMC_SUCCESS;
}

const char *rpmc_describe_result(const enum rpmc_result value)
{
	switch (value) {
		case RPMC_SUCCESS:
			return "Success\n";
		case RPMC_ERROR_SPI_TRANSMISSION:
			return "Error: Sending spi command failed\n";
		case RPMC_ERROR_OPENSSL:
			return "Error: Failure while calling into openssl\n";
		case RPMC_ERROR_TAG_MISMATCH:
			return "Error: The recieved tag doesn't match the one that was sent\n";
		case RPMC_ERROR_SIGNATURE_MISMATCH:
			return "Error: The recieved signature doesn't match the expected one\n";
		case RPMC_ERROR_INTERNAL:
			return "Internal error: Unexpected state reached, please inform the maintainers\n";
		case RPMC_ERROR_KEY_READ:
			return "Error: Failed to read the key from keyfile\n";
		case RPMC_ERROR_HARDENING_UNSUPPORTED:
			return "Error: RPMC commands are not supported on this device\n";
		case RPMC_ERROR_COUNTER_OUT_OF_RANGE:
			return "Error: Given counter address not in range for this device\n";
		case RPMC_ERROR_ROOT_KEY_OVERWRITE:
			return "Error: Root key for this counter address can't be overwritten\n";
		case RPMC_ERROR_COUNTER_UNINITIALIZED:
			return "Error: Root key for this counter is not initialized\n";
		case RPMC_ERROR_COUNTER_DATA_MISMATCH:
			return "Error: Previous value of this counter is not correct\n";
		case RPMC_ERROR_HMAC_KEY_REGISTER_UNINITIALIZED:
			return "Error: Hmac key register is not initialized\n";
		case RPMC_ERROR_WRONG_SIGNATURE:
			return "Error: The signature doesn't match (root key or key data is wrong)\n";
		default:
			return "Unknown internal error: Error code not recognised\n";
	}
}
