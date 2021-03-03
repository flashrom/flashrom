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

/*
 * This header is included in all files when they are built for unit test
 * environment, for all the files to be covered by dynamic memory allocation
 * checks (checks for memory leaks, buffer overflows and underflows).
 *
 * See flashrom_test_dep in meson.build for more details.
 *
 * https://api.cmocka.org/group__cmocka__alloc.html
 */

extern void* _test_malloc(const size_t size, const char* file, const int line);
extern void* _test_realloc(void *ptr, const size_t size, const char* file, const int line);
extern void* _test_calloc(const size_t number_of_elements, const size_t size,
                        const char* file, const int line);
extern void _test_free(void* const ptr, const char* file, const int line);

#ifdef malloc
#undef malloc
#endif
#ifdef calloc
#undef calloc
#endif
#ifdef realloc
#undef realloc
#endif
#ifdef free
#undef free
#endif

#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#define realloc(ptr, size) _test_realloc(ptr, size, __FILE__, __LINE__)
#define calloc(num, size) _test_calloc(num, size, __FILE__, __LINE__)
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)
