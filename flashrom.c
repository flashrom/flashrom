/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2004 Tyan Corp <yhlu@tyan.com>
 * Copyright (C) 2005-2008 coresystems GmbH
 * Copyright (C) 2008,2009 Carl-Daniel Hailfinger
 * Copyright (C) 2016 secunet Security Networks AG
 * (Written by Nico Huber <nico.huber@secunet.com> for secunet)
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

#include <stdio.h>
#include <sys/types.h>
#ifndef __LIBPAYLOAD__
#include <fcntl.h>
#include <sys/stat.h>
#endif
#include <string.h>
#include <unistd.h>
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
#include "hwaccess_physmap.h"
#include "chipdrivers.h"

const char flashrom_version[] = FLASHROM_VERSION;
const char *chip_to_probe = NULL;

static const struct programmer_entry *programmer = NULL;
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

#define SHUTDOWN_MAXFN 32
static int shutdown_fn_count = 0;
/** @private */
static struct shutdown_func_data {
	int (*func) (void *data);
	void *data;
} shutdown_fn[SHUTDOWN_MAXFN];
/* Initialize to 0 to make sure nobody registers a shutdown function before
 * programmer init.
 */
static int may_register_shutdown = 0;

/* Did we change something or was every erase/write skipped (if any)? */
static bool all_skipped = true;

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

int register_chip_restore(chip_restore_fn_cb_t func,
			  struct flashctx *flash, uint8_t status)
{
	if (flash->chip_restore_fn_count >= MAX_CHIP_RESTORE_FUNCTIONS) {
		msg_perr("Tried to register more than %i chip restore"
		         " functions.\n", MAX_CHIP_RESTORE_FUNCTIONS);
		return 1;
	}
	flash->chip_restore_fn[flash->chip_restore_fn_count].func = func;
	flash->chip_restore_fn[flash->chip_restore_fn_count].status = status;
	flash->chip_restore_fn_count++;

	return 0;
}

static int deregister_chip_restore(struct flashctx *flash)
{
	int rc = 0;

	while (flash->chip_restore_fn_count > 0) {
		int i = --flash->chip_restore_fn_count;
		rc |= flash->chip_restore_fn[i].func(
			flash, flash->chip_restore_fn[i].status);
	}

	return rc;
}

int programmer_init(const struct programmer_entry *prog, const char *param)
{
	int ret;

	if (prog == NULL) {
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
	msg_pdbg("Initializing %s programmer\n", programmer->name);
	ret = programmer->init();
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
	void *ret = programmer->map_flash_region(descr, phys_addr, len);
	msg_gspew("%s: mapping %s from 0x%0*" PRIxPTR " to 0x%0*" PRIxPTR "\n",
		  __func__, descr, PRIxPTR_WIDTH, phys_addr, PRIxPTR_WIDTH, (uintptr_t) ret);
	return ret;
}

void programmer_unmap_flash_region(void *virt_addr, size_t len)
{
	programmer->unmap_flash_region(virt_addr, len);
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
		programmer->delay(usecs);
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
static char *extract_param(const char *const *haystack, const char *needle, const char *delim)
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
static int check_erased_range(struct flashctx *flash, unsigned int start, unsigned int len)
{
	int ret;
	uint8_t *cmpbuf = malloc(len);
	const uint8_t erased_value = ERASED_VALUE(flash);

	if (!cmpbuf) {
		msg_gerr("Could not allocate memory!\n");
		exit(1);
	}
	memset(cmpbuf, erased_value, len);
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

	if (start + len > flash->chip->total_size * 1024) {
		msg_gerr("Error: %s called with start 0x%x + len 0x%x >"
			" total_size 0x%x\n", __func__, start, len,
			flash->chip->total_size * 1024);
		return -1;
	}

	if (!flash->chip->read) {
		msg_cerr("ERROR: flashrom has no read function for this flash chip.\n");
		return -1;
	}

	uint8_t *readbuf = malloc(len);
	if (!readbuf) {
		msg_gerr("Could not allocate memory!\n");
		return -1;
	}

	int ret = flash->chip->read(flash, readbuf, start, len);
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
static int need_erase_gran_bytes(const uint8_t *have, const uint8_t *want, unsigned int len,
                                 unsigned int gran, const uint8_t erased_value)
{
	unsigned int i, j, limit;
	for (j = 0; j < len / gran; j++) {
		limit = min (gran, len - j * gran);
		/* Are 'have' and 'want' identical? */
		if (!memcmp(have + j * gran, want + j * gran, limit))
			continue;
		/* have needs to be in erased state. */
		for (i = 0; i < limit; i++)
			if (have[j * gran + i] != erased_value)
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
int need_erase(const uint8_t *have, const uint8_t *want, unsigned int len,
               enum write_granularity gran, const uint8_t erased_value)
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
			if ((have[i] != want[i]) && (have[i] != erased_value)) {
				result = 1;
				break;
			}
		break;
	case write_gran_128bytes:
		result = need_erase_gran_bytes(have, want, len, 128, erased_value);
		break;
	case write_gran_256bytes:
		result = need_erase_gran_bytes(have, want, len, 256, erased_value);
		break;
	case write_gran_264bytes:
		result = need_erase_gran_bytes(have, want, len, 264, erased_value);
		break;
	case write_gran_512bytes:
		result = need_erase_gran_bytes(have, want, len, 512, erased_value);
		break;
	case write_gran_528bytes:
		result = need_erase_gran_bytes(have, want, len, 528, erased_value);
		break;
	case write_gran_1024bytes:
		result = need_erase_gran_bytes(have, want, len, 1024, erased_value);
		break;
	case write_gran_1056bytes:
		result = need_erase_gran_bytes(have, want, len, 1056, erased_value);
		break;
	case write_gran_64kbytes:
		result = need_erase_gran_bytes(have, want, len, 64*KiB, erased_value);
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
	case write_gran_128bytes:
		stride = 128;
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
	case write_gran_64kbytes:
		stride = 64*KiB;
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

/*
 * Return a string corresponding to the bustype parameter.
 * Memory is obtained with malloc() and must be freed with free() by the caller.
 */
char *flashbuses_to_text(enum chipbustype bustype)
{
	char *ret = calloc(1, 1);
	/*
	 * FIXME: Once all chipsets and flash chips have been updated, NONSPI
	 * will cease to exist and should be eliminated here as well.
	 */
	if (bustype == BUS_NONSPI) {
		ret = strcat_realloc(ret, "Non-SPI, ");
	} else {
		if (bustype & BUS_PARALLEL)
			ret = strcat_realloc(ret, "Parallel, ");
		if (bustype & BUS_LPC)
			ret = strcat_realloc(ret, "LPC, ");
		if (bustype & BUS_FWH)
			ret = strcat_realloc(ret, "FWH, ");
		if (bustype & BUS_SPI)
			ret = strcat_realloc(ret, "SPI, ");
		if (bustype & BUS_PROG)
			ret = strcat_realloc(ret, "Programmer-specific, ");
		if (bustype == BUS_NONE)
			ret = strcat_realloc(ret, "None, ");
	}
	/* Kill last comma. */
	ret[strlen(ret) - 2] = '\0';
	ret = realloc(ret, strlen(ret) + 1);
	return ret;
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
		/* Only probe for SPI25 chips by default. */
		if (chip->bustype == BUS_SPI && !chip_to_probe && chip->spi_cmd_set != SPI25)
			continue;
		msg_gdbg("Probing for %s %s, %d kB: ", chip->vendor, chip->name, chip->total_size);
		if (!chip->probe && !force) {
			msg_gdbg("failed! flashrom has no probe function for this flash chip.\n");
			continue;
		}

		/* Start filling in the dynamic data. */
		flash->chip = calloc(1, sizeof(*flash->chip));
		if (!flash->chip) {
			msg_gerr("Out of memory!\n");
			exit(1);
		}
		*flash->chip = *chip;
		flash->mst = mst;

		if (map_flash(flash) != 0)
			goto notfound;

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

	/* Fill default layout covering the whole chip. */
	if (flashrom_layout_new(&flash->default_layout) ||
	    flashrom_layout_add_region(flash->default_layout,
			0, flash->chip->total_size * 1024 - 1, "complete flash") ||
	    flashrom_layout_include_region(flash->default_layout, "complete flash"))
	        return -1;

	tmp = flashbuses_to_text(flash->chip->bustype);
	msg_cinfo("%s %s flash chip \"%s\" (%d kB, %s) ", force ? "Assuming" : "Found",
		  flash->chip->vendor, flash->chip->name, flash->chip->total_size, tmp);
	free(tmp);
#if CONFIG_INTERNAL == 1
	if (programmer->map_flash_region == physmap)
		msg_cinfo("mapped at physical address 0x%0*" PRIxPTR ".\n",
			  PRIxPTR_WIDTH, flash->physical_memory);
	else
#endif
		msg_cinfo("on %s.\n", programmer->name);

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
	int ret = 0;

	FILE *image;
	if (!strcmp(filename, "-"))
		image = fdopen(fileno(stdin), "rb");
	else
		image = fopen(filename, "rb");
	if (image == NULL) {
		msg_gerr("Error: opening file \"%s\" failed: %s\n", filename, strerror(errno));
		return 1;
	}

	struct stat image_stat;
	if (fstat(fileno(image), &image_stat) != 0) {
		msg_gerr("Error: getting metadata of file \"%s\" failed: %s\n", filename, strerror(errno));
		ret = 1;
		goto out;
	}
	if ((image_stat.st_size != (intmax_t)size) && strcmp(filename, "-")) {
		msg_gerr("Error: Image size (%jd B) doesn't match the expected size (%lu B)!\n",
			 (intmax_t)image_stat.st_size, size);
		ret = 1;
		goto out;
	}

	unsigned long numbytes = fread(buf, 1, size, image);
	if (numbytes != size) {
		msg_gerr("Error: Failed to read complete file. Got %ld bytes, "
			 "wanted %ld!\n", numbytes, size);
		ret = 1;
	}
out:
	(void)fclose(image);
	return ret;
#endif
}

/**
 * @brief Reads content to buffer from one or more files.
 *
 * Reads content to supplied buffer from files. If a filename is specified for
 * individual regions using the partial read syntax ('-i <region>[:<filename>]')
 * then this will read file data into the corresponding region in the
 * supplied buffer.
 *
 * @param flashctx Flash context to be used.
 * @param buf      Chip-sized buffer to write data to
 * @return 0 on success
 */
static int read_buf_from_include_args(const struct flashctx *const flash,
				      unsigned char *buf)
{
	const struct flashrom_layout *const layout = get_layout(flash);
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
		if (read_buf_from_file(buf + entry->start,
				       entry->end - entry->start + 1, entry->file))
			return 1;
	}
	return 0;
}

/**
 * @brief Writes passed data buffer into a file
 *
 * @param buf      Buffer with data to write
 * @param size     Size of buffer
 * @param filename File path to write to
 * @return 0 on success
 */
int write_buf_to_file(const unsigned char *buf, unsigned long size, const char *filename)
{
#ifdef __LIBPAYLOAD__
	msg_gerr("Error: No file I/O support in libpayload\n");
	return 1;
#else
	FILE *image;
	int ret = 0;

	if (!filename) {
		msg_gerr("No filename specified.\n");
		return 1;
	}
	if ((image = fopen(filename, "wb")) == NULL) {
		msg_gerr("Error: opening file \"%s\" failed: %s\n", filename, strerror(errno));
		return 1;
	}

	unsigned long numbytes = fwrite(buf, 1, size, image);
	if (numbytes != size) {
		msg_gerr("Error: file %s could not be written completely.\n", filename);
		ret = 1;
		goto out;
	}
	if (fflush(image)) {
		msg_gerr("Error: flushing file \"%s\" failed: %s\n", filename, strerror(errno));
		ret = 1;
	}
	// Try to fsync() only regular files and if that function is available at all (e.g. not on MinGW).
#if defined(_POSIX_FSYNC) && (_POSIX_FSYNC != -1)
	struct stat image_stat;
	if (fstat(fileno(image), &image_stat) != 0) {
		msg_gerr("Error: getting metadata of file \"%s\" failed: %s\n", filename, strerror(errno));
		ret = 1;
		goto out;
	}
	if (S_ISREG(image_stat.st_mode)) {
		if (fsync(fileno(image))) {
			msg_gerr("Error: fsyncing file \"%s\" failed: %s\n", filename, strerror(errno));
			ret = 1;
		}
	}
#endif
out:
	if (fclose(image)) {
		msg_gerr("Error: closing file \"%s\" failed: %s\n", filename, strerror(errno));
		ret = 1;
	}
	return ret;
#endif
}

/**
 * @brief Writes content from buffer to one or more files.
 *
 * Writes content from supplied buffer to files. If a filename is specified for
 * individual regions using the partial read syntax ('-i <region>[:<filename>]')
 * then this will write files using data from the corresponding region in the
 * supplied buffer.
 *
 * @param flashctx Flash context to be used.
 * @param buf      Chip-sized buffer to read data from
 * @return 0 on success
 */
static int write_buf_to_include_args(const struct flashctx *const flash,
				     unsigned char *buf)
{
	const struct flashrom_layout *const layout = get_layout(flash);
	const struct romentry *entry = NULL;

	while ((entry = layout_next_included(layout, entry))) {
		if (!entry->file)
			continue;
		if (write_buf_to_file(buf + entry->start,
				      entry->end - entry->start + 1, entry->file))
			return 1;
	}

	return 0;
}

/**
 * @brief Reads the included layout regions into a buffer.
 *
 * If there is no layout set in the given flash context, the whole chip will
 * be read.
 *
 * @param flashctx Flash context to be used.
 * @param buffer   Buffer of full chip size to read into.
 * @return 0 on success,
 *	   1 if any read fails.
 */
static int read_by_layout(struct flashctx *const flashctx, uint8_t *const buffer)
{
	const struct flashrom_layout *const layout = get_layout(flashctx);
	const struct romentry *entry = NULL;

	while ((entry = layout_next_included(layout, entry))) {
		const chipoff_t region_start	= entry->start;
		const chipsize_t region_len	= entry->end - entry->start + 1;

		if (flashctx->chip->read(flashctx, buffer + region_start, region_start, region_len))
			return 1;
	}
	return 0;
}

int read_flash_to_file(struct flashctx *flash, const char *filename)
{
	unsigned long size = flash->chip->total_size * 1024;
	unsigned char *buf = calloc(size, sizeof(unsigned char));
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
	if (read_by_layout(flash, buf)) {
		msg_cerr("Read operation failed!\n");
		ret = 1;
		goto out_free;
	}
	if (write_buf_to_include_args(flash, buf)) {
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

typedef int (*erasefn_t)(struct flashctx *, unsigned int addr, unsigned int len);
/**
 * @private
 *
 * For read-erase-write, `curcontents` and `newcontents` shall point
 * to buffers of the chip's size. Both are supposed to be prefilled
 * with at least the included layout regions of the current flash
 * contents (`curcontents`) and the data to be written to the flash
 * (`newcontents`).
 *
 * For erase, `curcontents` and `newcontents` shall be NULL-pointers.
 *
 * The `chipoff_t` values are used internally by `walk_by_layout()`.
 */
struct walk_info {
	uint8_t *curcontents;
	const uint8_t *newcontents;
	chipoff_t region_start;
	chipoff_t region_end;
	chipoff_t erase_start;
	chipoff_t erase_end;
};
/* returns 0 on success, 1 to retry with another erase function, 2 for immediate abort */
typedef int (*per_blockfn_t)(struct flashctx *, const struct walk_info *, erasefn_t);

static int walk_eraseblocks(struct flashctx *const flashctx,
			    struct walk_info *const info,
			    const size_t erasefunction, const per_blockfn_t per_blockfn)
{
	int ret;
	size_t i, j;
	bool first = true;
	struct block_eraser *const eraser = &flashctx->chip->block_erasers[erasefunction];

	info->erase_start = 0;
	for (i = 0; i < NUM_ERASEREGIONS; ++i) {
		/* count==0 for all automatically initialized array
		   members so the loop below won't be executed for them. */
		for (j = 0; j < eraser->eraseblocks[i].count; ++j, info->erase_start = info->erase_end + 1) {
			info->erase_end = info->erase_start + eraser->eraseblocks[i].size - 1;

			/* Skip any eraseblock that is completely outside the current region. */
			if (info->erase_end < info->region_start)
				continue;
			if (info->region_end < info->erase_start)
				break;

			/* Print this for every block except the first one. */
			if (first)
				first = false;
			else
				msg_cdbg(", ");
			msg_cdbg("0x%06x-0x%06x:", info->erase_start, info->erase_end);

			ret = per_blockfn(flashctx, info, eraser->block_erase);
			if (ret)
				return ret;
		}
		if (info->region_end < info->erase_start)
			break;
	}
	msg_cdbg("\n");
	return 0;
}

static int walk_by_layout(struct flashctx *const flashctx, struct walk_info *const info,
			  const per_blockfn_t per_blockfn)
{
	const struct flashrom_layout *const layout = get_layout(flashctx);
	const struct romentry *entry = NULL;

	all_skipped = true;
	msg_cinfo("Erasing and writing flash chip... ");

	while ((entry = layout_next_included(layout, entry))) {
		info->region_start = entry->start;
		info->region_end   = entry->end;

		size_t j;
		int error = 1; /* retry as long as it's 1 */
		for (j = 0; j < NUM_ERASEFUNCTIONS; ++j) {
			if (j != 0)
				msg_cinfo("Looking for another erase function.\n");
			msg_cdbg("Trying erase function %zi... ", j);
			if (check_block_eraser(flashctx, j, 1))
				continue;

			error = walk_eraseblocks(flashctx, info, j, per_blockfn);
			if (error != 1)
				break;

			if (info->curcontents) {
				msg_cinfo("Reading current flash chip contents... ");
				if (read_by_layout(flashctx, info->curcontents)) {
					/* Now we are truly screwed. Read failed as well. */
					msg_cerr("Can't read anymore! Aborting.\n");
					/* We have no idea about the flash chip contents, so
					   retrying with another erase function is pointless. */
					error = 2;
					break;
				}
				msg_cinfo("done. ");
			}
		}
		if (error == 1)
			msg_cinfo("No usable erase functions left.\n");
		if (error) {
			msg_cerr("FAILED!\n");
			return 1;
		}
	}
	if (all_skipped)
		msg_cinfo("\nWarning: Chip content is identical to the requested image.\n");
	msg_cinfo("Erase/write done.\n");
	return 0;
}

static int erase_block(struct flashctx *const flashctx,
		       const struct walk_info *const info, const erasefn_t erasefn)
{
	const unsigned int erase_len = info->erase_end + 1 - info->erase_start;
	const bool region_unaligned = info->region_start > info->erase_start ||
				      info->erase_end > info->region_end;
	uint8_t *backup_contents = NULL, *erased_contents = NULL;
	int ret = 2;

	/*
	 * If the region is not erase-block aligned, merge current flash con-
	 * tents into a new buffer `backup_contents`.
	 */
	if (region_unaligned) {
		backup_contents = malloc(erase_len);
		erased_contents = malloc(erase_len);
		if (!backup_contents || !erased_contents) {
			msg_cerr("Out of memory!\n");
			ret = 1;
			goto _free_ret;
		}
		memset(backup_contents, ERASED_VALUE(flashctx), erase_len);
		memset(erased_contents, ERASED_VALUE(flashctx), erase_len);

		msg_cdbg("R");
		/* Merge data preceding the current region. */
		if (info->region_start > info->erase_start) {
			const chipoff_t start	= info->erase_start;
			const chipsize_t len	= info->region_start - info->erase_start;
			if (flashctx->chip->read(flashctx, backup_contents, start, len)) {
				msg_cerr("Can't read! Aborting.\n");
				goto _free_ret;
			}
		}
		/* Merge data following the current region. */
		if (info->erase_end > info->region_end) {
			const chipoff_t start     = info->region_end + 1;
			const chipoff_t rel_start = start - info->erase_start; /* within this erase block */
			const chipsize_t len      = info->erase_end - info->region_end;
			if (flashctx->chip->read(flashctx, backup_contents + rel_start, start, len)) {
				msg_cerr("Can't read! Aborting.\n");
				goto _free_ret;
			}
		}
	}

	ret = 1;
	all_skipped = false;

	msg_cdbg("E");
	if (erasefn(flashctx, info->erase_start, erase_len))
		goto _free_ret;
	if (check_erased_range(flashctx, info->erase_start, erase_len)) {
		msg_cerr("ERASE FAILED!\n");
		goto _free_ret;
	}

	if (region_unaligned) {
		unsigned int starthere = 0, lenhere = 0, writecount = 0;
		/* get_next_write() sets starthere to a new value after the call. */
		while ((lenhere = get_next_write(erased_contents + starthere, backup_contents + starthere,
						 erase_len - starthere, &starthere, flashctx->chip->gran))) {
			if (!writecount++)
				msg_cdbg("W");
			/* Needs the partial write function signature. */
			if (flashctx->chip->write(flashctx, backup_contents + starthere,
						  info->erase_start + starthere, lenhere))
				goto _free_ret;
			starthere += lenhere;
		}
	}

	ret = 0;

_free_ret:
	free(erased_contents);
	free(backup_contents);
	return ret;
}

/**
 * @brief Erases the included layout regions.
 *
 * If there is no layout set in the given flash context, the whole chip will
 * be erased.
 *
 * @param flashctx Flash context to be used.
 * @return 0 on success,
 *	   1 if all available erase functions failed.
 */
static int erase_by_layout(struct flashctx *const flashctx)
{
	struct walk_info info = { 0 };
	return walk_by_layout(flashctx, &info, &erase_block);
}

static int read_erase_write_block(struct flashctx *const flashctx,
				  const struct walk_info *const info, const erasefn_t erasefn)
{
	const chipsize_t erase_len = info->erase_end + 1 - info->erase_start;
	const bool region_unaligned = info->region_start > info->erase_start ||
				      info->erase_end > info->region_end;
	const uint8_t *newcontents = NULL;
	int ret = 2;

	/*
	 * If the region is not erase-block aligned, merge current flash con-
	 * tents into `info->curcontents` and a new buffer `newc`. The former
	 * is necessary since we have no guarantee that the full erase block
	 * was already read into `info->curcontents`. For the latter a new
	 * buffer is used since `info->newcontents` might contain data for
	 * other unaligned regions that touch this erase block too.
	 */
	if (region_unaligned) {
		msg_cdbg("R");
		uint8_t *const newc = malloc(erase_len);
		if (!newc) {
			msg_cerr("Out of memory!\n");
			return 1;
		}
		memcpy(newc, info->newcontents + info->erase_start, erase_len);

		/* Merge data preceding the current region. */
		if (info->region_start > info->erase_start) {
			const chipoff_t start	= info->erase_start;
			const chipsize_t len	= info->region_start - info->erase_start;
			if (flashctx->chip->read(flashctx, newc, start, len)) {
				msg_cerr("Can't read! Aborting.\n");
				goto _free_ret;
			}
			memcpy(info->curcontents + start, newc, len);
		}
		/* Merge data following the current region. */
		if (info->erase_end > info->region_end) {
			const chipoff_t start     = info->region_end + 1;
			const chipoff_t rel_start = start - info->erase_start; /* within this erase block */
			const chipsize_t len      = info->erase_end - info->region_end;
			if (flashctx->chip->read(flashctx, newc + rel_start, start, len)) {
				msg_cerr("Can't read! Aborting.\n");
				goto _free_ret;
			}
			memcpy(info->curcontents + start, newc + rel_start, len);
		}

		newcontents = newc;
	} else {
		newcontents = info->newcontents + info->erase_start;
	}

	ret = 1;
	bool skipped = true;
	uint8_t *const curcontents = info->curcontents + info->erase_start;
	const uint8_t erased_value = ERASED_VALUE(flashctx);
	if (!(flashctx->chip->feature_bits & FEATURE_NO_ERASE) &&
			need_erase(curcontents, newcontents, erase_len, flashctx->chip->gran, erased_value)) {
		if (erase_block(flashctx, info, erasefn))
			goto _free_ret;
		/* Erase was successful. Adjust curcontents. */
		memset(curcontents, erased_value, erase_len);
		skipped = false;
	}

	unsigned int starthere = 0, lenhere = 0, writecount = 0;
	/* get_next_write() sets starthere to a new value after the call. */
	while ((lenhere = get_next_write(curcontents + starthere, newcontents + starthere,
					 erase_len - starthere, &starthere, flashctx->chip->gran))) {
		if (!writecount++)
			msg_cdbg("W");
		/* Needs the partial write function signature. */
		if (flashctx->chip->write(flashctx, newcontents + starthere,
					  info->erase_start + starthere, lenhere))
			goto _free_ret;
		starthere += lenhere;
		skipped = false;
	}
	if (skipped)
		msg_cdbg("S");
	else
		all_skipped = false;

	/* Update curcontents, other regions with overlapping erase blocks
	   might rely on this. */
	memcpy(curcontents, newcontents, erase_len);
	ret = 0;

_free_ret:
	if (region_unaligned)
		free((void *)newcontents);
	return ret;
}

/**
 * @brief Writes the included layout regions from a given image.
 *
 * If there is no layout set in the given flash context, the whole image
 * will be written.
 *
 * @param flashctx    Flash context to be used.
 * @param curcontents A buffer of full chip size with current chip contents of included regions.
 * @param newcontents The new image to be written.
 * @return 0 on success,
 *	   1 if anything has gone wrong.
 */
static int write_by_layout(struct flashctx *const flashctx,
			   void *const curcontents, const void *const newcontents)
{
	struct walk_info info;
	info.curcontents = curcontents;
	info.newcontents = newcontents;
	return walk_by_layout(flashctx, &info, read_erase_write_block);
}

/**
 * @brief Compares the included layout regions with content from a buffer.
 *
 * If there is no layout set in the given flash context, the whole chip's
 * contents will be compared.
 *
 * @param flashctx    Flash context to be used.
 * @param layout      Flash layout information.
 * @param curcontents A buffer of full chip size to read current chip contents into.
 * @param newcontents The new image to compare to.
 * @return 0 on success,
 *	   1 if reading failed,
 *	   3 if the contents don't match.
 */
static int verify_by_layout(
		struct flashctx *const flashctx,
		const struct flashrom_layout *const layout,
		void *const curcontents, const uint8_t *const newcontents)
{
	const struct romentry *entry = NULL;

	while ((entry = layout_next_included(layout, entry))) {
		const chipoff_t region_start	= entry->start;
		const chipsize_t region_len	= entry->end - entry->start + 1;

		if (flashctx->chip->read(flashctx, curcontents + region_start, region_start, region_len))
			return 1;
		if (compare_range(newcontents + region_start, curcontents + region_start,
				  region_start, region_len))
			return 3;
	}
	return 0;
}

static void nonfatal_help_message(void)
{
	msg_gerr("Good, writing to the flash chip apparently didn't do anything.\n");
#if CONFIG_INTERNAL == 1
	if (programmer == &programmer_internal)
		msg_gerr("This means we have to add special support for your board, programmer or flash\n"
			 "chip. Please report this to the mailing list at flashrom@flashrom.org or on\n"
			 "IRC (see https://www.flashrom.org/Contact for details), thanks!\n"
			 "-------------------------------------------------------------------------------\n"
			 "You may now reboot or simply leave the machine running.\n");
	else
#endif
		msg_gerr("Please check the connections (especially those to write protection pins) between\n"
			 "the programmer and the flash chip. If you think the error is caused by flashrom\n"
			 "please report this to the mailing list at flashrom@flashrom.org or on IRC (see\n"
			 "https://www.flashrom.org/Contact for details), thanks!\n");
}

static void emergency_help_message(void)
{
	msg_gerr("Your flash chip is in an unknown state.\n");
#if CONFIG_INTERNAL == 1
	if (programmer == &programmer_internal)
		msg_gerr("Get help on IRC (see https://www.flashrom.org/Contact) or mail\n"
			"flashrom@flashrom.org with the subject \"FAILED: <your board name>\"!"
			"-------------------------------------------------------------------------------\n"
			"DO NOT REBOOT OR POWEROFF!\n");
	else
#endif
		msg_gerr("Please report this to the mailing list at flashrom@flashrom.org or\n"
			 "on IRC (see https://www.flashrom.org/Contact for details), thanks!\n");
}

void list_programmers_linebreak(int startcol, int cols, int paren)
{
	const char *pname;
	int pnamelen;
	int remaining = 0, firstline = 1;
	size_t p;
	int i;

	for (p = 0; p < programmer_table_size; p++) {
		pname = programmer_table[p]->name;
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
		if (p < programmer_table_size - 1) {
			msg_ginfo(",");
			remaining--;
		} else {
			if (paren)
				msg_ginfo(")");
		}
	}
}

static void print_sysinfo(void)
{
#if IS_WINDOWS
	SYSTEM_INFO si = { 0 };
	OSVERSIONINFOEX osvi = { 0 };

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
	msg_ginfo("flashrom %s", flashrom_version);
	print_sysinfo();
	msg_ginfo("\n");
}

void print_banner(void)
{
	msg_ginfo("flashrom is free software, get the source code at "
		  "https://flashrom.org\n");
	msg_ginfo("\n");
}

int selfcheck(void)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < programmer_table_size; i++) {
		const struct programmer_entry *const p = programmer_table[i];
		if (p == NULL) {
			msg_gerr("Programmer with index %d is NULL instead of a valid pointer!\n", i);
			ret = 1;
			continue;
		}
		if (p->name == NULL) {
			msg_gerr("All programmers need a valid name, but the one with index %d does not!\n", i);
			ret = 1;
			/* This might hide other problems with this programmer, but allows for better error
			 * messages below without jumping through hoops. */
			continue;
		}
		switch (p->type) {
		case USB:
		case PCI:
		case OTHER:
			if (p->devs.note == NULL) {
				if (strcmp("internal", p->name) == 0)
					break; /* This one has its device list stored separately. */
				msg_gerr("Programmer %s has neither a device list nor a textual description!\n",
					 p->name);
				ret = 1;
			}
			break;
		default:
			msg_gerr("Programmer %s does not have a valid type set!\n", p->name);
			ret = 1;
			break;
		}
		if (p->init == NULL) {
			msg_gerr("Programmer %s does not have a valid init function!\n", p->name);
			ret = 1;
		}
		if (p->delay == NULL) {
			msg_gerr("Programmer %s does not have a valid delay function!\n", p->name);
			ret = 1;
		}
		if (p->map_flash_region == NULL) {
			msg_gerr("Programmer %s does not have a valid map_flash_region function!\n", p->name);
			ret = 1;
		}
		if (p->unmap_flash_region == NULL) {
			msg_gerr("Programmer %s does not have a valid unmap_flash_region function!\n", p->name);
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

/* FIXME: This function signature needs to be improved once prepare_flash_access()
 * has a better function signature.
 */
static int chip_safety_check(const struct flashctx *flash, int force,
			     int read_it, int write_it, int erase_it, int verify_it)
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

int prepare_flash_access(struct flashctx *const flash,
			 const bool read_it, const bool write_it,
			 const bool erase_it, const bool verify_it)
{
	if (chip_safety_check(flash, flash->flags.force, read_it, write_it, erase_it, verify_it)) {
		msg_cerr("Aborting.\n");
		return 1;
	}

	if (layout_sanity_checks(flash)) {
		msg_cerr("Requested regions can not be handled. Aborting.\n");
		return 1;
	}

	if (map_flash(flash) != 0)
		return 1;

	/* Given the existence of read locks, we want to unlock for read,
	   erase and write. */
	if (flash->chip->unlock)
		flash->chip->unlock(flash);

	flash->address_high_byte = -1;
	flash->in_4ba_mode = false;
	flash->chip_restore_fn_count = 0;

	/* Be careful about 4BA chips and broken masters */
	if (flash->chip->total_size > 16 * 1024 && spi_master_no_4ba_modes(flash)) {
		/* If we can't use native instructions, bail out */
		if ((flash->chip->feature_bits & FEATURE_4BA_NATIVE) != FEATURE_4BA_NATIVE
		    || !spi_master_4ba(flash)) {
			msg_cerr("Programmer doesn't support this chip. Aborting.\n");
			return 1;
		}
	}

	/* Enable/disable 4-byte addressing mode if flash chip supports it */
	if (flash->chip->feature_bits & (FEATURE_4BA_ENTER | FEATURE_4BA_ENTER_WREN | FEATURE_4BA_ENTER_EAR7)) {
		int ret;
		if (spi_master_4ba(flash))
			ret = spi_enter_4ba(flash);
		else
			ret = spi_exit_4ba(flash);
		if (ret) {
			msg_cerr("Failed to set correct 4BA mode! Aborting.\n");
			return 1;
		}
	}

	return 0;
}

void finalize_flash_access(struct flashctx *const flash)
{
	deregister_chip_restore(flash);
	unmap_flash(flash);
}

/**
 * @addtogroup flashrom-flash
 * @{
 */

/**
 * @brief Erase the specified ROM chip.
 *
 * If a layout is set in the given flash context, only included regions
 * will be erased.
 *
 * @param flashctx The context of the flash chip to erase.
 * @return 0 on success.
 */
int flashrom_flash_erase(struct flashctx *const flashctx)
{
	if (prepare_flash_access(flashctx, false, false, true, false))
		return 1;

	const int ret = erase_by_layout(flashctx);

	finalize_flash_access(flashctx);

	return ret;
}

/** @} */ /* end flashrom-flash */

/**
 * @defgroup flashrom-ops Operations
 * @{
 */

/**
 * @brief Read the current image from the specified ROM chip.
 *
 * If a layout is set in the specified flash context, only included regions
 * will be read.
 *
 * @param flashctx The context of the flash chip.
 * @param buffer Target buffer to write image to.
 * @param buffer_len Size of target buffer in bytes.
 * @return 0 on success,
 *         2 if buffer_len is too short for the flash chip's contents,
 *         or 1 on any other failure.
 */
int flashrom_image_read(struct flashctx *const flashctx, void *const buffer, const size_t buffer_len)
{
	const size_t flash_size = flashctx->chip->total_size * 1024;

	if (flash_size > buffer_len)
		return 2;

	if (prepare_flash_access(flashctx, true, false, false, false))
		return 1;

	msg_cinfo("Reading flash... ");

	int ret = 1;
	if (read_by_layout(flashctx, buffer)) {
		msg_cerr("Read operation failed!\n");
		msg_cinfo("FAILED.\n");
		goto _finalize_ret;
	}
	msg_cinfo("done.\n");
	ret = 0;

_finalize_ret:
	finalize_flash_access(flashctx);
	return ret;
}

static void combine_image_by_layout(const struct flashctx *const flashctx,
				    uint8_t *const newcontents, const uint8_t *const oldcontents)
{
	const struct flashrom_layout *const layout = get_layout(flashctx);
	const struct romentry *included;
	chipoff_t start = 0;

	while ((included = layout_next_included_region(layout, start))) {
		if (included->start > start) {
			/* copy everything up to the start of this included region */
			memcpy(newcontents + start, oldcontents + start, included->start - start);
		}
		/* skip this included region */
		start = included->end + 1;
		if (start == 0)
			return;
	}

	/* copy the rest of the chip */
	const chipsize_t copy_len = flashctx->chip->total_size * 1024 - start;
	memcpy(newcontents + start, oldcontents + start, copy_len);
}

/**
 * @brief Write the specified image to the ROM chip.
 *
 * If a layout is set in the specified flash context, only erase blocks
 * containing included regions will be touched.
 *
 * @param flashctx The context of the flash chip.
 * @param buffer Source buffer to read image from (may be altered for full verification).
 * @param buffer_len Size of source buffer in bytes.
 * @param refbuffer If given, assume flash chip contains same data as `refbuffer`.
 * @return 0 on success,
 *         4 if buffer_len doesn't match the size of the flash chip,
 *         3 if write was tried but nothing has changed,
 *         2 if write failed and flash contents changed,
 *         or 1 on any other failure.
 */
int flashrom_image_write(struct flashctx *const flashctx, void *const buffer, const size_t buffer_len,
                         const void *const refbuffer)
{
	const size_t flash_size = flashctx->chip->total_size * 1024;
	const bool verify_all = flashctx->flags.verify_whole_chip;
	const bool verify = flashctx->flags.verify_after_write;
	const struct flashrom_layout *const verify_layout =
		verify_all ? get_default_layout(flashctx) : get_layout(flashctx);

	if (buffer_len != flash_size)
		return 4;

	int ret = 1;

	uint8_t *const newcontents = buffer;
	const uint8_t *const refcontents = refbuffer;
	uint8_t *const curcontents = malloc(flash_size);
	uint8_t *oldcontents = NULL;
	if (verify_all)
		oldcontents = malloc(flash_size);
	if (!curcontents || (verify_all && !oldcontents)) {
		msg_gerr("Out of memory!\n");
		goto _free_ret;
	}

#if CONFIG_INTERNAL == 1
	if (programmer == &programmer_internal && cb_check_image(newcontents, flash_size) < 0) {
		if (flashctx->flags.force_boardmismatch) {
			msg_pinfo("Proceeding anyway because user forced us to.\n");
		} else {
			msg_perr("Aborting. You can override this with "
				 "-p internal:boardmismatch=force.\n");
			goto _free_ret;
		}
	}
#endif

	if (prepare_flash_access(flashctx, false, true, false, verify))
		goto _free_ret;

	/* If given, assume flash chip contains same data as `refcontents`. */
	if (refcontents) {
		msg_cinfo("Assuming old flash chip contents as ref-file...\n");
		memcpy(curcontents, refcontents, flash_size);
		if (oldcontents)
			memcpy(oldcontents, refcontents, flash_size);
	} else {
		/*
		 * Read the whole chip to be able to check whether regions need to be
		 * erased and to give better diagnostics in case write fails.
		 * The alternative is to read only the regions which are to be
		 * preserved, but in that case we might perform unneeded erase which
		 * takes time as well.
		 */
		msg_cinfo("Reading old flash chip contents... ");
		if (verify_all) {
			if (flashctx->chip->read(flashctx, oldcontents, 0, flash_size)) {
				msg_cinfo("FAILED.\n");
				goto _finalize_ret;
			}
			memcpy(curcontents, oldcontents, flash_size);
		} else {
			if (read_by_layout(flashctx, curcontents)) {
				msg_cinfo("FAILED.\n");
				goto _finalize_ret;
			}
		}
		msg_cinfo("done.\n");
	}

	if (write_by_layout(flashctx, curcontents, newcontents)) {
		msg_cerr("Uh oh. Erase/write failed. ");
		ret = 2;
		if (verify_all) {
			msg_cerr("Checking if anything has changed.\n");
			msg_cinfo("Reading current flash chip contents... ");
			if (!flashctx->chip->read(flashctx, curcontents, 0, flash_size)) {
				msg_cinfo("done.\n");
				if (!memcmp(oldcontents, curcontents, flash_size)) {
					nonfatal_help_message();
					goto _finalize_ret;
				}
				msg_cerr("Apparently at least some data has changed.\n");
			} else
				msg_cerr("Can't even read anymore!\n");
			emergency_help_message();
			goto _finalize_ret;
		} else {
			msg_cerr("\n");
		}
		emergency_help_message();
		goto _finalize_ret;
	}

	/* Verify only if we actually changed something. */
	if (verify && !all_skipped) {
		msg_cinfo("Verifying flash... ");

		/* Work around chips which need some time to calm down. */
		programmer_delay(1000*1000);

		if (verify_all)
			combine_image_by_layout(flashctx, newcontents, oldcontents);
		ret = verify_by_layout(flashctx, verify_layout, curcontents, newcontents);
		/* If we tried to write, and verification now fails, we
		   might have an emergency situation. */
		if (ret)
			emergency_help_message();
		else
			msg_cinfo("VERIFIED.\n");
	} else {
		/* We didn't change anything. */
		ret = 0;
	}

_finalize_ret:
	finalize_flash_access(flashctx);
_free_ret:
	free(oldcontents);
	free(curcontents);
	return ret;
}

/**
 * @brief Verify the ROM chip's contents with the specified image.
 *
 * If a layout is set in the specified flash context, only included regions
 * will be verified.
 *
 * @param flashctx The context of the flash chip.
 * @param buffer Source buffer to verify with.
 * @param buffer_len Size of source buffer in bytes.
 * @return 0 on success,
 *         3 if the chip's contents don't match,
 *         2 if buffer_len doesn't match the size of the flash chip,
 *         or 1 on any other failure.
 */
int flashrom_image_verify(struct flashctx *const flashctx, const void *const buffer, const size_t buffer_len)
{
	const struct flashrom_layout *const layout = get_layout(flashctx);
	const size_t flash_size = flashctx->chip->total_size * 1024;

	if (buffer_len != flash_size)
		return 2;

	const uint8_t *const newcontents = buffer;
	uint8_t *const curcontents = malloc(flash_size);
	if (!curcontents) {
		msg_gerr("Out of memory!\n");
		return 1;
	}

	int ret = 1;

	if (prepare_flash_access(flashctx, false, false, false, true))
		goto _free_ret;

	msg_cinfo("Verifying flash... ");
	ret = verify_by_layout(flashctx, layout, curcontents, newcontents);
	if (!ret)
		msg_cinfo("VERIFIED.\n");

	finalize_flash_access(flashctx);
_free_ret:
	free(curcontents);
	return ret;
}

/** @} */ /* end flashrom-ops */

int do_read(struct flashctx *const flash, const char *const filename)
{
	if (prepare_flash_access(flash, true, false, false, false))
		return 1;

	const int ret = read_flash_to_file(flash, filename);

	finalize_flash_access(flash);

	return ret;
}

int do_extract(struct flashctx *const flash)
{
	prepare_layout_for_extraction(flash);
	return do_read(flash, NULL);
}

int do_erase(struct flashctx *const flash)
{
	const int ret = flashrom_flash_erase(flash);

	/*
	 * FIXME: Do we really want the scary warning if erase failed?
	 * After all, after erase the chip is either blank or partially
	 * blank or it has the old contents. A blank chip won't boot,
	 * so if the user wanted erase and reboots afterwards, the user
	 * knows very well that booting won't work.
	 */
	if (ret)
		emergency_help_message();

	return ret;
}

int do_write(struct flashctx *const flash, const char *const filename, const char *const referencefile)
{
	const size_t flash_size = flash->chip->total_size * 1024;
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
	if (read_buf_from_include_args(flash, newcontents))
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

int do_verify(struct flashctx *const flash, const char *const filename)
{
	const size_t flash_size = flash->chip->total_size * 1024;
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
	if (read_buf_from_include_args(flash, newcontents))
		goto _free_ret;

	ret = flashrom_image_verify(flash, newcontents, flash_size);

_free_ret:
	free(newcontents);
	return ret;
}
