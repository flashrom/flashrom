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
 */

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <cli_classic.h>
#include "flash.h"
#include "flashchips.h"
#include "fmap.h"
#include "programmer.h"
#include "libflashrom.h"

enum {
	OPTION_IFD = 0x0100,
	OPTION_FMAP,
	OPTION_FMAP_FILE,
	OPTION_FLASH_CONTENTS,
	OPTION_FLASH_NAME,
	OPTION_FLASH_SIZE,
	OPTION_WP_STATUS,
	OPTION_WP_SET_RANGE,
	OPTION_WP_SET_REGION,
	OPTION_WP_ENABLE,
	OPTION_WP_DISABLE,
	OPTION_WP_LIST,
	OPTION_PROGRESS,
};

struct cli_options {
	bool read_it, extract_it, write_it, erase_it, verify_it;
	bool dont_verify_it, dont_verify_all;
	bool list_supported;
	char *filename;

	const struct programmer_entry *prog;
	char *pparam;

	bool ifd, fmap;
	struct flashrom_layout *layout;
	struct layout_include_args *include_args;
	char *layoutfile;
	char *fmapfile;

	unsigned int wp_start, wp_len;
	bool enable_wp, disable_wp, print_wp_status;
	bool set_wp_range, set_wp_region, print_wp_ranges;
	char *wp_region;

	bool force;
	bool flash_name, flash_size;
	bool show_progress;
	char *logfile;
	char *referencefile;
	const char *chip_to_probe;
};

static void cli_classic_usage(const char *name)
{
	printf("Usage: %s [-h|-R|-L|"
	       "\n\t-p <programmername>[:<parameters>] [-c <chipname>]\n"
	       "\t\t(--flash-name|--flash-size|\n"
	       "\t\t [-E|-x|(-r|-w|-v) <file>]\n"
	       "\t\t [(-l <layoutfile>|--ifd| --fmap|--fmap-file <file>) [-i <region>[:<file>]]...]\n"
	       "\t\t [-n] [-N] [-f])]\n"
	       "\t[-V[V[V]]] [-o <logfile>]\n\n", name);

	printf(" -h | --help                        print this help text\n"
	       " -R | --version                     print version (release)\n"
	       " -r | --read <file>                 read flash and save to <file>\n"
	       " -w | --write (<file>|-)            write <file> or the content provided\n"
	       "                                    on the standard input to flash\n"
	       " -v | --verify (<file>|-)           verify flash against <file>\n"
	       "                                    or the content provided on the standard input\n"
	       " -E | --erase                       erase flash memory\n"
	       " -V | --verbose                     more verbose output\n"
	       " -c | --chip <chipname>             probe only for specified flash chip\n"
	       " -f | --force                       force specific operations (see man page)\n"
	       " -n | --noverify                    don't auto-verify\n"
	       " -N | --noverify-all                verify included regions only (cf. -i)\n"
	       " -x | --extract                     extract regions to files\n"
	       " -l | --layout <layoutfile>         read ROM layout from <layoutfile>\n"
	       "      --wp-disable                  disable write protection\n"
	       "      --wp-enable                   enable write protection\n"
	       "      --wp-list                     list supported write protection ranges\n"
	       "      --wp-status                   show write protection status\n"
	       "      --wp-range=<start>,<len>      set write protection range (use --wp-range=0,0\n"
	       "                                    to unprotect the entire flash)\n"
	       "      --wp-region <region>          set write protection region\n"
	       "      --flash-name                  read out the detected flash name\n"
	       "      --flash-size                  read out the detected flash size\n"
	       "      --fmap                        read ROM layout from fmap embedded in ROM\n"
	       "      --fmap-file <fmapfile>        read ROM layout from fmap in <fmapfile>\n"
	       "      --ifd                         read layout from an Intel Firmware Descriptor\n"
	       " -i | --include <region>[:<file>]   only read/write image <region> from layout\n"
	       "                                    (optionally with data from <file>)\n"
	       "      --image <region>[:<file>]     deprecated, please use --include\n"
	       " -o | --output <logfile>            log output to <logfile>\n"
	       "      --flash-contents <ref-file>   assume flash contents to be <ref-file>\n"
	       " -L | --list-supported              print supported devices\n"
	       "      --progress                    show progress percentage on the standard output\n"
	       " -p | --programmer <name>[:<param>] specify the programmer device. One of\n");
	list_programmers_linebreak(4, 80, 0);
	printf(".\n\nYou can specify one of -h, -R, -L, "
	         "-E, -r, -w, -v or no operation.\n"
	       "If no operation is specified, flashrom will only probe for flash chips.\n");
}

static void cli_classic_abort_usage(const char *msg)
{
	if (msg)
		fprintf(stderr, "%s", msg);
	printf("Please run \"flashrom --help\" for usage info.\n");
	exit(1);
}

static void cli_classic_validate_singleop(int *operation_specified)
{
	if (++(*operation_specified) > 1) {
		cli_classic_abort_usage("More than one operation specified. Aborting.\n");
	}
}

static int check_filename(char *filename, const char *type)
{
	if (!filename || (filename[0] == '\0')) {
		fprintf(stderr, "Error: No %s file specified.\n", type);
		return 1;
	}
	/* Not an error, but maybe the user intended to specify a CLI option instead of a file name. */
	if (filename[0] == '-' && filename[1] != '\0')
		fprintf(stderr, "Warning: Supplied %s file name starts with -\n", type);
	return 0;
}

/* Ensure a file is open by means of fstat */
static bool check_file(FILE *file)
{
	struct stat statbuf;

	if (fstat(fileno(file), &statbuf) < 0)
		return false;
	return true;
}

static int parse_wp_range(unsigned int *start, unsigned int *len)
{
	char *endptr = NULL, *token = NULL;

	if (!optarg) {
		msg_gerr("Error: No wp-range values provided\n");
		return -1;
	}

	token = strtok(optarg, ",");
	if (!token) {
		msg_gerr("Error: Invalid wp-range argument format\n");
		return -1;
	}
	*start = strtoul(token, &endptr, 0);

	token = strtok(NULL, ",");
	if (!token) {
		msg_gerr("Error: Invalid wp-range argument format\n");
		return -1;
	}
	*len = strtoul(token, &endptr, 0);

	return 0;
}

static int print_wp_range(struct flashrom_flashctx *flash, size_t start, size_t len)
{
	/* Start address and length */
	msg_ginfo("start=0x%08zx length=0x%08zx ", start, len);

	/* Easily readable description like 'none' or 'lower 1/8' */
	size_t chip_len = flashrom_flash_getsize(flash);

	if (len == 0) {
		msg_ginfo("(none)");
	} else if (len == chip_len) {
		msg_ginfo("(all)");
	} else {
		const char *location = "";
		if (start == 0)
			location = "lower ";
		if (start == chip_len - len)
			location = "upper ";

		/* Remove common factors of 2 to simplify */
		/* the (range_len/chip_len) fraction. */
		while ((chip_len % 2) == 0 && (len % 2) == 0) {
			chip_len /= 2;
			len /= 2;
		}

		msg_ginfo("(%s%zu/%zu)", location, len, chip_len);
	}

	return 0;
}

static const char *get_wp_error_str(int err)
{
	switch (err) {
	case FLASHROM_WP_ERR_CHIP_UNSUPPORTED:
		return "WP operations are not implemented for this chip";
	case FLASHROM_WP_ERR_READ_FAILED:
		return "failed to read the current WP configuration";
	case FLASHROM_WP_ERR_WRITE_FAILED:
		return "failed to write the new WP configuration";
	case FLASHROM_WP_ERR_VERIFY_FAILED:
		return "unexpected WP configuration read back from chip";
	case FLASHROM_WP_ERR_MODE_UNSUPPORTED:
		return "the requested protection mode is not supported";
	case FLASHROM_WP_ERR_RANGE_UNSUPPORTED:
		return "the requested protection range is not supported";
	case FLASHROM_WP_ERR_RANGE_LIST_UNAVAILABLE:
		return "could not determine what protection ranges are available";
	case FLASHROM_WP_ERR_UNSUPPORTED_STATE:
		return "can't operate on current WP configuration of the chip";
	}
	return "unknown WP error";
}

static int wp_cli(
		struct flashctx *flash,
		bool enable_wp,
		bool disable_wp,
		bool print_wp_status,
		bool print_wp_ranges,
		bool set_wp_range,
		uint32_t wp_start,
		uint32_t wp_len)
{
	if (print_wp_ranges) {
		struct flashrom_wp_ranges *list;
		enum flashrom_wp_result ret = flashrom_wp_get_available_ranges(&list, flash);
		if (ret != FLASHROM_WP_OK) {
			msg_gerr("Failed to get list of protection ranges: %s\n",
				 get_wp_error_str(ret));
			return 1;
		}
		size_t count = flashrom_wp_ranges_get_count(list);

		msg_ginfo("Available protection ranges:\n");
		for (size_t i = 0; i < count; i++) {
			size_t start, len;

			flashrom_wp_ranges_get_range(&start, &len, list, i);
			msg_ginfo("\t");
			print_wp_range(flash, start, len);
			msg_ginfo("\n");
		}

		flashrom_wp_ranges_release(list);
	}

	if (set_wp_range || disable_wp || enable_wp) {
		enum flashrom_wp_mode old_mode = FLASHROM_WP_MODE_DISABLED;
		struct flashrom_wp_cfg *cfg = NULL;
		enum flashrom_wp_result ret = flashrom_wp_cfg_new(&cfg);

		if (ret == FLASHROM_WP_OK)
			ret = flashrom_wp_read_cfg(cfg, flash);

		if (ret == FLASHROM_WP_OK) {
			/* Store current WP mode for printing help text if */
			/* changing the cfg fails. */
			old_mode = flashrom_wp_get_mode(cfg);

			if (set_wp_range)
				flashrom_wp_set_range(cfg, wp_start, wp_len);

			if (disable_wp)
				flashrom_wp_set_mode(cfg, FLASHROM_WP_MODE_DISABLED);

			if (enable_wp)
				flashrom_wp_set_mode(cfg, FLASHROM_WP_MODE_HARDWARE);

			ret = flashrom_wp_write_cfg(flash, cfg);
		}

		flashrom_wp_cfg_release(cfg);

		if (ret != FLASHROM_WP_OK) {
			msg_gerr("Failed to apply new WP settings: %s\n",
				 get_wp_error_str(ret));

			/* Warn user if active WP is likely to have caused failure */
			if (ret == FLASHROM_WP_ERR_VERIFY_FAILED) {
				switch (old_mode) {
				case FLASHROM_WP_MODE_HARDWARE:
					msg_gerr("Note: hardware status register protection is enabled. "
						 "The chip's WP# pin must be set to an inactive voltage "
						 "level to be able to change the WP settings.\n");
					break;
				case FLASHROM_WP_MODE_POWER_CYCLE:
					msg_gerr("Note: power-cycle status register protection is enabled. "
						 "A power-off, power-on cycle is usually required to change "
						 "the chip's WP settings.\n");
					break;
				case FLASHROM_WP_MODE_PERMANENT:
					msg_gerr("Note: permanent status register protection is enabled. "
						 "The chip's WP settings cannot be modified.\n");
					break;
				default:
					break;
				}
			}

			return 1;
		}

		if (disable_wp)
			msg_ginfo("Disabled hardware protection\n");

		if (enable_wp)
			msg_ginfo("Enabled hardware protection\n");

		if (set_wp_range) {
			msg_ginfo("Activated protection range: ");
			print_wp_range(flash, wp_start, wp_len);
			msg_ginfo("\n");
		}
	}

	if (print_wp_status) {
		size_t start, len;
		enum flashrom_wp_mode mode;
		struct flashrom_wp_cfg *cfg = NULL;
		enum flashrom_wp_result ret = flashrom_wp_cfg_new(&cfg);

		if (ret == FLASHROM_WP_OK)
			ret = flashrom_wp_read_cfg(cfg, flash);

		if (ret != FLASHROM_WP_OK) {
			msg_gerr("Failed to get WP status: %s\n",
				 get_wp_error_str(ret));

			flashrom_wp_cfg_release(cfg);
			return 1;
		}

		flashrom_wp_get_range(&start, &len, cfg);
		mode = flashrom_wp_get_mode(cfg);
		flashrom_wp_cfg_release(cfg);

		msg_ginfo("Protection range: ");
		print_wp_range(flash, start, len);
		msg_ginfo("\n");

		msg_ginfo("Protection mode: ");
		switch (mode) {
		case FLASHROM_WP_MODE_DISABLED:
			msg_ginfo("disabled");
			break;
		case FLASHROM_WP_MODE_HARDWARE:
			msg_ginfo("hardware");
			break;
		case FLASHROM_WP_MODE_POWER_CYCLE:
			msg_ginfo("power_cycle");
			break;
		case FLASHROM_WP_MODE_PERMANENT:
			msg_ginfo("permanent");
			break;
		default:
			msg_ginfo("unknown");
			break;
		}
		msg_ginfo("\n");
	}

	return 0;
}

/**
 * @brief Reads content to buffer from one or more files.
 *
 * Reads content to supplied buffer from files. If a filename is specified for
 * individual regions using the partial read syntax ('-i <region>[:<filename>]')
 * then this will read file data into the corresponding region in the
 * supplied buffer.
 *
 * @param layout   The layout to be used.
 * @param buf      Chip-sized buffer to write data to
 * @return 0 on success
 */
static int read_buf_from_include_args(const struct flashrom_layout *const layout, unsigned char *buf)
{
	const struct romentry *entry = NULL;

	/*
	 * Content will be read from -i args, so they must not overlap since
	 * we need to know exactly what content to write to the ROM.
	 */
	if (included_regions_overlap(layout)) {
		msg_gerr("Error: Included regions must not overlap when writing.\n");
		return 1;
	}

	while ((entry = layout_next_included(layout, entry))) {
		if (!entry->file)
			continue;
		const struct flash_region *region = &entry->region;
		if (read_buf_from_file(buf + region->start,
				       region->end - region->start + 1, entry->file))
			return 1;
	}
	return 0;
}

/**
 * @brief Writes content from buffer to one or more files.
 *
 * Writes content from supplied buffer to files. If a filename is specified for
 * individual regions using the partial read syntax ('-i <region>[:<filename>]')
 * then this will write files using data from the corresponding region in the
 * supplied buffer.
 *
 * @param layout   The layout to be used.
 * @param buf      Chip-sized buffer to read data from
 * @return 0 on success
 */
static int write_buf_to_include_args(const struct flashrom_layout *const layout, unsigned char *buf)
{
	const struct romentry *entry = NULL;

	while ((entry = layout_next_included(layout, entry))) {
		if (!entry->file)
			continue;
		const struct flash_region *region = &entry->region;
		if (write_buf_to_file(buf + region->start,
				      region->end - region->start + 1, entry->file))
			return 1;
	}

	return 0;
}

static int do_read(struct flashctx *const flash, const char *const filename)
{
	int ret;

	unsigned long size = flashrom_flash_getsize(flash);
	unsigned char *buf = calloc(size, sizeof(unsigned char));
	if (!buf) {
		msg_gerr("Memory allocation failed!\n");
		return 1;
	}

	ret = flashrom_image_read(flash, buf, size);
	if (ret > 0)
		goto free_out;

	if (write_buf_to_include_args(get_layout(flash), buf)) {
		ret = 1;
		goto free_out;
	}
	if (filename)
		ret = write_buf_to_file(buf, size, filename);

free_out:
	free(buf);
	return ret;
}

static int do_extract(struct flashctx *const flash)
{
	prepare_layout_for_extraction(flash);
	return do_read(flash, NULL);
}

static int do_write(struct flashctx *const flash, const char *const filename, const char *const referencefile)
{
	const size_t flash_size = flashrom_flash_getsize(flash);
	int ret = 1;

	uint8_t *const newcontents = malloc(flash_size);
	uint8_t *const refcontents = referencefile ? malloc(flash_size) : NULL;

	if (!newcontents || (referencefile && !refcontents)) {
		msg_gerr("Out of memory!\n");
		goto _free_ret;
	}

	/* Read '-w' argument first... */
	if (read_buf_from_file(newcontents, flash_size, filename))
		goto _free_ret;
	/*
	 * ... then update newcontents with contents from files provided to '-i'
	 * args if needed.
	 */
	if (read_buf_from_include_args(get_layout(flash), newcontents))
		goto _free_ret;

	if (referencefile) {
		if (read_buf_from_file(refcontents, flash_size, referencefile))
			goto _free_ret;
	}

	ret = flashrom_image_write(flash, newcontents, flash_size, refcontents);

_free_ret:
	free(refcontents);
	free(newcontents);
	return ret;
}

static int do_verify(struct flashctx *const flash, const char *const filename)
{
	const size_t flash_size = flashrom_flash_getsize(flash);
	int ret = 1;

	uint8_t *const newcontents = malloc(flash_size);
	if (!newcontents) {
		msg_gerr("Out of memory!\n");
		goto _free_ret;
	}

	/* Read '-v' argument first... */
	if (read_buf_from_file(newcontents, flash_size, filename))
		goto _free_ret;
	/*
	 * ... then update newcontents with contents from files provided to '-i'
	 * args if needed.
	 */
	if (read_buf_from_include_args(get_layout(flash), newcontents))
		goto _free_ret;

	ret = flashrom_image_verify(flash, newcontents, flash_size);

_free_ret:
	free(newcontents);
	return ret;
}

/* Returns the number of buses commonly supported by the current programmer and flash chip where the latter
 * can not be completely accessed due to size/address limits of the programmer. */
static unsigned int count_max_decode_exceedings(const struct flashctx *flash,
		const struct decode_sizes *max_rom_decode_)
{
	unsigned int limitexceeded = 0;
	uint32_t size = flash->chip->total_size * 1024;
	enum chipbustype buses = flash->mst->buses_supported & flash->chip->bustype;

	if ((buses & BUS_PARALLEL) && (max_rom_decode_->parallel < size)) {
		limitexceeded++;
		msg_pdbg("Chip size %"PRIu32" kB is bigger than supported "
			 "size %"PRIu32" kB of chipset/board/programmer "
			 "for %s interface, "
			 "probe/read/erase/write may fail. ", size / 1024,
			 max_rom_decode_->parallel / 1024, "Parallel");
	}
	if ((buses & BUS_LPC) && (max_rom_decode_->lpc < size)) {
		limitexceeded++;
		msg_pdbg("Chip size %"PRIu32" kB is bigger than supported "
			 "size %"PRIu32" kB of chipset/board/programmer "
			 "for %s interface, "
			 "probe/read/erase/write may fail. ", size / 1024,
			 max_rom_decode_->lpc / 1024, "LPC");
	}
	if ((buses & BUS_FWH) && (max_rom_decode_->fwh < size)) {
		limitexceeded++;
		msg_pdbg("Chip size %"PRIu32" kB is bigger than supported "
			 "size %"PRIu32" kB of chipset/board/programmer "
			 "for %s interface, "
			 "probe/read/erase/write may fail. ", size / 1024,
			 max_rom_decode_->fwh / 1024, "FWH");
	}
	if ((buses & BUS_SPI) && (max_rom_decode_->spi < size)) {
		limitexceeded++;
		msg_pdbg("Chip size %"PRIu32" kB is bigger than supported "
			 "size %"PRIu32" kB of chipset/board/programmer "
			 "for %s interface, "
			 "probe/read/erase/write may fail. ", size / 1024,
			 max_rom_decode_->spi / 1024, "SPI");
	}
	return limitexceeded;
}

static void parse_options(int argc, char **argv, const char *optstring,
			  const struct option *long_options,
			  struct cli_options *options)
{
	const char *name;
	int namelen, opt;
	int option_index = 0, operation_specified = 0;

	/* FIXME: Delay all operation_specified checks until after command
	 * line parsing to allow --help overriding everything else.
	 */
	while ((opt = getopt_long(argc, argv, optstring,
				  long_options, &option_index)) != EOF) {
		switch (opt) {
		case 'r':
			cli_classic_validate_singleop(&operation_specified);
			options->filename = strdup(optarg);
			options->read_it = true;
			break;
		case 'w':
			cli_classic_validate_singleop(&operation_specified);
			options->filename = strdup(optarg);
			options->write_it = true;
			break;
		case 'v':
			//FIXME: gracefully handle superfluous -v
			cli_classic_validate_singleop(&operation_specified);
			if (options->dont_verify_it) {
				cli_classic_abort_usage("--verify and --noverify are mutually exclusive. Aborting.\n");
			}
			options->filename = strdup(optarg);
			options->verify_it = true;
			break;
		case 'n':
			if (options->verify_it) {
				cli_classic_abort_usage("--verify and --noverify are mutually exclusive. Aborting.\n");
			}
			options->dont_verify_it = true;
			break;
		case 'N':
			options->dont_verify_all = true;
			break;
		case 'x':
			cli_classic_validate_singleop(&operation_specified);
			options->extract_it = true;
			break;
		case 'c':
			options->chip_to_probe = strdup(optarg);
			break;
		case 'V':
			verbose_screen++;
			if (verbose_screen > FLASHROM_MSG_DEBUG2)
				verbose_logfile = verbose_screen;
			break;
		case 'E':
			cli_classic_validate_singleop(&operation_specified);
			options->erase_it = true;
			break;
		case 'f':
			options->force = true;
			break;
		case 'l':
			if (options->layoutfile)
				cli_classic_abort_usage("Error: --layout specified more than once. Aborting.\n");
			if (options->ifd)
				cli_classic_abort_usage("Error: --layout and --ifd both specified. Aborting.\n");
			if (options->fmap)
				cli_classic_abort_usage("Error: --layout and --fmap-file both specified. Aborting.\n");
			options->layoutfile = strdup(optarg);
			break;
		case OPTION_IFD:
			if (options->layoutfile)
				cli_classic_abort_usage("Error: --layout and --ifd both specified. Aborting.\n");
			if (options->fmap)
				cli_classic_abort_usage("Error: --fmap-file and --ifd both specified. Aborting.\n");
			options->ifd = true;
			break;
		case OPTION_FMAP_FILE:
			if (options->fmap)
				cli_classic_abort_usage("Error: --fmap or --fmap-file specified "
					"more than once. Aborting.\n");
			if (options->ifd)
				cli_classic_abort_usage("Error: --fmap-file and --ifd both specified. Aborting.\n");
			if (options->layoutfile)
				cli_classic_abort_usage("Error: --fmap-file and --layout both specified. Aborting.\n");
			options->fmapfile = strdup(optarg);
			options->fmap = true;
			break;
		case OPTION_FMAP:
			if (options->fmap)
				cli_classic_abort_usage("Error: --fmap or --fmap-file specified "
					"more than once. Aborting.\n");
			if (options->ifd)
				cli_classic_abort_usage("Error: --fmap and --ifd both specified. Aborting.\n");
			if (options->layoutfile)
				cli_classic_abort_usage("Error: --layout and --fmap both specified. Aborting.\n");
			options->fmap = true;
			break;
		case 'i':
			if (register_include_arg(&options->include_args, optarg))
				cli_classic_abort_usage(NULL);
			break;
		case OPTION_FLASH_CONTENTS:
			if (options->referencefile)
				cli_classic_abort_usage("Error: --flash-contents specified more than once."
							"Aborting.\n");
			options->referencefile = strdup(optarg);
			break;
		case OPTION_FLASH_NAME:
			cli_classic_validate_singleop(&operation_specified);
			options->flash_name = true;
			break;
		case OPTION_FLASH_SIZE:
			cli_classic_validate_singleop(&operation_specified);
			options->flash_size = true;
			break;
		case OPTION_WP_STATUS:
			options->print_wp_status = true;
			break;
		case OPTION_WP_LIST:
			options->print_wp_ranges = true;
			break;
		case OPTION_WP_SET_RANGE:
			if (parse_wp_range(&options->wp_start, &options->wp_len) < 0)
				cli_classic_abort_usage("Incorrect wp-range arguments provided.\n");

			options->set_wp_range = true;
			break;
		case OPTION_WP_SET_REGION:
			options->set_wp_region = true;
			options->wp_region = strdup(optarg);
			break;
		case OPTION_WP_ENABLE:
			options->enable_wp = true;
			break;
		case OPTION_WP_DISABLE:
			options->disable_wp = true;
			break;
		case 'L':
			cli_classic_validate_singleop(&operation_specified);
			options->list_supported = true;
			break;
		case 'p':
			if (options->prog != NULL) {
				cli_classic_abort_usage("Error: --programmer specified "
					"more than once. You can separate "
					"multiple\nparameters for a programmer "
					"with \",\". Please see the man page "
					"for details.\n");
			}
			size_t p;
			for (p = 0; p < programmer_table_size; p++) {
				name = programmer_table[p]->name;
				namelen = strlen(name);
				if (strncmp(optarg, name, namelen) == 0) {
					switch (optarg[namelen]) {
					case ':':
						options->pparam = strdup(optarg + namelen + 1);
						if (!strlen(options->pparam)) {
							free(options->pparam);
							options->pparam = NULL;
						}
						options->prog = programmer_table[p];
						break;
					case '\0':
						options->prog = programmer_table[p];
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
			if (options->prog == NULL) {
				fprintf(stderr, "Error: Unknown programmer \"%s\". Valid choices are:\n",
					optarg);
				list_programmers_linebreak(0, 80, 0);
				msg_ginfo(".\n");
				cli_classic_abort_usage(NULL);
			}
			break;
		case 'R':
			/* print_version() is always called during startup. */
			cli_classic_validate_singleop(&operation_specified);
			exit(0);
			break;
		case 'h':
			cli_classic_validate_singleop(&operation_specified);
			cli_classic_usage(argv[0]);
			exit(0);
			break;
		case 'o':
			if (options->logfile) {
				fprintf(stderr, "Warning: -o/--output specified multiple times.\n");
				free(options->logfile);
			}

			options->logfile = strdup(optarg);
			if (options->logfile[0] == '\0') {
				cli_classic_abort_usage("No log filename specified.\n");
			}
			break;
		case OPTION_PROGRESS:
			options->show_progress = true;
			break;
		default:
			cli_classic_abort_usage(NULL);
			break;
		}
	}

	if (optind < argc)
		cli_classic_abort_usage("Error: Extra parameter found.\n");
}

static void free_options(struct cli_options *options)
{
	cleanup_include_args(&options->include_args);
	free(options->filename);
	free(options->fmapfile);
	free(options->referencefile);
	free(options->layoutfile);
	free(options->pparam);
	free(options->wp_region);
	free(options->logfile);
	free((char *)options->chip_to_probe);
}

int main(int argc, char *argv[])
{
	const struct flashchip *chip = NULL;
	/* Probe for up to eight flash chips. */
	struct flashctx flashes[8] = {{0}};
	struct flashctx *fill_flash;
	char *tempstr = NULL;
	int startchip = -1, chipcount = 0;
	int i, j;
	int ret = 0;

	struct cli_options options = { 0 };
	static const char optstring[] = "r:Rw:v:nNVEfc:l:i:p:Lzho:x";
	static const struct option long_options[] = {
		{"read",		1, NULL, 'r'},
		{"write",		1, NULL, 'w'},
		{"erase",		0, NULL, 'E'},
		{"verify",		1, NULL, 'v'},
		{"noverify",		0, NULL, 'n'},
		{"noverify-all",	0, NULL, 'N'},
		{"extract",		0, NULL, 'x'},
		{"chip",		1, NULL, 'c'},
		{"verbose",		0, NULL, 'V'},
		{"force",		0, NULL, 'f'},
		{"layout",		1, NULL, 'l'},
		{"ifd",			0, NULL, OPTION_IFD},
		{"fmap",		0, NULL, OPTION_FMAP},
		{"fmap-file",		1, NULL, OPTION_FMAP_FILE},
		{"image",		1, NULL, 'i'}, // (deprecated): back compatibility.
		{"include",		1, NULL, 'i'},
		{"flash-contents",	1, NULL, OPTION_FLASH_CONTENTS},
		{"flash-name",		0, NULL, OPTION_FLASH_NAME},
		{"flash-size",		0, NULL, OPTION_FLASH_SIZE},
		{"get-size",		0, NULL, OPTION_FLASH_SIZE}, // (deprecated): back compatibility.
		{"wp-status",		0, NULL, OPTION_WP_STATUS},
		{"wp-list",		0, NULL, OPTION_WP_LIST},
		{"wp-range",		1, NULL, OPTION_WP_SET_RANGE},
		{"wp-region",		1, NULL, OPTION_WP_SET_REGION},
		{"wp-enable",		0, NULL, OPTION_WP_ENABLE},
		{"wp-disable",		0, NULL, OPTION_WP_DISABLE},
		{"list-supported",	0, NULL, 'L'},
		{"programmer",		1, NULL, 'p'},
		{"help",		0, NULL, 'h'},
		{"version",		0, NULL, 'R'},
		{"output",		1, NULL, 'o'},
		{"progress",		0, NULL, OPTION_PROGRESS},
		{NULL,			0, NULL, 0},
	};

	/*
	 * Safety-guard against a user who has (mistakenly) closed
	 * stdout or stderr before exec'ing flashrom.  We disable
	 * logging in this case to prevent writing log data to a flash
	 * chip when a flash device gets opened with fd 1 or 2.
	 */
	if (check_file(stdout) && check_file(stderr)) {
		flashrom_set_log_callback(&flashrom_print_cb);
	}

	print_version();
	print_banner();

	setbuf(stdout, NULL);

	parse_options(argc, argv, optstring, long_options, &options);

	if ((options.read_it | options.write_it | options.verify_it) && check_filename(options.filename, "image"))
		cli_classic_abort_usage(NULL);
	if (options.layoutfile && check_filename(options.layoutfile, "layout"))
		cli_classic_abort_usage(NULL);
	if (options.fmapfile && check_filename(options.fmapfile, "fmap"))
		cli_classic_abort_usage(NULL);
	if (options.referencefile && check_filename(options.referencefile, "reference"))
		cli_classic_abort_usage(NULL);
	if (options.logfile && check_filename(options.logfile, "log"))
		cli_classic_abort_usage(NULL);
	if (options.logfile && open_logfile(options.logfile))
		cli_classic_abort_usage(NULL);

	if (options.list_supported) {
		if (print_supported())
			ret = 1;
		goto out;
	}

	start_logging();

	print_buildinfo();
	msg_gdbg("Command line (%i args):", argc - 1);
	for (i = 0; i < argc; i++) {
		msg_gdbg(" %s", argv[i]);
	}
	msg_gdbg("\n");

	if (options.layoutfile && layout_from_file(&options.layout, options.layoutfile)) {
		ret = 1;
		goto out;
	}

	if (!options.ifd && !options.fmap && process_include_args(options.layout, options.include_args)) {
		ret = 1;
		goto out;
	}
	/* Does a chip with the requested name exist in the flashchips array? */
	if (options.chip_to_probe) {
		for (chip = flashchips; chip && chip->name; chip++)
			if (!strcmp(chip->name, options.chip_to_probe))
				break;
		if (!chip || !chip->name) {
			msg_cerr("Error: Unknown chip '%s' specified.\n", options.chip_to_probe);
			msg_gerr("Run flashrom -L to view the hardware supported in this flashrom version.\n");
			ret = 1;
			goto out;
		}
		/* Keep chip around for later usage in case a forced read is requested. */
	}

	if (options.prog == NULL) {
		const struct programmer_entry *const default_programmer = CONFIG_DEFAULT_PROGRAMMER_NAME;

		if (default_programmer) {
			options.prog = default_programmer;
			/* We need to strdup here because we free(pparam) unconditionally later. */
			options.pparam = strdup(CONFIG_DEFAULT_PROGRAMMER_ARGS);
			msg_pinfo("Using default programmer \"%s\" with arguments \"%s\".\n",
				  default_programmer->name, options.pparam);
		} else {
			msg_perr("Please select a programmer with the --programmer parameter.\n"
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
	if (flashrom_init(1))
		exit(1);

	if (programmer_init(options.prog, options.pparam)) {
		msg_perr("Error: Programmer initialization failed.\n");
		ret = 1;
		goto out_shutdown;
	}
	tempstr = flashbuses_to_text(get_buses_supported());
	msg_pdbg("The following protocols are supported: %s.\n", tempstr ? tempstr : "?");
	free(tempstr);

	for (j = 0; j < registered_master_count; j++) {
		startchip = 0;
		while (chipcount < (int)ARRAY_SIZE(flashes)) {
			startchip = probe_flash(&registered_masters[j], startchip, &flashes[chipcount], 0, options.chip_to_probe);
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
		if (!options.force || !options.chip_to_probe) {
			msg_cinfo("Note: flashrom can never write if the flash chip isn't found "
				  "automatically.\n");
		}
		if (options.force && options.read_it && options.chip_to_probe) {
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
				startchip = probe_flash(mst, 0, &flashes[0], 1, options.chip_to_probe);
				if (startchip != -1)
					break;
			}
			if (startchip == -1) {
				// FIXME: This should never happen! Ask for a bug report?
				msg_cinfo("Probing for flash chip '%s' failed.\n", options.chip_to_probe);
				ret = 1;
				goto out_shutdown;
			}
			msg_cinfo("Please note that forced reads most likely contain garbage.\n");
			flashrom_flag_set(&flashes[0], FLASHROM_FLAG_FORCE, options.force);
			ret = do_read(&flashes[0], options.filename);
			free(flashes[0].chip);
			goto out_shutdown;
		}
		ret = 1;
		goto out_shutdown;
	} else if (!options.chip_to_probe) {
		/* repeat for convenience when looking at foreign logs */
		tempstr = flashbuses_to_text(flashes[0].chip->bustype);
		msg_gdbg("Found %s flash chip \"%s\" (%d kB, %s).\n",
			 flashes[0].chip->vendor, flashes[0].chip->name, flashes[0].chip->total_size,
			 tempstr ? tempstr : "?");
		free(tempstr);
	}

	fill_flash = &flashes[0];

	unsigned int progress_user_data[FLASHROM_PROGRESS_NR];
	struct flashrom_progress progress_state = {
		 .user_data = progress_user_data
	};
	if (options.show_progress)
		flashrom_set_progress_callback(fill_flash, &flashrom_progress_cb, &progress_state);

	print_chip_support_status(fill_flash->chip);

	unsigned int limitexceeded = count_max_decode_exceedings(fill_flash, &max_rom_decode);
	if (limitexceeded > 0 && !options.force) {
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

	const bool any_wp_op =
		options.set_wp_range || options.set_wp_region || options.enable_wp ||
		options.disable_wp || options.print_wp_status || options.print_wp_ranges;

	const bool any_op = options.read_it || options.write_it || options.verify_it ||
		options.erase_it || options.flash_name || options.flash_size ||
		options.extract_it || any_wp_op;

	if (!any_op) {
		msg_ginfo("No operations were specified.\n");
		goto out_shutdown;
	}

	if (options.enable_wp && options.disable_wp) {
		msg_ginfo("Error: --wp-enable and --wp-disable are mutually exclusive\n");
		ret = 1;
		goto out_shutdown;
	}
	if (options.set_wp_range && options.set_wp_region) {
		msg_gerr("Error: Cannot use both --wp-range and --wp-region simultaneously.\n");
		ret = 1;
		goto out_shutdown;
	}

	if (options.flash_name) {
		if (fill_flash->chip->vendor && fill_flash->chip->name) {
			printf("vendor=\"%s\" name=\"%s\"\n",
				fill_flash->chip->vendor,
				fill_flash->chip->name);
		} else {
			ret = -1;
		}
		goto out_shutdown;
	}

	if (options.flash_size) {
		printf("%zu\n", flashrom_flash_getsize(fill_flash));
		goto out_shutdown;
	}

	if (options.ifd && (flashrom_layout_read_from_ifd(&options.layout, fill_flash, NULL, 0) ||
			   process_include_args(options.layout, options.include_args))) {
		ret = 1;
		goto out_shutdown;
	} else if (options.fmap && options.fmapfile) {
		struct stat s;
		if (stat(options.fmapfile, &s) != 0) {
			msg_gerr("Failed to stat fmapfile \"%s\"\n", options.fmapfile);
			ret = 1;
			goto out_shutdown;
		}

		size_t fmapfile_size = s.st_size;
		uint8_t *fmapfile_buffer = malloc(fmapfile_size);
		if (!fmapfile_buffer) {
			ret = 1;
			goto out_shutdown;
		}

		if (read_buf_from_file(fmapfile_buffer, fmapfile_size, options.fmapfile)) {
			ret = 1;
			free(fmapfile_buffer);
			goto out_shutdown;
		}

		if (flashrom_layout_read_fmap_from_buffer(&options.layout, fill_flash, fmapfile_buffer, fmapfile_size) ||
		    process_include_args(options.layout, options.include_args)) {
			ret = 1;
			free(fmapfile_buffer);
			goto out_shutdown;
		}
		free(fmapfile_buffer);
	} else if (options.fmap && (flashrom_layout_read_fmap_from_rom(&options.layout, fill_flash, 0,
				flashrom_flash_getsize(fill_flash)) ||
				process_include_args(options.layout, options.include_args))) {
		ret = 1;
		goto out_shutdown;
	}
	flashrom_layout_set(fill_flash, options.layout);

	if (any_wp_op) {
		if (options.set_wp_region && options.wp_region) {
			if (!options.layout) {
				msg_gerr("Error: A flash layout must be specified to use --wp-region.\n");
				ret = 1;
				goto out_release;
			}

			ret = flashrom_layout_get_region_range(options.layout, options.wp_region, &options.wp_start, &options.wp_len);
			if (ret) {
				msg_gerr("Error: Region %s not found in flash layout.\n", options.wp_region);
				goto out_release;
			}
			options.set_wp_range = true;
		}
		ret = wp_cli(
			fill_flash,
			options.enable_wp,
			options.disable_wp,
			options.print_wp_status,
			options.print_wp_ranges,
			options.set_wp_range,
			options.wp_start,
			options.wp_len
		);
		if (ret)
			goto out_release;
	}

	flashrom_flag_set(fill_flash, FLASHROM_FLAG_FORCE, options.force);
#if CONFIG_INTERNAL == 1
	flashrom_flag_set(fill_flash, FLASHROM_FLAG_FORCE_BOARDMISMATCH, force_boardmismatch);
#endif
	flashrom_flag_set(fill_flash, FLASHROM_FLAG_VERIFY_AFTER_WRITE, !options.dont_verify_it);
	flashrom_flag_set(fill_flash, FLASHROM_FLAG_VERIFY_WHOLE_CHIP, !options.dont_verify_all);

	/* FIXME: We should issue an unconditional chip reset here. This can be
	 * done once we have a .reset function in struct flashchip.
	 * Give the chip time to settle.
	 */
	programmer_delay(fill_flash, 100000);
	if (options.read_it)
		ret = do_read(fill_flash, options.filename);
	else if (options.extract_it)
		ret = do_extract(fill_flash);
	else if (options.erase_it) {
		ret = flashrom_flash_erase(fill_flash);
		/*
		 * FIXME: Do we really want the scary warning if erase failed?
		 * After all, after erase the chip is either blank or partially
		 * blank or it has the old contents. A blank chip won't boot,
		 * so if the user wanted erase and reboots afterwards, the user
		 * knows very well that booting won't work.
		 */
		if (ret)
			emergency_help_message();
	}
	else if (options.write_it)
		ret = do_write(fill_flash, options.filename, options.referencefile);
	else if (options.verify_it)
		ret = do_verify(fill_flash, options.filename);

out_release:
	flashrom_layout_release(options.layout);
out_shutdown:
	flashrom_programmer_shutdown(NULL);
out:
	for (i = 0; i < chipcount; i++) {
		flashrom_layout_release(flashes[i].default_layout);
		free(flashes[i].chip);
	}

	free_options(&options);
	ret |= close_logfile();
	return ret;
}
