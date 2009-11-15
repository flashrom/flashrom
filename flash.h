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

#if defined(__GLIBC__)
#include <sys/io.h>
#endif
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <pci/pci.h>

/* for iopl and outb under Solaris */
#if defined (__sun) && (defined(__i386) || defined(__amd64))
#include <strings.h>
#include <sys/sysi86.h>
#include <sys/psw.h>
#include <asm/sunddi.h>
#endif

#if (defined(__MACH__) && defined(__APPLE__))
#define __DARWIN__
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
  #include <machine/cpufunc.h>
  #define off64_t off_t
  #define lseek64 lseek
  #define OUTB(x, y) do { u_int tmp = (y); outb(tmp, (x)); } while (0)
  #define OUTW(x, y) do { u_int tmp = (y); outw(tmp, (x)); } while (0)
  #define OUTL(x, y) do { u_int tmp = (y); outl(tmp, (x)); } while (0)
  #define INB(x) __extension__ ({ u_int tmp = (x); inb(tmp); })
  #define INW(x) __extension__ ({ u_int tmp = (x); inw(tmp); })
  #define INL(x) __extension__ ({ u_int tmp = (x); inl(tmp); })
#else
#if defined(__DARWIN__)
    #include <DirectIO/darwinio.h>
    #define off64_t off_t
    #define lseek64 lseek
#endif
#if defined (__sun) && (defined(__i386) || defined(__amd64))
  /* Note different order for outb */
  #define OUTB(x,y) outb(y, x)
  #define OUTW(x,y) outw(y, x)
  #define OUTL(x,y) outl(y, x)
  #define INB  inb
  #define INW  inw
  #define INL  inl
#else
  #define OUTB outb
  #define OUTW outw
  #define OUTL outl
  #define INB  inb
  #define INW  inw
  #define INL  inl
#endif
#endif

typedef unsigned long chipaddr;

enum programmer {
	PROGRAMMER_INTERNAL,
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
	PROGRAMMER_IT87SPI,
#if FT2232_SPI_SUPPORT == 1
	PROGRAMMER_FT2232SPI,
#endif
#if SERPROG_SUPPORT == 1
	PROGRAMMER_SERPROG,
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
	 * Erase blocks and associated erase function. The default entry is a
	 * chip-sized virtual block together with the chip erase function.
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

/* udelay.c */
void myusec_delay(int usecs);
void myusec_calibrate_delay(void);

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

/* print.c */
char *flashbuses_to_text(enum chipbustype bustype);
void print_supported_chips(void);
void print_supported_chipsets(void);
void print_supported_boards(void);
void print_supported_pcidevs(struct pcidev_status *devs);
void print_wiki_tables(void);

/* board_enable.c */
void w836xx_ext_enter(uint16_t port);
void w836xx_ext_leave(uint16_t port);
uint8_t sio_read(uint16_t port, uint8_t reg);
void sio_write(uint16_t port, uint8_t reg, uint8_t data);
void sio_mask(uint16_t port, uint8_t reg, uint8_t data, uint8_t mask);
int board_flash_enable(const char *vendor, const char *part);

struct decode_sizes {
	uint32_t parallel;
	uint32_t lpc;
	uint32_t fwh;
	uint32_t spi;
};

/* chipset_enable.c */
extern enum chipbustype buses_supported;
int chipset_flash_enable(void);
extern struct decode_sizes max_rom_decode;

extern unsigned long flashbase;

/* physmap.c */
void *physmap(const char *descr, unsigned long phys_addr, size_t len);
void physunmap(void *virt_addr, size_t len);
int setup_cpu_msr(int cpu);
void cleanup_cpu_msr(void);
#if !defined(__DARWIN__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
typedef struct { uint32_t hi, lo; } msr_t;
msr_t rdmsr(int addr);
int wrmsr(int addr, msr_t msr);
#endif
#if defined(__FreeBSD__) || defined(__DragonFly__)
/* FreeBSD already has conflicting definitions for wrmsr/rdmsr. */
#undef rdmsr
#undef wrmsr
#define rdmsr freebsd_rdmsr
#define wrmsr freebsd_wrmsr
typedef struct { uint32_t hi, lo; } msr_t;
msr_t freebsd_rdmsr(int addr);
int freebsd_wrmsr(int addr, msr_t msr);
#endif

/* internal.c */
struct pci_dev *pci_dev_find_filter(struct pci_filter filter);
struct pci_dev *pci_dev_find_vendorclass(uint16_t vendor, uint16_t class);
struct pci_dev *pci_dev_find(uint16_t vendor, uint16_t device);
struct pci_dev *pci_card_find(uint16_t vendor, uint16_t device,
			      uint16_t card_vendor, uint16_t card_device);
void get_io_perms(void);
void release_io_perms(void);
int internal_init(void);
int internal_shutdown(void);
void internal_chip_writeb(uint8_t val, chipaddr addr);
void internal_chip_writew(uint16_t val, chipaddr addr);
void internal_chip_writel(uint32_t val, chipaddr addr);
uint8_t internal_chip_readb(const chipaddr addr);
uint16_t internal_chip_readw(const chipaddr addr);
uint32_t internal_chip_readl(const chipaddr addr);
void internal_chip_readn(uint8_t *buf, const chipaddr addr, size_t len);
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
#if defined(__FreeBSD__) || defined(__DragonFly__)
extern int io_fd;
#endif

/* dummyflasher.c */
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

/* nic3com.c */
int nic3com_init(void);
int nic3com_shutdown(void);
void nic3com_chip_writeb(uint8_t val, chipaddr addr);
uint8_t nic3com_chip_readb(const chipaddr addr);
extern struct pcidev_status nics_3com[];

/* gfxnvidia.c */
int gfxnvidia_init(void);
int gfxnvidia_shutdown(void);
void gfxnvidia_chip_writeb(uint8_t val, chipaddr addr);
uint8_t gfxnvidia_chip_readb(const chipaddr addr);
extern struct pcidev_status gfx_nvidia[];

/* drkaiser.c */
int drkaiser_init(void);
int drkaiser_shutdown(void);
void drkaiser_chip_writeb(uint8_t val, chipaddr addr);
uint8_t drkaiser_chip_readb(const chipaddr addr);
extern struct pcidev_status drkaiser_pcidev[];

/* satasii.c */
int satasii_init(void);
int satasii_shutdown(void);
void satasii_chip_writeb(uint8_t val, chipaddr addr);
uint8_t satasii_chip_readb(const chipaddr addr);
extern struct pcidev_status satas_sii[];

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

/* flashrom.c */
extern char *programmer_param;
extern int verbose;
extern const char *flashrom_version;
#define printf_debug(x...) { if (verbose) printf(x); }
void map_flash_registers(struct flashchip *flash);
int read_memmapped(struct flashchip *flash, uint8_t *buf, int start, int len);
int erase_flash(struct flashchip *flash);
int min(int a, int b);
int max(int a, int b);
int check_erased_range(struct flashchip *flash, int start, int len);
int verify_range(struct flashchip *flash, uint8_t *cmpbuf, int start, int len, char *message);
char *strcat_realloc(char *dest, const char *src);

#define OK 0
#define NT 1    /* Not tested */

/* layout.c */
int show_id(uint8_t *bios, int size, int force);
int read_romlayout(char *name);
int find_romentry(char *name);
int handle_romentries(uint8_t *buffer, struct flashchip *flash);

/* cbtable.c */
int coreboot_init(void);
extern char *lb_part, *lb_vendor;
extern int partvendor_from_cbtable;

/* spi.c */
enum spi_controller {
	SPI_CONTROLLER_NONE,
	SPI_CONTROLLER_ICH7,
	SPI_CONTROLLER_ICH9,
	SPI_CONTROLLER_IT87XX,
	SPI_CONTROLLER_SB600,
	SPI_CONTROLLER_VIA,
	SPI_CONTROLLER_WBSIO,
#if FT2232_SPI_SUPPORT == 1
	SPI_CONTROLLER_FT2232,
#endif
#if DUMMY_SUPPORT == 1
	SPI_CONTROLLER_DUMMY,
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
int probe_spi_rdid(struct flashchip *flash);
int probe_spi_rdid4(struct flashchip *flash);
int probe_spi_rems(struct flashchip *flash);
int probe_spi_res(struct flashchip *flash);
int spi_send_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr);
int spi_send_multicommand(struct spi_command *cmds);
int spi_write_enable(void);
int spi_write_disable(void);
int spi_chip_erase_60(struct flashchip *flash);
int spi_chip_erase_c7(struct flashchip *flash);
int spi_chip_erase_60_c7(struct flashchip *flash);
int spi_chip_erase_d8(struct flashchip *flash);
int spi_block_erase_20(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_52(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d8(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_60(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_c7(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_chip_write_1(struct flashchip *flash, uint8_t *buf);
int spi_chip_write_256(struct flashchip *flash, uint8_t *buf);
int spi_chip_read(struct flashchip *flash, uint8_t *buf, int start, int len);
uint8_t spi_read_status_register(void);
int spi_disable_blockprotect(void);
int spi_byte_program(int addr, uint8_t byte);
int spi_nbyte_program(int addr, uint8_t *bytes, int len);
int spi_nbyte_read(int addr, uint8_t *bytes, int len);
int spi_read_chunked(struct flashchip *flash, uint8_t *buf, int start, int len, int chunksize);
int spi_aai_write(struct flashchip *flash, uint8_t *buf);
uint32_t spi_get_valid_read_addr(void);
int default_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			     const unsigned char *writearr, unsigned char *readarr);
int default_spi_send_multicommand(struct spi_command *cmds);

/* 82802ab.c */
int probe_82802ab(struct flashchip *flash);
int erase_82802ab(struct flashchip *flash);
int write_82802ab(struct flashchip *flash, uint8_t *buf);

/* am29f040b.c */
int probe_29f040b(struct flashchip *flash);
int erase_29f040b(struct flashchip *flash);
int write_29f040b(struct flashchip *flash, uint8_t *buf);

/* pm29f002.c */
int write_pm29f002(struct flashchip *flash, uint8_t *buf);

/* en29f002a.c */
int probe_en29f002a(struct flashchip *flash);
int erase_en29f002a(struct flashchip *flash);
int write_en29f002a(struct flashchip *flash, uint8_t *buf);

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

/* jedec.c */
uint8_t oddparity(uint8_t val);
void toggle_ready_jedec(chipaddr dst);
void data_polling_jedec(chipaddr dst, uint8_t data);
void unprotect_jedec(chipaddr bios);
void protect_jedec(chipaddr bios);
int write_byte_program_jedec(chipaddr bios, uint8_t *src,
			     chipaddr dst);
int probe_jedec(struct flashchip *flash);
int erase_chip_jedec(struct flashchip *flash);
int write_jedec(struct flashchip *flash, uint8_t *buf);
int erase_sector_jedec(struct flashchip *flash, unsigned int page, unsigned int pagesize);
int erase_block_jedec(struct flashchip *flash, unsigned int page, unsigned int blocksize);
int write_sector_jedec(chipaddr bios, uint8_t *src,
		       chipaddr dst, unsigned int page_size);

/* m29f002.c */
int erase_m29f002(struct flashchip *flash);
int write_m29f002t(struct flashchip *flash, uint8_t *buf);
int write_m29f002b(struct flashchip *flash, uint8_t *buf);

/* m29f400bt.c */
int probe_m29f400bt(struct flashchip *flash);
int erase_m29f400bt(struct flashchip *flash);
int block_erase_m29f400bt(struct flashchip *flash, int start, int len);
int write_m29f400bt(struct flashchip *flash, uint8_t *buf);
int write_coreboot_m29f400bt(struct flashchip *flash, uint8_t *buf);
void toggle_ready_m29f400bt(chipaddr dst);
void data_polling_m29f400bt(chipaddr dst, uint8_t data);
void protect_m29f400bt(chipaddr bios);
void write_page_m29f400bt(chipaddr bios, uint8_t *src,
			  chipaddr dst, int page_size);

/* mx29f002.c */
int probe_29f002(struct flashchip *flash);
int erase_29f002(struct flashchip *flash);
int write_29f002(struct flashchip *flash, uint8_t *buf);

/* pm49fl00x.c */
int probe_49fl00x(struct flashchip *flash);
int erase_49fl00x(struct flashchip *flash);
int write_49fl00x(struct flashchip *flash, uint8_t *buf);

/* sharplhf00l04.c */
int probe_lhf00l04(struct flashchip *flash);
int erase_lhf00l04(struct flashchip *flash);
int write_lhf00l04(struct flashchip *flash, uint8_t *buf);
void toggle_ready_lhf00l04(chipaddr dst);
void data_polling_lhf00l04(chipaddr dst, uint8_t data);
void protect_lhf00l04(chipaddr bios);

/* sst28sf040.c */
int probe_28sf040(struct flashchip *flash);
int erase_28sf040(struct flashchip *flash);
int write_28sf040(struct flashchip *flash, uint8_t *buf);

/* sst39sf020.c */
int probe_39sf020(struct flashchip *flash);
int write_39sf020(struct flashchip *flash, uint8_t *buf);

/* sst49lf040.c */
int erase_49lf040(struct flashchip *flash);
int write_49lf040(struct flashchip *flash, uint8_t *buf);

/* sst49lfxxxc.c */
int probe_49lfxxxc(struct flashchip *flash);
int erase_49lfxxxc(struct flashchip *flash);
int write_49lfxxxc(struct flashchip *flash, uint8_t *buf);

/* sst_fwhub.c */
int probe_sst_fwhub(struct flashchip *flash);
int erase_sst_fwhub(struct flashchip *flash);
int erase_sst_fwhub_block(struct flashchip *flash, unsigned int offset, unsigned int page_size);
int write_sst_fwhub(struct flashchip *flash, uint8_t *buf);

/* w39v040c.c */
int probe_w39v040c(struct flashchip *flash);
int erase_w39v040c(struct flashchip *flash);
int write_w39v040c(struct flashchip *flash, uint8_t *buf);

/* w39V080fa.c */
int probe_winbond_fwhub(struct flashchip *flash);
int erase_winbond_fwhub(struct flashchip *flash);
int write_winbond_fwhub(struct flashchip *flash, uint8_t *buf);

/* w29ee011.c */
int probe_w29ee011(struct flashchip *flash);

/* w49f002u.c */
int write_49f002(struct flashchip *flash, uint8_t *buf);

/* wbsio_spi.c */
int wbsio_check_for_spi(const char *name);
int wbsio_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		      const unsigned char *writearr, unsigned char *readarr);
int wbsio_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int wbsio_spi_write_1(struct flashchip *flash, uint8_t *buf);

/* stm50flw0x0x.c */
int probe_stm50flw0x0x(struct flashchip *flash);
int erase_stm50flw0x0x(struct flashchip *flash);
int write_stm50flw0x0x(struct flashchip *flash, uint8_t *buf);

/* serprog.c */
int serprog_init(void);
int serprog_shutdown(void);
void serprog_chip_writeb(uint8_t val, chipaddr addr);
uint8_t serprog_chip_readb(const chipaddr addr);
void serprog_chip_readn(uint8_t *buf, const chipaddr addr, size_t len);
void serprog_delay(int delay);

#endif				/* !__FLASH_H__ */
