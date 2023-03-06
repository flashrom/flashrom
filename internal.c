/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Carl-Daniel Hailfinger
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

#include <strings.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "flash.h"
#include "programmer.h"
#include "platform/pci.h"

#if defined(__i386__) || defined(__x86_64__)
#include "hwaccess_x86_io.h"
#endif

bool force_boardmismatch = false;

enum chipbustype internal_buses_supported = BUS_NONE;


static int get_params(const struct programmer_cfg *cfg,
		bool *boardenable, bool *boardmismatch,
		bool *force_laptop, bool *not_a_laptop,
		char **board_vendor, char **board_model)
{
	char *arg;

	/* default values. */
	*force_laptop = false;
	*not_a_laptop = false;
	*board_vendor = NULL;
	*board_model = NULL;

	arg = extract_programmer_param_str(cfg, "boardenable");
	if (arg && !strcmp(arg,"force")) {
		*boardenable = true;
	} else if (arg && !strlen(arg)) {
		msg_perr("Missing argument for boardenable.\n");
		free(arg);
		return 1;
	} else if (arg) {
		msg_perr("Unknown argument for boardenable: %s\n", arg);
		free(arg);
		return 1;
	}
	free(arg);

	arg = extract_programmer_param_str(cfg, "boardmismatch");
	if (arg && !strcmp(arg,"force")) {
		*boardmismatch = true;
	} else if (arg && !strlen(arg)) {
		msg_perr("Missing argument for boardmismatch.\n");
		free(arg);
		return 1;
	} else if (arg) {
		msg_perr("Unknown argument for boardmismatch: %s\n", arg);
		free(arg);
		return 1;
	}
	free(arg);

	arg = extract_programmer_param_str(cfg, "laptop");
	if (arg && !strcmp(arg, "force_I_want_a_brick"))
		*force_laptop = true;
	else if (arg && !strcmp(arg, "this_is_not_a_laptop"))
		*not_a_laptop = true;
	else if (arg && !strlen(arg)) {
		msg_perr("Missing argument for laptop.\n");
		free(arg);
		return 1;
	} else if (arg) {
		msg_perr("Unknown argument for laptop: %s\n", arg);
		free(arg);
		return 1;
	}
	free(arg);

	arg = extract_programmer_param_str(cfg, "mainboard");
	if (arg && strlen(arg)) {
		if (board_parse_parameter(arg, board_vendor, board_model)) {
			free(arg);
			return 1;
		}
	} else if (arg && !strlen(arg)) {
		msg_perr("Missing argument for mainboard.\n");
		free(arg);
		return 1;
	}
	free(arg);

	return 0;
}

static void report_nonwl_laptop_detected(const struct board_cfg *bcfg)
{
	const int is_laptop = bcfg->is_laptop;
	const bool laptop_ok = bcfg->laptop_ok;

	if (!is_laptop || laptop_ok)
		return;

	msg_pinfo("========================================================================\n");
	if (is_laptop == 1) {
		msg_pinfo("You seem to be running flashrom on an unknown laptop. Some\n"
			  "internal buses have been disabled for safety reasons.\n\n");
	} else {
		msg_pinfo("You may be running flashrom on an unknown laptop. We could not\n"
			  "detect this for sure because your vendor has not set up the SMBIOS\n"
			  "tables correctly. Some internal buses have been disabled for\n"
			  "safety reasons. You can enforce using all buses by adding\n"
			  "  -p internal:laptop=this_is_not_a_laptop\n"
			  "to the command line, but please read the following warning if you\n"
			  "are not sure.\n\n");
	}
	msg_perr("Laptops, notebooks and netbooks are difficult to support and we\n"
		 "recommend to use the vendor flashing utility. The embedded controller\n"
		 "(EC) in these machines often interacts badly with flashing.\n"
		 "See the manpage and https://flashrom.org/Laptops for details.\n\n"
		 "If flash is shared with the EC, erase is guaranteed to brick your laptop\n"
		 "and write may brick your laptop.\n"
		 "Read and probe may irritate your EC and cause fan failure, backlight\n"
		 "failure and sudden poweroff.\n"
		 "You have been warned.\n"
		 "========================================================================\n");
}

static int internal_init(const struct programmer_cfg *cfg)
{
	int ret = 0;
	bool force_laptop;
	bool not_a_laptop;
	char *board_vendor;
	char *board_model;
#if defined(__i386__) || defined(__x86_64__)
	const char *cb_vendor = NULL;
	const char *cb_model = NULL;
#endif
	bool force_boardenable = false;
	struct board_cfg bcfg = {0};

	ret = get_params(cfg,
			 &force_boardenable, &force_boardmismatch,
			 &force_laptop, &not_a_laptop,
			 &board_vendor, &board_model);
	if (ret)
		return ret;

	/* Default to Parallel/LPC/FWH flash devices. If a known host controller
	 * is found, the host controller init routine sets the
	 * internal_buses_supported bitfield.
	 */
	internal_buses_supported = BUS_NONSPI;

	if (try_mtd(cfg) == 0) {
		ret = 0;
		goto internal_init_exit;
	}

	/* Initialize PCI access for flash enables */
	if (pci_init_common() != 0) {
		ret = 1;
		goto internal_init_exit;
	}

	if (processor_flash_enable()) {
		msg_perr("Processor detection/init failed.\n"
			 "Aborting.\n");
		ret = 1;
		goto internal_init_exit;
	}

#if defined(__i386__) || defined(__x86_64__)
	if (rget_io_perms()) {
		ret = 1;
		goto internal_init_exit;
	}

	if ((cb_parse_table(&cb_vendor, &cb_model) == 0) && (board_vendor != NULL) && (board_model != NULL)) {
		if (strcasecmp(board_vendor, cb_vendor) || strcasecmp(board_model, cb_model)) {
			msg_pwarn("Warning: The mainboard IDs set by -p internal:mainboard (%s:%s) do not\n"
				  "         match the current coreboot IDs of the mainboard (%s:%s).\n",
				  board_vendor, board_model, cb_vendor, cb_model);
			if (!force_boardmismatch) {
				ret = 1;
				goto internal_init_exit;
			}
			msg_pinfo("Continuing anyway.\n");
		}
	}

	bcfg.is_laptop = 2; /* Assume that we don't know by default. */

	dmi_init(&bcfg.is_laptop);

	/* In case Super I/O probing would cause pretty explosions. */
	board_handle_before_superio(&bcfg, force_boardenable);

	/* Probe for the Super I/O chip and fill global struct superio. */
	probe_superio();
#else
	/* FIXME: Enable cbtable searching on all non-x86 platforms supported
	 *        by coreboot.
	 * FIXME: Find a replacement for DMI on non-x86.
	 * FIXME: Enable Super I/O probing once port I/O is possible.
	 */
#endif

	/* Check laptop whitelist. */
	board_handle_before_laptop(&bcfg, force_boardenable);

	/*
	 * Disable all internal buses by default if we are not sure
	 * this isn't a laptop. Board-enables may override this,
	 * non-legacy buses (SPI and opaque atm) are probed anyway.
	 */
	if (bcfg.is_laptop && !(bcfg.laptop_ok || force_laptop || (not_a_laptop && bcfg.is_laptop == 2)))
		internal_buses_supported = BUS_NONE;

	/* try to enable it. Failure IS an option, since not all motherboards
	 * really need this to be done, etc., etc.
	 */
	struct programmer_cfg icfg = *cfg;
	icfg.bcfg = &bcfg;
	ret = chipset_flash_enable(&icfg);
	if (ret == -2) {
		msg_perr("WARNING: No chipset found. Flash detection "
			 "will most likely fail.\n");
	} else if (ret == ERROR_FLASHROM_FATAL) {
		goto internal_init_exit;
	}

#if defined(__i386__) || defined(__x86_64__)
	/* Probe unconditionally for ITE Super I/O chips. This enables LPC->SPI translation on IT87* and
	 * parallel writes on IT8705F. Also, this handles the manual chip select for Gigabyte's DualBIOS. */
	init_superio_ite(cfg);

	if (board_flash_enable(&bcfg,
            board_vendor, board_model, cb_vendor, cb_model, force_boardenable)) {
		msg_perr("Aborting to be safe.\n");
		ret = 1;
		goto internal_init_exit;
	}
#endif

	internal_par_init(internal_buses_supported);

	/* Report if a non-whitelisted laptop is detected that likely uses a legacy bus. */
	report_nonwl_laptop_detected(&bcfg);

	ret = 0;

internal_init_exit:
	free(board_vendor);
	free(board_model);

	return ret;
}

const struct programmer_entry programmer_internal = {
	.name			= "internal",
	.type			= OTHER,
	.devs.note		= NULL,
	.init			= internal_init,
};
