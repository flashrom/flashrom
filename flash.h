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

typedef unsigned long chipaddr;

enum programmer {
#if CONFIG_INTERNAL == 1
	PROGRAMMER_INTERNAL,
#endif
#if CONFIG_DUMMY == 1
	PROGRAMMER_DUMMY,
#endif
#if CONFIG_NIC3COM == 1
	PROGRAMMER_NIC3COM,
#endif
#if CONFIG_NICREALTEK == 1
	PROGRAMMER_NICREALTEK,
	PROGRAMMER_NICREALTEK2,
#endif	
#if CONFIG_NICNATSEMI == 1
	PROGRAMMER_NICNATSEMI,
#endif	
#if CONFIG_GFXNVIDIA == 1
	PROGRAMMER_GFXNVIDIA,
#endif
#if CONFIG_DRKAISER == 1
	PROGRAMMER_DRKAISER,
#endif
#if CONFIG_SATASII == 1
	PROGRAMMER_SATASII,
#endif
#if CONFIG_ATAHPT == 1
	PROGRAMMER_ATAHPT,
#endif
#if CONFIG_INTERNAL == 1
#if defined(__i386__) || defined(__x86_64__)
	PROGRAMMER_IT87SPI,
#endif
#endif
#if CONFIG_FT2232_SPI == 1
	PROGRAMMER_FT2232_SPI,
#endif
#if CONFIG_SERPROG == 1
	PROGRAMMER_SERPROG,
#endif
#if CONFIG_BUSPIRATE_SPI == 1
	PROGRAMMER_BUSPIRATE_SPI,
#endif
#if CONFIG_DEDIPROG == 1
	PROGRAMMER_DEDIPROG,
#endif
#if CONFIG_RAYER_SPI == 1
	PROGRAMMER_RAYER_SPI,
#endif
	PROGRAMMER_INVALID /* This must always be the last entry. */
};

extern enum programmer programmer;

struct programmer_entry {
	const char *vendor;
	const char *name;

	int (*init) (void);
	int (*shutdown) (void);

	void * (*map_flash_region) (const char *descr, unsigned long phys_addr,
				    size_t len);
	void (*unmap_flash_region) (void *virt_addr, size_t len);

	void (*chip_writeb) (uint8_t val, chipaddr addr);
	void (*chip_writew) (uint16_t val, chipaddr addr);
	void (*chip_writel) (uint32_t val, chipaddr addr);
	void (*chip_writen) (uint8_t *buf, chipaddr addr, size_t len);
	uint8_t (*chip_readb) (const chipaddr addr);
	uint16_t (*chip_readw) (const chipaddr addr);
	uint32_t (*chip_readl) (const chipaddr addr);
	void (*chip_readn) (uint8_t *buf, const chipaddr addr, size_t len);
	void (*delay) (int usecs);
};

extern const struct programmer_entry programmer_table[];

int register_shutdown(void (*function) (void *data), void *data);

int programmer_init(char *param);
int programmer_shutdown(void);
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

enum bitbang_spi_master_type {
	BITBANG_SPI_INVALID	= 0, /* This must always be the first entry. */
#if CONFIG_RAYER_SPI == 1
	BITBANG_SPI_MASTER_RAYER,
#endif
};

struct bitbang_spi_master {
	enum bitbang_spi_master_type type;

	/* Note that CS# is active low, so val=0 means the chip is active. */
	void (*set_cs) (int val);
	void (*set_sck) (int val);
	void (*set_mosi) (int val);
	int (*get_miso) (void);
};

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
 */
#define NUM_ERASEFUNCTIONS 5

#define FEATURE_REGISTERMAP	(1 << 0)
#define FEATURE_BYTEWRITES	(1 << 1)
#define FEATURE_LONG_RESET	(0 << 4)
#define FEATURE_SHORT_RESET	(1 << 4)
#define FEATURE_EITHER_RESET	FEATURE_LONG_RESET
#define FEATURE_ADDR_FULL	(0 << 2)
#define FEATURE_ADDR_MASK	(3 << 2)
#define FEATURE_ADDR_2AA	(1 << 2)
#define FEATURE_ADDR_AAA	(2 << 2)
#define FEATURE_ADDR_SHIFTED	(1 << 5)

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

	int total_size;
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
	int (*write) (struct flashchip *flash, uint8_t *buf);
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

extern struct flashchip flashchips[];

#if CONFIG_INTERNAL == 1
struct penable {
	uint16_t vendor_id;
	uint16_t device_id;
	int status;
	const char *vendor_name;
	const char *device_name;
	int (*doit) (struct pci_dev *dev, const char *name);
};

extern const struct penable chipset_enables[];

struct board_pciid_enable {
	/* Any device, but make it sensible, like the ISA bridge. */
	uint16_t first_vendor;
	uint16_t first_device;
	uint16_t first_card_vendor;
	uint16_t first_card_device;

	/* Any device, but make it sensible, like
	 * the host bridge. May be NULL.
	 */
	uint16_t second_vendor;
	uint16_t second_device;
	uint16_t second_card_vendor;
	uint16_t second_card_device;

	/* Pattern to match DMI entries */
	const char *dmi_pattern;

	/* The vendor / part name from the coreboot table. */
	const char *lb_vendor;
	const char *lb_part;

	const char *vendor_name;
	const char *board_name;

	int max_rom_decode_parallel;
	int status;
	int (*enable) (void);
};

extern const struct board_pciid_enable board_pciid_enables[];

struct board_info {
	const char *vendor;
	const char *name;
	const int working;
#ifdef CONFIG_PRINT_WIKI
	const char *url;
	const char *note;
#endif
};

extern const struct board_info boards_known[];
extern const struct board_info laptops_known[];

#endif

/* udelay.c */
void myusec_delay(int usecs);
void myusec_calibrate_delay(void);
void internal_delay(int usecs);

#if NEED_PCI == 1
/* pcidev.c */

extern uint32_t io_base_addr;
extern struct pci_access *pacc;
extern struct pci_dev *pcidev_dev;
struct pcidev_status {
	uint16_t vendor_id;
	uint16_t device_id;
	int status;
	const char *vendor_name;
	const char *device_name;
};
uint32_t pcidev_validate(struct pci_dev *dev, uint32_t bar, const struct pcidev_status *devs);
uint32_t pcidev_init(uint16_t vendor_id, uint32_t bar, const struct pcidev_status *devs);
#endif

/* print.c */
char *flashbuses_to_text(enum chipbustype bustype);
void print_supported(void);
#if CONFIG_NIC3COM+CONFIG_NICREALTEK+CONFIG_NICNATSEMI+CONFIG_GFXNVIDIA+CONFIG_DRKAISER+CONFIG_SATASII+CONFIG_ATAHPT >= 1
void print_supported_pcidevs(const struct pcidev_status *devs);
#endif
void print_supported_wiki(void);

/* board_enable.c */
void w836xx_ext_enter(uint16_t port);
void w836xx_ext_leave(uint16_t port);
int it8705f_write_enable(uint8_t port);
uint8_t sio_read(uint16_t port, uint8_t reg);
void sio_write(uint16_t port, uint8_t reg, uint8_t data);
void sio_mask(uint16_t port, uint8_t reg, uint8_t data, uint8_t mask);
int board_flash_enable(const char *vendor, const char *part);

/* chipset_enable.c */
int chipset_flash_enable(void);

/* processor_enable.c */
int processor_flash_enable(void);

/* physmap.c */
void *physmap(const char *descr, unsigned long phys_addr, size_t len);
void *physmap_try_ro(const char *descr, unsigned long phys_addr, size_t len);
void physunmap(void *virt_addr, size_t len);
int setup_cpu_msr(int cpu);
void cleanup_cpu_msr(void);

/* cbtable.c */
void lb_vendor_dev_from_string(char *boardstring);
int coreboot_init(void);
extern char *lb_part, *lb_vendor;
extern int partvendor_from_cbtable;

/* dmi.c */
extern int has_dmi_support;
void dmi_init(void);
int dmi_match(const char *pattern);

/* internal.c */
#if NEED_PCI == 1
struct superio {
	uint16_t vendor;
	uint16_t port;
	uint16_t model;
};
extern struct superio superio;
#define SUPERIO_VENDOR_NONE	0x0
#define SUPERIO_VENDOR_ITE	0x1
struct pci_dev *pci_dev_find_filter(struct pci_filter filter);
struct pci_dev *pci_dev_find_vendorclass(uint16_t vendor, uint16_t class);
struct pci_dev *pci_dev_find(uint16_t vendor, uint16_t device);
struct pci_dev *pci_card_find(uint16_t vendor, uint16_t device,
			      uint16_t card_vendor, uint16_t card_device);
#endif
void get_io_perms(void);
void release_io_perms(void);
#if CONFIG_INTERNAL == 1
extern int is_laptop;
extern int force_boardenable;
extern int force_boardmismatch;
void probe_superio(void);
int internal_init(void);
int internal_shutdown(void);
void internal_chip_writeb(uint8_t val, chipaddr addr);
void internal_chip_writew(uint16_t val, chipaddr addr);
void internal_chip_writel(uint32_t val, chipaddr addr);
uint8_t internal_chip_readb(const chipaddr addr);
uint16_t internal_chip_readw(const chipaddr addr);
uint32_t internal_chip_readl(const chipaddr addr);
void internal_chip_readn(uint8_t *buf, const chipaddr addr, size_t len);
#endif

/* hwaccess.c */
void mmio_writeb(uint8_t val, void *addr);
void mmio_writew(uint16_t val, void *addr);
void mmio_writel(uint32_t val, void *addr);
uint8_t mmio_readb(void *addr);
uint16_t mmio_readw(void *addr);
uint32_t mmio_readl(void *addr);
void mmio_le_writeb(uint8_t val, void *addr);
void mmio_le_writew(uint16_t val, void *addr);
void mmio_le_writel(uint32_t val, void *addr);
uint8_t mmio_le_readb(void *addr);
uint16_t mmio_le_readw(void *addr);
uint32_t mmio_le_readl(void *addr);
#define pci_mmio_writeb mmio_le_writeb
#define pci_mmio_writew mmio_le_writew
#define pci_mmio_writel mmio_le_writel
#define pci_mmio_readb mmio_le_readb
#define pci_mmio_readw mmio_le_readw
#define pci_mmio_readl mmio_le_readl

/* programmer.c */
int noop_shutdown(void);
void *fallback_map(const char *descr, unsigned long phys_addr, size_t len);
void fallback_unmap(void *virt_addr, size_t len);
uint8_t noop_chip_readb(const chipaddr addr);
void noop_chip_writeb(uint8_t val, chipaddr addr);
void fallback_chip_writew(uint16_t val, chipaddr addr);
void fallback_chip_writel(uint32_t val, chipaddr addr);
void fallback_chip_writen(uint8_t *buf, chipaddr addr, size_t len);
uint16_t fallback_chip_readw(const chipaddr addr);
uint32_t fallback_chip_readl(const chipaddr addr);
void fallback_chip_readn(uint8_t *buf, const chipaddr addr, size_t len);

/* dummyflasher.c */
#if CONFIG_DUMMY == 1
int dummy_init(void);
int dummy_shutdown(void);
void *dummy_map(const char *descr, unsigned long phys_addr, size_t len);
void dummy_unmap(void *virt_addr, size_t len);
void dummy_chip_writeb(uint8_t val, chipaddr addr);
void dummy_chip_writew(uint16_t val, chipaddr addr);
void dummy_chip_writel(uint32_t val, chipaddr addr);
void dummy_chip_writen(uint8_t *buf, chipaddr addr, size_t len);
uint8_t dummy_chip_readb(const chipaddr addr);
uint16_t dummy_chip_readw(const chipaddr addr);
uint32_t dummy_chip_readl(const chipaddr addr);
void dummy_chip_readn(uint8_t *buf, const chipaddr addr, size_t len);
int dummy_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		      const unsigned char *writearr, unsigned char *readarr);
int dummy_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int dummy_spi_write_256(struct flashchip *flash, uint8_t *buf, int start, int len);
#endif

/* nic3com.c */
#if CONFIG_NIC3COM == 1
int nic3com_init(void);
int nic3com_shutdown(void);
void nic3com_chip_writeb(uint8_t val, chipaddr addr);
uint8_t nic3com_chip_readb(const chipaddr addr);
extern const struct pcidev_status nics_3com[];
#endif

/* gfxnvidia.c */
#if CONFIG_GFXNVIDIA == 1
int gfxnvidia_init(void);
int gfxnvidia_shutdown(void);
void gfxnvidia_chip_writeb(uint8_t val, chipaddr addr);
uint8_t gfxnvidia_chip_readb(const chipaddr addr);
extern const struct pcidev_status gfx_nvidia[];
#endif

/* drkaiser.c */
#if CONFIG_DRKAISER == 1
int drkaiser_init(void);
int drkaiser_shutdown(void);
void drkaiser_chip_writeb(uint8_t val, chipaddr addr);
uint8_t drkaiser_chip_readb(const chipaddr addr);
extern const struct pcidev_status drkaiser_pcidev[];
#endif

/* nicrealtek.c */
#if CONFIG_NICREALTEK == 1
int nicrealtek_init(void);
int nicsmc1211_init(void);
int nicrealtek_shutdown(void);
void nicrealtek_chip_writeb(uint8_t val, chipaddr addr);
uint8_t nicrealtek_chip_readb(const chipaddr addr);
extern const struct pcidev_status nics_realtek[];
extern const struct pcidev_status nics_realteksmc1211[];
#endif

/* nicnatsemi.c */
#if CONFIG_NICNATSEMI == 1
int nicnatsemi_init(void);
int nicnatsemi_shutdown(void);
void nicnatsemi_chip_writeb(uint8_t val, chipaddr addr);
uint8_t nicnatsemi_chip_readb(const chipaddr addr);
extern const struct pcidev_status nics_natsemi[];
#endif

/* satasii.c */
#if CONFIG_SATASII == 1
int satasii_init(void);
int satasii_shutdown(void);
void satasii_chip_writeb(uint8_t val, chipaddr addr);
uint8_t satasii_chip_readb(const chipaddr addr);
extern const struct pcidev_status satas_sii[];
#endif

/* atahpt.c */
#if CONFIG_ATAHPT == 1
int atahpt_init(void);
int atahpt_shutdown(void);
void atahpt_chip_writeb(uint8_t val, chipaddr addr);
uint8_t atahpt_chip_readb(const chipaddr addr);
extern const struct pcidev_status ata_hpt[];
#endif

/* ft2232_spi.c */
#define FTDI_FT2232H 0x6010
#define FTDI_FT4232H 0x6011
int ft2232_spi_init(void);
int ft2232_spi_send_command(unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr);
int ft2232_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int ft2232_spi_write_256(struct flashchip *flash, uint8_t *buf, int start, int len);

/* rayer_spi.c */
#if CONFIG_RAYER_SPI == 1
int rayer_spi_init(void);
#endif

/* bitbang_spi.c */
int bitbang_spi_init(const struct bitbang_spi_master *master, int halfperiod);
int bitbang_spi_send_command(unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr);
int bitbang_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int bitbang_spi_write_256(struct flashchip *flash, uint8_t *buf, int start, int len);

/* buspirate_spi.c */
struct buspirate_spispeeds {
	const char *name;
	const int speed;
};
int buspirate_spi_init(void);
int buspirate_spi_shutdown(void);
int buspirate_spi_send_command(unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr);
int buspirate_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int buspirate_spi_write_256(struct flashchip *flash, uint8_t *buf, int start, int len);

/* dediprog.c */
int dediprog_init(void);
int dediprog_shutdown(void);
int dediprog_spi_send_command(unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr);
int dediprog_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);

/* flashrom.c */
enum write_granularity {
	write_gran_1bit,
	write_gran_1byte,
	write_gran_256bytes,
};
extern enum chipbustype buses_supported;
struct decode_sizes {
	uint32_t parallel;
	uint32_t lpc;
	uint32_t fwh;
	uint32_t spi;
};
extern struct decode_sizes max_rom_decode;
extern int programmer_may_write;
extern unsigned long flashbase;
extern int verbose;
extern const char * const flashrom_version;
extern char *chip_to_probe;
void map_flash_registers(struct flashchip *flash);
int read_memmapped(struct flashchip *flash, uint8_t *buf, int start, int len);
int erase_flash(struct flashchip *flash);
struct flashchip *probe_flash(struct flashchip *first_flash, int force);
int read_flash_to_file(struct flashchip *flash, char *filename);
void check_chip_supported(struct flashchip *flash);
int check_max_decode(enum chipbustype buses, uint32_t size);
int min(int a, int b);
int max(int a, int b);
char *extract_param(char **haystack, char *needle, char *delim);
char *extract_programmer_param(char *param_name);
int check_erased_range(struct flashchip *flash, int start, int len);
int verify_range(struct flashchip *flash, uint8_t *cmpbuf, int start, int len, char *message);
int need_erase(uint8_t *have, uint8_t *want, int len, enum write_granularity gran);
char *strcat_realloc(char *dest, const char *src);
void print_version(void);
void print_banner(void);
int selfcheck(void);
int doit(struct flashchip *flash, int force, char *filename, int read_it, int write_it, int erase_it, int verify_it);

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
int show_id(uint8_t *bios, int size, int force);
int read_romlayout(char *name);
int find_romentry(char *name);
int handle_romentries(uint8_t *buffer, struct flashchip *flash);

/* spi.c */
enum spi_controller {
	SPI_CONTROLLER_NONE,
#if CONFIG_INTERNAL == 1
#if defined(__i386__) || defined(__x86_64__)
	SPI_CONTROLLER_ICH7,
	SPI_CONTROLLER_ICH9,
	SPI_CONTROLLER_IT87XX,
	SPI_CONTROLLER_SB600,
	SPI_CONTROLLER_VIA,
	SPI_CONTROLLER_WBSIO,
#endif
#endif
#if CONFIG_FT2232_SPI == 1
	SPI_CONTROLLER_FT2232,
#endif
#if CONFIG_DUMMY == 1
	SPI_CONTROLLER_DUMMY,
#endif
#if CONFIG_BUSPIRATE_SPI == 1
	SPI_CONTROLLER_BUSPIRATE,
#endif
#if CONFIG_DEDIPROG == 1
	SPI_CONTROLLER_DEDIPROG,
#endif
#if CONFIG_RAYER_SPI == 1
	SPI_CONTROLLER_RAYER,
#endif
	SPI_CONTROLLER_INVALID /* This must always be the last entry. */
};
extern const int spi_programmer_count;
struct spi_command {
	unsigned int writecnt;
	unsigned int readcnt;
	const unsigned char *writearr;
	unsigned char *readarr;
};
struct spi_programmer {
	int (*command)(unsigned int writecnt, unsigned int readcnt,
		   const unsigned char *writearr, unsigned char *readarr);
	int (*multicommand)(struct spi_command *cmds);

	/* Optimized functions for this programmer */
	int (*read)(struct flashchip *flash, uint8_t *buf, int start, int len);
	int (*write_256)(struct flashchip *flash, uint8_t *buf, int start, int len);
};

extern enum spi_controller spi_controller;
extern const struct spi_programmer spi_programmer[];
int spi_send_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr);
int spi_send_multicommand(struct spi_command *cmds);
int default_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			     const unsigned char *writearr, unsigned char *readarr);
int default_spi_send_multicommand(struct spi_command *cmds);
uint32_t spi_get_valid_read_addr(void);

/* ichspi.c */
#if CONFIG_INTERNAL == 1
extern uint32_t ichspi_bbar;
int ich_init_spi(struct pci_dev *dev, uint32_t base, void *rcrb,
		    int ich_generation);
int via_init_spi(struct pci_dev *dev);
int ich_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		    const unsigned char *writearr, unsigned char *readarr);
int ich_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int ich_spi_write_256(struct flashchip *flash, uint8_t * buf, int start, int len);
int ich_spi_send_multicommand(struct spi_command *cmds);
#endif

/* it87spi.c */
void enter_conf_mode_ite(uint16_t port);
void exit_conf_mode_ite(uint16_t port);
struct superio probe_superio_ite(void);
int init_superio_ite(void);
int it87spi_init(void);
int it8716f_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			const unsigned char *writearr, unsigned char *readarr);
int it8716f_spi_chip_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int it8716f_spi_chip_write_256(struct flashchip *flash, uint8_t *buf, int start, int len);

/* sb600spi.c */
#if CONFIG_INTERNAL == 1
int sb600_probe_spi(struct pci_dev *dev);
int sb600_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		      const unsigned char *writearr, unsigned char *readarr);
int sb600_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int sb600_spi_write_256(struct flashchip *flash, uint8_t *buf, int start, int len);
#endif

/* wbsio_spi.c */
#if CONFIG_INTERNAL == 1
int wbsio_check_for_spi(void);
int wbsio_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		      const unsigned char *writearr, unsigned char *readarr);
int wbsio_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
#endif

/* serprog.c */
int serprog_init(void);
int serprog_shutdown(void);
void serprog_chip_writeb(uint8_t val, chipaddr addr);
uint8_t serprog_chip_readb(const chipaddr addr);
void serprog_chip_readn(uint8_t *buf, const chipaddr addr, size_t len);
void serprog_delay(int delay);

/* serial.c */
#if _WIN32
typedef HANDLE fdtype;
#else
typedef int fdtype;
#endif

void sp_flush_incoming(void);
fdtype sp_openserport(char *dev, unsigned int baud);
void __attribute__((noreturn)) sp_die(char *msg);
extern fdtype sp_fd;
int serialport_shutdown(void);
int serialport_write(unsigned char *buf, unsigned int writecnt);
int serialport_read(unsigned char *buf, unsigned int readcnt);

#endif				/* !__FLASH_H__ */
