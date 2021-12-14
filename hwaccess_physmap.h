/*
 * This file is part of the flashrom project.
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

#ifndef __HWACCESS_PHYSMAP_H__
#define __HWACCESS_PHYSMAP_H__

#include <stddef.h>
#include <stdint.h>

void *physmap(const char *descr, uintptr_t phys_addr, size_t len);
void *rphysmap(const char *descr, uintptr_t phys_addr, size_t len);
void *physmap_ro(const char *descr, uintptr_t phys_addr, size_t len);
void *physmap_ro_unaligned(const char *descr, uintptr_t phys_addr, size_t len);
void physunmap(void *virt_addr, size_t len);
void physunmap_unaligned(void *virt_addr, size_t len);

#endif /* __HWACCESS_PHYSMAP_H__ */