/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Carl-Daniel Hailfinger
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
 * Header file for hardware access and OS abstraction. Included from flash.h.
 */

#ifndef __HWACCESS_H__
#define __HWACCESS_H__ 1

void mmio_writeb(uint8_t val, void *addr);
void mmio_writew(uint16_t val, void *addr);
void mmio_writel(uint32_t val, void *addr);
uint8_t mmio_readb(const void *addr);
uint16_t mmio_readw(const void *addr);
uint32_t mmio_readl(const void *addr);
void mmio_readn(const void *addr, uint8_t *buf, size_t len);
void mmio_le_writeb(uint8_t val, void *addr);
void mmio_le_writew(uint16_t val, void *addr);
void mmio_le_writel(uint32_t val, void *addr);
uint8_t mmio_le_readb(const void *addr);
uint16_t mmio_le_readw(const void *addr);
uint32_t mmio_le_readl(const void *addr);
#define pci_mmio_writeb mmio_le_writeb
#define pci_mmio_writew mmio_le_writew
#define pci_mmio_writel mmio_le_writel
#define pci_mmio_readb mmio_le_readb
#define pci_mmio_readw mmio_le_readw
#define pci_mmio_readl mmio_le_readl
void rmmio_writeb(uint8_t val, void *addr);
void rmmio_writew(uint16_t val, void *addr);
void rmmio_writel(uint32_t val, void *addr);
void rmmio_le_writeb(uint8_t val, void *addr);
void rmmio_le_writew(uint16_t val, void *addr);
void rmmio_le_writel(uint32_t val, void *addr);
#define pci_rmmio_writeb rmmio_le_writeb
#define pci_rmmio_writew rmmio_le_writew
#define pci_rmmio_writel rmmio_le_writel
void rmmio_valb(void *addr);
void rmmio_valw(void *addr);
void rmmio_vall(void *addr);

#define ___constant_swab8(x) ((uint8_t) (				\
	(((uint8_t)(x) & (uint8_t)0xffU))))

#define ___constant_swab16(x) ((uint16_t) (				\
	(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |			\
	(((uint16_t)(x) & (uint16_t)0xff00U) >> 8)))

#define ___constant_swab32(x) ((uint32_t) (				\
	(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |		\
	(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |		\
	(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |		\
	(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

#define ___constant_swab64(x) ((uint64_t) (				\
	(((uint64_t)(x) & (uint64_t)0x00000000000000ffULL) << 56) |	\
	(((uint64_t)(x) & (uint64_t)0x000000000000ff00ULL) << 40) |	\
	(((uint64_t)(x) & (uint64_t)0x0000000000ff0000ULL) << 24) |	\
	(((uint64_t)(x) & (uint64_t)0x00000000ff000000ULL) <<  8) |	\
	(((uint64_t)(x) & (uint64_t)0x000000ff00000000ULL) >>  8) |	\
	(((uint64_t)(x) & (uint64_t)0x0000ff0000000000ULL) >> 24) |	\
	(((uint64_t)(x) & (uint64_t)0x00ff000000000000ULL) >> 40) |	\
	(((uint64_t)(x) & (uint64_t)0xff00000000000000ULL) >> 56)))

#if defined (__FLASHROM_BIG_ENDIAN__)

#define cpu_to_le(bits)							\
static inline uint##bits##_t cpu_to_le##bits(uint##bits##_t val)	\
{									\
	return ___constant_swab##bits(val);				\
}

cpu_to_le(8)
cpu_to_le(16)
cpu_to_le(32)
cpu_to_le(64)

#define cpu_to_be8
#define cpu_to_be16
#define cpu_to_be32
#define cpu_to_be64

#elif defined (__FLASHROM_LITTLE_ENDIAN__)

#define cpu_to_be(bits)							\
static inline uint##bits##_t cpu_to_be##bits(uint##bits##_t val)	\
{									\
	return ___constant_swab##bits(val);				\
}

cpu_to_be(8)
cpu_to_be(16)
cpu_to_be(32)
cpu_to_be(64)

#define cpu_to_le8
#define cpu_to_le16
#define cpu_to_le32
#define cpu_to_le64

#endif /* __FLASHROM_BIG_ENDIAN__ / __FLASHROM_LITTLE_ENDIAN__ */

#define be_to_cpu8 cpu_to_be8
#define be_to_cpu16 cpu_to_be16
#define be_to_cpu32 cpu_to_be32
#define be_to_cpu64 cpu_to_be64
#define le_to_cpu8 cpu_to_le8
#define le_to_cpu16 cpu_to_le16
#define le_to_cpu32 cpu_to_le32
#define le_to_cpu64 cpu_to_le64

#endif /* !__HWACCESS_H__ */
