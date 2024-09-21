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

#include <stdlib.h>

#include "lifecycle.h"

#if CONFIG_LINUX_MTD == 1
struct linux_mtd_io_state {
	char *fopen_path;
};

static FILE *linux_mtd_fopen(void *state, const char *pathname, const char *mode)
{
	struct linux_mtd_io_state *io_state = state;

	io_state->fopen_path = strdup(pathname);

	return not_null();
}

static size_t linux_mtd_fread(void *state, void *buf, size_t size, size_t len, FILE *fp)
{
	struct linux_mtd_fread_mock_entry {
		const char *path;
		const char *data;
	};
	const struct linux_mtd_fread_mock_entry fread_mock_map[] = {
		{ "/sys/class/mtd/mtd0//type",            "nor"    },
		{ "/sys/class/mtd/mtd0//name",            "Device" },
		{ "/sys/class/mtd/mtd0//flags",           ""       },
		{ "/sys/class/mtd/mtd0//size",            "1024"   },
		{ "/sys/class/mtd/mtd0//erasesize",       "512"    },
		{ "/sys/class/mtd/mtd0//numeraseregions", "0"      },
	};

	struct linux_mtd_io_state *io_state = state;
	unsigned int i;

	if (!io_state->fopen_path)
		return 0;

	for (i = 0; i < ARRAY_SIZE(fread_mock_map); i++) {
		const struct linux_mtd_fread_mock_entry *entry = &fread_mock_map[i];

		if (!strcmp(io_state->fopen_path, entry->path)) {
			size_t data_len = min(size * len, strlen(entry->data));
			memcpy(buf, entry->data, data_len);
			return data_len;
		}
	}

	return 0;
}

static int linux_mtd_fclose(void *state, FILE *fp)
{
	struct linux_mtd_io_state *io_state = state;

	free(io_state->fopen_path);

	return 0;
}

void linux_mtd_probe_lifecycle_test_success(void **state)
{
	struct linux_mtd_io_state linux_mtd_io_state = { NULL };
	struct io_mock_fallback_open_state linux_mtd_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock linux_mtd_io = {
		.state	= &linux_mtd_io_state,
		.iom_fopen	= linux_mtd_fopen,
		.iom_fread	= linux_mtd_fread,
		.iom_fclose = linux_mtd_fclose,
		.fallback_open_state = &linux_mtd_fallback_open_state,
	};

	run_probe_lifecycle(state, &linux_mtd_io, &programmer_linux_mtd, "", "Opaque flash chip");
}
#else
	SKIP_TEST(linux_mtd_probe_lifecycle_test_success)
#endif /* CONFIG_LINUX_MTD */
