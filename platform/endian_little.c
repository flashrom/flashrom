/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2022 secunet Security Networks AG
 * (written by Thomas Heijligen <thomas.heijligen@secunet.com)
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

#include "platform.h"
#include "platform/swap.h"

/* convert cpu native endian to little endian */
___return_same(cpu_to_le, 8)
___return_same(cpu_to_le, 16)
___return_same(cpu_to_le, 32)
___return_same(cpu_to_le, 64)

/* convert cpu native endian to big endian */
___return_swapped(cpu_to_be, 8)
___return_swapped(cpu_to_be, 16)
___return_swapped(cpu_to_be, 32)
___return_swapped(cpu_to_be, 64)

/* convert little endian to cpu native endian */
___return_same(le_to_cpu, 8)
___return_same(le_to_cpu, 16)
___return_same(le_to_cpu, 32)
___return_same(le_to_cpu, 64)

/* convert big endian to cpu native endian */
___return_swapped(be_to_cpu, 8)
___return_swapped(be_to_cpu, 16)
___return_swapped(be_to_cpu, 32)
___return_swapped(be_to_cpu, 64)
