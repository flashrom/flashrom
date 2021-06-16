/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010  Matthias Wenzel <bios at mazzoo dot de>
 * Copyright (C) 2011 Stefan Tauner
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

/*
 * dump information and binaries from BIOS images that are in descriptor mode
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "ich_descriptors.h"
/* Some DJGPP builds define __unix__ although they don't support mmap().
 * Cygwin defines __unix__ and supports mmap(), but it does not work well.
 */
#if !defined(__MSDOS__) && !IS_WINDOWS && (defined(unix) || defined(__unix__) || defined(__unix)) || (defined(__MACH__) && defined(__APPLE__))
#define HAVE_MMAP 1
#include <sys/mman.h>
#endif

static const char *const region_names[] = {
	"Descriptor", "BIOS", "ME", "GbE", "Platform",
	"Region5", "BIOS2", "Region7", "EC/BMC", "Region9",
	"IE", "10GbE", "Region12", "Region13", "Region14",
	"Region15"
};

static void dump_file(const char *prefix, const uint32_t *dump, unsigned int len,
		      const struct ich_desc_region *const reg, unsigned int i)
{
	int ret;
	char *fn;
	const char *reg_name;
	uint32_t file_len;
	uint32_t base = ICH_FREG_BASE(reg->FLREGs[i]);
	uint32_t limit = ICH_FREG_LIMIT(reg->FLREGs[i]);

	reg_name = region_names[i];
	if (base > limit) {
		printf("The %s region is unused and thus not dumped.\n", reg_name);
		return;
	}

	file_len = limit + 1 - base;
	if (base + file_len > len) {
		printf("The %s region is spanning 0x%08x-0x%08x, but it is "
		       "not (fully) included in the image (0-0x%08x), thus not "
		       "dumped.\n", reg_name, base, limit, len - 1);
		return;
	}

	fn = malloc(strlen(prefix) + strlen(reg_name) + strlen(".bin") + 2);
	if (!fn) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}
	snprintf(fn, strlen(prefix) + strlen(reg_name) + strlen(".bin") + 2,
		 "%s.%s.bin", prefix, reg_name);
	printf("Dumping %u bytes of the %s region from 0x%08x-0x%08x to %s... ",
	       file_len, region_names[i], base, limit, fn);
	int fh = open(fn, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (fh < 0) {
		fprintf(stderr,
			"ERROR: couldn't open(%s): %s\n", fn, strerror(errno));
		free(fn);
		exit(1);
	}
	free(fn);

	ret = write(fh, &dump[base >> 2], file_len);
	if (ret != file_len) {
		fprintf(stderr, "FAILED.\n");
		exit(1);
	}

	printf("done.\n");
	close(fh);
}

int min(int a, int b)
{
	return (a < b) ? a : b;
}

static void dump_files(const char *name, const uint32_t *buf, unsigned int len,
		       const enum ich_chipset cs, const struct ich_descriptors *const desc)
{
	ssize_t i;
	const ssize_t nr = min(ich_number_of_regions(cs, &desc->content), ARRAY_SIZE(region_names));
	printf("=== Dumping region files ===\n");
	for (i = 0; i < nr; i++)
		dump_file(name, buf, len, &desc->region, i);
	printf("\n");
}

static void usage(char *argv[], const char *error)
{
	if (error != NULL) {
		fprintf(stderr, "%s\n", error);
	}
	printf("usage: '%s -f <image file name> [-c <chipset name>] [-d]'\n\n"
"where <image file name> points to an image of the contents of the SPI flash.\n"
"In case the image is really in descriptor mode %s\n"
"will pretty print some of the contained information.\n"
"To also print the data stored in the descriptor straps you have to indicate\n"
"the chipset series with the '-c' parameter and one of the possible arguments:\n"
"\t- \"ich8\",\n"
"\t- \"ich9\",\n"
"\t- \"ich10\",\n"
"\t- \"silvermont\" for chipsets from Intel's Silvermont architecture (e.g. Bay Trail),\n"
"\t- \"apollo\" for Intel's Apollo Lake SoC.\n"
"\t- \"gemini\" for Intel's Gemini Lake SoC.\n"
"\t- \"5\" or \"ibex\" for Intel's 5 series chipsets,\n"
"\t- \"6\" or \"cougar\" for Intel's 6 series chipsets,\n"
"\t- \"7\" or \"panther\" for Intel's 7 series chipsets.\n"
"\t- \"8\" or \"lynx\" for Intel's 8 series chipsets.\n"
"\t- \"9\" or \"wildcat\" for Intel's 9 series chipsets.\n"
"\t- \"100\" or \"sunrise\" for Intel's 100 series chipsets.\n"
"\t- \"300\" or \"cannon\" for Intel's 300 series chipsets.\n"
"\t- \"400\" or \"comet\" for Intel's 400 series chipsets.\n"
"\t- \"500\" or \"tiger\" for Intel's 500 series chipsets.\n"
"If '-d' is specified some regions such as the BIOS image as seen by the CPU or\n"
"the GbE blob that is required to initialize the GbE are also dumped to files.\n",
	argv[0], argv[0]);
	exit(1);
}

int main(int argc, char *argv[])
{
	int fd;			/* file descriptor to flash file */
	int len;		/* file/buffer size in bytes */
	uint32_t *buf;		/* mmap'd file */
	uint8_t *pMAC;
	int opt, ret;

	int dump = 0;
	const char *fn = NULL;
	const char *csn = NULL;
	enum ich_chipset cs = CHIPSET_ICH_UNKNOWN;
	struct ich_descriptors desc = { 0 };

	while ((opt = getopt(argc, argv, "df:c:")) != -1) {
		switch (opt) {
		case 'd':
			dump = 1;
			break;
		case 'f':
			fn = optarg;
			break;
		case 'c':
			csn = optarg;
			break;
		default: /* '?' */
			usage(argv, NULL);
		}
	}
	if (fn == NULL)
		usage(argv, "Need the file name of a descriptor image to read from.");

	fd = open(fn, O_RDONLY);
	if (fd < 0)
		usage(argv, "No such file");
	len = lseek(fd, 0, SEEK_END);
	if (len < 0)
		usage(argv, "Seeking to the end of the file failed");

#ifdef HAVE_MMAP
	buf = (uint32_t *)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (buf == (void *) -1)
#endif
	{
		/* fallback for stupid OSes like cygwin */
		buf = malloc(len);
		if (!buf)
			usage(argv, "Could not allocate memory");
		lseek(fd, 0, SEEK_SET);
		if (len != read(fd, buf, len))
			usage(argv, "Reading the descriptor image file failed");
	}
	printf("The flash image has a size of %d [0x%x] bytes.\n", len, len);
	close(fd);

	if (csn != NULL) {
		if (strcmp(csn, "ich8") == 0)
			cs = CHIPSET_ICH8;
		else if (strcmp(csn, "ich9") == 0)
			cs = CHIPSET_ICH9;
		else if (strcmp(csn, "ich10") == 0)
			cs = CHIPSET_ICH10;
		else if ((strcmp(csn, "5") == 0) ||
			 (strcmp(csn, "ibex") == 0))
			cs = CHIPSET_5_SERIES_IBEX_PEAK;
		else if ((strcmp(csn, "6") == 0) ||
			 (strcmp(csn, "cougar") == 0))
			cs = CHIPSET_6_SERIES_COUGAR_POINT;
		else if ((strcmp(csn, "7") == 0) ||
			 (strcmp(csn, "panther") == 0))
			cs = CHIPSET_7_SERIES_PANTHER_POINT;
		else if ((strcmp(csn, "8") == 0) ||
			 (strcmp(csn, "lynx") == 0))
			cs = CHIPSET_8_SERIES_LYNX_POINT;
		else if ((strcmp(csn, "silvermont") == 0))
			cs = CHIPSET_BAYTRAIL;
		else if ((strcmp(csn, "9") == 0) ||
			 (strcmp(csn, "wildcat") == 0))
			cs = CHIPSET_9_SERIES_WILDCAT_POINT;
		else if ((strcmp(csn, "100") == 0) ||
			 (strcmp(csn, "sunrise") == 0))
			cs = CHIPSET_100_SERIES_SUNRISE_POINT;
		else if ((strcmp(csn, "300") == 0) ||
			 (strcmp(csn, "cannon") == 0))
			cs = CHIPSET_300_SERIES_CANNON_POINT;
		else if ((strcmp(csn, "400") == 0) ||
			 (strcmp(csn, "comet") == 0))
			cs = CHIPSET_400_SERIES_COMET_POINT;
		else if ((strcmp(csn, "500") == 0) ||
			 (strcmp(csn, "tiger") == 0))
			cs = CHIPSET_500_SERIES_TIGER_POINT;
		else if (strcmp(csn, "apollo") == 0)
			cs = CHIPSET_APOLLO_LAKE;
		else if (strcmp(csn, "gemini") == 0)
			cs = CHIPSET_GEMINI_LAKE;
	}

	ret = read_ich_descriptors_from_dump(buf, len, &cs, &desc);
	switch (ret) {
	case ICH_RET_OK:
		break;
	case ICH_RET_ERR:
		printf("Image not in descriptor mode.\n");
		exit(1);
	case ICH_RET_OOB:
		printf("Tried to access a location out of bounds of the image. - Corrupt image?\n");
		exit(1);
	default:
		printf("Unhandled return value at %s:%u, please report this.\n",
		       __FILE__, __LINE__);
		exit(1);
	}

	prettyprint_ich_descriptors(cs, &desc);

	pMAC = (uint8_t *) &buf[ICH_FREG_BASE(desc.region.FLREGs[3]) >> 2];
	if (len >= ICH_FREG_BASE(desc.region.FLREGs[3]) + 6 && pMAC[0] != 0xff)
		printf("The MAC address might be at offset 0x%x: "
		       "%02x:%02x:%02x:%02x:%02x:%02x\n",
		       ICH_FREG_BASE(desc.region.FLREGs[3]),
		       pMAC[0], pMAC[1], pMAC[2], pMAC[3], pMAC[4], pMAC[5]);

	if (dump == 1)
		dump_files(fn, buf, len, cs, &desc);

	return 0;
}
