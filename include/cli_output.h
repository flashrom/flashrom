/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2024 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
 */

#ifndef __CLI_OUTPUT_H__
#define __CLI_OUTPUT_H__

#include <stdarg.h>
#include "flash.h"

extern enum flashrom_log_level verbose_screen;
extern enum flashrom_log_level verbose_logfile;
int open_logfile(const char * const filename);
int close_logfile(void);
void start_logging(void);
int flashrom_print_cb(enum flashrom_log_level level, const char *fmt, va_list ap);
void flashrom_progress_cb(enum flashrom_progress_stage stage, size_t current, size_t total, void* user_data);

#endif /* __CLI_OUTPUT_H__ */
