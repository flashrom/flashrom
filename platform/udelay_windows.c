/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2009,2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2024 Google LLC
 */

#include "platform/udelay.h"

#include <sys/time.h>
#include <windows.h>

static const unsigned min_sleep = CONFIG_DELAY_MINIMUM_SLEEP_US;

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
	Sleep((usecs + 999) / 1000);
}

/* Precise delay. */
void default_delay(unsigned int usecs)
{
	if (usecs < min_sleep) {
		clock_usec_delay(usecs);
	} else {
		internal_sleep(usecs);
	}
}
