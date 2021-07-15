/*
 * This file is part of the flashrom project.
 *
 * Copyright 2021 Google LLC
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
#include <stdio.h>

#include "layout.h"
#include "libflashrom.h"

void included_regions_dont_overlap_test_success(void **state)
{
	(void) state; /* unused */

	printf("Creating layout...\n");
	struct flashrom_layout *layout;
	assert_int_equal(0, flashrom_layout_new(&layout));
	printf("done\n");

	printf("Adding and including first region...\n");
	assert_int_equal(0, flashrom_layout_add_region(layout, 0x00021000, 0x00031000, "first region"));
	assert_int_equal(0, flashrom_layout_include_region(layout, "first region"));
	printf("done\n");

	printf("Adding and including second (non-overlapping) region...\n");
	assert_int_equal(0, flashrom_layout_add_region(layout, 0x00031001, 0x0023efc0, "second region"));
	assert_int_equal(0, flashrom_layout_include_region(layout, "second region"));
	printf("done\n");

	printf("Asserting included regions do not overlap...\n");
	assert_int_equal(0, included_regions_overlap(layout));
	printf("done\n");

	printf("Releasing layout...\n");
	flashrom_layout_release(layout);
	printf("done\n");
}

void included_regions_overlap_test_success(void **state)
{
	(void) state; /* unused */

	printf("Creating layout...\n");
	struct flashrom_layout *layout;
	assert_int_equal(0, flashrom_layout_new(&layout));
	printf("done\n");

	printf("Adding and including first region...\n");
	assert_int_equal(0, flashrom_layout_add_region(layout, 0x00021000, 0x00031000, "first region"));
	assert_int_equal(0, flashrom_layout_include_region(layout, "first region"));
	printf("done\n");

	printf("Adding and including second (overlapping) region...\n");
	assert_int_equal(0, flashrom_layout_add_region(layout, 0x00027100, 0x0023efc0, "second region"));
	assert_int_equal(0, flashrom_layout_include_region(layout, "second region"));
	printf("done\n");

	printf("Asserting included regions overlap...\n");
	assert_int_equal(1, included_regions_overlap(layout));
	printf("done\n");

	printf("Releasing layout...\n");
	flashrom_layout_release(layout);
	printf("done\n");
}

void region_not_included_overlap_test_success(void **state)
{
	(void) state; /* unused */

	printf("Creating layout...\n");
	struct flashrom_layout *layout;
	assert_int_equal(0, flashrom_layout_new(&layout));
	printf("done\n");

	printf("Adding and including first region...\n");
	assert_int_equal(0, flashrom_layout_add_region(layout, 0x00021000, 0x00031000, "first region"));
	assert_int_equal(0, flashrom_layout_include_region(layout, "first region"));
	printf("done\n");

	printf("Adding second (overlapping) region, not included...\n");
	assert_int_equal(0, flashrom_layout_add_region(layout, 0x00027100, 0x0023efc0, "second region"));
	printf("done\n");

	printf("Asserting included regions do not overlap...\n");
	assert_int_equal(0, included_regions_overlap(layout));
	printf("done\n");

	printf("Releasing layout...\n");
	flashrom_layout_release(layout);
	printf("done\n");
}
