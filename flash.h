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

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include "hwaccess.h"

typedef unsigned long chipaddr;

enum programmer {
#if INTERNAL_SUPPORT == 1
	PROGRAMMER_INTERNAL,
#endif
#if DUMMY_SUPPORT == 1
	PROGRAMMER_DUMMY,
#endif
#if NIC3COM_SUPPORT == 1
	PROGRAMMER_NIC3COM,
#endif
#if GFXNVIDIA_SUPPORT == 1
	PROGRAMMER_GFXNVIDIA,
#endif
#if DRKAISER_SUPPORT == 1
	PROGRAMMER_DRKAISER,
#endif
#if SATASII_SUPPORT == 1
	PROGRAMMER_SATASII,
#endif
#if INTERNAL_SUPPORT == 1
	PROGRAMMER_IT87SPI,
#endif
#if FT2232_SPI_SUPPORT == 1
	PROGRAMMER_FT2232SPI,
#endif
#if SERPROG_SUPPORT == 1
	PROGRAMMER_SERPROG,
#endif
#if BUSPIRATE_SPI_SUPPORT == 1
	PROGRAMMER_BUSPIRATESPI,
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

int programmer_init(void);
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

enum bitbang_spi_master {
	BITBANG_SPI_INVALID /* This must always be the last entry. */
};

extern const int bitbang_spi_master_count;

extern enum bitbang_spi_master bitbang_spi_master;

struct bitbang_spi_master_entry {
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

	/*
	 * Indicate if flashrom has been tested with this flash chip and if
	 * everything worked correctly.
	 */
	uint32_t tested;

	int (*probe) (struct flashchip *flash);

	/* Delay after "enter/exit ID mode" commands in microseconds. */
	int probe_timing;
	int (*erase) (struct flashchip *flash);

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

#if INTERNAL_SUPPORT == 1
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

	/* The vendor / part name from the coreboot table. */
	const char *lb_vendor;
	const char *lb_part;

	const char *vendor_name;
	const char *board_name;

	int (*enable) (const char *name);
};

extern struct board_pciid_enable board_pciid_enables[];

struct board_info {
	const char *vendor;
	const char *name;
};

extern const struct board_info boards_ok[];
extern const struct board_info boards_bad[];
extern const struct board_info laptops_ok[];
extern const struct board_info laptops_bad[];
#endif

/* udelay.c */
void myusec_delay(int usecs);
void myusec_calibrate_delay(void);

#if NEED_PCI == 1
/* pcidev.c */
#define PCI_OK 0
#define PCI_NT 1    /* Not tested */

extern uint32_t io_base_addr;
extern struct pci_access *pacc;
extern struct pci_filter filter;
extern struct pci_dev *pcidev_dev;
struct pcidev_status {
	uint16_t vendor_id;
	uint16_t device_id;
	int status;
	const char *vendor_name;
	const char *device_name;
};
uint32_t pcidev_validate(struct pci_dev *dev, uint32_t bar, struct pcidev_status *devs);
uint32_t pcidev_init(uint16_t vendor_id, uint32_t bar, struct pcidev_status *devs, char *pcidev_bdf);
#endif

/* print.c */
char *flashbuses_to_text(enum chipbustype bustype);
void print_supported(void);
#if (NIC3COM_SUPPORT == 1) || (GFXNVIDIA_SUPPORT == 1) || (DRKAISER_SUPPORT == 1) || (SATASII_SUPPORT == 1)
void print_supported_pcidevs(struct pcidev_status *devs);
#endif
void print_supported_wiki(void);

/* board_enable.c */
void w836xx_ext_enter(uint16_t port);
void w836xx_ext_leave(uint16_t port);
uint8_t sio_read(uint16_t port, uint8_t reg);
void sio_write(uint16_t port, uint8_t reg, uint8_t data);
void sio_mask(uint16_t port, uint8_t reg, uint8_t data, uint8_t mask);
int board_flash_enable(const char *vendor, const char *part);

/* chipset_enable.c */
int chipset_flash_enable(void);

/* physmap.c */
void *physmap(const char *descr, unsigned long phys_addr, size_t len);
void physunmap(void *virt_addr, size_t len);
int setup_cpu_msr(int cpu);
void cleanup_cpu_msr(void);

/* cbtable.c */
void lb_vendor_dev_from_string(char *boardstring);
int coreboot_init(void);
extern char *lb_part, *lb_vendor;
extern int partvendor_from_cbtable;

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
#if INTERNAL_SUPPORT == 1
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
void mmio_writeb(uint8_t val, void *addr);
void mmio_writew(uint16_t val, void *addr);
void mmio_writel(uint32_t val, void *addr);
uint8_t mmio_readb(void *addr);
uint16_t mmio_readw(void *addr);
uint32_t mmio_readl(void *addr);
void internal_delay(int usecs);
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
#if DUMMY_SUPPORT == 1
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
#endif

/* nic3com.c */
#if NIC3COM_SUPPORT == 1
int nic3com_init(void);
int nic3com_shutdown(void);
void nic3com_chip_writeb(uint8_t val, chipaddr addr);
uint8_t nic3com_chip_readb(const chipaddr addr);
extern struct pcidev_status nics_3com[];
#endif

/* gfxnvidia.c */
#if GFXNVIDIA_SUPPORT == 1
int gfxnvidia_init(void);
int gfxnvidia_shutdown(void);
void gfxnvidia_chip_writeb(uint8_t val, chipaddr addr);
uint8_t gfxnvidia_chip_readb(const chipaddr addr);
extern struct pcidev_status gfx_nvidia[];
#endif

/* drkaiser.c */
#if DRKAISER_SUPPORT == 1
int drkaiser_init(void);
int drkaiser_shutdown(void);
void drkaiser_chip_writeb(uint8_t val, chipaddr addr);
uint8_t drkaiser_chip_readb(const chipaddr addr);
extern struct pcidev_status drkaiser_pcidev[];
#endif

/* satasii.c */
#if SATASII_SUPPORT == 1
int satasii_init(void);
int satasii_shutdown(void);
void satasii_chip_writeb(uint8_t val, chipaddr addr);
uint8_t satasii_chip_readb(const chipaddr addr);
extern struct pcidev_status satas_sii[];
#endif

/* ft2232_spi.c */
#define FTDI_FT2232H 0x6010
#define FTDI_FT4232H 0x6011
int ft2232_spi_init(void);
int ft2232_spi_send_command(unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr);
int ft2232_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int ft2232_spi_write_256(struct flashchip *flash, uint8_t *buf);

/* bitbang_spi.c */
extern int bitbang_spi_half_period;
extern const struct bitbang_spi_master_entry bitbang_spi_master_table[];
int bitbang_spi_init(void);
int bitbang_spi_send_command(unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr);
int bitbang_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int bitbang_spi_write_256(struct flashchip *flash, uint8_t *buf);

/* buspirate_spi.c */
struct buspirate_spispeeds {
	const char *name;
	const int speed;
};
int buspirate_spi_init(void);
int buspirate_spi_shutdown(void);
int buspirate_spi_send_command(unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr);
int buspirate_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);

/* flashrom.c */
extern enum chipbustype buses_supported;
struct decode_sizes {
	uint32_t parallel;
	uint32_t lpc;
	uint32_t fwh;
	uint32_t spi;
};
extern struct decode_sizes max_rom_decode;
extern char *programmer_param;
extern unsigned long flashbase;
extern int verbose;
extern const char *flashrom_version;
#define printf_debug(x...) { if (verbose) printf(x); }
void map_flash_registers(struct flashchip *flash);
int read_memmapped(struct flashchip *flash, uint8_t *buf, int start, int len);
int erase_flash(struct flashchip *flash);
int min(int a, int b);
int max(int a, int b);
char *extract_param(char **haystack, char *needle, char *delim);
int check_erased_range(struct flashchip *flash, int start, int len);
int verify_range(struct flashchip *flash, uint8_t *cmpbuf, int start, int len, char *message);
char *strcat_realloc(char *dest, const char *src);
int doit(struct flashchip *flash, int force, char *filename, int read_it, int write_it, int erase_it, int verify_it);

#define OK 0
#define NT 1    /* Not tested */

/* layout.c */
int show_id(uint8_t *bios, int size, int force);
int read_romlayout(char *name);
int find_romentry(char *name);
int handle_romentries(uint8_t *buffer, struct flashchip *flash);

/* spi.c */
enum spi_controller {
	SPI_CONTROLLER_NONE,
#if INTERNAL_SUPPORT == 1
	SPI_CONTROLLER_ICH7,
	SPI_CONTROLLER_ICH9,
	SPI_CONTROLLER_IT87XX,
	SPI_CONTROLLER_SB600,
	SPI_CONTROLLER_VIA,
	SPI_CONTROLLER_WBSIO,
#endif
#if FT2232_SPI_SUPPORT == 1
	SPI_CONTROLLER_FT2232,
#endif
#if DUMMY_SUPPORT == 1
	SPI_CONTROLLER_DUMMY,
#endif
#if BUSPIRATE_SPI_SUPPORT == 1
	SPI_CONTROLLER_BUSPIRATE,
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
	int (*write_256)(struct flashchip *flash, uint8_t *buf);
};

extern enum spi_controller spi_controller;
extern const struct spi_programmer spi_programmer[];
extern void *spibar;
int spi_send_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr);
int spi_send_multicommand(struct spi_command *cmds);
int default_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			     const unsigned char *writearr, unsigned char *readarr);
int default_spi_send_multicommand(struct spi_command *cmds);
uint32_t spi_get_valid_read_addr(void);

/* ichspi.c */
int ich_init_opcodes(void);
int ich_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		    const unsigned char *writearr, unsigned char *readarr);
int ich_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int ich_spi_write_256(struct flashchip *flash, uint8_t * buf);
int ich_spi_send_multicommand(struct spi_command *cmds);

/* it87spi.c */
extern uint16_t it8716f_flashport;
void enter_conf_mode_ite(uint16_t port);
void exit_conf_mode_ite(uint16_t port);
struct superio probe_superio_ite(void);
int it87spi_init(void);
int it87xx_probe_spi_flash(const char *name);
int it8716f_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			const unsigned char *writearr, unsigned char *readarr);
int it8716f_spi_chip_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int it8716f_spi_chip_write_256(struct flashchip *flash, uint8_t *buf);

/* sb600spi.c */
int sb600_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		      const unsigned char *writearr, unsigned char *readarr);
int sb600_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int sb600_spi_write_1(struct flashchip *flash, uint8_t *buf);
extern uint8_t *sb600_spibar;

/* wbsio_spi.c */
int wbsio_check_for_spi(const char *name);
int wbsio_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		      const unsigned char *writearr, unsigned char *readarr);
int wbsio_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int wbsio_spi_write_1(struct flashchip *flash, uint8_t *buf);

/* serprog.c */
int serprog_init(void);
int serprog_shutdown(void);
void serprog_chip_writeb(uint8_t val, chipaddr addr);
uint8_t serprog_chip_readb(const chipaddr addr);
void serprog_chip_readn(uint8_t *buf, const chipaddr addr, size_t len);
void serprog_delay(int delay);

/* serial.c */
void sp_flush_incoming(void);
int sp_openserport(char *dev, unsigned int baud);
void __attribute__((noreturn)) sp_die(char *msg);
extern int sp_fd;

#include "chipdrivers.h"

#endif				/* !__FLASH_H__ */
