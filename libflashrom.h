/*
 * This file is part of the flashrom project.
 *
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

#include <stdarg.h>

int flashrom_init(int perform_selfcheck);
int flashrom_shutdown(void);
/** @ingroup flashrom-general */
enum flashrom_log_level {
	FLASHROM_MSG_ERROR	= 0,
	FLASHROM_MSG_WARN	= 1,
	FLASHROM_MSG_INFO	= 2,
	FLASHROM_MSG_DEBUG	= 3,
	FLASHROM_MSG_DEBUG2	= 4,
	FLASHROM_MSG_SPEW	= 5,
};
/** @ingroup flashrom-general */
typedef int(flashrom_log_callback)(enum flashrom_log_level, const char *format, va_list);
typedef int(flashrom_progress_callback)(float progress);

void flashrom_set_log_callback(flashrom_log_callback *);
void flashrom_set_progress_callback(flashrom_progress_callback *);

struct flashrom_programmer;
int flashrom_programmer_init(struct flashrom_programmer **, const char *prog_name, const char *prog_params);
int flashrom_programmer_shutdown(struct flashrom_programmer *);

struct flashrom_flashctx;
int flashrom_flash_probe(struct flashrom_flashctx **, const struct flashrom_programmer *, const char *chip_name);
size_t flashrom_flash_getsize(const struct flashrom_flashctx *);
int flashrom_flash_erase(struct flashrom_flashctx *);
void flashrom_flash_release(struct flashrom_flashctx *);

/** @ingroup flashrom-flash */
enum flashrom_flag {
	FLASHROM_FLAG_FORCE,
	FLASHROM_FLAG_FORCE_BOARDMISMATCH,
	FLASHROM_FLAG_VERIFY_AFTER_WRITE,
	FLASHROM_FLAG_VERIFY_WHOLE_CHIP,
};
void flashrom_flag_set(struct flashrom_flashctx *, enum flashrom_flag, bool value);
bool flashrom_flag_get(const struct flashrom_flashctx *, enum flashrom_flag);

int flashrom_image_read(struct flashrom_flashctx *, void *buffer, size_t buffer_len);
int flashrom_image_write(struct flashrom_flashctx *, void *buffer, size_t buffer_len, const void *refbuffer);
int flashrom_image_verify(struct flashrom_flashctx *, const void *buffer, size_t buffer_len);

struct flashrom_layout;
int flashrom_layout_read_from_ifd(struct flashrom_layout **, struct flashrom_flashctx *, const void *dump, size_t len);
int flashrom_layout_read_fmap_from_rom(struct flashrom_layout **,
		struct flashrom_flashctx *, off_t offset, size_t length);
int flashrom_layout_read_fmap_from_buffer(struct flashrom_layout **layout,
		struct flashrom_flashctx *, const uint8_t *buf, size_t len);
int flashrom_layout_include_region(struct flashrom_layout *, const char *name);
void flashrom_layout_release(struct flashrom_layout *);
void flashrom_layout_set(struct flashrom_flashctx *, const struct flashrom_layout *);

#endif				/* !__LIBFLASHROM_H__ */
