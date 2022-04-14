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

#endif /* __HWACCESS_PHYSMAP_H__ */