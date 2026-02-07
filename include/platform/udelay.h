/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2025 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
 *
 * This header provides declarations for delay functions used to implement
 * microsecond-level timing delays in a platform-independent manner.
 */

#ifndef __UDELAY_H__
#define __UDELAY_H__

/**
 * @brief Delay for at least the specified number of microseconds.
 * @details
 * For very short delays this function polls the wall time provided by the OS,
 * otherwise it will use an OS-provided sleep()-like function. The delay will never
 * be shorter than the precision of an OS clock source (which usually have nanosecond
 * precision).
 * @param usecs The number of microseconds to delay.
 */
void default_delay(unsigned int usecs);

#endif
