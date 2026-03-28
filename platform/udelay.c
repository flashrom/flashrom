/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2009,2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2024 Google LLC
 */

#include "platform/udelay.h"

#include <stddef.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

static void clock_usec_delay(int usecs)
{
	static clockid_t clock_id =
#ifdef _POSIX_MONOTONIC_CLOCK
		CLOCK_MONOTONIC;
#else
		CLOCK_REALTIME;
#endif

	struct timespec now;
	if (clock_gettime(clock_id, &now)) {
		/* Fall back to realtime clock if monotonic doesn't work */
		if (clock_id != CLOCK_REALTIME && errno == EINVAL) {
			clock_id = CLOCK_REALTIME;
			clock_gettime(clock_id, &now);
		}
	}

	const long end_nsec = now.tv_nsec + usecs * 1000L;
	const struct timespec end = {
		end_nsec / (1000 * 1000 * 1000) + now.tv_sec,
		end_nsec % (1000 * 1000 * 1000)
	};
	do {
		clock_gettime(clock_id, &now);
	} while (now.tv_sec < end.tv_sec || (now.tv_sec == end.tv_sec && now.tv_nsec < end.tv_nsec));
}

/**
 * @brief Delay for at least the specified number of microseconds, with low precision.
 * @details
 * This function will always us an OS-provided sleep()-like function which will usually
 * save power but may delay for significantly longer than intended, especially if
 * the delay is very short.
 * @param usecs The number of microseconds to delay.
 */
static void internal_sleep(unsigned int usecs)
{
	nanosleep(&(struct timespec){usecs / 1000000, (usecs * 1000) % 1000000000UL}, NULL);
}

static const unsigned min_sleep = CONFIG_DELAY_MINIMUM_SLEEP_US;

/* Precise delay. */
void default_delay(unsigned int usecs)
{
	if (usecs < min_sleep) {
		clock_usec_delay(usecs);
	} else {
		internal_sleep(usecs);
	}
}
