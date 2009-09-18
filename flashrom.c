/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2004 Tyan Corp <yhlu@tyan.com>
 * Copyright (C) 2005-2008 coresystems GmbH
 * Copyright (C) 2008,2009 Carl-Daniel Hailfinger
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "flash.h"
#include "flashchips.h"

const char *flashrom_version = FLASHROM_VERSION;
char *chip_to_probe = NULL;
int verbose = 0;
enum programmer programmer = PROGRAMMER_INTERNAL;
char *programmer_param = NULL;

const struct programmer_entry programmer_table[] = {
	{
		.name			= "internal",
		.init			= internal_init,
		.shutdown		= internal_shutdown,
		.map_flash_region	= physmap,
		.unmap_flash_region	= physunmap,
		.chip_readb		= internal_chip_readb,
		.chip_readw		= internal_chip_readw,
		.chip_readl		= internal_chip_readl,
		.chip_readn		= internal_chip_readn,
		.chip_writeb		= internal_chip_writeb,
		.chip_writew		= internal_chip_writew,
		.chip_writel		= internal_chip_writel,
		.chip_writen		= fallback_chip_writen,
		.delay			= internal_delay,
	},

#if DUMMY_SUPPORT == 1
	{
		.name			= "dummy",
		.init			= dummy_init,
		.shutdown		= dummy_shutdown,
		.map_flash_region	= dummy_map,
		.unmap_flash_region	= dummy_unmap,
		.chip_readb		= dummy_chip_readb,
		.chip_readw		= dummy_chip_readw,
		.chip_readl		= dummy_chip_readl,
		.chip_readn		= dummy_chip_readn,
		.chip_writeb		= dummy_chip_writeb,
		.chip_writew		= dummy_chip_writew,
		.chip_writel		= dummy_chip_writel,
		.chip_writen		= dummy_chip_writen,
		.delay			= internal_delay,
	},
#endif

#if NIC3COM_SUPPORT == 1
	{
		.name			= "nic3com",
		.init			= nic3com_init,
		.shutdown		= nic3com_shutdown,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.chip_readb		= nic3com_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= nic3com_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
		.delay			= internal_delay,
	},
#endif

#if DRKAISER_SUPPORT == 1
	{
		.name			= "drkaiser",
		.init			= drkaiser_init,
		.shutdown		= drkaiser_shutdown,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.chip_readb		= drkaiser_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= drkaiser_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
		.delay			= internal_delay,
	},
#endif

#if SATASII_SUPPORT == 1
	{
		.name			= "satasii",
		.init			= satasii_init,
		.shutdown		= satasii_shutdown,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.chip_readb		= satasii_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= satasii_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
		.delay			= internal_delay,
	},
#endif

	{
		.name			= "it87spi",
		.init			= it87spi_init,
		.shutdown		= noop_shutdown,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.chip_readb		= noop_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= noop_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
		.delay			= internal_delay,
	},

#if FT2232_SPI_SUPPORT == 1
	{
		.name			= "ft2232spi",
		.init			= ft2232_spi_init,
		.shutdown		= noop_shutdown, /* Missing shutdown */
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.chip_readb		= noop_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= noop_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
		.delay			= internal_delay,
	},
#endif

#if SERPROG_SUPPORT == 1
	{
		.name			= "serprog",
		.init			= serprog_init,
		.shutdown		= serprog_shutdown,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.chip_readb		= serprog_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= serprog_chip_readn,
		.chip_writeb		= serprog_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
		.delay			= serprog_delay,
	},
#endif

	{}, /* This entry corresponds to PROGRAMMER_INVALID. */
};

int programmer_init(void)
{
	return programmer_table[programmer].init();
}

int programmer_shutdown(void)
{
	return programmer_table[programmer].shutdown();
}

void *programmer_map_flash_region(const char *descr, unsigned long phys_addr,
				  size_t len)
{
	return programmer_table[programmer].map_flash_region(descr,
							     phys_addr, len);
}

void programmer_unmap_flash_region(void *virt_addr, size_t len)
{
	programmer_table[programmer].unmap_flash_region(virt_addr, len);
}

void chip_writeb(uint8_t val, chipaddr addr)
{
	programmer_table[programmer].chip_writeb(val, addr);
}

void chip_writew(uint16_t val, chipaddr addr)
{
	programmer_table[programmer].chip_writew(val, addr);
}

void chip_writel(uint32_t val, chipaddr addr)
{
	programmer_table[programmer].chip_writel(val, addr);
}

void chip_writen(uint8_t *buf, chipaddr addr, size_t len)
{
	programmer_table[programmer].chip_writen(buf, addr, len);
}

uint8_t chip_readb(const chipaddr addr)
{
	return programmer_table[programmer].chip_readb(addr);
}

uint16_t chip_readw(const chipaddr addr)
{
	return programmer_table[programmer].chip_readw(addr);
}

uint32_t chip_readl(const chipaddr addr)
{
	return programmer_table[programmer].chip_readl(addr);
}

void chip_readn(uint8_t *buf, chipaddr addr, size_t len)
{
	programmer_table[programmer].chip_readn(buf, addr, len);
}

void programmer_delay(int usecs)
{
	programmer_table[programmer].delay(usecs);
}

void map_flash_registers(struct flashchip *flash)
{
	size_t size = flash->total_size * 1024;
	/* Flash registers live 4 MByte below the flash. */
	/* FIXME: This is incorrect for nonstandard flashbase. */
	flash->virtual_registers = (chipaddr)programmer_map_flash_region("flash chip registers", (0xFFFFFFFF - 0x400000 - size + 1), size);
}

int read_memmapped(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	chip_readn(buf, flash->virtual_memory + start, len);
		
	return 0;
}

int min(int a, int b)
{
	return (a < b) ? a : b;
}

int max(int a, int b)
{
	return (a > b) ? a : b;
}

char *strcat_realloc(char *dest, const char *src)
{
	dest = realloc(dest, strlen(dest) + strlen(src) + 1);
	if (!dest)
		return NULL;
	strcat(dest, src);
	return dest;
}

/* start is an offset to the base address of the flash chip */
int check_erased_range(struct flashchip *flash, int start, int len)
{
	int ret;
	uint8_t *cmpbuf = malloc(len);

	if (!cmpbuf) {
		fprintf(stderr, "Could not allocate memory!\n");
		exit(1);
	}
	memset(cmpbuf, 0xff, len);
	ret = verify_range(flash, cmpbuf, start, len, "ERASE");
	free(cmpbuf);
	return ret;
}

/**
 * @cmpbuf	buffer to compare against
 * @start	offset to the base address of the flash chip
 * @len		length of the verified area
 * @message	string to print in the "FAILED" message
 * @return	0 for success, -1 for failure
 */
int verify_range(struct flashchip *flash, uint8_t *cmpbuf, int start, int len, char *message)
{
	int i, j, starthere, lenhere, ret = 0;
	int page_size = flash->page_size;
	uint8_t *readbuf = malloc(page_size);
	int failcount = 0;

	if (!len)
		goto out_free;

	if (!flash->read) {
		fprintf(stderr, "ERROR: flashrom has no read function for this flash chip.\n");
		return 1;
	}
	if (!readbuf) {
		fprintf(stderr, "Could not allocate memory!\n");
		exit(1);
	}

	if (start + len > flash->total_size * 1024) {
		fprintf(stderr, "Error: %s called with start 0x%x + len 0x%x >"
			" total_size 0x%x\n", __func__, start, len,
			flash->total_size * 1024);
		ret = -1;
		goto out_free;
	}
	if (!message)
		message = "VERIFY";
	
	/* Warning: This loop has a very unusual condition and body.
	 * The loop needs to go through each page with at least one affected
	 * byte. The lowest page number is (start / page_size) since that
	 * division rounds down. The highest page number we want is the page
	 * where the last byte of the range lives. That last byte has the
	 * address (start + len - 1), thus the highest page number is
	 * (start + len - 1) / page_size. Since we want to include that last
	 * page as well, the loop condition uses <=.
	 */
	for (i = start / page_size; i <= (start + len - 1) / page_size; i++) {
		/* Byte position of the first byte in the range in this page. */
		starthere = max(start, i * page_size);
		/* Length of bytes in the range in this page. */
		lenhere = min(start + len, (i + 1) * page_size) - starthere;
		flash->read(flash, readbuf, starthere, lenhere);
		for (j = 0; j < lenhere; j++) {
			if (cmpbuf[starthere - start + j] != readbuf[j]) {
				/* Only print the first failure. */
				if (!failcount++)
					fprintf(stderr, "%s FAILED at 0x%08x! "
						"Expected=0x%02x, Read=0x%02x,",
						message, starthere + j,
						cmpbuf[starthere - start + j],
						readbuf[j]);
			}
		}
	}
	if (failcount) {
		fprintf(stderr, " failed byte count from 0x%08x-0x%08x: 0x%x\n",
			start, start + len - 1, failcount);
		ret = -1;
	}

out_free:
	free(readbuf);
	return ret;
}

struct flashchip *probe_flash(struct flashchip *first_flash, int force)
{
	struct flashchip *flash;
	unsigned long base = 0, size;
	char *tmp;

	for (flash = first_flash; flash && flash->name; flash++) {
		if (chip_to_probe && strcmp(flash->name, chip_to_probe) != 0)
			continue;
		printf_debug("Probing for %s %s, %d KB: ",
			     flash->vendor, flash->name, flash->total_size);
		if (!flash->probe && !force) {
			printf_debug("failed! flashrom has no probe function for this flash chip.\n");
			continue;
		}
		if (!(buses_supported & flash->bustype)) {
			tmp = flashbuses_to_text(buses_supported);
			printf_debug("skipped. Host bus type %s ", tmp);
			free(tmp);
			tmp = flashbuses_to_text(flash->bustype);
			printf_debug("and chip bus type %s are incompatible.\n", tmp);
			free(tmp);
			continue;
		}

		size = flash->total_size * 1024;

		base = flashbase ? flashbase : (0xffffffff - size + 1);
		flash->virtual_memory = (chipaddr)programmer_map_flash_region("flash chip", base, size);

		if (force)
			break;

		if (flash->probe(flash) != 1)
			goto notfound;

		if (first_flash == flashchips
		    || flash->model_id != GENERIC_DEVICE_ID)
			break;

notfound:
		programmer_unmap_flash_region((void *)flash->virtual_memory, size);
	}

	if (!flash || !flash->name)
		return NULL;

	printf("Found chip \"%s %s\" (%d KB, %s) at physical address 0x%lx.\n",
	       flash->vendor, flash->name, flash->total_size,
	       flashbuses_to_text(flash->bustype), base);

	return flash;
}

int verify_flash(struct flashchip *flash, uint8_t *buf)
{
	int ret;
	int total_size = flash->total_size * 1024;

	printf("Verifying flash... ");

	ret = verify_range(flash, buf, 0, total_size, NULL);

	if (!ret)
		printf("VERIFIED.          \n");

	return ret;
}

int read_flash(struct flashchip *flash, char *filename)
{
	unsigned long numbytes;
	FILE *image;
	unsigned long size = flash->total_size * 1024;
	unsigned char *buf = calloc(size, sizeof(char));

	if (!filename) {
		printf("Error: No filename specified.\n");
		return 1;
	}
	if ((image = fopen(filename, "w")) == NULL) {
		perror(filename);
		exit(1);
	}
	printf("Reading flash... ");
	if (!flash->read) {
		printf("FAILED!\n");
		fprintf(stderr, "ERROR: flashrom has no read function for this flash chip.\n");
		return 1;
	} else
		flash->read(flash, buf, 0, size);

	numbytes = fwrite(buf, 1, size, image);
	fclose(image);
	free(buf);
	printf("%s.\n", numbytes == size ? "done" : "FAILED");
	if (numbytes != size)
		return 1;
	return 0;
}

int erase_flash(struct flashchip *flash)
{
	int i, j, k, ret = 0, found = 0;

	printf("Erasing flash chip... ");
	for (k = 0; k < NUM_ERASEFUNCTIONS; k++) {
		unsigned long done = 0;
		struct block_eraser eraser = flash->block_erasers[k];

		printf_debug("Looking at blockwise erase function %i... ", k);
		if (!eraser.block_erase && !eraser.eraseblocks[0].count) {
			printf_debug("not defined. "
				"Looking for another erase function.\n");
			continue;
		}
		if (!eraser.block_erase && eraser.eraseblocks[0].count) {
			printf_debug("eraseblock layout is known, but no "
				"matching block erase function found. "
				"Looking for another erase function.\n");
			continue;
		}
		if (eraser.block_erase && !eraser.eraseblocks[0].count) {
			printf_debug("block erase function found, but "
				"eraseblock layout is unknown. "
				"Looking for another erase function.\n");
			continue;
		}
		found = 1;
		printf_debug("trying... ");
		for (i = 0; i < NUM_ERASEREGIONS; i++) {
			/* count==0 for all automatically initialized array
			 * members so the loop below won't be executed for them.
			 */
			for (j = 0; j < eraser.eraseblocks[i].count; j++) {
				ret = eraser.block_erase(flash, done + eraser.eraseblocks[i].size * j, eraser.eraseblocks[i].size);
				if (ret)
					break;
			}
			if (ret)
				break;
		}
		/* If everything is OK, don't try another erase function. */
		if (!ret)
			break;
	}
	/* If no block erase function was found or block erase failed, retry. */
	if ((!found || ret) && (flash->erase)) {
		found = 1;
		printf_debug("Trying whole-chip erase function... ");
		ret = flash->erase(flash);
	}
	if (!found) {
		fprintf(stderr, "ERROR: flashrom has no erase function for this flash chip.\n");
		return 1;
	}

	if (ret) {
		fprintf(stderr, "FAILED!\n");
	} else {
		printf("SUCCESS.\n");
	}
	return ret;
}

void emergency_help_message()
{
	fprintf(stderr, "Your flash chip is in an unknown state.\n"
		"Get help on IRC at irc.freenode.net channel #flashrom or\n"
		"mail flashrom@flashrom.org\n"
		"------------------------------------------------------------\n"
		"DO NOT REBOOT OR POWEROFF!\n");
}

void usage(const char *name)
{
	const char *pname;
	int pnamelen;
	int remaining = 0;
	enum programmer p;

	printf("usage: %s [-VfLzhR] [-E|-r file|-w file|-v file] [-c chipname]\n"
              "       [-m [vendor:]part] [-l file] [-i image] [-p programmer]\n\n", name);

	printf("Please note that the command line interface for flashrom will "
		"change before\nflashrom 1.0. Do not use flashrom in scripts "
		"or other automated tools without\nchecking that your flashrom"
		" version won't interpret options in a different way.\n\n");

	printf
	    ("   -r | --read:                      read flash and save into file\n"
	     "   -w | --write:                     write file into flash\n"
	     "   -v | --verify:                    verify flash against file\n"
	     "   -n | --noverify:                  don't verify flash against file\n"
	     "   -E | --erase:                     erase flash device\n"
	     "   -V | --verbose:                   more verbose output\n"
	     "   -c | --chip <chipname>:           probe only for specified flash chip\n"
	     "   -m | --mainboard <[vendor:]part>: override mainboard settings\n"
	     "   -f | --force:                     force write without checking image\n"
	     "   -l | --layout <file.layout>:      read ROM layout from file\n"
	     "   -i | --image <name>:              only flash image name from flash layout\n"
	     "   -L | --list-supported:            print supported devices\n"
#if PRINT_WIKI_SUPPORT == 1
	     "   -z | --list-supported-wiki:       print supported devices in wiki syntax\n"
#endif
	     "   -p | --programmer <name>:         specify the programmer device");

	for (p = 0; p < PROGRAMMER_INVALID; p++) {
		pname = programmer_table[p].name;
		pnamelen = strlen(pname);
		if (remaining - pnamelen - 2 < 0) {
			printf("\n                                     ");
			remaining = 43;
		} else {
			printf(" ");
			remaining--;
		}
		if (p == 0) {
			printf("(");
			remaining--;
		}
		printf("%s", pname);
		remaining -= pnamelen;
		if (p < PROGRAMMER_INVALID - 1) {
			printf(",");
			remaining--;
		} else {
			printf(")\n");
		}
	}
		
	printf(
	     "   -h | --help:                      print this help text\n"
	     "   -R | --version:                   print the version (release)\n"
	     "\nYou can specify one of -E, -r, -w, -v or no operation. If no operation is\n"
	     "specified, then all that happens is that flash info is dumped.\n\n");
	exit(1);
}

void print_version(void)
{
	printf("flashrom v%s\n", flashrom_version);
}

int main(int argc, char *argv[])
{
	uint8_t *buf;
	unsigned long size, numbytes;
	FILE *image;
	/* Probe for up to three flash chips. */
	struct flashchip *flash, *flashes[3];
	const char *name;
	int namelen;
	int opt;
	int option_index = 0;
	int force = 0;
	int read_it = 0, write_it = 0, erase_it = 0, verify_it = 0;
	int dont_verify_it = 0, list_supported = 0;
#if PRINT_WIKI_SUPPORT == 1
	int list_supported_wiki = 0;
#endif
	int operation_specified = 0;
	int ret = 0, i;

#if PRINT_WIKI_SUPPORT == 1
	const char *optstring = "rRwvnVEfc:m:l:i:p:Lzh";
#else
	const char *optstring = "rRwvnVEfc:m:l:i:p:Lh";
#endif
	static struct option long_options[] = {
		{"read", 0, 0, 'r'},
		{"write", 0, 0, 'w'},
		{"erase", 0, 0, 'E'},
		{"verify", 0, 0, 'v'},
		{"noverify", 0, 0, 'n'},
		{"chip", 1, 0, 'c'},
		{"mainboard", 1, 0, 'm'},
		{"verbose", 0, 0, 'V'},
		{"force", 0, 0, 'f'},
		{"layout", 1, 0, 'l'},
		{"image", 1, 0, 'i'},
		{"list-supported", 0, 0, 'L'},
#if PRINT_WIKI_SUPPORT == 1
		{"list-supported-wiki", 0, 0, 'z'},
#endif
		{"programmer", 1, 0, 'p'},
		{"help", 0, 0, 'h'},
		{"version", 0, 0, 'R'},
		{0, 0, 0, 0}
	};

	char *filename = NULL;

	char *tempstr = NULL, *tempstr2 = NULL;

	print_version();

	if (argc > 1) {
		/* Yes, print them. */
		int i;
		printf_debug("The arguments are:\n");
		for (i = 1; i < argc; ++i)
			printf_debug("%s\n", argv[i]);
	}

	/* Safety check. */
	if (ARRAY_SIZE(programmer_table) - 1 != PROGRAMMER_INVALID) {
		fprintf(stderr, "Programmer table miscompilation!\n");
		exit(1);
	}
	if (spi_programmer_count - 1 != SPI_CONTROLLER_INVALID) {
		fprintf(stderr, "SPI programmer table miscompilation!\n");
		exit(1);
	}

	setbuf(stdout, NULL);
	while ((opt = getopt_long(argc, argv, optstring,
				  long_options, &option_index)) != EOF) {
		switch (opt) {
		case 'r':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				exit(1);
			}
			read_it = 1;
			break;
		case 'w':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				exit(1);
			}
			write_it = 1;
			break;
		case 'v':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				exit(1);
			}
			verify_it = 1;
			break;
		case 'n':
			dont_verify_it = 1;
			break;
		case 'c':
			chip_to_probe = strdup(optarg);
			break;
		case 'V':
			verbose = 1;
			break;
		case 'E':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				exit(1);
			}
			erase_it = 1;
			break;
		case 'm':
			tempstr = strdup(optarg);
			strtok(tempstr, ":");
			tempstr2 = strtok(NULL, ":");
			if (tempstr2) {
				lb_vendor = tempstr;
				lb_part = tempstr2;
			} else {
				lb_vendor = NULL;
				lb_part = tempstr;
			}
			break;
		case 'f':
			force = 1;
			break;
		case 'l':
			tempstr = strdup(optarg);
			if (read_romlayout(tempstr))
				exit(1);
			break;
		case 'i':
			tempstr = strdup(optarg);
			find_romentry(tempstr);
			break;
		case 'L':
			list_supported = 1;
			break;
#if PRINT_WIKI_SUPPORT == 1
		case 'z':
			list_supported_wiki = 1;
			break;
#endif
		case 'p':
			for (programmer = 0; programmer < PROGRAMMER_INVALID; programmer++) {
				name = programmer_table[programmer].name;
				namelen = strlen(name);
				if (strncmp(optarg, name, namelen) == 0) {
					switch (optarg[namelen]) {
					case ':':
						programmer_param = strdup(optarg + namelen + 1);
						break;
					case '\0':
						break;
					default:
						/* The continue refers to the
						 * for loop. It is here to be
						 * able to differentiate between
						 * foo and foobar.
						 */
						continue;
					}
					break;
				}
			}
			if (programmer == PROGRAMMER_INVALID) {
				printf("Error: Unknown programmer %s.\n", optarg);
				exit(1);
			}
			break;
		case 'R':
			/* print_version() is always called during startup. */
			exit(0);
			break;
		case 'h':
		default:
			usage(argv[0]);
			break;
		}
	}

	if (list_supported) {
		print_supported_chips();
		print_supported_chipsets();
		print_supported_boards();
		printf("\nSupported PCI devices flashrom can use "
		       "as programmer:\n\n");
#if NIC3COM_SUPPORT == 1
		print_supported_pcidevs(nics_3com);
#endif
#if DRKAISER_SUPPORT == 1
		print_supported_pcidevs(drkaiser_pcidev);
#endif
#if SATASII_SUPPORT == 1
		print_supported_pcidevs(satas_sii);
#endif
		exit(0);
	}

#if PRINT_WIKI_SUPPORT == 1
	if (list_supported_wiki) {
		print_wiki_tables();
		exit(0);
	}
#endif

	if (read_it && write_it) {
		printf("Error: -r and -w are mutually exclusive.\n");
		usage(argv[0]);
	}

	if (optind < argc)
		filename = argv[optind++];

	if (programmer_init()) {
		fprintf(stderr, "Error: Programmer initialization failed.\n");
		exit(1);
	}

	myusec_calibrate_delay();

	for (i = 0; i < ARRAY_SIZE(flashes); i++) {
		flashes[i] =
		    probe_flash(i ? flashes[i - 1] + 1 : flashchips, 0);
		if (!flashes[i])
			for (i++; i < ARRAY_SIZE(flashes); i++)
				flashes[i] = NULL;
	}

	if (flashes[1]) {
		printf("Multiple flash chips were detected:");
		for (i = 0; i < ARRAY_SIZE(flashes) && flashes[i]; i++)
			printf(" %s", flashes[i]->name);
		printf("\nPlease specify which chip to use with the -c <chipname> option.\n");
		exit(1);
	} else if (!flashes[0]) {
		printf("No EEPROM/flash device found.\n");
		if (!force || !chip_to_probe) {
			printf("If you know which flash chip you have, and if this version of flashrom\n");
			printf("supports a similar flash chip, you can try to force read your chip. Run:\n");
			printf("flashrom -f -r -c similar_supported_flash_chip filename\n");
			printf("\n");
			printf("Note: flashrom can never write when the flash chip isn't found automatically.\n");
		}
		if (force && read_it && chip_to_probe) {
			printf("Force read (-f -r -c) requested, forcing chip probe success:\n");
			flashes[0] = probe_flash(flashchips, 1);
			if (!flashes[0]) {
				printf("flashrom does not support a flash chip named '%s'.\n", chip_to_probe);
				printf("Run flashrom -L to view the hardware supported in this flashrom version.\n");
				exit(1);
			}
			printf("Please note that forced reads most likely contain garbage.\n");
			return read_flash(flashes[0], filename);
		}
		// FIXME: flash writes stay enabled!
		exit(1);
	}

	flash = flashes[0];

	if (TEST_OK_MASK != (flash->tested & TEST_OK_MASK)) {
		printf("===\n");
		if (flash->tested & TEST_BAD_MASK) {
			printf("This flash part has status NOT WORKING for operations:");
			if (flash->tested & TEST_BAD_PROBE)
				printf(" PROBE");
			if (flash->tested & TEST_BAD_READ)
				printf(" READ");
			if (flash->tested & TEST_BAD_ERASE)
				printf(" ERASE");
			if (flash->tested & TEST_BAD_WRITE)
				printf(" WRITE");
			printf("\n");
		}
		if ((!(flash->tested & TEST_BAD_PROBE) && !(flash->tested & TEST_OK_PROBE)) ||
		    (!(flash->tested & TEST_BAD_READ) && !(flash->tested & TEST_OK_READ)) ||
		    (!(flash->tested & TEST_BAD_ERASE) && !(flash->tested & TEST_OK_ERASE)) ||
		    (!(flash->tested & TEST_BAD_WRITE) && !(flash->tested & TEST_OK_WRITE))) {
			printf("This flash part has status UNTESTED for operations:");
			if (!(flash->tested & TEST_BAD_PROBE) && !(flash->tested & TEST_OK_PROBE))
				printf(" PROBE");
			if (!(flash->tested & TEST_BAD_READ) && !(flash->tested & TEST_OK_READ))
				printf(" READ");
			if (!(flash->tested & TEST_BAD_ERASE) && !(flash->tested & TEST_OK_ERASE))
				printf(" ERASE");
			if (!(flash->tested & TEST_BAD_WRITE) && !(flash->tested & TEST_OK_WRITE))
				printf(" WRITE");
			printf("\n");
		}
		printf("Please email a report to flashrom@flashrom.org if any "
		       "of the above operations\nwork correctly for you with "
		       "this flash part. Please include the flashrom\noutput "
		       "with the additional -V option for all operations you "
		       "tested (-V, -rV,\n-wV, -EV), and mention which "
		       "mainboard you tested. Thanks for your help!\n===\n");
	}

	if (!(read_it | write_it | verify_it | erase_it)) {
		printf("No operations were specified.\n");
		// FIXME: flash writes stay enabled!
		exit(1);
	}

	if (!filename && !erase_it) {
		printf("Error: No filename specified.\n");
		// FIXME: flash writes stay enabled!
		exit(1);
	}

	/* Always verify write operations unless -n is used. */
	if (write_it && !dont_verify_it)
		verify_it = 1;

	size = flash->total_size * 1024;
	buf = (uint8_t *) calloc(size, sizeof(char));

	if (erase_it) {
		if (flash->tested & TEST_BAD_ERASE) {
			fprintf(stderr, "Erase is not working on this chip. ");
			if (!force) {
				fprintf(stderr, "Aborting.\n");
				return 1;
			} else {
				fprintf(stderr, "Continuing anyway.\n");
			}
		}
		if (erase_flash(flash)) {
			emergency_help_message();
			return 1;
		}
	} else if (read_it) {
		if (read_flash(flash, filename))
			return 1;
	} else {
		struct stat image_stat;

		if (flash->tested & TEST_BAD_ERASE) {
			fprintf(stderr, "Erase is not working on this chip "
				"and erase is needed for write. ");
			if (!force) {
				fprintf(stderr, "Aborting.\n");
				return 1;
			} else {
				fprintf(stderr, "Continuing anyway.\n");
			}
		}
		if (flash->tested & TEST_BAD_WRITE) {
			fprintf(stderr, "Write is not working on this chip. ");
			if (!force) {
				fprintf(stderr, "Aborting.\n");
				return 1;
			} else {
				fprintf(stderr, "Continuing anyway.\n");
			}
		}
		if ((image = fopen(filename, "r")) == NULL) {
			perror(filename);
			exit(1);
		}
		if (fstat(fileno(image), &image_stat) != 0) {
			perror(filename);
			exit(1);
		}
		if (image_stat.st_size != flash->total_size * 1024) {
			fprintf(stderr, "Error: Image size doesn't match\n");
			exit(1);
		}

		numbytes = fread(buf, 1, size, image);
		show_id(buf, size, force);
		fclose(image);
		if (numbytes != size) {
			fprintf(stderr, "Error: Failed to read file. Got %ld bytes, wanted %ld!\n", numbytes, size);
			return 1;
		}
	}

	// This should be moved into each flash part's code to do it 
	// cleanly. This does the job.
	handle_romentries(buf, flash);

	// ////////////////////////////////////////////////////////////

	if (write_it) {
		printf("Writing flash chip... ");
		if (!flash->write) {
			fprintf(stderr, "Error: flashrom has no write function for this flash chip.\n");
			return 1;
		}
		ret = flash->write(flash, buf);
		if (ret) {
			fprintf(stderr, "FAILED!\n");
			emergency_help_message();
			return 1;
		} else {
			printf("COMPLETE.\n");
		}
	}

	if (verify_it) {
		/* Work around chips which need some time to calm down. */
		if (write_it)
			programmer_delay(1000*1000);
		ret = verify_flash(flash, buf);
		/* If we tried to write, and now we don't properly verify, we
		 * might have an emergency situation.
		 */
		if (ret && write_it)
			emergency_help_message();
	}

	programmer_shutdown();

	return ret;
}
