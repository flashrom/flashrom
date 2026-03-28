/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2009,2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2024 Google LLC
 */

#include "platform/udelay.h"

#include <libpayload.h>

void default_delay(unsigned int usecs)
{
	udelay(usecs);
}
