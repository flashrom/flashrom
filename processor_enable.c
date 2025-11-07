/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2010 Carl-Daniel Hailfinger
 */

/*
 * Contains the processor specific flash enables and system settings.
 */

#include "flash.h"
#include "programmer.h"

#if defined (__MIPSEL__) && defined (__linux)
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int is_loongson(void)
{
	FILE *cpuinfo;
	cpuinfo = fopen("/proc/cpuinfo", "rb");
	if (!cpuinfo)
		return 0;
	while (!feof(cpuinfo)) {
		char line[512], *ptr;
		if (fgets(line, sizeof(line), cpuinfo) == NULL)
			break;
		ptr = line;
		while (*ptr && isspace((unsigned char)*ptr))
			ptr++;
		/* "cpu" part appears only with some Linux versions.  */
		if (strncmp(ptr, "cpu", strlen("cpu")) == 0)
			ptr += strlen("cpu");
		while (*ptr && isspace((unsigned char)*ptr))
			ptr++;
		if (strncmp(ptr, "model", strlen("model")) != 0)
			continue;
		ptr += strlen("model");
		while (*ptr && isspace((unsigned char)*ptr))
			ptr++;
		if (*ptr != ':')
			continue;
		ptr++;
		while (*ptr && isspace((unsigned char)*ptr))
			ptr++;
		(void)fclose(cpuinfo);
		return (strncmp(ptr, "ICT Loongson-2 V0.3", strlen("ICT Loongson-2 V0.3")) == 0) ||
		       (strncmp(ptr, "Godson2 V0.3  FPU V0.1", strlen("Godson2 V0.3  FPU V0.1")) == 0);
	}
	(void)fclose(cpuinfo);
	return 0;
}
#endif

int processor_flash_enable(void)
{
	/* Default to 1 to catch not implemented architectures. */
	int ret = 1;

	/* FIXME: detect loongson on FreeBSD and OpenBSD as well.  */
#if defined (__MIPSEL__) && defined (__linux)
	if (is_loongson()) {
		flashbase = 0x1fc00000;
		ret = 0;
	}
#elif defined(__i386__) || defined(__x86_64__)
	/* On x86, flash access is not processor specific except on
	 * AMD Elan SC520, AMD Geode and maybe other SoC-style CPUs.
	 * FIXME: Move enable_flash_cs5536 and get_flashbase_sc520 here.
	 */
	ret = 0;
#endif
	return ret;
}
