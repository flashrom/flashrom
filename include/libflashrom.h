/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Google Inc.
 * Copyright (C) 2012 secunet Security Networks AG
 * (Written by Nico Huber <nico.huber@secunet.com> for secunet)
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

#ifndef __LIBFLASHROM_H__
#define __LIBFLASHROM_H__ 1

/**
 * @mainpage
 *
 * Have a look at the Modules section for a function reference.
 */

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

/**
 * @defgroup flashrom-general General
 * @{
 */

/**
 * @brief Initialize libflashrom.
 *
 * @param perform_selfcheck If not zero, perform a self check.
 * @return 0 on success
 */
int flashrom_init(int perform_selfcheck);
/**
 * @brief Shut down libflashrom.
 * @return 0 on success
 */
int flashrom_shutdown(void);
enum flashrom_log_level {
	FLASHROM_MSG_ERROR	= 0,
	FLASHROM_MSG_WARN	= 1,
	FLASHROM_MSG_INFO	= 2,
	FLASHROM_MSG_DEBUG	= 3,
	FLASHROM_MSG_DEBUG2	= 4,
	FLASHROM_MSG_SPEW	= 5,
};
typedef int(flashrom_log_callback)(enum flashrom_log_level, const char *format, va_list);
/**
 * @brief Set the log callback function.
 *
 * Set a callback function which will be invoked whenever libflashrom wants
 * to output messages. This allows frontends to do whatever they see fit with
 * such messages, e.g. write them to syslog, or to a file, or print them in a
 * GUI window, etc.
 *
 * @param log_callback Pointer to the new log callback function.
 */
void flashrom_set_log_callback(flashrom_log_callback *log_callback);

enum flashrom_progress_stage {
	FLASHROM_PROGRESS_READ,
	FLASHROM_PROGRESS_WRITE,
	FLASHROM_PROGRESS_ERASE,
	FLASHROM_PROGRESS_NR,
};
struct flashrom_progress {
	enum flashrom_progress_stage stage;
	size_t current;
	size_t total;
	void *user_data;
};
struct flashrom_flashctx;
typedef void(flashrom_progress_callback)(struct flashrom_flashctx *flashctx);
/**
 * @brief Set the progress callback function.
 *
 * Set a callback function which will be invoked whenever libflashrom wants
 * to indicate the progress has changed. This allows frontends to do whatever
 * they see fit with such values, e.g. update a progress bar in a GUI tool.
 *
 * @param progress_callback Pointer to the new progress callback function.
 * @param progress_state Pointer to progress state to include with the progress
 * callback.
 */
void flashrom_set_progress_callback(struct flashrom_flashctx *const flashctx,
		flashrom_progress_callback *progress_callback, struct flashrom_progress *progress_state);

/** @} */ /* end flashrom-general */

/**
 * @defgroup flashrom-query Querying
 * @{
 */

enum flashrom_test_state {
	FLASHROM_TESTED_OK  = 0,
	FLASHROM_TESTED_NT  = 1,
	FLASHROM_TESTED_BAD = 2,
	FLASHROM_TESTED_DEP = 3,
	FLASHROM_TESTED_NA  = 4,
};

struct flashrom_flashchip_info {
	const char *vendor;
	const char *name;
	unsigned int total_size;
	struct flashrom_tested {
		enum flashrom_test_state probe;
		enum flashrom_test_state read;
		enum flashrom_test_state erase;
		enum flashrom_test_state write;
		enum flashrom_test_state wp;
	} tested;
};

struct flashrom_board_info {
	const char *vendor;
	const char *name;
	enum flashrom_test_state working;
};

struct flashrom_chipset_info {
	const char *vendor;
	const char *chipset;
	uint16_t vendor_id;
	uint16_t chipset_id;
	enum flashrom_test_state status;
};

/**
 * @brief Returns flashrom version
 * @return flashrom version
 */
const char *flashrom_version_info(void);
/**
 * @brief Returns list of supported flash chips
 * @return List of supported flash chips, or NULL if an error occurred
 */
struct flashrom_flashchip_info *flashrom_supported_flash_chips(void);
/**
 * @brief Returns list of supported mainboards
 * @return List of supported mainboards, or NULL if an error occurred
 */
struct flashrom_board_info *flashrom_supported_boards(void);
/**
 * @brief Returns list of supported chipsets
 * @return List of supported chipsets, or NULL if an error occurred
 */
struct flashrom_chipset_info *flashrom_supported_chipsets(void);
/**
 * @brief Frees memory allocated by libflashrom API
 * @param p Pointer to block of memory which should be freed
 * @return 0 on success
 */
int flashrom_data_free(void *const p);

/** @} */ /* end flashrom-query */

/**
 * @defgroup flashrom-prog Programmers
 * @{
 */
struct flashrom_programmer;

/**
 * @brief Initialize the specified programmer.
 *
 * Currently, only one programmer may be initialized at a time.
 *
 * @param[out] flashprog Points to a pointer of type struct flashrom_programmer
 *                       that will be set if programmer initialization succeeds.
 *                       *flashprog has to be shutdown by the caller with @ref
 *                       flashrom_programmer_shutdown.
 * @param[in] prog_name Name of the programmer to initialize.
 * @param[in] prog_params Pointer to programmer specific parameters.
 * @return 0 on success
 */
int flashrom_programmer_init(struct flashrom_programmer **flashprog, const char *prog_name, const char *prog_params);
/**
 * @brief Shut down the initialized programmer.
 *
 * @param flashprog The programmer to shut down.
 * @return 0 on success
 */
int flashrom_programmer_shutdown(struct flashrom_programmer *flashprog);

/** @} */ /* end flashrom-prog */

/**
 * @defgroup flashrom-flash Flash chips
 * @{
 */

/**
 * @brief Probe for a flash chip.
 *
 * Probes for a flash chip and returns a flash context, that can be used
 * later with flash chip and @ref flashrom-ops "image operations", if
 * exactly one matching chip is found.
 *
 * @param[out] flashctx Points to a pointer of type struct flashrom_flashctx
 *                      that will be set if exactly one chip is found. *flashctx
 *                      has to be freed by the caller with @ref flashrom_flash_release.
 * @param[in] flashprog The flash programmer used to access the chip.
 * @param[in] chip_name Name of a chip to probe for, or NULL to probe for
 *                      all known chips.
 * @return 0 on success,
 *         3 if multiple chips were found,
 *         2 if no chip was found,
 *         or 1 on any other error.
 */
int flashrom_flash_probe(struct flashrom_flashctx **flashctx, const struct flashrom_programmer *flashprog, const char *chip_name);
/**
 * @brief Returns the size of the specified flash chip in bytes.
 *
 * @param flashctx The queried flash context.
 * @return Size of flash chip in bytes.
 */
size_t flashrom_flash_getsize(const struct flashrom_flashctx *flashctx);
/**
 * @brief Erase the specified ROM chip.
 *
 * If a layout is set in the given flash context, only included regions
 * will be erased.
 *
 * @param flashctx The context of the flash chip to erase.
 * @return 0 on success.
 */
int flashrom_flash_erase(struct flashrom_flashctx *flashctx);
/**
 * @brief Free a flash context.
 *
 * @param flashctx Flash context to free.
 */
void flashrom_flash_release(struct flashrom_flashctx *flashctx);

enum flashrom_flag {
	FLASHROM_FLAG_FORCE,
	FLASHROM_FLAG_FORCE_BOARDMISMATCH,
	FLASHROM_FLAG_VERIFY_AFTER_WRITE,
	FLASHROM_FLAG_VERIFY_WHOLE_CHIP,
	FLASHROM_FLAG_SKIP_UNREADABLE_REGIONS,
	FLASHROM_FLAG_SKIP_UNWRITABLE_REGIONS,
};

/**
 * @brief Set a flag in the given flash context.
 *
 * @param flashctx Flash context to alter.
 * @param flag	   Flag that is to be set / cleared.
 * @param value	   Value to set.
 */
void flashrom_flag_set(struct flashrom_flashctx *flashctx, enum flashrom_flag flag, bool value);
/**
 * @brief Return the current value of a flag in the given flash context.
 *
 * @param flashctx Flash context to read from.
 * @param flag	   Flag to be read.
 * @return Current value of the flag.
 */
bool flashrom_flag_get(const struct flashrom_flashctx *flashctx, enum flashrom_flag flag);

/** @} */ /* end flashrom-flash */

/**
 * @defgroup flashrom-ops Operations
 * @{
 */

/**
 * @brief Read the current image from the specified ROM chip.
 *
 * If a layout is set in the specified flash context, only included regions
 * will be read.
 *
 * @param flashctx The context of the flash chip.
 * @param buffer Target buffer to write image to.
 * @param buffer_len Size of target buffer in bytes.
 * @return 0 on success,
 *         2 if buffer_len is too small for the flash chip's contents,
 *         or 1 on any other failure.
 */
int flashrom_image_read(struct flashrom_flashctx *flashctx, void *buffer, size_t buffer_len);
/**
 * @brief Write the specified image to the ROM chip.
 *
 * If a layout is set in the specified flash context, only erase blocks
 * containing included regions will be touched.
 *
 * @param flashctx The context of the flash chip.
 * @param buffer Source buffer to read image from (may be altered for full verification).
 * @param buffer_len Size of source buffer in bytes.
 * @param refbuffer If given, assume flash chip contains same data as `refbuffer`.
 * @return 0 on success,
 *         4 if buffer_len doesn't match the size of the flash chip,
 *         3 if write was tried but nothing has changed,
 *         2 if write failed and flash contents changed,
 *         or 1 on any other failure.
 */
int flashrom_image_write(struct flashrom_flashctx *flashctx, void *buffer, size_t buffer_len, const void *refbuffer);
/**
 * @brief Verify the ROM chip's contents with the specified image.
 *
 * If a layout is set in the specified flash context, only included regions
 * will be verified.
 *
 * @param flashctx The context of the flash chip.
 * @param buffer Source buffer to verify with.
 * @param buffer_len Size of source buffer in bytes.
 * @return 0 on success,
 *         3 if the chip's contents don't match,
 *         2 if buffer_len doesn't match the size of the flash chip,
 *         or 1 on any other failure.
 */
int flashrom_image_verify(struct flashrom_flashctx *flashctx, const void *buffer, size_t buffer_len);

/** @} */ /* end flashrom-ops */

/**
 * @defgroup flashrom-layout Layout handling
 * @{
 */

struct flashrom_layout;
/**
 * @brief Create a new, empty layout.
 *
 * @param layout Pointer to returned layout reference.
 *
 * @return 0 on success,
 *         1 if out of memory.
 */
int flashrom_layout_new(struct flashrom_layout **layout);

/**
 * @brief Read a layout from the Intel ICH descriptor in the flash.
 *
 * Optionally verify that the layout matches the one in the given
 * descriptor dump.
 *
 * @param[out] layout Points to a struct flashrom_layout pointer that
 *                    gets set if the descriptor is read and parsed
 *                    successfully.
 * @param[in] flashctx Flash context to read the descriptor from flash.
 * @param[in] dump     The descriptor dump to compare to or NULL.
 * @param[in] len      The length of the descriptor dump.
 *
 * @return 0 on success,
 *         6 if descriptor parsing isn't implemented for the host,
 *         5 if the descriptors don't match,
 *         4 if the descriptor dump couldn't be parsed,
 *         3 if the descriptor on flash couldn't be parsed,
 *         2 if the descriptor on flash couldn't be read,
 *         1 on any other error.
 */
int flashrom_layout_read_from_ifd(struct flashrom_layout **layout, struct flashrom_flashctx *flashctx, const void *dump, size_t len);
/**
 * @brief Read a layout by searching the flash chip for fmap.
 *
 * @param[out] layout Points to a struct flashrom_layout pointer that
 *                    gets set if the fmap is read and parsed successfully.
 * @param[in] flashctx Flash context
 * @param[in] offset Offset to begin searching for fmap.
 * @param[in] length Length of address space to search.
 *
 * @return 0 on success,
 *         3 if fmap parsing isn't implemented for the host,
 *         2 if the fmap couldn't be read,
 *         1 on any other error.
 */
int flashrom_layout_read_fmap_from_rom(struct flashrom_layout **layout,
		struct flashrom_flashctx *flashctx, size_t offset, size_t length);
/**
 * @brief Read a layout by searching a buffer for fmap.
 *
 * @param[out] layout Points to a struct flashrom_layout pointer that
 *                    gets set if the fmap is read and parsed successfully.
 * @param[in] flashctx Flash context
 * @param[in] buffer Buffer to search in
 * @param[in] len Size of buffer to search
 *
 * @return 0 on success,
 *         3 if fmap parsing isn't implemented for the host,
 *         2 if the fmap couldn't be read,
 *         1 on any other error.
 */
int flashrom_layout_read_fmap_from_buffer(struct flashrom_layout **layout,
		struct flashrom_flashctx *flashctx, const uint8_t *buffer, size_t len);
/**
 * @brief Add a region to an existing layout.
 *
 * @param layout The existing layout.
 * @param start  Start address of the region.
 * @param end    End address (inclusive) of the region.
 * @param name   Name of the region.
 *
 * @return 0 on success,
 *         1 if out of memory.
 */
int flashrom_layout_add_region(struct flashrom_layout *layout, size_t start, size_t end, const char *name);
/**
 * @brief Mark given region as included.
 *
 * @param layout The layout to alter.
 * @param name   The name of the region to include.
 *
 * @return 0 on success,
 *         1 if the given name can't be found.
 */
int flashrom_layout_include_region(struct flashrom_layout *layout, const char *name);
/**
 * @brief Mark given region as not included.
 *
 * @param layout The layout to alter.
 * @param name   The name of the region to exclude.
 *
 * @return 0 on success,
 *         1 if the given name can't be found.
 */
int flashrom_layout_exclude_region(struct flashrom_layout *layout, const char *name);
/**
 * @brief Get given region's offset and length.
 *
 * @param[in]  layout The existing layout.
 * @param[in]  name   The name of the region.
 * @param[out] start  The start address to be written.
 * @param[out] len    The length of the region to be written.
 *
 * @return 0 on success,
 *         1 if the given name can't be found.
 */
int flashrom_layout_get_region_range(struct flashrom_layout *layout, const char *name,
		     unsigned int *start, unsigned int *len);
/**
 * @brief Free a layout.
 *
 * @param layout Layout to free.
 */
void flashrom_layout_release(struct flashrom_layout *layout);
/**
 * @brief Set the active layout for a flash context.
 *
 * Note: The caller must not release the layout as long as it is used through
 *       the given flash context.
 *
 * @param flashctx Flash context whose layout will be set.
 * @param layout   Layout to bet set.
 */
void flashrom_layout_set(struct flashrom_flashctx *flashctx, const struct flashrom_layout *layout);

/** @} */ /* end flashrom-layout */

/**
 * @defgroup flashrom-wp Write Protect
 * @{
 */

enum flashrom_wp_result {
	FLASHROM_WP_OK = 0,
	FLASHROM_WP_ERR_CHIP_UNSUPPORTED = 1,
	FLASHROM_WP_ERR_OTHER = 2,
	FLASHROM_WP_ERR_READ_FAILED = 3,
	FLASHROM_WP_ERR_WRITE_FAILED = 4,
	FLASHROM_WP_ERR_VERIFY_FAILED = 5,
	FLASHROM_WP_ERR_RANGE_UNSUPPORTED = 6,
	FLASHROM_WP_ERR_MODE_UNSUPPORTED = 7,
	FLASHROM_WP_ERR_RANGE_LIST_UNAVAILABLE = 8,
	FLASHROM_WP_ERR_UNSUPPORTED_STATE = 9
};

enum flashrom_wp_mode {
	FLASHROM_WP_MODE_DISABLED,
	FLASHROM_WP_MODE_HARDWARE,
	FLASHROM_WP_MODE_POWER_CYCLE,
	FLASHROM_WP_MODE_PERMANENT
};
struct flashrom_wp_cfg;
struct flashrom_wp_ranges;
/**
 * @brief Create a new empty WP configuration.
 *
 * @param[out] cfg Points to a pointer of type struct flashrom_wp_cfg that will
 *                 be set if creation succeeds. *cfg has to be freed by the
 *                 caller with @ref flashrom_wp_cfg_release.
 * @return  0 on success
 *         >0 on failure
 */
enum flashrom_wp_result flashrom_wp_cfg_new(struct flashrom_wp_cfg **cfg);
/**
 * @brief Free a WP configuration.
 *
 * @param[in] cfg Pointer to the flashrom_wp_cfg to free.
 */
void flashrom_wp_cfg_release(struct flashrom_wp_cfg *cfg);
/**
 * @brief Set the protection mode for a WP configuration.
 *
 * @param[in]  mode The protection mode to set.
 * @param[out] cfg  Pointer to the flashrom_wp_cfg structure to modify.
 */
void flashrom_wp_set_mode(struct flashrom_wp_cfg *cfg, enum flashrom_wp_mode mode);
/**
 * @brief Get the protection mode from a WP configuration.
 *
 * @param[in] cfg The WP configuration to get the protection mode from.
 * @return        The configuration's protection mode.
 */
enum flashrom_wp_mode flashrom_wp_get_mode(const struct flashrom_wp_cfg *cfg);
/**
 * @brief Set the protection range for a WP configuration.
 *
 * @param[out] cfg   Pointer to the flashrom_wp_cfg structure to modify.
 * @param[in]  start The range's start address.
 * @param[in]  len   The range's length.
 */
void flashrom_wp_set_range(struct flashrom_wp_cfg *cfg, size_t start, size_t len);
/**
 * @brief Get the protection range from a WP configuration.
 *
 * @param[out] start Points to a size_t to write the range start to.
 * @param[out] len   Points to a size_t to write the range length to.
 * @param[in]  cfg   The WP configuration to get the range from.
 */
void flashrom_wp_get_range(size_t *start, size_t *len, const struct flashrom_wp_cfg *cfg);

/**
 * @brief Read the current WP configuration from a flash chip.
 *
 * @param[out] cfg   Pointer to a struct flashrom_wp_cfg to store the chip's
 *                   configuration in.
 * @param[in]  flash The flash context used to access the chip.
 * @return  0 on success
 *         >0 on failure
 */
enum flashrom_wp_result flashrom_wp_read_cfg(struct flashrom_wp_cfg *cfg, struct flashrom_flashctx *flash);
/**
 * @brief Write a WP configuration to a flash chip.
 *
 * @param[in] flash The flash context used to access the chip.
 * @param[in] cfg   The WP configuration to write to the chip.
 * @return  0 on success
 *         >0 on failure
 */
enum flashrom_wp_result flashrom_wp_write_cfg(struct flashrom_flashctx *flash, const struct flashrom_wp_cfg *cfg);

/**
 * @brief Get a list of protection ranges supported by the flash chip.
 *
 * @param[out] ranges Points to a pointer of type struct flashrom_wp_ranges
 *                    that will be set if available ranges are found. Finding
 *                    available ranges may not always be possible, even if the
 *                    chip's protection range can be read or modified. *ranges
 *                    must be freed using @ref flashrom_wp_ranges_release.
 * @param[in] flash   The flash context used to access the chip.
 * @return  0 on success
 *         >0 on failure
 */
enum flashrom_wp_result flashrom_wp_get_available_ranges(struct flashrom_wp_ranges **ranges, struct flashrom_flashctx *flash);
/**
 * @brief Get a number of protection ranges in a range list.
 *
 * @param[in]  ranges The range list to get the count from.
 * @return Number of ranges in the list.
 */
size_t flashrom_wp_ranges_get_count(const struct flashrom_wp_ranges *ranges);
/**
 * @brief Get a protection range from a range list.
 *
 * @param[out] start  Points to a size_t to write the range's start to.
 * @param[out] len    Points to a size_t to write the range's length to.
 * @param[in]  ranges The range list to get the range from.
 * @param[in]  index  Index of the range to get.
 * @return  0 on success
 *         >0 on failure
 */
enum flashrom_wp_result flashrom_wp_ranges_get_range(size_t *start, size_t *len, const struct flashrom_wp_ranges *ranges, unsigned int index);
/**
 * @brief Free a WP range list.
 *
 * @param[out] ranges Pointer to the flashrom_wp_ranges to free.
 */
void flashrom_wp_ranges_release(struct flashrom_wp_ranges *ranges);

/** @} */ /* end flashrom-wp */

#endif				/* !__LIBFLASHROM_H__ */
