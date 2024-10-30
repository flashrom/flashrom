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

#ifndef __RPMC_H__
#define __RPMC_H__ 1

#include <stdint.h>
#include "flash.h" // for flashctx

/**
 * @defgroup flashrom-rpmc RPMC operations
 * @{
 */

#define RPMC_OP1_MSG_HEADER_LENGTH 4
#define RPMC_SIGNATURE_LENGTH 32
#define RPMC_COUNTER_LENGTH 4
#define RPMC_KEY_DATA_LENGTH 4
#define RPMC_TAG_LENGTH 12
#define RPMC_HMAC_KEY_LENGTH 32
#define RPMC_TRUNCATED_SIG_LENGTH 28

enum rpmc_result {
	RPMC_SUCCESS = 0,
	RPMC_ERROR_SPI_TRANSMISSION,
	RPMC_ERROR_OPENSSL,
	RPMC_ERROR_TAG_MISMATCH,
	RPMC_ERROR_SIGNATURE_MISMATCH,
	RPMC_ERROR_INTERNAL,
	RPMC_ERROR_KEY_READ,
	RPMC_ERROR_HARDENING_UNSUPPORTED,
	RPMC_ERROR_COUNTER_OUT_OF_RANGE,
	RPMC_ERROR_ROOT_KEY_OVERWRITE,
	RPMC_ERROR_COUNTER_UNINITIALIZED,
	RPMC_ERROR_COUNTER_DATA_MISMATCH,
	RPMC_ERROR_HMAC_KEY_REGISTER_UNINITIALIZED,
	RPMC_ERROR_WRONG_SIGNATURE
};

struct rpmc_status_register {
	/*
	 * Values:
	 * 0b00000000 -> Power on state
	 * 0b10000000 -> Success
	 * 0b0xxxxxx1 -> Busy
	 * 0b0xxxxx1x -> Error: Root key register overwrite,
	 * 			counter Address out of range,
	 *                      truncated signature mismatch
	 *                      or monotonic counter uninitialized
	 * 0b0xxxx1xx -> Error: Signature mismatch,
	 *                      counter address out of range,
	 *                      cmdtype out of range
	 *                      or incorrect payload size
	 * 0b0xxx1xxx -> Error: Hmac key register uninitialized
	 * 0b0xx1xxxx -> Error: Counter data mismatch
	 * 0b0x1xxxxx -> Fatal device error
	 *
	 * Some bits might exclude others or their meaning might be dependent
	 * on previous commands.
	 * Read JESD260 or your device's data sheet for more details.
	 */
	uint8_t status;
	unsigned char tag[RPMC_TAG_LENGTH];
	uint32_t counter_data;
	unsigned char signature[RPMC_SIGNATURE_LENGTH];
};

/**
 * @brief Write root key on flashchip
 *
 * @param[in] flash Flash context which rpmc options will be used
 * @param[in] keyfile Location of 32-byte key to use
 * @param[in] counter_address Address of counter (starts at 0)
 *
 * @return The result of the operation
 */
enum rpmc_result rpmc_write_root_key(struct flashrom_flashctx *flash,
				     const char *keyfile,
				     unsigned int counter_address);

/**
 * @brief Update hmac key register
 *
 * @param[in] flash Flash context which rpmc options will be used
 * @param[in] keyfile Location of 32-byte key to use
 * @param[in] key_data 4-bytes of data to use as key data
 * @param[in] counter_address Address of counter (starts at 0)
 *
 * @return The result of the operation
 */
enum rpmc_result rpmc_update_hmac_key(struct flashrom_flashctx *flash,
				      const char *keyfile,
				      uint32_t key_data,
				      unsigned int counter_address);

/**
 * @brief Increment monotonic counter value
 *
 * @param[in] flash Flash context which rpmc options will be used
 * @param[in] keyfile Location of 32-byte key to use
 * @param[in] key_data 4-bytes of data to use as key data
 * @param[in] counter_address Address of counter (starts at 0)
 * @param[in] previous_value Previous value of counter
 *
 * @return The result of the operation
 */
enum rpmc_result rpmc_increment_counter(struct flashrom_flashctx *flash,
					const char *keyfile,
					uint32_t key_data,
					unsigned int counter_address,
					uint32_t previous_value);

/**
 * @brief Get monotonic counter value
 *
 * @param[in] flash Flash context which rpmc options will be used
 * @param[in] keyfile Location of 32-byte key to use
 * @param[in] key_data 4-bytes of data to use as key data
 * @param[in] counter_address Address of counter (starts at 0)
 * @param[out] counter_value Pointer to write the counter value to
 *
 * @return The result of the operation
 */
enum rpmc_result rpmc_get_monotonic_counter(struct flashrom_flashctx *flash,
					    const char *keyfile,
					    uint32_t key_data,
					    unsigned int counter_address,
					    uint32_t *counter_value);

/**
 * @brief Read the full JESD260 extended status register
 *
 * @param[in] flash Flash context which rpmc options will be used
 * @param[out] status Status register to write data into
 *
 * @return The result of the operation
 */
enum rpmc_result rpmc_read_data(struct flashrom_flashctx *flash,
				struct rpmc_status_register *status);

/**
 * @brief Get a string description for rpmc result
 *
 * @param[in] value A rpmc result
 *
 * @return String description
 */
const char * rpmc_describe_result(enum rpmc_result value);

/** @} */ /* end flashrom-rpmc */

#endif /* !__RPMC_H__ */
