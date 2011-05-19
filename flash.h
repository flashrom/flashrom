/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2000 Ronald G. Minnich <rminnich@gmail.com>
 * Copyright (C) 2005-2009 coresystems GmbH
 * Copyright (C) 2006-2009 Carl-Daniel Hailfinger
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

#ifndef __FLASH_H__
#define __FLASH_H__ 1

#include <stdint.h>
#include <stddef.h>
#include "hwaccess.h"
#ifdef _WIN32
#include <windows.h>
#undef min
#undef max
#endif

#define ERROR_PTR ((void*)-1)

/* Error codes */
#define TIMEOUT_ERROR	-101

typedef unsigned long chipaddr;

int register_shutdown(void (*function) (void *data), void *data);
void *programmer_map_flash_region(const char *descr, unsigned long phys_addr,
				  size_t len);
void programmer_unmap_flash_region(void *virt_addr, size_t len);
void chip_writeb(uint8_t val, chipaddr addr);
void chip_writew(uint16_t val, chipaddr addr);
void chip_writel(uint32_t val, chipaddr addr);
void chip_writen(uint8_t *buf, chipaddr addr, size_t len);
uint8_t chip_readb(const chipaddr addr);
uint16_t chip_readw(const chipaddr addr);
uint32_t chip_readl(const chipaddr addr);
void chip_readn(uint8_t *buf, const chipaddr addr, size_t len);
void programmer_delay(int usecs);

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

enum chipbustype {
	CHIP_BUSTYPE_NONE	= 0,
	CHIP_BUSTYPE_PARALLEL	= 1 << 0,
	CHIP_BUSTYPE_LPC	= 1 << 1,
	CHIP_BUSTYPE_FWH	= 1 << 2,
	CHIP_BUSTYPE_SPI	= 1 << 3,
	CHIP_BUSTYPE_NONSPI	= CHIP_BUSTYPE_PARALLEL | CHIP_BUSTYPE_LPC | CHIP_BUSTYPE_FWH,
	CHIP_BUSTYPE_UNKNOWN	= CHIP_BUSTYPE_PARALLEL | CHIP_BUSTYPE_LPC | CHIP_BUSTYPE_FWH | CHIP_BUSTYPE_SPI,
};

/*
 * How many different contiguous runs of erase blocks with one size each do
 * we have for a given erase function?
 */
#define NUM_ERASEREGIONS 5

/*
 * How many different erase functions do we have per chip?
 * Atmel AT25FS010 has 6 different functions.
 */
#define NUM_ERASEFUNCTIONS 6

#define FEATURE_REGISTERMAP	(1 << 0)
#define FEATURE_BYTEWRITES	(1 << 1)
#define FEATURE_LONG_RESET	(0 << 4)
#define FEATURE_SHORT_RESET	(1 << 4)
#define FEATURE_EITHER_RESET	FEATURE_LONG_RESET
#define FEATURE_RESET_MASK	(FEATURE_LONG_RESET | FEATURE_SHORT_RESET)
#define FEATURE_ADDR_FULL	(0 << 2)
#define FEATURE_ADDR_MASK	(3 << 2)
#define FEATURE_ADDR_2AA	(1 << 2)
#define FEATURE_ADDR_AAA	(2 << 2)
#define FEATURE_ADDR_SHIFTED	(1 << 5)
#define FEATURE_WRSR_EWSR	(1 << 6)
#define FEATURE_WRSR_WREN	(1 << 7)
#define FEATURE_WRSR_EITHER	(FEATURE_WRSR_EWSR | FEATURE_WRSR_WREN)

struct flashchip {
	const char *vendor;
	const char *name;

	enum chipbustype bustype;

	/*
	 * With 32bit manufacture_id and model_id we can cover IDs up to
	 * (including) the 4th bank of JEDEC JEP106W Standard Manufacturer's
	 * Identification code.
	 */
	uint32_t manufacture_id;
	uint32_t model_id;

	/* Total chip size in kilobytes */
	int total_size;
	/* Chip page size in bytes */
	int page_size;
	int feature_bits;

	/*
	 * Indicate if flashrom has been tested with this flash chip and if
	 * everything worked correctly.
	 */
	uint32_t tested;

	int (*probe) (struct flashchip *flash);

	/* Delay after "enter/exit ID mode" commands in microseconds. */
	int probe_timing;

	/*
	 * Erase blocks and associated erase function. Any chip erase function
	 * is stored as chip-sized virtual block together with said function.
	 * The first one that fits will be chosen. There is currently no way to
	 * influence that behaviour. For testing just comment out the other
	 * elements or set the function pointer to NULL.
	 */
	struct block_eraser {
		struct eraseblock{
			unsigned int size; /* Eraseblock size */
			unsigned int count; /* Number of contiguous blocks with that size */
		} eraseblocks[NUM_ERASEREGIONS];
		int (*block_erase) (struct flashchip *flash, unsigned int blockaddr, unsigned int blocklen);
	} block_erasers[NUM_ERASEFUNCTIONS];

	int (*printlock) (struct flashchip *flash);
	int (*unlock) (struct flashchip *flash);
	int (*write) (struct flashchip *flash, uint8_t *buf, int start, int len);
	int (*read) (struct flashchip *flash, uint8_t *buf, int start, int len);

	/* Some flash devices have an additional register space. */
	chipaddr virtual_memory;
	chipaddr virtual_registers;
};

#define TEST_UNTESTED	0

#define TEST_OK_PROBE	(1 << 0)
#define TEST_OK_READ	(1 << 1)
#define TEST_OK_ERASE	(1 << 2)
#define TEST_OK_WRITE	(1 << 3)
#define TEST_OK_PR	(TEST_OK_PROBE | TEST_OK_READ)
#define TEST_OK_PRE	(TEST_OK_PROBE | TEST_OK_READ | TEST_OK_ERASE)
#define TEST_OK_PRW	(TEST_OK_PROBE | TEST_OK_READ | TEST_OK_WRITE)
#define TEST_OK_PREW	(TEST_OK_PROBE | TEST_OK_READ | TEST_OK_ERASE | TEST_OK_WRITE)
#define TEST_OK_MASK	0x0f

#define TEST_BAD_PROBE	(1 << 4)
#define TEST_BAD_READ	(1 << 5)
#define TEST_BAD_ERASE	(1 << 6)
#define TEST_BAD_WRITE	(1 << 7)
#define TEST_BAD_PREW	(TEST_BAD_PROBE | TEST_BAD_READ | TEST_BAD_ERASE | TEST_BAD_WRITE)
#define TEST_BAD_MASK	0xf0

/* Timing used in probe routines. ZERO is -2 to differentiate between an unset
 * field and zero delay.
 * 
 * SPI devices will always have zero delay and ignore this field.
 */
#define TIMING_FIXME	-1
/* this is intentionally same value as fixme */
#define TIMING_IGNORED	-1
#define TIMING_ZERO	-2

extern const struct flashchip flashchips[];

/* print.c */
char *flashbuses_to_text(enum chipbustype bustype);
void print_supported(void);
void print_supported_wiki(void);

/* flashrom.c */
enum write_granularity {
	write_gran_1bit,
	write_gran_1byte,
	write_gran_256bytes,
};
extern enum chipbustype buses_supported;
extern int verbose;
extern const char flashrom_version[];
extern char *chip_to_probe;
void map_flash_registers(struct flashchip *flash);
int read_memmapped(struct flashchip *flash, uint8_t *buf, int start, int len);
int erase_flash(struct flashchip *flash);
int probe_flash(int startchip, struct flashchip *fill_flash, int force);
int read_flash_to_file(struct flashchip *flash, char *filename);
int min(int a, int b);
int max(int a, int b);
void tolower_string(char *str);
char *extract_param(char **haystack, char *needle, char *delim);
int check_erased_range(struct flashchip *flash, int start, int len);
int verify_range(struct flashchip *flash, uint8_t *cmpbuf, int start, int len, char *message);
int need_erase(uint8_t *have, uint8_t *want, int len, enum write_granularity gran);
char *strcat_realloc(char *dest, const char *src);
void print_version(void);
void print_banner(void);
void list_programmers_linebreak(int startcol, int cols, int paren);
int selfcheck(void);
int doit(struct flashchip *flash, int force, char *filename, int read_it, int write_it, int erase_it, int verify_it);
int read_buf_from_file(unsigned char *buf, unsigned long size, char *filename);
int write_buf_to_file(unsigned char *buf, unsigned long size, char *filename);

#define OK 0
#define NT 1    /* Not tested */

/* Something happened that shouldn't happen, but we can go on */
#define ERROR_NONFATAL 0x100

/* cli_output.c */
/* Let gcc and clang check for correct printf-style format strings. */
int print(int type, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#define MSG_ERROR	0
#define MSG_INFO	1
#define MSG_DEBUG	2
#define MSG_BARF	3
#define msg_gerr(...)	print(MSG_ERROR, __VA_ARGS__)	/* general errors */
#define msg_perr(...)	print(MSG_ERROR, __VA_ARGS__)	/* programmer errors */
#define msg_cerr(...)	print(MSG_ERROR, __VA_ARGS__)	/* chip errors */
#define msg_ginfo(...)	print(MSG_INFO, __VA_ARGS__)	/* general info */
#define msg_pinfo(...)	print(MSG_INFO, __VA_ARGS__)	/* programmer info */
#define msg_cinfo(...)	print(MSG_INFO, __VA_ARGS__)	/* chip info */
#define msg_gdbg(...)	print(MSG_DEBUG, __VA_ARGS__)	/* general debug */
#define msg_pdbg(...)	print(MSG_DEBUG, __VA_ARGS__)	/* programmer debug */
#define msg_cdbg(...)	print(MSG_DEBUG, __VA_ARGS__)	/* chip debug */
#define msg_gspew(...)	print(MSG_BARF, __VA_ARGS__)	/* general debug barf  */
#define msg_pspew(...)	print(MSG_BARF, __VA_ARGS__)	/* programmer debug barf  */
#define msg_cspew(...)	print(MSG_BARF, __VA_ARGS__)	/* chip debug barf  */

/* cli_classic.c */
int cli_classic(int argc, char *argv[]);

/* layout.c */
int read_romlayout(char *name);
int find_romentry(char *name);
int handle_romentries(struct flashchip *flash, uint8_t *oldcontents, uint8_t *newcontents);

/* spi.c */
struct spi_command {
	unsigned int writecnt;
	unsigned int readcnt;
	const unsigned char *writearr;
	unsigned char *readarr;
};
int spi_send_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr);
int spi_send_multicommand(struct spi_command *cmds);
uint32_t spi_get_valid_read_addr(void);

#endif				/* !__FLASH_H__ */
