/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2021, TUXEDO Computers GmbH
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

#if defined(__i386__) || defined(__x86_64__)

#ifndef ITE_EC_H
#define ITE_EC_H

#include <stdint.h>
#include <stddef.h>

int ite_ec_verify_file_project(uint8_t *const newcontents,
				uint8_t *const curcontents,
				const size_t flash_size);

#endif /* ITE_EC_H */
#endif
