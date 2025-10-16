/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2025 Google LLC
 */

#ifndef CLIENT_TESTS_H
#define CLIENT_TESTS_H

#include <include/test.h>

/* external_client.c */
void flashrom_init_probe_erase_shutdown(void **state);

#endif /* CLIENT_TESTS_H */
