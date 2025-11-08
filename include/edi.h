/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2015 Paul Kocialkowski <contact@paulk.fr>
 */

#ifndef __EDI_H__
#define __EDI_H__ 1

#define EDI_READ			0x30
#define EDI_WRITE			0x40
#define EDI_DISABLE			0xf3

#define EDI_NOT_READY			0x5f
#define EDI_READY			0x50

#define EDI_READ_BUFFER_LENGTH_DEFAULT	3
#define EDI_READ_BUFFER_LENGTH_MAX	32

#endif
