/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2022 Nicholas Chin <nic.c3.14@gmail.com>
 */

#ifndef __CH347_SPI_H__
#define __CH347_SPI_H__ 1

#define CH347_CMD_SPI_SET_CFG	0xC0
#define CH347_CMD_SPI_CS_CTRL	0xC1
#define CH347_CMD_SPI_OUT_IN	0xC2
#define CH347_CMD_SPI_IN	0xC3
#define CH347_CMD_SPI_OUT	0xC4
#define CH347_CMD_SPI_GET_CFG	0xCA

#define CH347_CS_ASSERT		0x00
#define CH347_CS_DEASSERT	0x40
#define CH347_CS_CHANGE		0x80
#define CH347_CS_IGNORE		0x00

#define CH347_WRITE_EP		0x06
#define CH347_READ_EP		0x86

/* The USB descriptor says the max transfer size is 512 bytes, but the
 * vendor driver only seems to transfer a maximum of 510 bytes at once,
 * leaving 507 bytes for data as the command + length take up 3 bytes
 */
#define CH347_PACKET_SIZE 510
#define CH347_MAX_DATA_LEN (CH347_PACKET_SIZE - 3)

#endif
