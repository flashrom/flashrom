/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2020 Google LLC
 */

#include <include/test.h>

#include "tests.h"
#include "helpers.h"
#include "platform/string.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void address_to_bits_test_success(void **state)
{
	(void) state; /* unused */
	assert_int_equal(16, address_to_bits(0xAA55));
}

void bitcount_test_success(void **state)
{
	(void) state; /* unused */
	assert_int_equal(4, bitcount(0xAA));
}

void minmax_test_success(void **state)
{
	(void) state; /* unused */
	assert_int_equal(0x55, min(0xAA, 0x55));
	assert_int_equal(0xAA, max(0xAA, 0x55));
}

void strcat_realloc_test_success(void **state)
{
	(void) state; /* unused */
	const char src0[] = "hello";
	const char src1[] = " world";
	char *dest = calloc(1, 1);
	assert_non_null(dest);
	dest = strcat_realloc(dest, src0);
	dest = strcat_realloc(dest, src1);
	assert_string_equal("hello world", dest);
	free(dest);
}

void tolower_string_test_success(void **state)
{
	(void) state; /* unused */
	char str[] = "HELLO AGAIN";
	assert_string_equal("HELLO AGAIN", str);
	tolower_string(str);
	assert_string_equal("hello again", str);
}

void reverse_byte_test_success(void **state)
{
	(void) state; /* unused */
	assert_int_equal(0x5A, reverse_byte(0x5A));
	assert_int_equal(0x0F, reverse_byte(0xF0));
}

void reverse_bytes_test_success(void **state)
{
	(void) state; /* unused */
	uint8_t src[] = { 0xAA, 0x55 };
	uint8_t dst[2];
	reverse_bytes(dst, src, 2);
	assert_int_equal(src[0], dst[1]);
	assert_int_equal(src[1], dst[0]);
}

void parse_voltage_success(void **state)
{
	(void) state; /* unused */

	const char *volt[] = {"2.3", "2,3", "3.5V", "3,5V", "1950mV", "2700mv", "1950milliv"};
	const int result[] = {2300, 2300, 3500, 3500, 1950, 2700, 1950};
	const int count = sizeof(volt) / sizeof((volt)[0]);

	for (int i = 0; i < count; i++) {
		char *voltage = strdup(volt[i]);
		assert_int_equal(result[i], parse_voltage(voltage));
		free(voltage);
	}
}

void parse_voltage_invalid(void **state)
{
	(void) state; /* unused */

	const char *invalid_volt[] = {
		"2300millimeter",
		"___",
		"village",
		"2.3village",
		"2300village",
		"milliv1950",
	};
	const int count = sizeof(invalid_volt) / sizeof((invalid_volt)[0]);

	for (int i = 0; i < count; i++) {
		char *voltage = strdup(invalid_volt[i]);
		assert_int_equal(-1, parse_voltage(voltage));
		free(voltage);
	}
}

void parse_usbpath_success(void **state)
{
	const uint8_t max_ports = 7;
	uint8_t usbpath[max_ports];
	int num_ports = parse_usbpath("0-1.2.3", usbpath, max_ports);
	assert_int_equal(4, num_ports);
	for (uint8_t i = 0; i < num_ports; i++) {
		assert_true(usbpath[i] == i);
	}

	num_ports = parse_usbpath("11-22.33.44.55.66.77", usbpath, max_ports);
	assert_int_equal(7, num_ports);
	for (uint8_t i = 0; i < num_ports; i++) {
		assert_true(usbpath[i] == 11*(i+1));
	}

	num_ports = parse_usbpath("111-222", usbpath, max_ports);
	assert_int_equal(2, num_ports);
	for (uint8_t i = 0; i < num_ports; i++) {
		assert_true(usbpath[i] == 111*(i+1));
	}
}

void parse_usbpath_invalid(void **state)
{
	uint8_t max_ports = 7;
	uint8_t usbpath[max_ports];

	const char *invalid_usbpath[] = {
		"one-2.3.4",
		"1--2.3.4",
		"1.2.3.4",
		"1-2..3.4",
		"1234-2.3.4",
		"1-two.3.4",
		"0-0.3.4",
		"1-256.3.4",
		"1-2-3-4",
		"1-2.",
		"1-2.3.4.5.6.7.8",
		"1-",
	};

	const int count = sizeof(invalid_usbpath) / sizeof((invalid_usbpath)[0]);

	for (int i = 0; i < count; i++) {
		char *badpath = strdup(invalid_usbpath[i]);
		assert_int_equal(-1, parse_usbpath(badpath, usbpath, max_ports));
		free(badpath);
	}

	// test strtoul overflow for bus, port
	char overflow_usbpath[32];
	snprintf(overflow_usbpath, sizeof(overflow_usbpath), "%lu0-2.3.4", ULONG_MAX);
	assert_int_equal(-1, parse_usbpath(overflow_usbpath, usbpath, max_ports));
	snprintf(overflow_usbpath, sizeof(overflow_usbpath), "1-%lu0.3.4", ULONG_MAX);
	assert_int_equal(-1, parse_usbpath(overflow_usbpath, usbpath, max_ports));
}