/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2022 secunet Security Networks AG (Written by Thomas Heijligen <thomas.heijligen@secunet.com)
 */

/*
 * This file contains prototypes for x86 I/O Port access.
 */

#ifndef __HWACCESS_X86_IO_H__
#define __HWACCESS_X86_IO_H__ 1

#include <stdint.h>
/**
 */
int rget_io_perms(void);

void OUTB(uint8_t value, uint16_t port);
void OUTW(uint16_t value, uint16_t port);
void OUTL(uint32_t value, uint16_t port);
uint8_t INB(uint16_t port);
uint16_t INW(uint16_t port);
uint32_t INL(uint16_t port);

#endif /* __HWACCESS_X86_IO_H__ */
