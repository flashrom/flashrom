/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2024 Google LLC
 */
#include <include/test.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "platform/udelay.h"
#include "tests.h"

static uint64_t now_us(void) {
#if HAVE_CLOCK_GETTIME == 1
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_nsec / 1000) + (ts.tv_sec * 1000000);
#else
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return tv.tv_usec + (tv.tv_sec * 1000000);
#endif
}

static const int64_t min_sleep = CONFIG_DELAY_MINIMUM_SLEEP_US;

/*
 * A short delay should delay for at least as long as requested,
 * and more than 10x as long would be worrisome.
 *
 * This test could fail spuriously on a heavily-loaded system, or if we need
 * to use gettimeofday() and a time change (such as DST) occurs during the
 * test.
 */
void udelay_test_short(void **state) {
    /*
     * Delay for 100 microseconds, or short enough that we won't sleep.
     * It's not useful to test the sleep path because we assume the OS won't
     * sleep for less time than we ask.
     */
    int64_t delay_us = 100;
    if (delay_us >= min_sleep)
      delay_us = min_sleep - 1;
    /* No point in running this test if delay always sleeps. */
    if (delay_us <= 0)
      skip();

    uint64_t start = now_us();
    default_delay(delay_us);
    uint64_t elapsed = now_us() - start;

    assert_in_range(elapsed, delay_us, 10 * delay_us);
}
