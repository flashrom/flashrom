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

#ifndef __PROGRAMMER_H__
#define __PROGRAMMER_H__ 1

#include <stdint.h>

#include "flash.h"	/* for chipaddr and flashctx */

enum programmer_type {
	PCI = 1, /* to detect uninitialized values */
	USB,
	OTHER,
};

struct dev_entry {
	uint16_t vendor_id;
	uint16_t device_id;
	const enum test_state status;
	const char *vendor_name;
	const char *device_name;
};

struct programmer_entry {
	const char *name;
	const enum programmer_type type;
	union {
		const struct dev_entry *const dev;
		const char *const note;
	} devs;

	int (*init) (void);

	void *(*map_flash_region) (const char *descr, uintptr_t phys_addr, size_t len);
	void (*unmap_flash_region) (void *virt_addr, size_t len);

	void (*delay) (unsigned int usecs);
};

extern const struct programmer_entry *const programmer_table[];
extern const size_t programmer_table_size;

/* programmer drivers */
extern const struct programmer_entry programmer_atahpt;
extern const struct programmer_entry programmer_atapromise;
extern const struct programmer_entry programmer_atavia;
extern const struct programmer_entry programmer_buspirate_spi;
extern const struct programmer_entry programmer_ch341a_spi;
extern const struct programmer_entry programmer_dediprog;
extern const struct programmer_entry programmer_developerbox;
extern const struct programmer_entry programmer_digilent_spi;
extern const struct programmer_entry programmer_drkaiser;
extern const struct programmer_entry programmer_dummy;
extern const struct programmer_entry programmer_ft2232_spi;
extern const struct programmer_entry programmer_gfxnvidia;
extern const struct programmer_entry programmer_internal;
extern const struct programmer_entry programmer_it8212;
extern const struct programmer_entry programmer_jlink_spi;
extern const struct programmer_entry programmer_linux_mtd;
extern const struct programmer_entry programmer_linux_spi;
extern const struct programmer_entry programmer_lspcon_i2c_spi;
extern const struct programmer_entry programmer_mstarddc_spi;
extern const struct programmer_entry programmer_ni845x_spi;
extern const struct programmer_entry programmer_nic3com;
extern const struct programmer_entry programmer_nicintel;
extern const struct programmer_entry programmer_nicintel_eeprom;
extern const struct programmer_entry programmer_nicintel_spi;
extern const struct programmer_entry programmer_nicnatsemi;
extern const struct programmer_entry programmer_nicrealtek;
extern const struct programmer_entry programmer_ogp_spi;
extern const struct programmer_entry programmer_pickit2_spi;
extern const struct programmer_entry programmer_pony_spi;
extern const struct programmer_entry programmer_raiden_debug_spi;
extern const struct programmer_entry programmer_rayer_spi;
extern const struct programmer_entry programmer_realtek_mst_i2c_spi;
extern const struct programmer_entry programmer_satamv;
extern const struct programmer_entry programmer_satasii;
extern const struct programmer_entry programmer_serprog;
extern const struct programmer_entry programmer_stlinkv3_spi;
extern const struct programmer_entry programmer_usbblaster_spi;

int programmer_init(const struct programmer_entry *prog, const char *param);
int programmer_shutdown(void);

struct bitbang_spi_master {
	/* Note that CS# is active low, so val=0 means the chip is active. */
	void (*set_cs) (int val, void *spi_data);
	void (*set_sck) (int val, void *spi_data);
	void (*set_mosi) (int val, void *spi_data);
	int (*get_miso) (void *spi_data);
	void (*request_bus) (void *spi_data);
	void (*release_bus) (void *spi_data);
	/* optional functions to optimize xfers */
	void (*set_sck_set_mosi) (int sck, int mosi, void *spi_data);
	int (*set_sck_get_miso) (int sck, void *spi_data);
	/* Length of half a clock period in usecs. */
	unsigned int half_period;
};

#if NEED_PCI == 1
struct pci_dev;

/* pcidev.c */
// FIXME: This needs to be local, not global(?)
extern struct pci_access *pacc;
int pci_init_common(void);
uintptr_t pcidev_readbar(struct pci_dev *dev, int bar);
struct pci_dev *pcidev_init(const struct dev_entry *devs, int bar);
/* rpci_write_* are reversible writes. The original PCI config space register
 * contents will be restored on shutdown.
 * To clone the pci_dev instances internally, the `pacc` global
 * variable has to reference a pci_access method that is compatible
 * with the given pci_dev handle. The referenced pci_access (not
 * the variable) has to stay valid until the shutdown handlers are
 * finished.
 */
int rpci_write_byte(struct pci_dev *dev, int reg, uint8_t data);
int rpci_write_word(struct pci_dev *dev, int reg, uint16_t data);
int rpci_write_long(struct pci_dev *dev, int reg, uint32_t data);
#endif

#if CONFIG_INTERNAL == 1
struct penable {
	uint16_t vendor_id;
	uint16_t device_id;
	enum chipbustype buses;
	const enum test_state status;
	const char *vendor_name;
	const char *device_name;
	int (*doit) (struct pci_dev *dev, const char *name);
};

extern const struct penable chipset_enables[];

enum board_match_phase {
	P1,
	P2,
	P3
};

struct board_match {
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

	/* Pattern to match DMI entries. May be NULL. */
	const char *dmi_pattern;

	/* The vendor / part name from the coreboot table. May be NULL. */
	const char *lb_vendor;
	const char *lb_part;

	enum board_match_phase phase;

	const char *vendor_name;
	const char *board_name;

	int max_rom_decode_parallel;
	const enum test_state status;
	int (*enable) (void); /* May be NULL. */
};

extern const struct board_match board_matches[];

struct board_info {
	const char *vendor;
	const char *name;
	const enum test_state working;
#ifdef CONFIG_PRINT_WIKI
	const char *url;
	const char *note;
#endif
};

extern const struct board_info boards_known[];
extern const struct board_info laptops_known[];
#endif

/* udelay.c */
void myusec_delay(unsigned int usecs);
void myusec_calibrate_delay(void);
void internal_sleep(unsigned int usecs);
void internal_delay(unsigned int usecs);

#if CONFIG_INTERNAL == 1
/* board_enable.c */
int selfcheck_board_enables(void);
int board_parse_parameter(const char *boardstring, char **vendor, char **model);
void w836xx_ext_enter(uint16_t port);
void w836xx_ext_leave(uint16_t port);
void probe_superio_winbond(void);
int it8705f_write_enable(uint8_t port);
uint8_t sio_read(uint16_t port, uint8_t reg);
void sio_write(uint16_t port, uint8_t reg, uint8_t data);
void sio_mask(uint16_t port, uint8_t reg, uint8_t data, uint8_t mask);
void board_handle_before_superio(void);
void board_handle_before_laptop(void);
int board_flash_enable(const char *vendor, const char *model, const char *cb_vendor, const char *cb_model);

/* chipset_enable.c */
int chipset_flash_enable(void);

/* processor_enable.c */
int processor_flash_enable(void);
#endif

#if CONFIG_INTERNAL == 1
/* cbtable.c */
int cb_parse_table(const char **vendor, const char **model);
int cb_check_image(const uint8_t *bios, unsigned int size);

/* dmi.c */
#if defined(__i386__) || defined(__x86_64__)
extern int has_dmi_support;
void dmi_init(void);
int dmi_match(const char *pattern);
#endif // defined(__i386__) || defined(__x86_64__)

/* internal.c */
struct superio {
	uint16_t vendor;
	uint16_t port;
	uint16_t model;
};
extern struct superio superios[];
extern int superio_count;
#define SUPERIO_VENDOR_NONE	0x0
#define SUPERIO_VENDOR_ITE	0x1
#define SUPERIO_VENDOR_WINBOND	0x2
#endif
#if NEED_PCI == 1
struct pci_dev *pci_dev_find_vendorclass(uint16_t vendor, uint16_t devclass);
struct pci_dev *pci_dev_find(uint16_t vendor, uint16_t device);
struct pci_dev *pci_card_find(uint16_t vendor, uint16_t device,
			      uint16_t card_vendor, uint16_t card_device);
#endif
#if CONFIG_INTERNAL == 1
extern int is_laptop;
extern int laptop_ok;
extern int force_boardenable;
extern int force_boardmismatch;
void probe_superio(void);
int register_superio(struct superio s);
extern enum chipbustype internal_buses_supported;
#endif

/* bitbang_spi.c */
int register_spi_bitbang_master(const struct bitbang_spi_master *master, void *spi_data);


/* flashrom.c */
struct decode_sizes {
	uint32_t parallel;
	uint32_t lpc;
	uint32_t fwh;
	uint32_t spi;
};
// FIXME: These need to be local, not global
extern struct decode_sizes max_rom_decode;
extern int programmer_may_write;
extern unsigned long flashbase;
unsigned int count_max_decode_exceedings(const struct flashctx *flash);
char *extract_programmer_param(const char *param_name);

/* spi.c */
#define MAX_DATA_UNSPECIFIED 0
#define MAX_DATA_READ_UNLIMITED 64 * 1024
#define MAX_DATA_WRITE_UNLIMITED 256

#define SPI_MASTER_4BA			(1U << 0)  /**< Can handle 4-byte addresses */
#define SPI_MASTER_NO_4BA_MODES		(1U << 1)  /**< Compatibility modes (i.e. extended address
						        register, 4BA mode switch) don't work */

struct spi_master {
	uint32_t features;
	unsigned int max_data_read; // (Ideally,) maximum data read size in one go (excluding opcode+address).
	unsigned int max_data_write; // (Ideally,) maximum data write size in one go (excluding opcode+address).
	int (*command)(const struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
		   const unsigned char *writearr, unsigned char *readarr);
	int (*multicommand)(const struct flashctx *flash, struct spi_command *cmds);

	/* Optimized functions for this master */
	int (*read)(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len);
	int (*write_256)(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
	int (*write_aai)(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
	int (*shutdown)(void *data);
	void *data;
};

int default_spi_send_command(const struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
			     const unsigned char *writearr, unsigned char *readarr);
int default_spi_send_multicommand(const struct flashctx *flash, struct spi_command *cmds);
int default_spi_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len);
int default_spi_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
int default_spi_write_aai(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
int register_spi_master(const struct spi_master *mst, void *data);

/* The following enum is needed by ich_descriptor_tool and ich* code as well as in chipset_enable.c. */
enum ich_chipset {
	CHIPSET_ICH_UNKNOWN,
	CHIPSET_ICH,
	CHIPSET_ICH2345,
	CHIPSET_ICH6,
	CHIPSET_POULSBO, /* SCH U* */
	CHIPSET_TUNNEL_CREEK, /* Atom E6xx */
	CHIPSET_CENTERTON, /* Atom S1220 S1240 S1260 */
	CHIPSET_ICH7,
	CHIPSET_ICH8,
	CHIPSET_ICH9,
	CHIPSET_ICH10,
	CHIPSET_5_SERIES_IBEX_PEAK,
	CHIPSET_6_SERIES_COUGAR_POINT,
	CHIPSET_7_SERIES_PANTHER_POINT,
	CHIPSET_8_SERIES_LYNX_POINT,
	CHIPSET_BAYTRAIL, /* Actually all with Silvermont architecture: Bay Trail, Avoton/Rangeley */
	CHIPSET_8_SERIES_LYNX_POINT_LP,
	CHIPSET_8_SERIES_WELLSBURG,
	CHIPSET_9_SERIES_WILDCAT_POINT,
	CHIPSET_9_SERIES_WILDCAT_POINT_LP,
	CHIPSET_100_SERIES_SUNRISE_POINT, /* also 6th/7th gen Core i/o (LP) variants */
	CHIPSET_C620_SERIES_LEWISBURG,
	CHIPSET_300_SERIES_CANNON_POINT,
	CHIPSET_400_SERIES_COMET_POINT,
	CHIPSET_500_SERIES_TIGER_POINT,
	CHIPSET_600_SERIES_ALDER_POINT,
	CHIPSET_APOLLO_LAKE,
	CHIPSET_GEMINI_LAKE,
};

/* ichspi.c */
#if CONFIG_INTERNAL == 1
int ich_init_spi(void *spibar, enum ich_chipset ich_generation);
int via_init_spi(uint32_t mmio_base);

/* amd_imc.c */
int amd_imc_shutdown(struct pci_dev *dev);

/* it85spi.c */
int it85xx_spi_init(struct superio s);

/* it87spi.c */
void enter_conf_mode_ite(uint16_t port);
void exit_conf_mode_ite(uint16_t port);
void probe_superio_ite(void);
int init_superio_ite(void);

#if CONFIG_LINUX_MTD == 1
/* trivial wrapper to avoid cluttering internal_init() with #if */
static inline int try_mtd(void) { return programmer_linux_mtd.init(); };
#else
static inline int try_mtd(void) { return 1; };
#endif

/* mcp6x_spi.c */
int mcp6x_spi_init(int want_spi);



/* sb600spi.c */
int sb600_probe_spi(struct pci_dev *dev);

/* wbsio_spi.c */
int wbsio_check_for_spi(void);
#endif

/* opaque.c */
struct opaque_master {
	int max_data_read;
	int max_data_write;
	/* Specific functions for this master */
	int (*probe) (struct flashctx *flash);
	int (*read) (struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len);
	int (*write) (struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
	int (*erase) (struct flashctx *flash, unsigned int blockaddr, unsigned int blocklen);
	int (*shutdown)(void *data);
	void *data;
};
int register_opaque_master(const struct opaque_master *mst, void *data);

/* programmer.c */
void *fallback_map(const char *descr, uintptr_t phys_addr, size_t len);
void fallback_unmap(void *virt_addr, size_t len);
void fallback_chip_writew(const struct flashctx *flash, uint16_t val, chipaddr addr);
void fallback_chip_writel(const struct flashctx *flash, uint32_t val, chipaddr addr);
void fallback_chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len);
uint16_t fallback_chip_readw(const struct flashctx *flash, const chipaddr addr);
uint32_t fallback_chip_readl(const struct flashctx *flash, const chipaddr addr);
void fallback_chip_readn(const struct flashctx *flash, uint8_t *buf, const chipaddr addr, size_t len);
struct par_master {
	void (*chip_writeb) (const struct flashctx *flash, uint8_t val, chipaddr addr);
	void (*chip_writew) (const struct flashctx *flash, uint16_t val, chipaddr addr);
	void (*chip_writel) (const struct flashctx *flash, uint32_t val, chipaddr addr);
	void (*chip_writen) (const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len);
	uint8_t (*chip_readb) (const struct flashctx *flash, const chipaddr addr);
	uint16_t (*chip_readw) (const struct flashctx *flash, const chipaddr addr);
	uint32_t (*chip_readl) (const struct flashctx *flash, const chipaddr addr);
	void (*chip_readn) (const struct flashctx *flash, uint8_t *buf, const chipaddr addr, size_t len);
	int (*shutdown)(void *data);
	void *data;
};
int register_par_master(const struct par_master *mst, const enum chipbustype buses, void *data);
struct registered_master {
	enum chipbustype buses_supported;
	struct {
		struct par_master par;
		struct spi_master spi;
		struct opaque_master opaque;
	};
};
extern struct registered_master registered_masters[];
extern int registered_master_count;
int register_master(const struct registered_master *mst);



/* serial.c */
#if IS_WINDOWS
typedef HANDLE fdtype;
#define SER_INV_FD	INVALID_HANDLE_VALUE
#else
typedef int fdtype;
#define SER_INV_FD	-1
#endif

void sp_flush_incoming(void);
fdtype sp_openserport(char *dev, int baud);
extern fdtype sp_fd;
int serialport_config(fdtype fd, int baud);
int serialport_shutdown(void *data);
int serialport_write(const unsigned char *buf, unsigned int writecnt);
int serialport_write_nonblock(const unsigned char *buf, unsigned int writecnt, unsigned int timeout, unsigned int *really_wrote);
int serialport_read(unsigned char *buf, unsigned int readcnt);
int serialport_read_nonblock(unsigned char *c, unsigned int readcnt, unsigned int timeout, unsigned int *really_read);

/* Serial port/pin mapping:

  1	CD	<-
  2	RXD	<-
  3	TXD	->
  4	DTR	->
  5	GND     --
  6	DSR	<-
  7	RTS	->
  8	CTS	<-
  9	RI	<-
*/
enum SP_PIN {
	PIN_CD = 1,
	PIN_RXD,
	PIN_TXD,
	PIN_DTR,
	PIN_GND,
	PIN_DSR,
	PIN_RTS,
	PIN_CTS,
	PIN_RI,
};

void sp_set_pin(enum SP_PIN pin, int val);
int sp_get_pin(enum SP_PIN pin);

/* spi_master feature checks */
static inline bool spi_master_4ba(const struct flashctx *const flash)
{
	return flash->mst->buses_supported & BUS_SPI &&
		flash->mst->spi.features & SPI_MASTER_4BA;
}
static inline bool spi_master_no_4ba_modes(const struct flashctx *const flash)
{
	return flash->mst->buses_supported & BUS_SPI &&
		flash->mst->spi.features & SPI_MASTER_NO_4BA_MODES;
}

/* usbdev.c */
struct libusb_device_handle;
struct libusb_context;
struct libusb_device_handle *usb_dev_get_by_vid_pid_serial(
		struct libusb_context *usb_ctx, uint16_t vid, uint16_t pid, const char *serialno);
struct libusb_device_handle *usb_dev_get_by_vid_pid_number(
		struct libusb_context *usb_ctx, uint16_t vid, uint16_t pid, unsigned int num);


#endif				/* !__PROGRAMMER_H__ */
