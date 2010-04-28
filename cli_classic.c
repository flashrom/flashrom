/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2004 Tyan Corp <yhlu@tyan.com>
 * Copyright (C) 2005-2008 coresystems GmbH
 * Copyright (C) 2008,2009,2010 Carl-Daniel Hailfinger
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

void cli_classic_usage(const char *name)
{
	const char *pname;
	int pnamelen;
	int remaining = 0;
	enum programmer p;

	printf("Usage: %s [-VfLzhR] [-E|-r file|-w file|-v file] [-c chipname]\n"
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
#if INTERNAL_SUPPORT == 1
	     "   -m | --mainboard <[vendor:]part>: override mainboard settings\n"
#endif
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

int cli_classic(int argc, char *argv[])
{
	unsigned long size;
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
	int i;

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

	char *tempstr = NULL;

	print_version();

	if (argc > 1) {
		/* Yes, print them. */
		printf_debug("The arguments are:\n");
		for (i = 1; i < argc; ++i)
			printf_debug("%s\n", argv[i]);
	}

	if (selfcheck())
		exit(1);

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
			//FIXME: gracefully handle superfluous -v
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				exit(1);
			}
			if (dont_verify_it) {
				fprintf(stderr, "--verify and --noverify are"
					"mutually exclusive. Aborting.\n");
				exit(1);
			}
			verify_it = 1;
			break;
		case 'n':
			if (verify_it) {
				fprintf(stderr, "--verify and --noverify are"
					"mutually exclusive. Aborting.\n");
				exit(1);
			}
			dont_verify_it = 1;
			break;
		case 'c':
			chip_to_probe = strdup(optarg);
			break;
		case 'V':
			verbose++;
			break;
		case 'E':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				exit(1);
			}
			erase_it = 1;
			break;
#if INTERNAL_SUPPORT == 1
		case 'm':
			tempstr = strdup(optarg);
			lb_vendor_dev_from_string(tempstr);
			break;
#endif
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
						if (!strlen(programmer_param)) {
							free(programmer_param);
							programmer_param = NULL;
						}
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
			cli_classic_usage(argv[0]);
			break;
		}
	}

	if (list_supported) {
		print_supported();
		exit(0);
	}

#if PRINT_WIKI_SUPPORT == 1
	if (list_supported_wiki) {
		print_supported_wiki();
		exit(0);
	}
#endif

	if (read_it && write_it) {
		printf("Error: -r and -w are mutually exclusive.\n");
		cli_classic_usage(argv[0]);
	}

	if (optind < argc)
		filename = argv[optind++];
	
	if (optind < argc) {
		printf("Error: Extra parameter found.\n");
		cli_classic_usage(argv[0]);
	}

	if (chip_to_probe) {
		for (flash = flashchips; flash && flash->name; flash++)
			if (!strcmp(flash->name, chip_to_probe))
				break;
		if (!flash || !flash->name) {
			fprintf(stderr, "Error: Unknown chip '%s' specified.\n",
				chip_to_probe);
			printf("Run flashrom -L to view the hardware supported "
				"in this flashrom version.\n");
			exit(1);
		}
		/* Clean up after the check. */
		flash = NULL;
	}
		
	if (programmer_init()) {
		fprintf(stderr, "Error: Programmer initialization failed.\n");
		exit(1);
	}

	// FIXME: Delay calibration should happen in programmer code.
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
		programmer_shutdown();
		exit(1);
	} else if (!flashes[0]) {
		printf("No EEPROM/flash device found.\n");
		if (!force || !chip_to_probe) {
			printf("Note: flashrom can never write if the flash chip isn't found automatically.\n");
		}
		if (force && read_it && chip_to_probe) {
			printf("Force read (-f -r -c) requested, pretending the chip is there:\n");
			flashes[0] = probe_flash(flashchips, 1);
			if (!flashes[0]) {
				printf("Probing for flash chip '%s' failed.\n", chip_to_probe);
				programmer_shutdown();
				exit(1);
			}
			printf("Please note that forced reads most likely contain garbage.\n");
			return read_flash(flashes[0], filename);
		}
		// FIXME: flash writes stay enabled!
		programmer_shutdown();
		exit(1);
	}

	flash = flashes[0];

	check_chip_supported(flash);

	size = flash->total_size * 1024;
	if (check_max_decode((buses_supported & flash->bustype), size) &&
	    (!force)) {
		fprintf(stderr, "Chip is too big for this programmer "
			"(-V gives details). Use --force to override.\n");
		programmer_shutdown();
		return 1;
	}

	if (!(read_it | write_it | verify_it | erase_it)) {
		printf("No operations were specified.\n");
		// FIXME: flash writes stay enabled!
		programmer_shutdown();
		exit(1);
	}

	if (!filename && !erase_it) {
		printf("Error: No filename specified.\n");
		// FIXME: flash writes stay enabled!
		programmer_shutdown();
		exit(1);
	}

	/* Always verify write operations unless -n is used. */
	if (write_it && !dont_verify_it)
		verify_it = 1;

	return doit(flash, force, filename, read_it, write_it, erase_it, verify_it);
}
