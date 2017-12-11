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
#include "chipdrivers.h"
#include "flash.h"
#include "flashchips.h"
#include "programmer.h"
#include "libflashrom.h"
#include "writeprotect.h"

enum LONGOPT_RETURN_VALUES {
	/* Start after ASCII chars */
	LONGOPT_IFD = 256,
	LONGOPT_PRINT_STATUSREG = 512,
	LONGOPT_PRINT_WP_STATUS,
	LONGOPT_LIST_BP_RANGES,
	LONGOPT_WP_ENABLE,
	LONGOPT_WP_DISABLE,
	LONGOPT_SET_BP_RANGE,
};

static void cli_classic_usage(const char *name)
{
	printf("Please note that the command line interface for flashrom has changed between\n"
	       "0.9.5 and 0.9.6 and will change again before flashrom 1.0.\n\n");

	printf("Usage: %s [-h|-R|-L|"
#if CONFIG_PRINT_WIKI == 1
	       "-z|"
#endif
	       "-p <programmername>[:<parameters>] [-c <chipname>]\n"
	       "[-E|(-r|-w|-v) <file>] [(-l <layoutfile>|--ifd) [-i <imagename>]...] [-n] [-N] [-f]]\n"
	       "[-V[V[V]]] [-o <logfile>] [--print-status-reg] [--print-wp-status]\n"
	       "[--wp-list] [--wp-enable[=<MODE>]] [--wp-disable]\n"
	       "[--wp-set-range start=<start>,len=<len>]\n\n", name);

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
	       " -N | --noverify-all                verify included regions only (cf. -i)\n"
	       " -l | --layout <layoutfile>         read ROM layout from <layoutfile>\n"
	       "      --ifd                         read layout from an Intel Firmware Descriptor\n"
	       " -i | --image <name>                only flash image <name> from flash layout\n"
	       " -o | --output <logfile>            log output to <logfile>\n"
	       " -L | --list-supported              print supported devices\n"
#if CONFIG_PRINT_WIKI == 1
	       " -z | --list-supported-wiki         print supported devices in wiki syntax\n"
#endif
	       "      --print-status-reg            print detailed contents of status register(s)\n"
	       "      --print-wp-status             print write protection mode of status register(s)\n"
	       "      --wp-list                     print list of write protection ranges\n"
	       "      --wp-enable[=MODE]            enable write protection of status register(s)\n"
	       "                                    MODE can be one of HARDWARE (default),\n"
	       "                                    PERMANENT, POWER_CYCLE, SOFTWARE (see man page)\n"
	       "      --wp-disable                  disable any write protection of status register(s)\n"
	       "      --wp-set-range start=<start>,len=<len>\n"
	       "                                    set write protection range (see man page)\n"
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

static void cli_statreg_wp_support(struct flashctx *flash)
{
	msg_ginfo("flashrom does not (yet) support write protection for chip \"%s\".\n"
		  "You could add support and send the patch to flashrom at flashrom.org\n",
		  flash->chip->name);
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
	int startchip = -1, chipcount = 0, option_index = 0, force = 0, ifd = 0;
#if CONFIG_PRINT_WIKI == 1
	int list_supported_wiki = 0;
#endif
	int read_it = 0, write_it = 0, erase_it = 0, verify_it = 0, print_status_reg = 0;
	int print_wp_status = 0, wp_list = 0, wp_enable = 0, wp_disable = 0, wp_set_range = 0;
	int dont_verify_it = 0, dont_verify_all = 0, list_supported = 0, operation_specified = 0;
	struct flashrom_layout *layout = NULL;
	enum programmer prog = PROGRAMMER_INVALID;
	int ret = 0;

	static const char optstring[] = "r:Rw:v:nNVEfc:l:i:p:Lzho:";
	static const struct option long_options[] = {
		{"read",		1, NULL, 'r'},
		{"write",		1, NULL, 'w'},
		{"erase",		0, NULL, 'E'},
		{"verify",		1, NULL, 'v'},
		{"noverify",		0, NULL, 'n'},
		{"noverify-all",	0, NULL, 'N'},
		{"chip",		1, NULL, 'c'},
		{"verbose",		0, NULL, 'V'},
		{"force",		0, NULL, 'f'},
		{"layout",		1, NULL, 'l'},
		{"ifd",			0, NULL, LONGOPT_IFD},
		{"image",		1, NULL, 'i'},
		{"list-supported",	0, NULL, 'L'},
		{"list-supported-wiki",	0, NULL, 'z'},
		{"programmer",		1, NULL, 'p'},
		{"help",		0, NULL, 'h'},
		{"version",		0, NULL, 'R'},
		{"output",		1, NULL, 'o'},
		{"print-status-reg",	0, NULL, LONGOPT_PRINT_STATUSREG},
		{"print-wp-status",	0, NULL, LONGOPT_PRINT_WP_STATUS},
		{"wp-list",		0, NULL, LONGOPT_LIST_BP_RANGES},
		{"wp-enable",		optional_argument, NULL, LONGOPT_WP_ENABLE},
		{"wp-disable",		0, NULL, LONGOPT_WP_DISABLE},
		{"wp-set-range",	1, NULL, LONGOPT_SET_BP_RANGE},
		{NULL,			0, NULL, 0},
	};

	char *filename = NULL;
	char *layoutfile = NULL;
#ifndef STANDALONE
	char *logfile = NULL;
#endif /* !STANDALONE */
	char *tempstr = NULL;
	char *pparam = NULL;
	char *wp_mode_opt = NULL;
	char const *wp_set_range_opt = NULL;

	flashrom_set_log_callback((flashrom_log_callback *)&flashrom_print_cb);

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
		case 'N':
			dont_verify_all = 1;
			break;
		case 'c':
			chip_to_probe = strdup(optarg);
			break;
		case 'V':
			verbose_screen++;
			if (verbose_screen > FLASHROM_MSG_DEBUG2)
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
			if (ifd) {
				fprintf(stderr, "Error: --layout and --ifd both specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			layoutfile = strdup(optarg);
			break;
		case LONGOPT_IFD:
			if (layoutfile) {
				fprintf(stderr, "Error: --layout and --ifd both specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			ifd = 1;
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
		/* FIXME(hatim): For the following long options, not _all_
		 * of them are mutually exclusive per se (like wp_set_range
		 * and wp_enable makes sense). There is scope for improvement
		 * here, but for now let's treat each one as separate operation. */
		case LONGOPT_PRINT_STATUSREG:
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			print_status_reg = 1;
			break;
		case LONGOPT_PRINT_WP_STATUS:
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			print_wp_status = 1;
			break;
		case LONGOPT_LIST_BP_RANGES:
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			wp_list = 1;
			break;
		case LONGOPT_WP_ENABLE:
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			wp_enable = 1;
			if (optarg)
				wp_mode_opt = strdup(optarg);
			break;
		case LONGOPT_WP_DISABLE:
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			wp_disable = 1;
			break;
		case LONGOPT_SET_BP_RANGE:
			if (++operation_specified > 1) {
				fprintf(stderr, "More than one operation "
					"specified. Aborting.\n");
				cli_classic_abort_usage();
			}
			wp_set_range_opt = strdup(optarg);
			wp_set_range = 1;
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
	if (!ifd && process_include_args(get_global_layout())) {
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

	if (!(read_it | write_it | verify_it | erase_it | print_status_reg |
	      print_wp_status | wp_list | wp_enable | wp_disable | wp_set_range)) {
		msg_ginfo("No operations were specified.\n");
		goto out_shutdown;
	}

	if (layoutfile) {
		layout = get_global_layout();
	} else if (ifd && (flashrom_layout_read_from_ifd(&layout, fill_flash, NULL, 0) ||
			   process_include_args(layout))) {
		ret = 1;
		goto out_shutdown;
	}

	flashrom_layout_set(fill_flash, layout);
	flashrom_flag_set(fill_flash, FLASHROM_FLAG_FORCE, !!force);
#if CONFIG_INTERNAL == 1
	flashrom_flag_set(fill_flash, FLASHROM_FLAG_FORCE_BOARDMISMATCH, !!force_boardmismatch);
#endif
	flashrom_flag_set(fill_flash, FLASHROM_FLAG_VERIFY_AFTER_WRITE, !dont_verify_it);
	flashrom_flag_set(fill_flash, FLASHROM_FLAG_VERIFY_WHOLE_CHIP, !dont_verify_all);

	if (print_status_reg) {
		verbose_screen++;
		if (fill_flash->chip->status_register) {
			for (enum status_register_num SRn = SR1; SRn <= top_status_register(fill_flash); SRn++)
				fill_flash->chip->status_register->print(fill_flash, SRn);
			fill_flash->chip->status_register->print_wp_mode(fill_flash);
			if (fill_flash->chip->wp)
				print_range_generic(fill_flash);
		} else
			cli_statreg_wp_support(fill_flash);
		goto out_shutdown;
	}

	if (print_wp_status) {
		verbose_screen++;
		if (fill_flash->chip->status_register || fill_flash->chip->wp) {
			msg_ginfo("WP status -\n");
			fill_flash->chip->status_register->print_wp_mode(fill_flash);
			if (fill_flash->chip->wp)
				print_range_generic(fill_flash);
		} else
			cli_statreg_wp_support(fill_flash);
		goto out_shutdown;
	}

	if (wp_list) {
		verbose_screen++;
		if (fill_flash->chip->wp) {
			msg_ginfo("Valid write protection ranges for chip \"%s\" are -\n",
				  fill_flash->chip->name);
			fill_flash->chip->wp->print_table(fill_flash);
		} else
			cli_statreg_wp_support(fill_flash);
		goto out_shutdown;
	}

	if (wp_disable) {
		verbose_screen++;
		if (fill_flash->chip->wp) {
			ret = fill_flash->chip->wp->disable(fill_flash);
		} else
			cli_statreg_wp_support(fill_flash);
		goto out_shutdown;
	}

	if (wp_set_range) {
		verbose_screen++;
		if (fill_flash->chip->wp) {
			char *endptr = NULL;
			uint8_t start_cmp, end_cmp, has_cmp = pos_bit(fill_flash, CMP) != -1;
			if (has_cmp)
				start_cmp = get_cmp(fill_flash);
			/* FIXME(hatim): Implement error checking */
			uint32_t start = strtoul(extract_param(&wp_set_range_opt, "start", ","), &endptr, 0);
			uint32_t len = strtoul(extract_param(&wp_set_range_opt, "len", ","), &endptr, 0);
			msg_ginfo("Trying to protect %d kB starting from address 0x%06x...\n", len, start);
			ret = fill_flash->chip->wp->set_range(fill_flash, start, len);
			if (has_cmp && start_cmp != (end_cmp = get_cmp(fill_flash)))
				msg_ginfo("CMP bit was %sset\n", end_cmp ? "" : "un");
			if (ret)
				msg_gerr("Failed to protect\n");
			else
				msg_ginfo("Protection successful!\n");
		} else
			cli_statreg_wp_support(fill_flash);
		goto out_shutdown;
	}

	if (wp_enable) {
		verbose_screen++;
		if (fill_flash->chip->status_register) {
			enum wp_mode wp_mode = WP_MODE_HARDWARE_UNPROTECTED;

			if (wp_mode_opt) {
				if (!strcasecmp(wp_mode_opt, "PERMANENT")) {
					wp_mode = WP_MODE_PERMANENT;
				} else if (!strcasecmp(wp_mode_opt, "POWER_CYCLE")) {
					wp_mode = WP_MODE_POWER_CYCLE;
				} else if (!strcasecmp(wp_mode_opt, "SOFTWARE")) {
					wp_mode = WP_MODE_SOFTWARE;
				} else if (!strcasecmp(wp_mode_opt, "HARDWARE")) {
					wp_mode = WP_MODE_HARDWARE_UNPROTECTED;
				} else {
					msg_gerr("Invalid write protection mode for status register(s)\n");
					cli_classic_abort_usage();
					ret = 1;
					goto out_shutdown;
				}
			}
			msg_ginfo("Setting write protection mode to %s ...\n",
				  wp_mode_opt ? wp_mode_opt : "HARDWARE");
			fill_flash->chip->status_register->set_wp_mode(fill_flash, wp_mode);
			ret = !(wp_mode == fill_flash->chip->status_register->get_wp_mode(fill_flash));
			msg_gerr("%s\n", ret ? "Failed" : "Success");
		} else
			cli_statreg_wp_support(fill_flash);
		goto out_shutdown;
	}

	/* FIXME: We should issue an unconditional chip reset here. This can be
	 * done once we have a .reset function in struct flashchip.
	 * Give the chip time to settle.
	 */
	programmer_delay(100000);
	if (read_it)
		ret = do_read(fill_flash, filename);
	else if (erase_it)
		ret = do_erase(fill_flash);
	else if (write_it)
		ret = do_write(fill_flash, filename);
	else if (verify_it)
		ret = do_verify(fill_flash, filename);

	flashrom_layout_release(layout);

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
