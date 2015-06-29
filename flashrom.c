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

#include <stdio.h>
#include <sys/types.h>
#ifndef __LIBPAYLOAD__
#include <fcntl.h>
#include <sys/stat.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <getopt.h>
#if HAVE_UTSNAME == 1
#include <sys/utsname.h>
#endif
#include "flash.h"
#include "flashchips.h"
#include "programmer.h"
#include "hwaccess.h"

const char flashrom_version[] = FLASHROM_VERSION;
const char *chip_to_probe = NULL;

static enum programmer programmer = PROGRAMMER_INVALID;
static const char *programmer_param = NULL;

/*
 * Programmers supporting multiple buses can have differing size limits on
 * each bus. Store the limits for each bus in a common struct.
 */
struct decode_sizes max_rom_decode;

/* If nonzero, used as the start address of bottom-aligned flash. */
unsigned long flashbase;

/* Is writing allowed with this programmer? */
int programmer_may_write;

const struct programmer_entry programmer_table[] = {
#if CONFIG_INTERNAL == 1
	{
		.name			= "internal",
		.type			= OTHER,
		.devs.note		= NULL,
		.init			= internal_init,
		.map_flash_region	= physmap,
		.unmap_flash_region	= physunmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_DUMMY == 1
	{
		.name			= "dummy",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "Dummy device, does nothing and logs all accesses\n",
		.init			= dummy_init,
		.map_flash_region	= dummy_map,
		.unmap_flash_region	= dummy_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NIC3COM == 1
	{
		.name			= "nic3com",
		.type			= PCI,
		.devs.dev		= nics_3com,
		.init			= nic3com_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICREALTEK == 1
	{
		/* This programmer works for Realtek RTL8139 and SMC 1211. */
		.name			= "nicrealtek",
		.type			= PCI,
		.devs.dev		= nics_realtek,
		.init			= nicrealtek_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICNATSEMI == 1
	{
		.name			= "nicnatsemi",
		.type			= PCI,
		.devs.dev		= nics_natsemi,
		.init			= nicnatsemi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_GFXNVIDIA == 1
	{
		.name			= "gfxnvidia",
		.type			= PCI,
		.devs.dev		= gfx_nvidia,
		.init			= gfxnvidia_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_DRKAISER == 1
	{
		.name			= "drkaiser",
		.type			= PCI,
		.devs.dev		= drkaiser_pcidev,
		.init			= drkaiser_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_SATASII == 1
	{
		.name			= "satasii",
		.type			= PCI,
		.devs.dev		= satas_sii,
		.init			= satasii_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_ATAHPT == 1
	{
		.name			= "atahpt",
		.type			= PCI,
		.devs.dev		= ata_hpt,
		.init			= atahpt_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_ATAVIA == 1
	{
		.name			= "atavia",
		.type			= PCI,
		.devs.dev		= ata_via,
		.init			= atavia_init,
		.map_flash_region	= atavia_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_IT8212 == 1
	{
		.name			= "it8212",
		.type			= PCI,
		.devs.dev		= devs_it8212,
		.init			= it8212_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_FT2232_SPI == 1
	{
		.name			= "ft2232_spi",
		.type			= USB,
		.devs.dev		= devs_ft2232spi,
		.init			= ft2232_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_SERPROG == 1
	{
		.name			= "serprog",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "All programmer devices speaking the serprog protocol\n",
		.init			= serprog_init,
		.map_flash_region	= serprog_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= serprog_delay,
	},
#endif

#if CONFIG_BUSPIRATE_SPI == 1
	{
		.name			= "buspirate_spi",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "Dangerous Prototypes Bus Pirate\n",
		.init			= buspirate_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_DEDIPROG == 1
	{
		.name			= "dediprog",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "Dediprog SF100\n",
		.init			= dediprog_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_RAYER_SPI == 1
	{
		.name			= "rayer_spi",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "RayeR parallel port programmer\n",
		.init			= rayer_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_PONY_SPI == 1
	{
		.name			= "pony_spi",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "Programmers compatible with SI-Prog, serbang or AJAWe\n",
		.init			= pony_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICINTEL == 1
	{
		.name			= "nicintel",
		.type			= PCI,
		.devs.dev		= nics_intel,
		.init			= nicintel_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICINTEL_SPI == 1
	{
		.name			= "nicintel_spi",
		.type			= PCI,
		.devs.dev		= nics_intel_spi,
		.init			= nicintel_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICINTEL_EEPROM == 1
	{
		.name			= "nicintel_eeprom",
		.type			= PCI,
		.devs.dev		= nics_intel_ee,
		.init			= nicintel_ee_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_OGP_SPI == 1
	{
		.name			= "ogp_spi",
		.type			= PCI,
		.devs.dev		= ogp_spi,
		.init			= ogp_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_SATAMV == 1
	{
		.name			= "satamv",
		.type			= PCI,
		.devs.dev		= satas_mv,
		.init			= satamv_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_LINUX_SPI == 1
	{
		.name			= "linux_spi",
		.type			= OTHER,
		.devs.note		= "Device files /dev/spidev*.*\n",
		.init			= linux_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_USBBLASTER_SPI == 1
	{
		.name			= "usbblaster_spi",
		.type			= USB,
		.devs.dev		= devs_usbblasterspi,
		.init			= usbblaster_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_MSTARDDC_SPI == 1
	{
		.name			= "mstarddc_spi",
		.type			= OTHER,
		.devs.note		= "MSTAR DDC devices addressable via /dev/i2c-* on Linux.\n",
		.init			= mstarddc_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_PICKIT2_SPI == 1
	{
		.name			= "pickit2_spi",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "Microchip PICkit2\n",
		.init			= pickit2_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

	{0}, /* This entry corresponds to PROGRAMMER_INVALID. */
};

#define SHUTDOWN_MAXFN 32
static int shutdown_fn_count = 0;
struct shutdown_func_data {
	int (*func) (void *data);
	void *data;
} static shutdown_fn[SHUTDOWN_MAXFN];
/* Initialize to 0 to make sure nobody registers a shutdown function before
 * programmer init.
 */
static int may_register_shutdown = 0;

/* Did we change something or was every erase/write skipped (if any)? */
static bool all_skipped = true;

static int check_block_eraser(const struct flashctx *flash, int k, int log);

int shutdown_free(void *data)
{
	free(data);
	return 0;
}

/* Register a function to be executed on programmer shutdown.
 * The advantage over atexit() is that you can supply a void pointer which will
 * be used as parameter to the registered function upon programmer shutdown.
 * This pointer can point to arbitrary data used by said function, e.g. undo
 * information for GPIO settings etc. If unneeded, set data=NULL.
 * Please note that the first (void *data) belongs to the function signature of
 * the function passed as first parameter.
 */
int register_shutdown(int (*function) (void *data), void *data)
{
	if (shutdown_fn_count >= SHUTDOWN_MAXFN) {
		msg_perr("Tried to register more than %i shutdown functions.\n",
			 SHUTDOWN_MAXFN);
		return 1;
	}
	if (!may_register_shutdown) {
		msg_perr("Tried to register a shutdown function before "
			 "programmer init.\n");
		return 1;
	}
	shutdown_fn[shutdown_fn_count].func = function;
	shutdown_fn[shutdown_fn_count].data = data;
	shutdown_fn_count++;

	return 0;
}

int programmer_init(enum programmer prog, const char *param)
{
	int ret;

	if (prog >= PROGRAMMER_INVALID) {
		msg_perr("Invalid programmer specified!\n");
		return -1;
	}
	programmer = prog;
	/* Initialize all programmer specific data. */
	/* Default to unlimited decode sizes. */
	max_rom_decode = (const struct decode_sizes) {
		.parallel	= 0xffffffff,
		.lpc		= 0xffffffff,
		.fwh		= 0xffffffff,
		.spi		= 0xffffffff,
	};
	/* Default to top aligned flash at 4 GB. */
	flashbase = 0;
	/* Registering shutdown functions is now allowed. */
	may_register_shutdown = 1;
	/* Default to allowing writes. Broken programmers set this to 0. */
	programmer_may_write = 1;

	programmer_param = param;
	msg_pdbg("Initializing %s programmer\n", programmer_table[programmer].name);
	ret = programmer_table[programmer].init();
	if (programmer_param && strlen(programmer_param)) {
		if (ret != 0) {
			/* It is quite possible that any unhandled programmer parameter would have been valid,
			 * but an error in actual programmer init happened before the parameter was evaluated.
			 */
			msg_pwarn("Unhandled programmer parameters (possibly due to another failure): %s\n",
				  programmer_param);
		} else {
			/* Actual programmer init was successful, but the user specified an invalid or unusable
			 * (for the current programmer configuration) parameter.
			 */
			msg_perr("Unhandled programmer parameters: %s\n", programmer_param);
			msg_perr("Aborting.\n");
			ret = ERROR_FATAL;
		}
	}
	return ret;
}

/** Calls registered shutdown functions and resets internal programmer-related variables.
 * Calling it is safe even without previous initialization, but further interactions with programmer support
 * require a call to programmer_init() (afterwards).
 *
 * @return The OR-ed result values of all shutdown functions (i.e. 0 on success). */
int programmer_shutdown(void)
{
	int ret = 0;

	/* Registering shutdown functions is no longer allowed. */
	may_register_shutdown = 0;
	while (shutdown_fn_count > 0) {
		int i = --shutdown_fn_count;
		ret |= shutdown_fn[i].func(shutdown_fn[i].data);
	}

	programmer_param = NULL;
	registered_master_count = 0;

	return ret;
}

void *programmer_map_flash_region(const char *descr, uintptr_t phys_addr, size_t len)
{
	void *ret = programmer_table[programmer].map_flash_region(descr, phys_addr, len);
	msg_gspew("%s: mapping %s from 0x%0*" PRIxPTR " to 0x%0*" PRIxPTR "\n",
		  __func__, descr, PRIxPTR_WIDTH, phys_addr, PRIxPTR_WIDTH, (uintptr_t) ret);
	return ret;
}

void programmer_unmap_flash_region(void *virt_addr, size_t len)
{
	programmer_table[programmer].unmap_flash_region(virt_addr, len);
	msg_gspew("%s: unmapped 0x%0*" PRIxPTR "\n", __func__, PRIxPTR_WIDTH, (uintptr_t)virt_addr);
}

void chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	flash->mst->par.chip_writeb(flash, val, addr);
}

void chip_writew(const struct flashctx *flash, uint16_t val, chipaddr addr)
{
	flash->mst->par.chip_writew(flash, val, addr);
}

void chip_writel(const struct flashctx *flash, uint32_t val, chipaddr addr)
{
	flash->mst->par.chip_writel(flash, val, addr);
}

void chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len)
{
	flash->mst->par.chip_writen(flash, buf, addr, len);
}

uint8_t chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	return flash->mst->par.chip_readb(flash, addr);
}

uint16_t chip_readw(const struct flashctx *flash, const chipaddr addr)
{
	return flash->mst->par.chip_readw(flash, addr);
}

uint32_t chip_readl(const struct flashctx *flash, const chipaddr addr)
{
	return flash->mst->par.chip_readl(flash, addr);
}

void chip_readn(const struct flashctx *flash, uint8_t *buf, chipaddr addr,
		size_t len)
{
	flash->mst->par.chip_readn(flash, buf, addr, len);
}

void programmer_delay(unsigned int usecs)
{
	if (usecs > 0)
		programmer_table[programmer].delay(usecs);
}

int read_memmapped(struct flashctx *flash, uint8_t *buf, unsigned int start,
		   int unsigned len)
{
	chip_readn(flash, buf, flash->virtual_memory + start, len);

	return 0;
}

/* This is a somewhat hacked function similar in some ways to strtok().
 * It will look for needle with a subsequent '=' in haystack, return a copy of
 * needle and remove everything from the first occurrence of needle to the next
 * delimiter from haystack.
 */
char *extract_param(const char *const *haystack, const char *needle, const char *delim)
{
	char *param_pos, *opt_pos, *rest;
	char *opt = NULL;
	int optlen;
	int needlelen;

	needlelen = strlen(needle);
	if (!needlelen) {
		msg_gerr("%s: empty needle! Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		return NULL;
	}
	/* No programmer parameters given. */
	if (*haystack == NULL)
		return NULL;
	param_pos = strstr(*haystack, needle);
	do {
		if (!param_pos)
			return NULL;
		/* Needle followed by '='? */
		if (param_pos[needlelen] == '=') {
			
			/* Beginning of the string? */
			if (param_pos == *haystack)
				break;
			/* After a delimiter? */
			if (strchr(delim, *(param_pos - 1)))
				break;
		}
		/* Continue searching. */
		param_pos++;
		param_pos = strstr(param_pos, needle);
	} while (1);

	if (param_pos) {
		/* Get the string after needle and '='. */
		opt_pos = param_pos + needlelen + 1;
		optlen = strcspn(opt_pos, delim);
		/* Return an empty string if the parameter was empty. */
		opt = malloc(optlen + 1);
		if (!opt) {
			msg_gerr("Out of memory!\n");
			exit(1);
		}
		strncpy(opt, opt_pos, optlen);
		opt[optlen] = '\0';
		rest = opt_pos + optlen;
		/* Skip all delimiters after the current parameter. */
		rest += strspn(rest, delim);
		memmove(param_pos, rest, strlen(rest) + 1);
		/* We could shrink haystack, but the effort is not worth it. */
	}

	return opt;
}

char *extract_programmer_param(const char *param_name)
{
	return extract_param(&programmer_param, param_name, ",");
}

/* Returns the number of well-defined erasers for a chip. */
static unsigned int count_usable_erasers(const struct flashctx *flash)
{
	unsigned int usable_erasefunctions = 0;
	int k;
	for (k = 0; k < NUM_ERASEFUNCTIONS; k++) {
		if (!check_block_eraser(flash, k, 0))
			usable_erasefunctions++;
	}
	return usable_erasefunctions;
}

static int compare_range(const uint8_t *wantbuf, const uint8_t *havebuf, unsigned int start, unsigned int len)
{
	int ret = 0, failcount = 0;
	unsigned int i;
	for (i = 0; i < len; i++) {
		if (wantbuf[i] != havebuf[i]) {
			/* Only print the first failure. */
			if (!failcount++)
				msg_cerr("FAILED at 0x%08x! Expected=0x%02x, Found=0x%02x,",
					 start + i, wantbuf[i], havebuf[i]);
		}
	}
	if (failcount) {
		msg_cerr(" failed byte count from 0x%08x-0x%08x: 0x%x\n",
			 start, start + len - 1, failcount);
		ret = -1;
	}
	return ret;
}

/* start is an offset to the base address of the flash chip */
int check_erased_range(struct flashctx *flash, unsigned int start,
		       unsigned int len)
{
	int ret;
	uint8_t *cmpbuf = malloc(len);

	if (!cmpbuf) {
		msg_gerr("Could not allocate memory!\n");
		exit(1);
	}
	memset(cmpbuf, 0xff, len);
	ret = verify_range(flash, cmpbuf, start, len);
	free(cmpbuf);
	return ret;
}

/*
 * @cmpbuf	buffer to compare against, cmpbuf[0] is expected to match the
 *		flash content at location start
 * @start	offset to the base address of the flash chip
 * @len		length of the verified area
 * @return	0 for success, -1 for failure
 */
int verify_range(struct flashctx *flash, const uint8_t *cmpbuf, unsigned int start, unsigned int len)
{
	if (!len)
		return -1;

	if (!flash->chip->read) {
		msg_cerr("ERROR: flashrom has no read function for this flash chip.\n");
		return -1;
	}

	uint8_t *readbuf = malloc(len);
	if (!readbuf) {
		msg_gerr("Could not allocate memory!\n");
		return -1;
	}
	int ret = 0;

	if (start + len > flash->chip->total_size * 1024) {
		msg_gerr("Error: %s called with start 0x%x + len 0x%x >"
			" total_size 0x%x\n", __func__, start, len,
			flash->chip->total_size * 1024);
		ret = -1;
		goto out_free;
	}

	ret = flash->chip->read(flash, readbuf, start, len);
	if (ret) {
		msg_gerr("Verification impossible because read failed "
			 "at 0x%x (len 0x%x)\n", start, len);
		ret = -1;
		goto out_free;
	}

	ret = compare_range(cmpbuf, readbuf, start, len);
out_free:
	free(readbuf);
	return ret;
}

/* Helper function for need_erase() that focuses on granularities of gran bytes. */
static int need_erase_gran_bytes(const uint8_t *have, const uint8_t *want, unsigned int len, unsigned int gran)
{
	unsigned int i, j, limit;
	for (j = 0; j < len / gran; j++) {
		limit = min (gran, len - j * gran);
		/* Are 'have' and 'want' identical? */
		if (!memcmp(have + j * gran, want + j * gran, limit))
			continue;
		/* have needs to be in erased state. */
		for (i = 0; i < limit; i++)
			if (have[j * gran + i] != 0xff)
				return 1;
	}
	return 0;
}

/*
 * Check if the buffer @have can be programmed to the content of @want without
 * erasing. This is only possible if all chunks of size @gran are either kept
 * as-is or changed from an all-ones state to any other state.
 *
 * Warning: This function assumes that @have and @want point to naturally
 * aligned regions.
 *
 * @have        buffer with current content
 * @want        buffer with desired content
 * @len		length of the checked area
 * @gran	write granularity (enum, not count)
 * @return      0 if no erase is needed, 1 otherwise
 */
int need_erase(const uint8_t *have, const uint8_t *want, unsigned int len, enum write_granularity gran)
{
	int result = 0;
	unsigned int i;

	switch (gran) {
	case write_gran_1bit:
		for (i = 0; i < len; i++)
			if ((have[i] & want[i]) != want[i]) {
				result = 1;
				break;
			}
		break;
	case write_gran_1byte:
		for (i = 0; i < len; i++)
			if ((have[i] != want[i]) && (have[i] != 0xff)) {
				result = 1;
				break;
			}
		break;
	case write_gran_256bytes:
		result = need_erase_gran_bytes(have, want, len, 256);
		break;
	case write_gran_264bytes:
		result = need_erase_gran_bytes(have, want, len, 264);
		break;
	case write_gran_512bytes:
		result = need_erase_gran_bytes(have, want, len, 512);
		break;
	case write_gran_528bytes:
		result = need_erase_gran_bytes(have, want, len, 528);
		break;
	case write_gran_1024bytes:
		result = need_erase_gran_bytes(have, want, len, 1024);
		break;
	case write_gran_1056bytes:
		result = need_erase_gran_bytes(have, want, len, 1056);
		break;
	case write_gran_1byte_implicit_erase:
		/* Do not erase, handle content changes from anything->0xff by writing 0xff. */
		result = 0;
		break;
	default:
		msg_cerr("%s: Unsupported granularity! Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
	}
	return result;
}

/**
 * Check if the buffer @have needs to be programmed to get the content of @want.
 * If yes, return 1 and fill in first_start with the start address of the
 * write operation and first_len with the length of the first to-be-written
 * chunk. If not, return 0 and leave first_start and first_len undefined.
 *
 * Warning: This function assumes that @have and @want point to naturally
 * aligned regions.
 *
 * @have	buffer with current content
 * @want	buffer with desired content
 * @len		length of the checked area
 * @gran	write granularity (enum, not count)
 * @first_start	offset of the first byte which needs to be written (passed in
 *		value is increased by the offset of the first needed write
 *		relative to have/want or unchanged if no write is needed)
 * @return	length of the first contiguous area which needs to be written
 *		0 if no write is needed
 *
 * FIXME: This function needs a parameter which tells it about coalescing
 * in relation to the max write length of the programmer and the max write
 * length of the chip.
 */
static unsigned int get_next_write(const uint8_t *have, const uint8_t *want, unsigned int len,
			  unsigned int *first_start,
			  enum write_granularity gran)
{
	int need_write = 0;
	unsigned int rel_start = 0, first_len = 0;
	unsigned int i, limit, stride;

	switch (gran) {
	case write_gran_1bit:
	case write_gran_1byte:
	case write_gran_1byte_implicit_erase:
		stride = 1;
		break;
	case write_gran_256bytes:
		stride = 256;
		break;
	case write_gran_264bytes:
		stride = 264;
		break;
	case write_gran_512bytes:
		stride = 512;
		break;
	case write_gran_528bytes:
		stride = 528;
		break;
	case write_gran_1024bytes:
		stride = 1024;
		break;
	case write_gran_1056bytes:
		stride = 1056;
		break;
	default:
		msg_cerr("%s: Unsupported granularity! Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		/* Claim that no write was needed. A write with unknown
		 * granularity is too dangerous to try.
		 */
		return 0;
	}
	for (i = 0; i < len / stride; i++) {
		limit = min(stride, len - i * stride);
		/* Are 'have' and 'want' identical? */
		if (memcmp(have + i * stride, want + i * stride, limit)) {
			if (!need_write) {
				/* First location where have and want differ. */
				need_write = 1;
				rel_start = i * stride;
			}
		} else {
			if (need_write) {
				/* First location where have and want
				 * do not differ anymore.
				 */
				break;
			}
		}
	}
	if (need_write)
		first_len = min(i * stride - rel_start, len);
	*first_start += rel_start;
	return first_len;
}

/* This function generates various test patterns useful for testing controller
 * and chip communication as well as chip behaviour.
 *
 * If a byte can be written multiple times, each time keeping 0-bits at 0
 * and changing 1-bits to 0 if the new value for that bit is 0, the effect
 * is essentially an AND operation. That's also the reason why this function
 * provides the result of AND between various patterns.
 *
 * Below is a list of patterns (and their block length).
 * Pattern 0 is 05 15 25 35 45 55 65 75 85 95 a5 b5 c5 d5 e5 f5 (16 Bytes)
 * Pattern 1 is 0a 1a 2a 3a 4a 5a 6a 7a 8a 9a aa ba ca da ea fa (16 Bytes)
 * Pattern 2 is 50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f (16 Bytes)
 * Pattern 3 is a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af (16 Bytes)
 * Pattern 4 is 00 10 20 30 40 50 60 70 80 90 a0 b0 c0 d0 e0 f0 (16 Bytes)
 * Pattern 5 is 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f (16 Bytes)
 * Pattern 6 is 00 (1 Byte)
 * Pattern 7 is ff (1 Byte)
 * Patterns 0-7 have a big-endian block number in the last 2 bytes of each 256
 * byte block.
 *
 * Pattern 8 is 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11... (256 B)
 * Pattern 9 is ff fe fd fc fb fa f9 f8 f7 f6 f5 f4 f3 f2 f1 f0 ef ee... (256 B)
 * Pattern 10 is 00 00 00 01 00 02 00 03 00 04... (128 kB big-endian counter)
 * Pattern 11 is ff ff ff fe ff fd ff fc ff fb... (128 kB big-endian downwards)
 * Pattern 12 is 00 (1 Byte)
 * Pattern 13 is ff (1 Byte)
 * Patterns 8-13 have no block number.
 *
 * Patterns 0-3 are created to detect and efficiently diagnose communication
 * slips like missed bits or bytes and their repetitive nature gives good visual
 * cues to the person inspecting the results. In addition, the following holds:
 * AND Pattern 0/1 == Pattern 4
 * AND Pattern 2/3 == Pattern 5
 * AND Pattern 0/1/2/3 == AND Pattern 4/5 == Pattern 6
 * A weakness of pattern 0-5 is the inability to detect swaps/copies between
 * any two 16-byte blocks except for the last 16-byte block in a 256-byte bloc.
 * They work perfectly for detecting any swaps/aliasing of blocks >= 256 bytes.
 * 0x5 and 0xa were picked because they are 0101 and 1010 binary.
 * Patterns 8-9 are best for detecting swaps/aliasing of blocks < 256 bytes.
 * Besides that, they provide for bit testing of the last two bytes of every
 * 256 byte block which contains the block number for patterns 0-6.
 * Patterns 10-11 are special purpose for detecting subblock aliasing with
 * block sizes >256 bytes (some Dataflash chips etc.)
 * AND Pattern 8/9 == Pattern 12
 * AND Pattern 10/11 == Pattern 12
 * Pattern 13 is the completely erased state.
 * None of the patterns can detect aliasing at boundaries which are a multiple
 * of 16 MBytes (but such chips do not exist anyway for Parallel/LPC/FWH/SPI).
 */
int generate_testpattern(uint8_t *buf, uint32_t size, int variant)
{
	int i;

	if (!buf) {
		msg_gerr("Invalid buffer!\n");
		return 1;
	}

	switch (variant) {
	case 0:
		for (i = 0; i < size; i++)
			buf[i] = (i & 0xf) << 4 | 0x5;
		break;
	case 1:
		for (i = 0; i < size; i++)
			buf[i] = (i & 0xf) << 4 | 0xa;
		break;
	case 2:
		for (i = 0; i < size; i++)
			buf[i] = 0x50 | (i & 0xf);
		break;
	case 3:
		for (i = 0; i < size; i++)
			buf[i] = 0xa0 | (i & 0xf);
		break;
	case 4:
		for (i = 0; i < size; i++)
			buf[i] = (i & 0xf) << 4;
		break;
	case 5:
		for (i = 0; i < size; i++)
			buf[i] = i & 0xf;
		break;
	case 6:
		memset(buf, 0x00, size);
		break;
	case 7:
		memset(buf, 0xff, size);
		break;
	case 8:
		for (i = 0; i < size; i++)
			buf[i] = i & 0xff;
		break;
	case 9:
		for (i = 0; i < size; i++)
			buf[i] = ~(i & 0xff);
		break;
	case 10:
		for (i = 0; i < size % 2; i++) {
			buf[i * 2] = (i >> 8) & 0xff;
			buf[i * 2 + 1] = i & 0xff;
		}
		if (size & 0x1)
			buf[i * 2] = (i >> 8) & 0xff;
		break;
	case 11:
		for (i = 0; i < size % 2; i++) {
			buf[i * 2] = ~((i >> 8) & 0xff);
			buf[i * 2 + 1] = ~(i & 0xff);
		}
		if (size & 0x1)
			buf[i * 2] = ~((i >> 8) & 0xff);
		break;
	case 12:
		memset(buf, 0x00, size);
		break;
	case 13:
		memset(buf, 0xff, size);
		break;
	}

	if ((variant >= 0) && (variant <= 7)) {
		/* Write block number in the last two bytes of each 256-byte
		 * block, big endian for easier reading of the hexdump.
		 * Note that this wraps around for chips larger than 2^24 bytes
		 * (16 MB).
		 */
		for (i = 0; i < size / 256; i++) {
			buf[i * 256 + 254] = (i >> 8) & 0xff;
			buf[i * 256 + 255] = i & 0xff;
		}
	}

	return 0;
}

/* Returns the number of busses commonly supported by the current programmer and flash chip where the latter
 * can not be completely accessed due to size/address limits of the programmer. */
unsigned int count_max_decode_exceedings(const struct flashctx *flash)
{
	unsigned int limitexceeded = 0;
	uint32_t size = flash->chip->total_size * 1024;
	enum chipbustype buses = flash->mst->buses_supported & flash->chip->bustype;

	if ((buses & BUS_PARALLEL) && (max_rom_decode.parallel < size)) {
		limitexceeded++;
		msg_pdbg("Chip size %u kB is bigger than supported "
			 "size %u kB of chipset/board/programmer "
			 "for %s interface, "
			 "probe/read/erase/write may fail. ", size / 1024,
			 max_rom_decode.parallel / 1024, "Parallel");
	}
	if ((buses & BUS_LPC) && (max_rom_decode.lpc < size)) {
		limitexceeded++;
		msg_pdbg("Chip size %u kB is bigger than supported "
			 "size %u kB of chipset/board/programmer "
			 "for %s interface, "
			 "probe/read/erase/write may fail. ", size / 1024,
			 max_rom_decode.lpc / 1024, "LPC");
	}
	if ((buses & BUS_FWH) && (max_rom_decode.fwh < size)) {
		limitexceeded++;
		msg_pdbg("Chip size %u kB is bigger than supported "
			 "size %u kB of chipset/board/programmer "
			 "for %s interface, "
			 "probe/read/erase/write may fail. ", size / 1024,
			 max_rom_decode.fwh / 1024, "FWH");
	}
	if ((buses & BUS_SPI) && (max_rom_decode.spi < size)) {
		limitexceeded++;
		msg_pdbg("Chip size %u kB is bigger than supported "
			 "size %u kB of chipset/board/programmer "
			 "for %s interface, "
			 "probe/read/erase/write may fail. ", size / 1024,
			 max_rom_decode.spi / 1024, "SPI");
	}
	return limitexceeded;
}

void unmap_flash(struct flashctx *flash)
{
	if (flash->virtual_registers != (chipaddr)ERROR_PTR) {
		programmer_unmap_flash_region((void *)flash->virtual_registers, flash->chip->total_size * 1024);
		flash->physical_registers = 0;
		flash->virtual_registers = (chipaddr)ERROR_PTR;
	}

	if (flash->virtual_memory != (chipaddr)ERROR_PTR) {
		programmer_unmap_flash_region((void *)flash->virtual_memory, flash->chip->total_size * 1024);
		flash->physical_memory = 0;
		flash->virtual_memory = (chipaddr)ERROR_PTR;
	}
}

int map_flash(struct flashctx *flash)
{
	/* Init pointers to the fail-safe state to distinguish them later from legit values. */
	flash->virtual_memory = (chipaddr)ERROR_PTR;
	flash->virtual_registers = (chipaddr)ERROR_PTR;

	/* FIXME: This avoids mapping (and unmapping) of flash chip definitions with size 0.
	 * These are used for various probing-related hacks that would not map successfully anyway and should be
	 * removed ASAP. */
	if (flash->chip->total_size == 0)
		return 0;

	const chipsize_t size = flash->chip->total_size * 1024;
	uintptr_t base = flashbase ? flashbase : (0xffffffff - size + 1);
	void *addr = programmer_map_flash_region(flash->chip->name, base, size);
	if (addr == ERROR_PTR) {
		msg_perr("Could not map flash chip %s at 0x%0*" PRIxPTR ".\n",
			 flash->chip->name, PRIxPTR_WIDTH, base);
		return 1;
	}
	flash->physical_memory = base;
	flash->virtual_memory = (chipaddr)addr;

	/* FIXME: Special function registers normally live 4 MByte below flash space, but it might be somewhere
	 * completely different on some chips and programmers, or not mappable at all.
	 * Ignore these problems for now and always report success. */
	if (flash->chip->feature_bits & FEATURE_REGISTERMAP) {
		base = 0xffffffff - size - 0x400000 + 1;
		addr = programmer_map_flash_region("flash chip registers", base, size);
		if (addr == ERROR_PTR) {
			msg_pdbg2("Could not map flash chip registers %s at 0x%0*" PRIxPTR ".\n",
				 flash->chip->name, PRIxPTR_WIDTH, base);
			return 0;
		}
		flash->physical_registers = base;
		flash->virtual_registers = (chipaddr)addr;
	}
	return 0;
}

int probe_flash(struct registered_master *mst, int startchip, struct flashctx *flash, int force)
{
	const struct flashchip *chip;
	enum chipbustype buses_common;
	char *tmp;

	for (chip = flashchips + startchip; chip && chip->name; chip++) {
		if (chip_to_probe && strcmp(chip->name, chip_to_probe) != 0)
			continue;
		buses_common = mst->buses_supported & chip->bustype;
		if (!buses_common)
			continue;
		msg_gdbg("Probing for %s %s, %d kB: ", chip->vendor, chip->name, chip->total_size);
		if (!chip->probe && !force) {
			msg_gdbg("failed! flashrom has no probe function for this flash chip.\n");
			continue;
		}

		/* Start filling in the dynamic data. */
		flash->chip = calloc(1, sizeof(struct flashchip));
		if (!flash->chip) {
			msg_gerr("Out of memory!\n");
			exit(1);
		}
		memcpy(flash->chip, chip, sizeof(struct flashchip));
		flash->mst = mst;

		if (map_flash(flash) != 0)
			return -1;

		/* We handle a forced match like a real match, we just avoid probing. Note that probe_flash()
		 * is only called with force=1 after normal probing failed.
		 */
		if (force)
			break;

		if (flash->chip->probe(flash) != 1)
			goto notfound;

		/* If this is the first chip found, accept it.
		 * If this is not the first chip found, accept it only if it is
		 * a non-generic match. SFDP and CFI are generic matches.
		 * startchip==0 means this call to probe_flash() is the first
		 * one for this programmer interface (master) and thus no other chip has
		 * been found on this interface.
		 */
		if (startchip == 0 && flash->chip->model_id == SFDP_DEVICE_ID) {
			msg_cinfo("===\n"
				  "SFDP has autodetected a flash chip which is "
				  "not natively supported by flashrom yet.\n");
			if (count_usable_erasers(flash) == 0)
				msg_cinfo("The standard operations read and "
					  "verify should work, but to support "
					  "erase, write and all other "
					  "possible features");
			else
				msg_cinfo("All standard operations (read, "
					  "verify, erase and write) should "
					  "work, but to support all possible "
					  "features");

			msg_cinfo(" we need to add them manually.\n"
				  "You can help us by mailing us the output of the following command to "
				  "flashrom@flashrom.org:\n"
				  "'flashrom -VV [plus the -p/--programmer parameter]'\n"
				  "Thanks for your help!\n"
				  "===\n");
		}

		/* First flash chip detected on this bus. */
		if (startchip == 0)
			break;
		/* Not the first flash chip detected on this bus, but not a generic match either. */
		if ((flash->chip->model_id != GENERIC_DEVICE_ID) && (flash->chip->model_id != SFDP_DEVICE_ID))
			break;
		/* Not the first flash chip detected on this bus, and it's just a generic match. Ignore it. */
notfound:
		unmap_flash(flash);
		free(flash->chip);
		flash->chip = NULL;
	}

	if (!flash->chip)
		return -1;


	tmp = flashbuses_to_text(flash->chip->bustype);
	msg_cinfo("%s %s flash chip \"%s\" (%d kB, %s) ", force ? "Assuming" : "Found",
		  flash->chip->vendor, flash->chip->name, flash->chip->total_size, tmp);
	free(tmp);
#if CONFIG_INTERNAL == 1
	if (programmer_table[programmer].map_flash_region == physmap)
		msg_cinfo("mapped at physical address 0x%0*" PRIxPTR ".\n",
			  PRIxPTR_WIDTH, flash->physical_memory);
	else
#endif
		msg_cinfo("on %s.\n", programmer_table[programmer].name);

	/* Flash registers may more likely not be mapped if the chip was forced.
	 * Lock info may be stored in registers, so avoid lock info printing. */
	if (!force)
		if (flash->chip->printlock)
			flash->chip->printlock(flash);

	/* Get out of the way for later runs. */
	unmap_flash(flash);

	/* Return position of matching chip. */
	return chip - flashchips;
}

int read_buf_from_file(unsigned char *buf, unsigned long size,
		       const char *filename)
{
#ifdef __LIBPAYLOAD__
	msg_gerr("Error: No file I/O support in libpayload\n");
	return 1;
#else
	unsigned long numbytes;
	FILE *image;
	struct stat image_stat;

	if ((image = fopen(filename, "rb")) == NULL) {
		msg_gerr("Error: opening file \"%s\" failed: %s\n", filename, strerror(errno));
		return 1;
	}
	if (fstat(fileno(image), &image_stat) != 0) {
		msg_gerr("Error: getting metadata of file \"%s\" failed: %s\n", filename, strerror(errno));
		fclose(image);
		return 1;
	}
	if (image_stat.st_size != size) {
		msg_gerr("Error: Image size (%jd B) doesn't match the flash chip's size (%lu B)!\n",
			 (intmax_t)image_stat.st_size, size);
		fclose(image);
		return 1;
	}
	numbytes = fread(buf, 1, size, image);
	if (fclose(image)) {
		msg_gerr("Error: closing file \"%s\" failed: %s\n", filename, strerror(errno));
		return 1;
	}
	if (numbytes != size) {
		msg_gerr("Error: Failed to read complete file. Got %ld bytes, "
			 "wanted %ld!\n", numbytes, size);
		return 1;
	}
	return 0;
#endif
}

int write_buf_to_file(const unsigned char *buf, unsigned long size, const char *filename)
{
#ifdef __LIBPAYLOAD__
	msg_gerr("Error: No file I/O support in libpayload\n");
	return 1;
#else
	unsigned long numbytes;
	FILE *image;

	if (!filename) {
		msg_gerr("No filename specified.\n");
		return 1;
	}
	if ((image = fopen(filename, "wb")) == NULL) {
		msg_gerr("Error: opening file \"%s\" failed: %s\n", filename, strerror(errno));
		return 1;
	}

	numbytes = fwrite(buf, 1, size, image);
	fclose(image);
	if (numbytes != size) {
		msg_gerr("File %s could not be written completely.\n",
			 filename);
		return 1;
	}
	return 0;
#endif
}

int read_flash_to_file(struct flashctx *flash, const char *filename)
{
	unsigned long size = flash->chip->total_size * 1024;
	unsigned char *buf = calloc(size, sizeof(char));
	int ret = 0;

	msg_cinfo("Reading flash... ");
	if (!buf) {
		msg_gerr("Memory allocation failed!\n");
		msg_cinfo("FAILED.\n");
		return 1;
	}
	if (!flash->chip->read) {
		msg_cerr("No read function available for this flash chip.\n");
		ret = 1;
		goto out_free;
	}
	if (flash->chip->read(flash, buf, 0, size)) {
		msg_cerr("Read operation failed!\n");
		ret = 1;
		goto out_free;
	}

	ret = write_buf_to_file(buf, size, filename);
out_free:
	free(buf);
	msg_cinfo("%s.\n", ret ? "FAILED" : "done");
	return ret;
}

/* Even if an error is found, the function will keep going and check the rest. */
static int selfcheck_eraseblocks(const struct flashchip *chip)
{
	int i, j, k;
	int ret = 0;

	for (k = 0; k < NUM_ERASEFUNCTIONS; k++) {
		unsigned int done = 0;
		struct block_eraser eraser = chip->block_erasers[k];

		for (i = 0; i < NUM_ERASEREGIONS; i++) {
			/* Blocks with zero size are bugs in flashchips.c. */
			if (eraser.eraseblocks[i].count &&
			    !eraser.eraseblocks[i].size) {
				msg_gerr("ERROR: Flash chip %s erase function "
					"%i region %i has size 0. Please report"
					" a bug at flashrom@flashrom.org\n",
					chip->name, k, i);
				ret = 1;
			}
			/* Blocks with zero count are bugs in flashchips.c. */
			if (!eraser.eraseblocks[i].count &&
			    eraser.eraseblocks[i].size) {
				msg_gerr("ERROR: Flash chip %s erase function "
					"%i region %i has count 0. Please report"
					" a bug at flashrom@flashrom.org\n",
					chip->name, k, i);
				ret = 1;
			}
			done += eraser.eraseblocks[i].count *
				eraser.eraseblocks[i].size;
		}
		/* Empty eraseblock definition with erase function.  */
		if (!done && eraser.block_erase)
			msg_gspew("Strange: Empty eraseblock definition with "
				  "non-empty erase function. Not an error.\n");
		if (!done)
			continue;
		if (done != chip->total_size * 1024) {
			msg_gerr("ERROR: Flash chip %s erase function %i "
				"region walking resulted in 0x%06x bytes total,"
				" expected 0x%06x bytes. Please report a bug at"
				" flashrom@flashrom.org\n", chip->name, k,
				done, chip->total_size * 1024);
			ret = 1;
		}
		if (!eraser.block_erase)
			continue;
		/* Check if there are identical erase functions for different
		 * layouts. That would imply "magic" erase functions. The
		 * easiest way to check this is with function pointers.
		 */
		for (j = k + 1; j < NUM_ERASEFUNCTIONS; j++) {
			if (eraser.block_erase ==
			    chip->block_erasers[j].block_erase) {
				msg_gerr("ERROR: Flash chip %s erase function "
					"%i and %i are identical. Please report"
					" a bug at flashrom@flashrom.org\n",
					chip->name, k, j);
				ret = 1;
			}
		}
	}
	return ret;
}

static int erase_and_write_block_helper(struct flashctx *flash,
					unsigned int start, unsigned int len,
					uint8_t *curcontents,
					uint8_t *newcontents,
					int (*erasefn) (struct flashctx *flash,
							unsigned int addr,
							unsigned int len))
{
	unsigned int starthere = 0, lenhere = 0;
	int ret = 0, skip = 1, writecount = 0;
	enum write_granularity gran = flash->chip->gran;

	/* curcontents and newcontents are opaque to walk_eraseregions, and
	 * need to be adjusted here to keep the impression of proper abstraction
	 */
	curcontents += start;
	newcontents += start;
	msg_cdbg(":");
	if (need_erase(curcontents, newcontents, len, gran)) {
		msg_cdbg("E");
		ret = erasefn(flash, start, len);
		if (ret)
			return ret;
		if (check_erased_range(flash, start, len)) {
			msg_cerr("ERASE FAILED!\n");
			return -1;
		}
		/* Erase was successful. Adjust curcontents. */
		memset(curcontents, 0xff, len);
		skip = 0;
	}
	/* get_next_write() sets starthere to a new value after the call. */
	while ((lenhere = get_next_write(curcontents + starthere,
					 newcontents + starthere,
					 len - starthere, &starthere, gran))) {
		if (!writecount++)
			msg_cdbg("W");
		/* Needs the partial write function signature. */
		ret = flash->chip->write(flash, newcontents + starthere,
				   start + starthere, lenhere);
		if (ret)
			return ret;
		starthere += lenhere;
		skip = 0;
	}
	if (skip)
		msg_cdbg("S");
	else
		all_skipped = false;
	return ret;
}

static int walk_eraseregions(struct flashctx *flash, int erasefunction,
			     int (*do_something) (struct flashctx *flash,
						  unsigned int addr,
						  unsigned int len,
						  uint8_t *param1,
						  uint8_t *param2,
						  int (*erasefn) (
							struct flashctx *flash,
							unsigned int addr,
							unsigned int len)),
			     void *param1, void *param2)
{
	int i, j;
	unsigned int start = 0;
	unsigned int len;
	struct block_eraser eraser = flash->chip->block_erasers[erasefunction];

	for (i = 0; i < NUM_ERASEREGIONS; i++) {
		/* count==0 for all automatically initialized array
		 * members so the loop below won't be executed for them.
		 */
		len = eraser.eraseblocks[i].size;
		for (j = 0; j < eraser.eraseblocks[i].count; j++) {
			/* Print this for every block except the first one. */
			if (i || j)
				msg_cdbg(", ");
			msg_cdbg("0x%06x-0x%06x", start,
				     start + len - 1);
			if (do_something(flash, start, len, param1, param2,
					 eraser.block_erase)) {
				return 1;
			}
			start += len;
		}
	}
	msg_cdbg("\n");
	return 0;
}

static int check_block_eraser(const struct flashctx *flash, int k, int log)
{
	struct block_eraser eraser = flash->chip->block_erasers[k];

	if (!eraser.block_erase && !eraser.eraseblocks[0].count) {
		if (log)
			msg_cdbg("not defined. ");
		return 1;
	}
	if (!eraser.block_erase && eraser.eraseblocks[0].count) {
		if (log)
			msg_cdbg("eraseblock layout is known, but matching "
				 "block erase function is not implemented. ");
		return 1;
	}
	if (eraser.block_erase && !eraser.eraseblocks[0].count) {
		if (log)
			msg_cdbg("block erase function found, but "
				 "eraseblock layout is not defined. ");
		return 1;
	}
	// TODO: Once erase functions are annotated with allowed buses, check that as well.
	return 0;
}

int erase_and_write_flash(struct flashctx *flash, uint8_t *oldcontents, uint8_t *newcontents)
{
	int k, ret = 1;
	uint8_t *curcontents;
	unsigned long size = flash->chip->total_size * 1024;
	unsigned int usable_erasefunctions = count_usable_erasers(flash);

	msg_cinfo("Erasing and writing flash chip... ");
	curcontents = malloc(size);
	if (!curcontents) {
		msg_gerr("Out of memory!\n");
		exit(1);
	}
	/* Copy oldcontents to curcontents to avoid clobbering oldcontents. */
	memcpy(curcontents, oldcontents, size);

	for (k = 0; k < NUM_ERASEFUNCTIONS; k++) {
		if (k != 0)
			msg_cinfo("Looking for another erase function.\n");
		if (!usable_erasefunctions) {
			msg_cinfo("No usable erase functions left.\n");
			break;
		}
		msg_cdbg("Trying erase function %i... ", k);
		if (check_block_eraser(flash, k, 1))
			continue;
		usable_erasefunctions--;
		ret = walk_eraseregions(flash, k, &erase_and_write_block_helper,
					curcontents, newcontents);
		/* If everything is OK, don't try another erase function. */
		if (!ret)
			break;
		/* Write/erase failed, so try to find out what the current chip
		 * contents are. If no usable erase functions remain, we can
		 * skip this: the next iteration will break immediately anyway.
		 */
		if (!usable_erasefunctions)
			continue;
		/* Reading the whole chip may take a while, inform the user even
		 * in non-verbose mode.
		 */
		msg_cinfo("Reading current flash chip contents... ");
		if (flash->chip->read(flash, curcontents, 0, size)) {
			/* Now we are truly screwed. Read failed as well. */
			msg_cerr("Can't read anymore! Aborting.\n");
			/* We have no idea about the flash chip contents, so
			 * retrying with another erase function is pointless.
			 */
			break;
		}
		msg_cinfo("done. ");
	}
	/* Free the scratchpad. */
	free(curcontents);

	if (ret) {
		msg_cerr("FAILED!\n");
	} else {
		if (all_skipped)
			msg_cinfo("\nWarning: Chip content is identical to the requested image.\n");
		msg_cinfo("Erase/write done.\n");
	}
	return ret;
}

static void nonfatal_help_message(void)
{
	msg_gerr("Good, writing to the flash chip apparently didn't do anything.\n");
#if CONFIG_INTERNAL == 1
	if (programmer == PROGRAMMER_INTERNAL)
		msg_gerr("This means we have to add special support for your board, programmer or flash\n"
			 "chip. Please report this on IRC at chat.freenode.net (channel #flashrom) or\n"
			 "mail flashrom@flashrom.org, thanks!\n"
			 "-------------------------------------------------------------------------------\n"
			 "You may now reboot or simply leave the machine running.\n");
	else
#endif
		msg_gerr("Please check the connections (especially those to write protection pins) between\n"
			 "the programmer and the flash chip. If you think the error is caused by flashrom\n"
			 "please report this on IRC at chat.freenode.net (channel #flashrom) or\n"
			 "mail flashrom@flashrom.org, thanks!\n");
}

static void emergency_help_message(void)
{
	msg_gerr("Your flash chip is in an unknown state.\n");
#if CONFIG_INTERNAL == 1
	if (programmer == PROGRAMMER_INTERNAL)
		msg_gerr("Get help on IRC at chat.freenode.net (channel #flashrom) or\n"
			"mail flashrom@flashrom.org with the subject \"FAILED: <your board name>\"!\n"
			"-------------------------------------------------------------------------------\n"
			"DO NOT REBOOT OR POWEROFF!\n");
	else
#endif
		msg_gerr("Please report this on IRC at chat.freenode.net (channel #flashrom) or\n"
			 "mail flashrom@flashrom.org, thanks!\n");
}

/* The way to go if you want a delimited list of programmers */
void list_programmers(const char *delim)
{
	enum programmer p;
	for (p = 0; p < PROGRAMMER_INVALID; p++) {
		msg_ginfo("%s", programmer_table[p].name);
		if (p < PROGRAMMER_INVALID - 1)
			msg_ginfo("%s", delim);
	}
	msg_ginfo("\n");	
}

void list_programmers_linebreak(int startcol, int cols, int paren)
{
	const char *pname;
	int pnamelen;
	int remaining = 0, firstline = 1;
	enum programmer p;
	int i;

	for (p = 0; p < PROGRAMMER_INVALID; p++) {
		pname = programmer_table[p].name;
		pnamelen = strlen(pname);
		if (remaining - pnamelen - 2 < 0) {
			if (firstline)
				firstline = 0;
			else
				msg_ginfo("\n");
			for (i = 0; i < startcol; i++)
				msg_ginfo(" ");
			remaining = cols - startcol;
		} else {
			msg_ginfo(" ");
			remaining--;
		}
		if (paren && (p == 0)) {
			msg_ginfo("(");
			remaining--;
		}
		msg_ginfo("%s", pname);
		remaining -= pnamelen;
		if (p < PROGRAMMER_INVALID - 1) {
			msg_ginfo(",");
			remaining--;
		} else {
			if (paren)
				msg_ginfo(")");
		}
	}
}

void print_sysinfo(void)
{
#if IS_WINDOWS
	SYSTEM_INFO si;
	OSVERSIONINFOEX osvi;

	memset(&si, 0, sizeof(SYSTEM_INFO));
	memset(&osvi, 0, sizeof(OSVERSIONINFOEX));
	msg_ginfo(" on Windows");
	/* Tell Windows which version of the structure we want. */
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (GetVersionEx((OSVERSIONINFO*) &osvi))
		msg_ginfo(" %lu.%lu", osvi.dwMajorVersion, osvi.dwMinorVersion);
	else
		msg_ginfo(" unknown version");
	GetSystemInfo(&si);
	switch (si.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64:
		msg_ginfo(" (x86_64)");
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		msg_ginfo(" (x86)");
		break;
	default:
		msg_ginfo(" (unknown arch)");
		break;
	}
#elif HAVE_UTSNAME == 1
	struct utsname osinfo;

	uname(&osinfo);
	msg_ginfo(" on %s %s (%s)", osinfo.sysname, osinfo.release,
		  osinfo.machine);
#else
	msg_ginfo(" on unknown machine");
#endif
}

void print_buildinfo(void)
{
	msg_gdbg("flashrom was built with");
#if NEED_PCI == 1
#ifdef PCILIB_VERSION
	msg_gdbg(" libpci %s,", PCILIB_VERSION);
#else
	msg_gdbg(" unknown PCI library,");
#endif
#endif
#ifdef __clang__
	msg_gdbg(" LLVM Clang");
#ifdef __clang_version__
	msg_gdbg(" %s,", __clang_version__);
#else
	msg_gdbg(" unknown version (before r102686),");
#endif
#elif defined(__GNUC__)
	msg_gdbg(" GCC");
#ifdef __VERSION__
	msg_gdbg(" %s,", __VERSION__);
#else
	msg_gdbg(" unknown version,");
#endif
#else
	msg_gdbg(" unknown compiler,");
#endif
#if defined (__FLASHROM_LITTLE_ENDIAN__)
	msg_gdbg(" little endian");
#elif defined (__FLASHROM_BIG_ENDIAN__)
	msg_gdbg(" big endian");
#else
#error Endianness could not be determined
#endif
	msg_gdbg("\n");
}

void print_version(void)
{
	msg_ginfo("flashrom v%s", flashrom_version);
	print_sysinfo();
	msg_ginfo("\n");
}

void print_banner(void)
{
	msg_ginfo("flashrom is free software, get the source code at "
		  "http://www.flashrom.org\n");
	msg_ginfo("\n");
}

int selfcheck(void)
{
	unsigned int i;
	int ret = 0;

	/* Safety check. Instead of aborting after the first error, check
	 * if more errors exist.
	 */
	if (ARRAY_SIZE(programmer_table) - 1 != PROGRAMMER_INVALID) {
		msg_gerr("Programmer table miscompilation!\n");
		ret = 1;
	}
	for (i = 0; i < PROGRAMMER_INVALID; i++) {
		const struct programmer_entry p = programmer_table[i];
		if (p.name == NULL) {
			msg_gerr("All programmers need a valid name, but the one with index %d does not!\n", i);
			ret = 1;
			/* This might hide other problems with this programmer, but allows for better error
			 * messages below without jumping through hoops. */
			continue;
		}
		switch (p.type) {
		case USB:
		case PCI:
		case OTHER:
			if (p.devs.note == NULL) {
				if (strcmp("internal", p.name) == 0)
					break; /* This one has its device list stored separately. */
				msg_gerr("Programmer %s has neither a device list nor a textual description!\n",
					 p.name);
				ret = 1;
			}
			break;
		default:
			msg_gerr("Programmer %s does not have a valid type set!\n", p.name);
			ret = 1;
			break;
		}
		if (p.init == NULL) {
			msg_gerr("Programmer %s does not have a valid init function!\n", p.name);
			ret = 1;
		}
		if (p.delay == NULL) {
			msg_gerr("Programmer %s does not have a valid delay function!\n", p.name);
			ret = 1;
		}
		if (p.map_flash_region == NULL) {
			msg_gerr("Programmer %s does not have a valid map_flash_region function!\n", p.name);
			ret = 1;
		}
		if (p.unmap_flash_region == NULL) {
			msg_gerr("Programmer %s does not have a valid unmap_flash_region function!\n", p.name);
			ret = 1;
		}
	}

	/* It would be favorable if we could check for the correct layout (especially termination) of various
	 * constant arrays: flashchips, chipset_enables, board_matches, boards_known, laptops_known.
	 * They are all defined as externs in this compilation unit so we don't know their sizes which vary
	 * depending on compiler flags, e.g. the target architecture, and can sometimes be 0.
	 * For 'flashchips' we export the size explicitly to work around this and to be able to implement the
	 * checks below. */
	if (flashchips_size <= 1 || flashchips[flashchips_size - 1].name != NULL) {
		msg_gerr("Flashchips table miscompilation!\n");
		ret = 1;
	} else {
		for (i = 0; i < flashchips_size - 1; i++) {
			const struct flashchip *chip = &flashchips[i];
			if (chip->vendor == NULL || chip->name == NULL || chip->bustype == BUS_NONE) {
				ret = 1;
				msg_gerr("ERROR: Some field of flash chip #%d (%s) is misconfigured.\n"
					 "Please report a bug at flashrom@flashrom.org\n", i,
					 chip->name == NULL ? "unnamed" : chip->name);
			}
			if (selfcheck_eraseblocks(chip)) {
				ret = 1;
			}
		}
	}

#if CONFIG_INTERNAL == 1
	ret |= selfcheck_board_enables();
#endif

	/* TODO: implement similar sanity checks for other arrays where deemed necessary. */
	return ret;
}

/* FIXME: This function signature needs to be improved once doit() has a better
 * function signature.
 */
int chip_safety_check(const struct flashctx *flash, int force, int read_it, int write_it, int erase_it,
		      int verify_it)
{
	const struct flashchip *chip = flash->chip;

	if (!programmer_may_write && (write_it || erase_it)) {
		msg_perr("Write/erase is not working yet on your programmer in "
			 "its current configuration.\n");
		/* --force is the wrong approach, but it's the best we can do
		 * until the generic programmer parameter parser is merged.
		 */
		if (!force)
			return 1;
		msg_cerr("Continuing anyway.\n");
	}

	if (read_it || erase_it || write_it || verify_it) {
		/* Everything needs read. */
		if (chip->tested.read == BAD) {
			msg_cerr("Read is not working on this chip. ");
			if (!force)
				return 1;
			msg_cerr("Continuing anyway.\n");
		}
		if (!chip->read) {
			msg_cerr("flashrom has no read function for this "
				 "flash chip.\n");
			return 1;
		}
	}
	if (erase_it || write_it) {
		/* Write needs erase. */
		if (chip->tested.erase == NA) {
			msg_cerr("Erase is not possible on this chip.\n");
			return 1;
		}
		if (chip->tested.erase == BAD) {
			msg_cerr("Erase is not working on this chip. ");
			if (!force)
				return 1;
			msg_cerr("Continuing anyway.\n");
		}
		if(count_usable_erasers(flash) == 0) {
			msg_cerr("flashrom has no erase function for this "
				 "flash chip.\n");
			return 1;
		}
	}
	if (write_it) {
		if (chip->tested.write == NA) {
			msg_cerr("Write is not possible on this chip.\n");
			return 1;
		}
		if (chip->tested.write == BAD) {
			msg_cerr("Write is not working on this chip. ");
			if (!force)
				return 1;
			msg_cerr("Continuing anyway.\n");
		}
		if (!chip->write) {
			msg_cerr("flashrom has no write function for this "
				 "flash chip.\n");
			return 1;
		}
	}
	return 0;
}

/* This function signature is horrible. We need to design a better interface,
 * but right now it allows us to split off the CLI code.
 * Besides that, the function itself is a textbook example of abysmal code flow.
 */
int doit(struct flashctx *flash, int force, const char *filename, int read_it,
	 int write_it, int erase_it, int verify_it)
{
	uint8_t *oldcontents;
	uint8_t *newcontents;
	int ret = 0;
	unsigned long size = flash->chip->total_size * 1024;
	int read_all_first = 1; /* FIXME: Make this configurable. */

	if (chip_safety_check(flash, force, read_it, write_it, erase_it, verify_it)) {
		msg_cerr("Aborting.\n");
		return 1;
	}

	if (normalize_romentries(flash)) {
		msg_cerr("Requested regions can not be handled. Aborting.\n");
		return 1;
	}

	/* Given the existence of read locks, we want to unlock for read,
	 * erase and write.
	 */
	if (flash->chip->unlock)
		flash->chip->unlock(flash);

	if (read_it) {
		return read_flash_to_file(flash, filename);
	}

	oldcontents = malloc(size);
	if (!oldcontents) {
		msg_gerr("Out of memory!\n");
		exit(1);
	}
	/* Assume worst case: All bits are 0. */
	memset(oldcontents, 0x00, size);
	newcontents = malloc(size);
	if (!newcontents) {
		msg_gerr("Out of memory!\n");
		exit(1);
	}
	/* Assume best case: All bits should be 1. */
	memset(newcontents, 0xff, size);
	/* Side effect of the assumptions above: Default write action is erase
	 * because newcontents looks like a completely erased chip, and
	 * oldcontents being completely 0x00 means we have to erase everything
	 * before we can write.
	 */

	if (erase_it) {
		/* FIXME: Do we really want the scary warning if erase failed?
		 * After all, after erase the chip is either blank or partially
		 * blank or it has the old contents. A blank chip won't boot,
		 * so if the user wanted erase and reboots afterwards, the user
		 * knows very well that booting won't work.
		 */
		if (erase_and_write_flash(flash, oldcontents, newcontents)) {
			emergency_help_message();
			ret = 1;
		}
		goto out;
	}

	if (write_it || verify_it) {
		if (read_buf_from_file(newcontents, size, filename)) {
			ret = 1;
			goto out;
		}

#if CONFIG_INTERNAL == 1
		if (programmer == PROGRAMMER_INTERNAL && cb_check_image(newcontents, size) < 0) {
			if (force_boardmismatch) {
				msg_pinfo("Proceeding anyway because user forced us to.\n");
			} else {
				msg_perr("Aborting. You can override this with "
					 "-p internal:boardmismatch=force.\n");
				ret = 1;
				goto out;
			}
		}
#endif
	}

	/* Read the whole chip to be able to check whether regions need to be
	 * erased and to give better diagnostics in case write fails.
	 * The alternative is to read only the regions which are to be
	 * preserved, but in that case we might perform unneeded erase which
	 * takes time as well.
	 */
	if (read_all_first) {
		msg_cinfo("Reading old flash chip contents... ");
		if (flash->chip->read(flash, oldcontents, 0, size)) {
			ret = 1;
			msg_cinfo("FAILED.\n");
			goto out;
		}
	}
	msg_cinfo("done.\n");

	/* Build a new image taking the given layout into account. */
	if (build_new_image(flash, read_all_first, oldcontents, newcontents)) {
		msg_gerr("Could not prepare the data to be written, aborting.\n");
		ret = 1;
		goto out;
	}

	// ////////////////////////////////////////////////////////////

	if (write_it && erase_and_write_flash(flash, oldcontents, newcontents)) {
		msg_cerr("Uh oh. Erase/write failed. ");
		if (read_all_first) {
			msg_cerr("Checking if anything has changed.\n");
			msg_cinfo("Reading current flash chip contents... ");
			if (!flash->chip->read(flash, newcontents, 0, size)) {
				msg_cinfo("done.\n");
				if (!memcmp(oldcontents, newcontents, size)) {
					nonfatal_help_message();
					ret = 1;
					goto out;
				}
				msg_cerr("Apparently at least some data has changed.\n");
			} else
				msg_cerr("Can't even read anymore!\n");
			emergency_help_message();
			ret = 1;
			goto out;
		} else
			msg_cerr("\n");
		emergency_help_message();
		ret = 1;
		goto out;
	}

	/* Verify only if we either did not try to write (verify operation) or actually changed something. */
	if (verify_it && (!write_it || !all_skipped)) {
		msg_cinfo("Verifying flash... ");

		if (write_it) {
			/* Work around chips which need some time to calm down. */
			programmer_delay(1000*1000);
			ret = verify_range(flash, newcontents, 0, size);
			/* If we tried to write, and verification now fails, we
			 * might have an emergency situation.
			 */
			if (ret)
				emergency_help_message();
		} else {
			ret = compare_range(newcontents, oldcontents, 0, size);
		}
		if (!ret)
			msg_cinfo("VERIFIED.\n");
	}

out:
	free(oldcontents);
	free(newcontents);
	return ret;
}
