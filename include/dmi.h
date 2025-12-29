/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __DMI_H__
#define __DMI_H__

#if defined(__i386__) || defined(__x86_64__)

#include <stdbool.h>

void dmi_init(int *is_laptop);

bool dmi_is_supported(void);

int dmi_match(const char *pattern);

#endif // defined(__i386__) || defined(__x86_64__)

#endif // __DMI_H__
