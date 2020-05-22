#include <include/test.h>

#include "flash.h"

#include <stdint.h>
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
