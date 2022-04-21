/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Carl-Daniel Hailfinger
 * Copyright (C) 2022 secunet Security Networks AG
 * (written by Thomas Heijligen <thomas.heijligen@secunet.com)
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
 * Header file for platform abstraction.
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__ 1

#include <stddef.h>
#include <stdint.h>

/* convert cpu native endian to little endian */
uint8_t  cpu_to_le8 (uint8_t  value);
uint16_t cpu_to_le16(uint16_t value);
uint32_t cpu_to_le32(uint32_t value);
uint64_t cpu_to_le64(uint64_t value);

/* convert cpu native endian to big endian */
uint8_t  cpu_to_be8 (uint8_t  value);
uint16_t cpu_to_be16(uint16_t value);
uint32_t cpu_to_be32(uint32_t value);
uint64_t cpu_to_be64(uint64_t value);

/* convert little endian to cpu native endian */
uint8_t  le_to_cpu8 (uint8_t  value);
uint16_t le_to_cpu16(uint16_t value);
uint32_t le_to_cpu32(uint32_t value);
uint64_t le_to_cpu64(uint64_t value);

/* convert big endian to cpu native endian */
uint8_t  be_to_cpu8 (uint8_t  value);
uint16_t be_to_cpu16(uint16_t value);
uint32_t be_to_cpu32(uint32_t value);
uint64_t be_to_cpu64(uint64_t value);

/* read value from base at offset in little endian */
uint8_t  read_le8 (const void *base, size_t offset);
uint16_t read_le16(const void *base, size_t offset);
uint32_t read_le32(const void *base, size_t offset);
uint64_t read_le64(const void *base, size_t offset);

/* read value from base at offset in big endian */
uint8_t  read_be8 (const void *base, size_t offset);
uint16_t read_be16(const void *base, size_t offset);
uint32_t read_be32(const void *base, size_t offset);
uint64_t read_be64(const void *base, size_t offset);

#endif /* !__PLATFORM_H__ */
