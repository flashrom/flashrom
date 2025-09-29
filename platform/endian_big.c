/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2022 secunet Security Networks AG and Thomas Heijligen <thomas.heijligen@secunet.com>
 */

#include "platform.h"
#include "platform/swap.h"

/* convert cpu native endian to little endian */
___return_swapped(cpu_to_le, 8)
___return_swapped(cpu_to_le, 16)
___return_swapped(cpu_to_le, 32)
___return_swapped(cpu_to_le, 64)

/* convert cpu native endian to big endian */
___return_same(cpu_to_be, 8)
___return_same(cpu_to_be, 16)
___return_same(cpu_to_be, 32)
___return_same(cpu_to_be, 64)

/* convert little endian to cpu native endian */
___return_swapped(le_to_cpu, 8)
___return_swapped(le_to_cpu, 16)
___return_swapped(le_to_cpu, 32)
___return_swapped(le_to_cpu, 64)

/* convert big endian to cpu native endian */
___return_same(be_to_cpu, 8)
___return_same(be_to_cpu, 16)
___return_same(be_to_cpu, 32)
___return_same(be_to_cpu, 64)
