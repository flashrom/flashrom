/*
 * This file is part of the flashrom project.
 *
 * Copyright 2024 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
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

#ifndef __CLI_OUTPUT_H__
#define __CLI_OUTPUT_H__

#include <stdarg.h>
#include <flash.h>

extern enum flashrom_log_level verbose_screen;
extern enum flashrom_log_level verbose_logfile;
int open_logfile(const char * const filename);
int close_logfile(void);
void start_logging(void);
int flashrom_print_cb(enum flashrom_log_level level, const char *fmt, va_list ap);
void flashrom_progress_cb(struct flashrom_flashctx *flashctx);

#endif /* __CLI_OUTPUT_H__ */
