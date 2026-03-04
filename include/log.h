/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2000 Ronald G. Minnich <rminnich@gmail.com>
 * SPDX-FileCopyrightText: 2005-2009 coresystems GmbH
 * SPDX-FileCopyrightText: 2006-2009 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2026 Antonio Vázquez Blanco
 */

#ifndef __LOG_H__
#define __LOG_H__

#include "libflashrom.h"
#include <stdio.h>

/*
 * Let gcc and clang check for correct printf-style format strings.
 * Avoid using formatted messages longer than 256 characters.
 * The new public logging API formats messages and passes a ready-to-print
 * string  to the user callback for performance reasons.
 * Therefore, any message exceeding 256 characters will be truncated,
 * and an ERANGE error code will be returned.
 */
int print(enum flashrom_log_level level, const char *fmt, ...)
#ifdef __MINGW32__
#  ifndef __MINGW_PRINTF_FORMAT
#    define __MINGW_PRINTF_FORMAT gnu_printf
#  endif
__attribute__((format(__MINGW_PRINTF_FORMAT, 2, 3)));
#else
__attribute__((format(printf, 2, 3)));
#endif

#define msg_gerr(...)	print(FLASHROM_MSG_ERROR, __VA_ARGS__)	/* general errors */
#define msg_perr(...)	print(FLASHROM_MSG_ERROR, __VA_ARGS__)	/* programmer errors */
#define msg_cerr(...)	print(FLASHROM_MSG_ERROR, __VA_ARGS__)	/* chip errors */
#define msg_gwarn(...)	print(FLASHROM_MSG_WARN, __VA_ARGS__)	/* general warnings */
#define msg_pwarn(...)	print(FLASHROM_MSG_WARN, __VA_ARGS__)	/* programmer warnings */
#define msg_cwarn(...)	print(FLASHROM_MSG_WARN, __VA_ARGS__)	/* chip warnings */
#define msg_ginfo(...)	print(FLASHROM_MSG_INFO, __VA_ARGS__)	/* general info */
#define msg_pinfo(...)	print(FLASHROM_MSG_INFO, __VA_ARGS__)	/* programmer info */
#define msg_cinfo(...)	print(FLASHROM_MSG_INFO, __VA_ARGS__)	/* chip info */
#define msg_gdbg(...)	print(FLASHROM_MSG_DEBUG, __VA_ARGS__)	/* general debug */
#define msg_pdbg(...)	print(FLASHROM_MSG_DEBUG, __VA_ARGS__)	/* programmer debug */
#define msg_cdbg(...)	print(FLASHROM_MSG_DEBUG, __VA_ARGS__)	/* chip debug */
#define msg_gdbg2(...)	print(FLASHROM_MSG_DEBUG2, __VA_ARGS__)	/* general debug2 */
#define msg_pdbg2(...)	print(FLASHROM_MSG_DEBUG2, __VA_ARGS__)	/* programmer debug2 */
#define msg_cdbg2(...)	print(FLASHROM_MSG_DEBUG2, __VA_ARGS__)	/* chip debug2 */
#define msg_gspew(...)	print(FLASHROM_MSG_SPEW, __VA_ARGS__)	/* general debug spew  */
#define msg_pspew(...)	print(FLASHROM_MSG_SPEW, __VA_ARGS__)	/* programmer debug spew  */
#define msg_cspew(...)	print(FLASHROM_MSG_SPEW, __VA_ARGS__)	/* chip debug spew  */

#endif
