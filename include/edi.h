/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2015 Paul Kocialkowski <contact@paulk.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
