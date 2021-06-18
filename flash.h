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
 */

#ifndef __FLASH_H__
#define __FLASH_H__ 1

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#if IS_WINDOWS
#include <windows.h>
#undef min
#undef max
#endif

#include "libflashrom.h"
#include "layout.h"

#define KiB (1024)
#define MiB (1024 * KiB)

/* Assumes `n` and `a` are at most 64-bit wide (to avoid typeof() operator). */
#define ALIGN_DOWN(n, a) ((n) & ~((uint64_t)(a) - 1))

#define ERROR_PTR ((void*)-1)

/* Error codes */
#define ERROR_OOM	-100
#define TIMEOUT_ERROR	-101

/* TODO: check using code for correct usage of types */
typedef uintptr_t chipaddr;
#define PRIxPTR_WIDTH ((int)(sizeof(uintptr_t)*2))

int register_shutdown(int (*function) (void *data), void *data);
void *programmer_map_flash_region(const char *descr, uintptr_t phys_addr, size_t len);
void programmer_unmap_flash_region(void *virt_addr, size_t len);
void programmer_delay(unsigned int usecs);

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

enum chipbustype {
	BUS_NONE	= 0,
	BUS_PARALLEL	= 1 << 0,
	BUS_LPC		= 1 << 1,
	BUS_FWH		= 1 << 2,
	BUS_SPI		= 1 << 3,
	BUS_PROG	= 1 << 4,
	BUS_NONSPI	= BUS_PARALLEL | BUS_LPC | BUS_FWH,
};

/*
 * The following enum defines possible write granularities of flash chips. These tend to reflect the properties
 * of the actual hardware not necesserily the write function(s) defined by the respective struct flashchip.
 * The latter might (and should) be more precisely specified, e.g. they might bail out early if their execution
 * would result in undefined chip contents.
 */
enum write_granularity {
	/* We assume 256 byte granularity by default. */
	write_gran_256bytes = 0,/* If less than 256 bytes are written, the unwritten bytes are undefined. */
	write_gran_1bit,	/* Each bit can be cleared individually. */
	write_gran_1byte,	/* A byte can be written once. Further writes to an already written byte cause
				 * its contents to be either undefined or to stay unchanged. */
	write_gran_128bytes,	/* If less than 128 bytes are written, the unwritten bytes are undefined. */
	write_gran_264bytes,	/* If less than 264 bytes are written, the unwritten bytes are undefined. */
	write_gran_512bytes,	/* If less than 512 bytes are written, the unwritten bytes are undefined. */
	write_gran_528bytes,	/* If less than 528 bytes are written, the unwritten bytes are undefined. */
	write_gran_1024bytes,	/* If less than 1024 bytes are written, the unwritten bytes are undefined. */
	write_gran_1056bytes,	/* If less than 1056 bytes are written, the unwritten bytes are undefined. */
	write_gran_64kbytes,    /* If less than 64 kbytes are written, the unwritten bytes are undefined. */
	write_gran_1byte_implicit_erase, /* EEPROMs and other chips with implicit erase and 1-byte writes. */
};

/*
 * How many different contiguous runs of erase blocks with one size each do
 * we have for a given erase function?
 */
#define NUM_ERASEREGIONS 5

/*
 * How many different erase functions do we have per chip?
 * Macronix MX25L25635F has 8 different functions.
 */
#define NUM_ERASEFUNCTIONS 8

#define MAX_CHIP_RESTORE_FUNCTIONS 4

/* Feature bits used for non-SPI only */
#define FEATURE_REGISTERMAP	(1 << 0)
#define FEATURE_LONG_RESET	(0 << 4)
#define FEATURE_SHORT_RESET	(1 << 4)
#define FEATURE_EITHER_RESET	FEATURE_LONG_RESET
#define FEATURE_RESET_MASK	(FEATURE_LONG_RESET | FEATURE_SHORT_RESET)
#define FEATURE_ADDR_FULL	(0 << 2)
#define FEATURE_ADDR_MASK	(3 << 2)
#define FEATURE_ADDR_2AA	(1 << 2)
#define FEATURE_ADDR_AAA	(2 << 2)
#define FEATURE_ADDR_SHIFTED	(1 << 5)
/* Feature bits used for SPI only */
#define FEATURE_WRSR_EWSR	(1 << 6)
#define FEATURE_WRSR_WREN	(1 << 7)
#define FEATURE_WRSR_EITHER	(FEATURE_WRSR_EWSR | FEATURE_WRSR_WREN)
#define FEATURE_OTP		(1 << 8)
#define FEATURE_QPI		(1 << 9)
#define FEATURE_4BA_ENTER	(1 << 10) /**< Can enter/exit 4BA mode with instructions 0xb7/0xe9 w/o WREN */
#define FEATURE_4BA_ENTER_WREN	(1 << 11) /**< Can enter/exit 4BA mode with instructions 0xb7/0xe9 after WREN */
#define FEATURE_4BA_ENTER_EAR7	(1 << 12) /**< Can enter/exit 4BA mode by setting bit7 of the ext addr reg */
#define FEATURE_4BA_EXT_ADDR	(1 << 13) /**< Regular 3-byte operations can be used by writing the most
					       significant address byte into an extended address register. */
#define FEATURE_4BA_READ	(1 << 14) /**< Native 4BA read instruction (0x13) is supported. */
#define FEATURE_4BA_FAST_READ	(1 << 15) /**< Native 4BA fast read instruction (0x0c) is supported. */
#define FEATURE_4BA_WRITE	(1 << 16) /**< Native 4BA byte program (0x12) is supported. */
/* 4BA Shorthands */
#define FEATURE_4BA_NATIVE	(FEATURE_4BA_READ | FEATURE_4BA_FAST_READ | FEATURE_4BA_WRITE)
#define FEATURE_4BA		(FEATURE_4BA_ENTER | FEATURE_4BA_EXT_ADDR | FEATURE_4BA_NATIVE)
#define FEATURE_4BA_WREN	(FEATURE_4BA_ENTER_WREN | FEATURE_4BA_EXT_ADDR | FEATURE_4BA_NATIVE)
#define FEATURE_4BA_EAR7	(FEATURE_4BA_ENTER_EAR7 | FEATURE_4BA_EXT_ADDR | FEATURE_4BA_NATIVE)
/*
 * Most flash chips are erased to ones and programmed to zeros. However, some
 * other flash chips, such as the ENE KB9012 internal flash, work the opposite way.
 */
#define FEATURE_ERASED_ZERO	(1 << 17)
#define FEATURE_NO_ERASE	(1 << 18)

#define ERASED_VALUE(flash)	(((flash)->chip->feature_bits & FEATURE_ERASED_ZERO) ? 0x00 : 0xff)

enum test_state {
	OK = 0,
	NT = 1,	/* Not tested */
	BAD,	/* Known to not work */
	DEP,	/* Support depends on configuration (e.g. Intel flash descriptor) */
	NA,	/* Not applicable (e.g. write support on ROM chips) */
};

#define TEST_UNTESTED	(struct tested){ .probe = NT, .read = NT, .erase = NT, .write = NT }

#define TEST_OK_PROBE	(struct tested){ .probe = OK, .read = NT, .erase = NT, .write = NT }
#define TEST_OK_PR	(struct tested){ .probe = OK, .read = OK, .erase = NT, .write = NT }
#define TEST_OK_PRE	(struct tested){ .probe = OK, .read = OK, .erase = OK, .write = NT }
#define TEST_OK_PREW	(struct tested){ .probe = OK, .read = OK, .erase = OK, .write = OK }

#define TEST_BAD_PROBE	(struct tested){ .probe = BAD, .read = NT, .erase = NT, .write = NT }
#define TEST_BAD_PR	(struct tested){ .probe = BAD, .read = BAD, .erase = NT, .write = NT }
#define TEST_BAD_PRE	(struct tested){ .probe = BAD, .read = BAD, .erase = BAD, .write = NT }
#define TEST_BAD_PREW	(struct tested){ .probe = BAD, .read = BAD, .erase = BAD, .write = BAD }

struct flashrom_flashctx;
#define flashctx flashrom_flashctx /* TODO: Agree on a name and convert all occurences. */
typedef int (erasefunc_t)(struct flashctx *flash, unsigned int addr, unsigned int blocklen);

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
	unsigned int total_size;
	/* Chip page size in bytes */
	unsigned int page_size;
	int feature_bits;

	/* Indicate how well flashrom supports different operations of this flash chip. */
	struct tested {
		enum test_state probe;
		enum test_state read;
		enum test_state erase;
		enum test_state write;
	} tested;

	/*
	 * Group chips that have common command sets. This should ensure that
	 * no chip gets confused by a probing command for a very different class
	 * of chips.
	 */
	enum {
		/* SPI25 is very common. Keep it at zero so we don't have
		   to specify it for each and every chip in the database.*/
		SPI25 = 0,
		SPI_EDI = 1,
	} spi_cmd_set;

	int (*probe) (struct flashctx *flash);

	/* Delay after "enter/exit ID mode" commands in microseconds.
	 * NB: negative values have special meanings, see TIMING_* below.
	 */
	signed int probe_timing;

	/*
	 * Erase blocks and associated erase function. Any chip erase function
	 * is stored as chip-sized virtual block together with said function.
	 * The first one that fits will be chosen. There is currently no way to
	 * influence that behaviour. For testing just comment out the other
	 * elements or set the function pointer to NULL.
	 */
	struct block_eraser {
		struct eraseblock {
			unsigned int size; /* Eraseblock size in bytes */
			unsigned int count; /* Number of contiguous blocks with that size */
		} eraseblocks[NUM_ERASEREGIONS];
		/* a block_erase function should try to erase one block of size
		 * 'blocklen' at address 'blockaddr' and return 0 on success. */
		int (*block_erase) (struct flashctx *flash, unsigned int blockaddr, unsigned int blocklen);
	} block_erasers[NUM_ERASEFUNCTIONS];

	int (*printlock) (struct flashctx *flash);
	int (*unlock) (struct flashctx *flash);
	int (*write) (struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
	int (*read) (struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len);
	uint8_t (*read_status) (const struct flashctx *flash);
	int (*write_status) (const struct flashctx *flash, int status);
	struct voltage {
		uint16_t min;
		uint16_t max;
	} voltage;
	enum write_granularity gran;

	/* SPI specific options (TODO: Make it a union in case other bustypes get specific options.) */
	uint8_t wrea_override; /**< override opcode for write extended address register */

	struct wp *wp;
};

typedef int (*chip_restore_fn_cb_t)(struct flashctx *flash, uint8_t status);

struct flashrom_flashctx {
	struct flashchip *chip;
	/* FIXME: The memory mappings should be saved in a more structured way. */
	/* The physical_* fields store the respective addresses in the physical address space of the CPU. */
	uintptr_t physical_memory;
	/* The virtual_* fields store where the respective physical address is mapped into flashrom's address
	 * space. A value equivalent to (chipaddr)ERROR_PTR indicates an invalid mapping (or none at all). */
	chipaddr virtual_memory;
	/* Some flash devices have an additional register space; semantics are like above. */
	uintptr_t physical_registers;
	chipaddr virtual_registers;
	struct registered_master *mst;
	const struct flashrom_layout *layout;
	struct flashrom_layout *default_layout;
	struct {
		bool force;
		bool force_boardmismatch;
		bool verify_after_write;
		bool verify_whole_chip;
	} flags;
	/* We cache the state of the extended address register (highest byte
	 * of a 4BA for 3BA instructions) and the state of the 4BA mode here.
	 * If possible, we enter 4BA mode early. If that fails, we make use
	 * of the extended address register.
	 */
	int address_high_byte;
	bool in_4ba_mode;

	int chip_restore_fn_count;
	struct chip_restore_func_data {
		chip_restore_fn_cb_t func;
		uint8_t status;
	} chip_restore_fn[MAX_CHIP_RESTORE_FUNCTIONS];
};

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
extern const unsigned int flashchips_size;

void chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr);
void chip_writew(const struct flashctx *flash, uint16_t val, chipaddr addr);
void chip_writel(const struct flashctx *flash, uint32_t val, chipaddr addr);
void chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len);
uint8_t chip_readb(const struct flashctx *flash, const chipaddr addr);
uint16_t chip_readw(const struct flashctx *flash, const chipaddr addr);
uint32_t chip_readl(const struct flashctx *flash, const chipaddr addr);
void chip_readn(const struct flashctx *flash, uint8_t *buf, const chipaddr addr, size_t len);

/* print.c */
int print_supported(void);
void print_supported_wiki(void);

/* helpers.c */
uint32_t address_to_bits(uint32_t addr);
unsigned int bitcount(unsigned long a);
#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int max(int a, int b);
int min(int a, int b);
char *strcat_realloc(char *dest, const char *src);
void tolower_string(char *str);
uint8_t reverse_byte(uint8_t x);
void reverse_bytes(uint8_t *dst, const uint8_t *src, size_t length);
#ifdef __MINGW32__
char* strtok_r(char *str, const char *delim, char **nextp);
char *strndup(const char *str, size_t size);
#endif
#if defined(__DJGPP__) || (!defined(__LIBPAYLOAD__) && !defined(HAVE_STRNLEN))
size_t strnlen(const char *str, size_t n);
#endif

/* flashrom.c */
extern const char flashrom_version[];
extern const char *chip_to_probe;
char *flashbuses_to_text(enum chipbustype bustype);
int map_flash(struct flashctx *flash);
void unmap_flash(struct flashctx *flash);
int read_memmapped(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len);
int erase_flash(struct flashctx *flash);
int probe_flash(struct registered_master *mst, int startchip, struct flashctx *fill_flash, int force);
int read_flash_to_file(struct flashctx *flash, const char *filename);
int verify_range(struct flashctx *flash, const uint8_t *cmpbuf, unsigned int start, unsigned int len);
int need_erase(const uint8_t *have, const uint8_t *want, unsigned int len, enum write_granularity gran, const uint8_t erased_value);
void print_version(void);
void print_buildinfo(void);
void print_banner(void);
void list_programmers_linebreak(int startcol, int cols, int paren);
int selfcheck(void);
int read_buf_from_file(unsigned char *buf, unsigned long size, const char *filename);
int write_buf_to_file(const unsigned char *buf, unsigned long size, const char *filename);
int prepare_flash_access(struct flashctx *, bool read_it, bool write_it, bool erase_it, bool verify_it);
void finalize_flash_access(struct flashctx *);
int do_read(struct flashctx *, const char *filename);
int do_extract(struct flashctx *);
int do_erase(struct flashctx *);
int do_write(struct flashctx *, const char *const filename, const char *const referencefile);
int do_verify(struct flashctx *, const char *const filename);
int register_chip_restore(chip_restore_fn_cb_t func, struct flashctx *flash, uint8_t status);

/* Something happened that shouldn't happen, but we can go on. */
#define ERROR_NONFATAL 0x100

/* Something happened that shouldn't happen, we'll abort. */
#define ERROR_FATAL -0xee
#define ERROR_FLASHROM_BUG -200
/* We reached one of the hardcoded limits of flashrom. This can be fixed by
 * increasing the limit of a compile-time allocation or by switching to dynamic
 * allocation.
 * Note: If this warning is triggered, check first for runaway registrations.
 */
#define ERROR_FLASHROM_LIMIT -201

/* cli_common.c */
void print_chip_support_status(const struct flashchip *chip);

/* cli_output.c */
extern enum flashrom_log_level verbose_screen;
extern enum flashrom_log_level verbose_logfile;
#ifndef STANDALONE
int open_logfile(const char * const filename);
int close_logfile(void);
void start_logging(void);
#endif
int flashrom_print_cb(enum flashrom_log_level level, const char *fmt, va_list ap);
/* Let gcc and clang check for correct printf-style format strings. */
int print(enum flashrom_log_level level, const char *fmt, ...)
#ifdef __MINGW32__
#  ifndef __MINGW_PRINTF_FORMAT
#    define __MINGW_PRINTF_FORMAT gnu_printf
#  endif
__attribute__((format(__MINGW_PRINTF_FORMAT, 2, 3)));
#else
__attribute__((format(printf, 2, 3)));
#endif
#define msg_gerr(...)	print(FLASHROM_MSG_ERROR, __VA_ARGS__)	/* general errors */
#define msg_perr(...)	print(FLASHROM_MSG_ERROR, __VA_ARGS__)	/* programmer errors */
#define msg_cerr(...)	print(FLASHROM_MSG_ERROR, __VA_ARGS__)	/* chip errors */
#define msg_gwarn(...)	print(FLASHROM_MSG_WARN, __VA_ARGS__)	/* general warnings */
#define msg_pwarn(...)	print(FLASHROM_MSG_WARN, __VA_ARGS__)	/* programmer warnings */
#define msg_cwarn(...)	print(FLASHROM_MSG_WARN, __VA_ARGS__)	/* chip warnings */
#define msg_ginfo(...)	print(FLASHROM_MSG_INFO, __VA_ARGS__)	/* general info */
#define msg_pinfo(...)	print(FLASHROM_MSG_INFO, __VA_ARGS__)	/* programmer info */
#define msg_cinfo(...)	print(FLASHROM_MSG_INFO, __VA_ARGS__)	/* chip info */
#define msg_gdbg(...)	print(FLASHROM_MSG_DEBUG, __VA_ARGS__)	/* general debug */
#define msg_pdbg(...)	print(FLASHROM_MSG_DEBUG, __VA_ARGS__)	/* programmer debug */
#define msg_cdbg(...)	print(FLASHROM_MSG_DEBUG, __VA_ARGS__)	/* chip debug */
#define msg_gdbg2(...)	print(FLASHROM_MSG_DEBUG2, __VA_ARGS__)	/* general debug2 */
#define msg_pdbg2(...)	print(FLASHROM_MSG_DEBUG2, __VA_ARGS__)	/* programmer debug2 */
#define msg_cdbg2(...)	print(FLASHROM_MSG_DEBUG2, __VA_ARGS__)	/* chip debug2 */
#define msg_gspew(...)	print(FLASHROM_MSG_SPEW, __VA_ARGS__)	/* general debug spew  */
#define msg_pspew(...)	print(FLASHROM_MSG_SPEW, __VA_ARGS__)	/* programmer debug spew  */
#define msg_cspew(...)	print(FLASHROM_MSG_SPEW, __VA_ARGS__)	/* chip debug spew  */

/* spi.c */
struct spi_command {
	unsigned int writecnt;
	unsigned int readcnt;
	const unsigned char *writearr;
	unsigned char *readarr;
};
#define NULL_SPI_CMD { 0, 0, NULL, NULL, }
int spi_send_command(const struct flashctx *flash, unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr);
int spi_send_multicommand(const struct flashctx *flash, struct spi_command *cmds);

enum chipbustype get_buses_supported(void);
#endif				/* !__FLASH_H__ */
