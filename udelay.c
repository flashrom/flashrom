/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
 * Copyright (C) 2024 Google LLC
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

#ifndef __LIBPAYLOAD__

#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <limits.h>
#include "flash.h"
#include "programmer.h"

#if HAVE_CLOCK_GETTIME == 1

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

#else

static void clock_usec_delay(unsigned int usecs) {
	struct timeval start, end;
	unsigned long elapsed = 0;

	gettimeofday(&start, NULL);

	while (elapsed < usecs) {
		gettimeofday(&end, NULL);
		elapsed = 1000000 * (end.tv_sec - start.tv_sec) +
			   (end.tv_usec - start.tv_usec);
		/* Protect against time going forward too much. */
		if ((end.tv_sec > start.tv_sec) &&
		    ((end.tv_sec - start.tv_sec) >= LONG_MAX / 1000000 - 1))
			elapsed = 0;
		/* Protect against time going backwards during leap seconds. */
		if ((end.tv_sec < start.tv_sec) || (elapsed > LONG_MAX))
			elapsed = 0;
	}
}

#endif /* HAVE_CLOCK_GETTIME == 1 */

/* Not very precise sleep. */
void internal_sleep(unsigned int usecs)
{
#if IS_WINDOWS
	Sleep((usecs + 999) / 1000);
#else
	nanosleep(&(struct timespec){usecs / 1000000, (usecs * 1000) % 1000000000UL}, NULL);
#endif
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

#else
#include <libpayload.h>

void default_delay(unsigned int usecs)
{
	udelay(usecs);
}
#endif
