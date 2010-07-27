/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Carl-Daniel Hailfinger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 * Contains the processor specific flash enables and system settings.
 */

#include "flash.h"
#include "programmer.h"

#if defined(__i386__) || defined(__x86_64__)

int processor_flash_enable(void)
{
	/* On x86, flash access is not processor specific except on
	 * AMD Elan SC520, AMD Geode and maybe other SoC-style CPUs.
	 * FIXME: Move enable_flash_cs5536 and get_flashbase_sc520 here.
	 */
	return 0;
}

#else

int processor_flash_enable(void)
{
	/* Not implemented yet. Oh well. */
	return 1;
}

#endif
