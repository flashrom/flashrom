/*
 * This file is part of the flashrom project.
 *
 * Copyright 2024 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <include/test.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "programmer.h"
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

/*
 * A short delay should delay for at least as long as requested,
 * and more than 10x as long would be worrisome.
 *
 * This test could fail spuriously on a heavily-loaded system, or if we need
 * to use gettimeofday() and a time change (such as DST) occurs during the
 * test.
 */
void udelay_test_short(void **state) {
    uint64_t start = now_us();
    default_delay(100);
    uint64_t elapsed = now_us() - start;

    assert_in_range(elapsed, 100, 1000);
}
