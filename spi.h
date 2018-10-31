/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2007, 2008 Carl-Daniel Hailfinger
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

#ifndef __SPI_H__
#define __SPI_H__ 1

/*
 * Contains the generic SPI headers
 */

/* Error codes */
#define SPI_GENERIC_ERROR	-1
#define SPI_INVALID_OPCODE	-2
#define SPI_INVALID_ADDRESS	-3
#define SPI_INVALID_LENGTH	-4
#define SPI_FLASHROM_BUG	-5
#define SPI_PROGRAMMER_ERROR	-6

#endif		/* !__SPI_H__ */
