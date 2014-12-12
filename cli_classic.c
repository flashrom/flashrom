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

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "flash.h"
#include "flashchips.h"
#include "programmer.h"

static void cli_classic_usage(const char *name)
{
	printf("Please note that the command line interface for flashrom has changed between\n"
	       "0.9.5 and 0.9.6 and will change again before flashrom 1.0.\n\n");

	printf("Usage: %s [-h|-R|-L|"
#if CONFIG_PRINT_WIKI == 1
	       "-z|"
#endif
	       "-p <programmername>[:<parameters>] [-c <chipname>]\n"
	       "[-E|(-r|-w|-v) <file>] [-l <layoutfile> [-i <imagename>]...] [-n] [-f]]\n"
	       "[-V[V[V]]] [-o <logfile>]\n\n", name);

	printf(" -h | --help                        print this help text\n"
	       " -R | --version                     print version (release)\n"
	       " -r | --read <file>                 read flash and save to <file>\n"
	       " -w | --write <file>                write <file> to flash\n"
	       " -v | --verify <file>               verify flash against <file>\n"
	       " -E | --erase                       erase flash memory\n"
	       " -V | --verbose                     more verbose output\n"
	       " -c | --chip <chipname>             probe only for specified flash chip\n"
	       " -f | --force                       force specific operations (see man page)\n"
	       " -n | --noverify                    don't auto-verify\n"
	       " -l | --layout <layoutfile>         read ROM layout from <layoutfile>\n"
	       " -i | --image <name>                only flash image <name> from flash layout\n"
	       " -o | --output <logfile>            log output to <logfile>\n"
	       " -L | --list-supported              print supported devices\n"
#if CONFIG_PRINT_WIKI == 1
	       " -z | --list-supported-wiki         print supported devices in wiki syntax\n"
#endif
	       " -p | --programmer <name>[:<param>] specify the programmer device. One of\n");
	list_programmers_linebreak(4, 80, 0);
	printf(".\n\nYou can specify one of -h, -R, -L, "
#if CONFIG_PRINT_WIKI == 1
	         "-z, "
#endif
	         "-E, -r, -w, -v or no operation.\n"
	       "If no operation is specified, flashrom will only probe for flash chips.\n");
}

static void cli_classic_abort_usage(void)
{
	printf("Please run \"flashrom --help\" for usage info.\n");
	exit(1);
}

static int check_filename(char *filename, char *type)
{
	if (!filename || (filename[0] == '\0')) {
		fprintf(stderr, "Error: No %s file specified.\n", type);
		return 1;
	}
	/* Not an error, but maybe the user intended to specify a CLI option instead of a file name. */
	if (filename[0] == '-')
		fprintf(stderr, "Warning: Supplied %s file name starts with -\n", type);
	return 0;
}

int main(int argc, char *argv[])
{
	const struct flashchip *chip = NULL;
	/* Probe for up to eight flash chips. */
	struct flashctx flashes[8] = {{0}};
	struct flashctx *fill_flash;
	const char *name;
	int namelen, opt, i, j;
	int startchip = -1, chipcount = 0, option_index = 0, force = 0;
#if CONFIG_PRINT_WIKI == 1
	int list_supported_wiki = 0;
#endif
	int read_it = 0, write_it = 0, erase_it = 0, verify_it = 0;
	int dont_verify_it = 0, list_supported = 0, operation_specified = 0;
	enum programmer prog = PROGRAMMER_INVALID;
	int ret = 0;

	static const char optstring[] = "r:Rw:v:nVEfc:l:i:p:Lzho:";
	static const struct option long_options[] = {
		{"read",		1, NULL, 'r'},
		{"write",		1, NULL, 'w'},
		{"erase",		0, NULL, 'E'},
		{"verify",		1, NULL, 'v'},
		{"noverify",		0, NULL, 'n'},
		{"chip",		1, NULL, 'c'},
		{"verbose",		0, NULL, 'V'},
		{"force",		0, NULL, 'f'},
		{"layout",		1, NULL, 'l'},
		{"image",		1, NULL, 'i'},
		{"list-supported",	0, NULL, 'L'},
		{"list-supported-wiki",	0, NULL, 'z'},
		{"programmer",		1, NULL, 'p'},
		{"help",		0, NULL, 'h'},
		{"version",		0, NULL, 'R'},
		{"output",		1, NULL, 'o'},
		{NULL,			0, NULL, 0},
	};

	char *filename = NULL;
	char *layoutfile = NULL;
#ifndef STANDALONE
	char *logfile = NULL;
#endif /* !STANDALONE */
	char *tempstr = NULL;
	char *pparam = NULL;

	print_version();
	print_banner();

	if (selfcheck())
		exit(1);

	setbuf(stdout, NULL);
	/* FIXME: Delay all operation_specified checks until after command
	 * line parsing to allow --help overriding everything else.
	 */
	while ((opt = getopt_long(argc, argv, optstring,
				  long_options, &option_index)) != EOF) {
		switch (opt) {
		case 'r':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			filename = strdup(optarg);
			read_it = 1;
			break;
		case 'w':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			filename = strdup(optarg);
			write_it = 1;
			break;
		case 'v':
			//FIXME: gracefully handle superfluous -v
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			if (dont_verify_it) {
				fprintf(stderr, "--verify and --noverify are mutually exclusive. Aborting.\n");
				cli_classic_abort_usage();
			}
			filename = strdup(optarg);
			verify_it = 1;
			break;
		case 'n':
			if (verify_it) {
				fprintf(stderr, "--verify and --noverify are mutually exclusive. Aborting.\n");
				cli_classic_abort_usage();
			}
			dont_verify_it = 1;
			break;
		case 'c':
			chip_to_probe = strdup(optarg);
			break;
		case 'V':
			verbose_screen++;
			if (verbose_screen > MSG_DEBUG2)
				verbose_logfile = verbose_screen;
			break;
		case 'E':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			erase_it = 1;
			break;
		case 'f':
			force = 1;
			break;
		case 'l':
			if (layoutfile) {
				fprintf(stderr, "Error: --layout specified "
					"more than once. Aborting.\n");
				cli_classic_abort_usage();
			}
			layoutfile = strdup(optarg);
			break;
		case 'i':
			tempstr = strdup(optarg);
			if (register_include_arg(tempstr)) {
				free(tempstr);
				cli_classic_abort_usage();
			}
			break;
		case 'L':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			list_supported = 1;
			break;
		case 'z':
#if CONFIG_PRINT_WIKI == 1
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			list_supported_wiki = 1;
#else
			fprintf(stderr, "Error: Wiki output was not compiled "
				"in. Aborting.\n");
			cli_classic_abort_usage();
#endif
			break;
		case 'p':
			if (prog != PROGRAMMER_INVALID) {
				fprintf(stderr, "Error: --programmer specified "
					"more than once. You can separate "
					"multiple\nparameters for a programmer "
					"with \",\". Please see the man page "
					"for details.\n");
				cli_classic_abort_usage();
			}
			for (prog = 0; prog < PROGRAMMER_INVALID; prog++) {
				name = programmer_table[prog].name;
				namelen = strlen(name);
				if (strncmp(optarg, name, namelen) == 0) {
					switch (optarg[namelen]) {
					case ':':
						pparam = strdup(optarg + namelen + 1);
						if (!strlen(pparam)) {
							free(pparam);
							pparam = NULL;
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
			if (prog == PROGRAMMER_INVALID) {
				fprintf(stderr, "Error: Unknown programmer \"%s\". Valid choices are:\n",
					optarg);
				list_programmers_linebreak(0, 80, 0);
				msg_ginfo(".\n");
				cli_classic_abort_usage();
			}
			break;
		case 'R':
			/* print_version() is always called during startup. */
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			exit(0);
			break;
		case 'h':
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			cli_classic_usage(argv[0]);
			exit(0);
			break;
		case 'o':
#ifdef STANDALONE
			fprintf(stderr, "Log file not supported in standalone mode. Aborting.\n");
			cli_classic_abort_usage();
#else /* STANDALONE */
			logfile = strdup(optarg);
			if (logfile[0] == '\0') {
				fprintf(stderr, "No log filename specified.\n");
				cli_classic_abort_usage();
			}
#endif /* STANDALONE */
			break;
		default:
			cli_classic_abort_usage();
			break;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "Error: Extra parameter found.\n");
		cli_classic_abort_usage();
	}

	if ((read_it | write_it | verify_it) && check_filename(filename, "image")) {
		cli_classic_abort_usage();
	}
	if (layoutfile && check_filename(layoutfile, "layout")) {
		cli_classic_abort_usage();
	}

#ifndef STANDALONE
	if (logfile && check_filename(logfile, "log"))
		cli_classic_abort_usage();
	if (logfile && open_logfile(logfile))
		cli_classic_abort_usage();
#endif /* !STANDALONE */

#if CONFIG_PRINT_WIKI == 1
	if (list_supported_wiki) {
		print_supported_wiki();
		goto out;
	}
#endif

	if (list_supported) {
		if (print_supported())
			ret = 1;
		goto out;
	}

#ifndef STANDALONE
	start_logging();
#endif /* !STANDALONE */

	print_buildinfo();
	msg_gdbg("Command line (%i args):", argc - 1);
	for (i = 0; i < argc; i++) {
		msg_gdbg(" %s", argv[i]);
	}
	msg_gdbg("\n");

	if (layoutfile && read_romlayout(layoutfile)) {
		ret = 1;
		goto out;
	}
	if (layoutfile != NULL && !write_it) {
		msg_gerr("Layout files are currently supported for write operations only.\n");
		ret = 1;
		goto out;
	}

	if (process_include_args()) {
		ret = 1;
		goto out;
	}
	/* Does a chip with the requested name exist in the flashchips array? */
	if (chip_to_probe) {
		for (chip = flashchips; chip && chip->name; chip++)
			if (!strcmp(chip->name, chip_to_probe))
				break;
		if (!chip || !chip->name) {
			msg_cerr("Error: Unknown chip '%s' specified.\n", chip_to_probe);
			msg_gerr("Run flashrom -L to view the hardware supported in this flashrom version.\n");
			ret = 1;
			goto out;
		}
		/* Keep chip around for later usage in case a forced read is requested. */
	}

	if (prog == PROGRAMMER_INVALID) {
		if (CONFIG_DEFAULT_PROGRAMMER != PROGRAMMER_INVALID) {
			prog = CONFIG_DEFAULT_PROGRAMMER;
			/* We need to strdup here because we free(pparam) unconditionally later. */
			pparam = strdup(CONFIG_DEFAULT_PROGRAMMER_ARGS);
			msg_pinfo("Using default programmer \"%s\" with arguments \"%s\".\n",
				  programmer_table[CONFIG_DEFAULT_PROGRAMMER].name, pparam);
		} else {
			msg_perr("Please select a programmer with the --programmer parameter.\n"
				 "Previously this was not necessary because there was a default set.\n"
#if CONFIG_INTERNAL == 1
				 "To choose the mainboard of this computer use 'internal'. "
#endif
				 "Valid choices are:\n");
			list_programmers_linebreak(0, 80, 0);
			msg_ginfo(".\n");
			ret = 1;
			goto out;
		}
	}

	/* FIXME: Delay calibration should happen in programmer code. */
	myusec_calibrate_delay();

	if (programmer_init(prog, pparam)) {
		msg_perr("Error: Programmer initialization failed.\n");
		ret = 1;
		goto out_shutdown;
	}
	tempstr = flashbuses_to_text(get_buses_supported());
	msg_pdbg("The following protocols are supported: %s.\n", tempstr);
	free(tempstr);

	for (j = 0; j < registered_master_count; j++) {
		startchip = 0;
		while (chipcount < ARRAY_SIZE(flashes)) {
			startchip = probe_flash(&registered_masters[j], startchip, &flashes[chipcount], 0);
			if (startchip == -1)
				break;
			chipcount++;
			startchip++;
		}
	}

	if (chipcount > 1) {
		msg_cinfo("Multiple flash chip definitions match the detected chip(s): \"%s\"",
			  flashes[0].chip->name);
		for (i = 1; i < chipcount; i++)
			msg_cinfo(", \"%s\"", flashes[i].chip->name);
		msg_cinfo("\nPlease specify which chip definition to use with the -c <chipname> option.\n");
		ret = 1;
		goto out_shutdown;
	} else if (!chipcount) {
		msg_cinfo("No EEPROM/flash device found.\n");
		if (!force || !chip_to_probe) {
			msg_cinfo("Note: flashrom can never write if the flash chip isn't found "
				  "automatically.\n");
		}
		if (force && read_it && chip_to_probe) {
			struct registered_master *mst;
			int compatible_masters = 0;
			msg_cinfo("Force read (-f -r -c) requested, pretending the chip is there:\n");
			/* This loop just counts compatible controllers. */
			for (j = 0; j < registered_master_count; j++) {
				mst = &registered_masters[j];
				/* chip is still set from the chip_to_probe earlier in this function. */
				if (mst->buses_supported & chip->bustype)
					compatible_masters++;
			}
			if (!compatible_masters) {
				msg_cinfo("No compatible controller found for the requested flash chip.\n");
				ret = 1;
				goto out_shutdown;
			}
			if (compatible_masters > 1)
				msg_cinfo("More than one compatible controller found for the requested flash "
					  "chip, using the first one.\n");
			for (j = 0; j < registered_master_count; j++) {
				mst = &registered_masters[j];
				startchip = probe_flash(mst, 0, &flashes[0], 1);
				if (startchip != -1)
					break;
			}
			if (startchip == -1) {
				// FIXME: This should never happen! Ask for a bug report?
				msg_cinfo("Probing for flash chip '%s' failed.\n", chip_to_probe);
				ret = 1;
				goto out_shutdown;
			}
			if (map_flash(&flashes[0]) != 0) {
				free(flashes[0].chip);
				ret = 1;
				goto out_shutdown;
			}
			msg_cinfo("Please note that forced reads most likely contain garbage.\n");
			ret = read_flash_to_file(&flashes[0], filename);
			unmap_flash(&flashes[0]);
			free(flashes[0].chip);
			goto out_shutdown;
		}
		ret = 1;
		goto out_shutdown;
	} else if (!chip_to_probe) {
		/* repeat for convenience when looking at foreign logs */
		tempstr = flashbuses_to_text(flashes[0].chip->bustype);
		msg_gdbg("Found %s flash chip \"%s\" (%d kB, %s).\n",
			 flashes[0].chip->vendor, flashes[0].chip->name, flashes[0].chip->total_size, tempstr);
		free(tempstr);
	}

	fill_flash = &flashes[0];

	print_chip_support_status(fill_flash->chip);

	unsigned int limitexceeded = count_max_decode_exceedings(fill_flash);
	if (limitexceeded > 0 && !force) {
		enum chipbustype commonbuses = fill_flash->mst->buses_supported & fill_flash->chip->bustype;

		/* Sometimes chip and programmer have more than one bus in common,
		 * and the limit is not exceeded on all buses. Tell the user. */
		if ((bitcount(commonbuses) > limitexceeded)) {
			msg_pdbg("There is at least one interface available which could support the size of\n"
				 "the selected flash chip.\n");
		}
		msg_cerr("This flash chip is too big for this programmer (--verbose/-V gives details).\n"
			 "Use --force/-f to override at your own risk.\n");
		ret = 1;
		goto out_shutdown;
	}

	if (!(read_it | write_it | verify_it | erase_it)) {
		msg_ginfo("No operations were specified.\n");
		goto out_shutdown;
	}

	/* Always verify write operations unless -n is used. */
	if (write_it && !dont_verify_it)
		verify_it = 1;

	/* Map the selected flash chip again. */
	if (map_flash(fill_flash) != 0) {
		ret = 1;
		goto out_shutdown;
	}

	/* FIXME: We should issue an unconditional chip reset here. This can be
	 * done once we have a .reset function in struct flashchip.
	 * Give the chip time to settle.
	 */
	programmer_delay(100000);
	ret |= doit(fill_flash, force, filename, read_it, write_it, erase_it, verify_it);

	unmap_flash(fill_flash);
out_shutdown:
	programmer_shutdown();
out:
	for (i = 0; i < chipcount; i++)
		free(flashes[i].chip);

	layout_cleanup();
	free(filename);
	free(layoutfile);
	free(pparam);
	/* clean up global variables */
	free((char *)chip_to_probe); /* Silence! Freeing is not modifying contents. */
	chip_to_probe = NULL;
#ifndef STANDALONE
	free(logfile);
	ret |= close_logfile();
#endif /* !STANDALONE */
	return ret;
}
