/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2025 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
 */

#ifndef __UDELAY_H__
#define __UDELAY_H__

void internal_sleep(unsigned int usecs);
void default_delay(unsigned int usecs);

#endif
