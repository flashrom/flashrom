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
 * @brief Delay for at least the specified number of microseconds, with low precision.
 * @details
 * This function will always us an OS-provided sleep()-like function which will usually
 * save power but may delay for significantly longer than intended, especially if
 * the delay is very short. Callers should prefer to use default_delay().
 * @param usecs The number of microseconds to delay.
 */
void internal_sleep(unsigned int usecs);

/**
 * @brief Delay for at least the specified number of microseconds.
 * @details
 * For very short delays this function polls the wall time provided by the OS,
 * otherwise it will use an OS-provided sleep()-like function. The delay will never
 * be shorter than the precision of an OS clock source (which usually have nanosecond
 * precision), but will usually be closer to the requested time than an equivalent
 * delay using internal_sleep().
 * @param usecs The number of microseconds to delay.
 */
void default_delay(unsigned int usecs);

#endif
