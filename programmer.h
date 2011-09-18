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

#ifndef __PROGRAMMER_H__
#define __PROGRAMMER_H__ 1

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
#if CONFIG_NICINTEL == 1
	PROGRAMMER_NICINTEL,
#endif
#if CONFIG_NICINTEL_SPI == 1
	PROGRAMMER_NICINTEL_SPI,
#endif
#if CONFIG_OGP_SPI == 1
	PROGRAMMER_OGP_SPI,
#endif
#if CONFIG_SATAMV == 1
	PROGRAMMER_SATAMV,
#endif
#if CONFIG_LINUX_SPI == 1
	PROGRAMMER_LINUX_SPI,
#endif
	PROGRAMMER_INVALID /* This must always be the last entry. */
};

struct programmer_entry {
	const char *vendor;
	const char *name;

	int (*init) (void);

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

int programmer_init(enum programmer prog, char *param);
int programmer_shutdown(void);

enum bitbang_spi_master_type {
	BITBANG_SPI_INVALID	= 0, /* This must always be the first entry. */
#if CONFIG_RAYER_SPI == 1
	BITBANG_SPI_MASTER_RAYER,
#endif
#if CONFIG_NICINTEL_SPI == 1
	BITBANG_SPI_MASTER_NICINTEL,
#endif
#if CONFIG_INTERNAL == 1
#if defined(__i386__) || defined(__x86_64__)
	BITBANG_SPI_MASTER_MCP,
#endif
#endif
#if CONFIG_OGP_SPI == 1
	BITBANG_SPI_MASTER_OGP,
#endif
};

struct bitbang_spi_master {
	enum bitbang_spi_master_type type;

	/* Note that CS# is active low, so val=0 means the chip is active. */
	void (*set_cs) (int val);
	void (*set_sck) (int val);
	void (*set_mosi) (int val);
	int (*get_miso) (void);
	void (*request_bus) (void);
	void (*release_bus) (void);
};

#if CONFIG_INTERNAL == 1
struct penable {
	uint16_t vendor_id;
	uint16_t device_id;
	int status; /* OK=0 and NT=1 are defines only. Beware! */
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
	int status;
	int (*enable) (void); /* May be NULL. */
};

extern const struct board_match board_matches[];

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
uintptr_t pcidev_validate(struct pci_dev *dev, int bar, const struct pcidev_status *devs);
uintptr_t pcidev_init(int bar, const struct pcidev_status *devs);
/* rpci_write_* are reversible writes. The original PCI config space register
 * contents will be restored on shutdown.
 */
int rpci_write_byte(struct pci_dev *dev, int reg, uint8_t data);
int rpci_write_word(struct pci_dev *dev, int reg, uint16_t data);
int rpci_write_long(struct pci_dev *dev, int reg, uint32_t data);
#endif

/* print.c */
#if CONFIG_NIC3COM+CONFIG_NICREALTEK+CONFIG_NICNATSEMI+CONFIG_GFXNVIDIA+CONFIG_DRKAISER+CONFIG_SATASII+CONFIG_ATAHPT+CONFIG_NICINTEL+CONFIG_NICINTEL_SPI+CONFIG_OGP_SPI+CONFIG_SATAMV >= 1
void print_supported_pcidevs(const struct pcidev_status *devs);
#endif

#if CONFIG_INTERNAL == 1
/* board_enable.c */
void w836xx_ext_enter(uint16_t port);
void w836xx_ext_leave(uint16_t port);
int it8705f_write_enable(uint8_t port);
uint8_t sio_read(uint16_t port, uint8_t reg);
void sio_write(uint16_t port, uint8_t reg, uint8_t data);
void sio_mask(uint16_t port, uint8_t reg, uint8_t data, uint8_t mask);
void board_handle_before_superio(void);
void board_handle_before_laptop(void);
int board_flash_enable(const char *vendor, const char *part);

/* chipset_enable.c */
int chipset_flash_enable(void);

/* processor_enable.c */
int processor_flash_enable(void);
#endif

/* physmap.c */
void *physmap(const char *descr, unsigned long phys_addr, size_t len);
void *physmap_try_ro(const char *descr, unsigned long phys_addr, size_t len);
void physunmap(void *virt_addr, size_t len);
#if CONFIG_INTERNAL == 1
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
struct superio {
	uint16_t vendor;
	uint16_t port;
	uint16_t model;
};
extern struct superio superios[];
extern int superio_count;
#define SUPERIO_VENDOR_NONE	0x0
#define SUPERIO_VENDOR_ITE	0x1
#endif
#if NEED_PCI == 1
struct pci_dev *pci_dev_find_filter(struct pci_filter filter);
struct pci_dev *pci_dev_find_vendorclass(uint16_t vendor, uint16_t devclass);
struct pci_dev *pci_dev_find(uint16_t vendor, uint16_t device);
struct pci_dev *pci_card_find(uint16_t vendor, uint16_t device,
			      uint16_t card_vendor, uint16_t card_device);
#endif
void get_io_perms(void);
void release_io_perms(void);
#if CONFIG_INTERNAL == 1
extern int is_laptop;
extern int laptop_ok;
extern int force_boardenable;
extern int force_boardmismatch;
void probe_superio(void);
int register_superio(struct superio s);
int internal_init(void);
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
void rmmio_writeb(uint8_t val, void *addr);
void rmmio_writew(uint16_t val, void *addr);
void rmmio_writel(uint32_t val, void *addr);
void rmmio_le_writeb(uint8_t val, void *addr);
void rmmio_le_writew(uint16_t val, void *addr);
void rmmio_le_writel(uint32_t val, void *addr);
#define pci_rmmio_writeb rmmio_le_writeb
#define pci_rmmio_writew rmmio_le_writew
#define pci_rmmio_writel rmmio_le_writel
void rmmio_valb(void *addr);
void rmmio_valw(void *addr);
void rmmio_vall(void *addr);

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
#endif

/* nic3com.c */
#if CONFIG_NIC3COM == 1
int nic3com_init(void);
void nic3com_chip_writeb(uint8_t val, chipaddr addr);
uint8_t nic3com_chip_readb(const chipaddr addr);
extern const struct pcidev_status nics_3com[];
#endif

/* gfxnvidia.c */
#if CONFIG_GFXNVIDIA == 1
int gfxnvidia_init(void);
void gfxnvidia_chip_writeb(uint8_t val, chipaddr addr);
uint8_t gfxnvidia_chip_readb(const chipaddr addr);
extern const struct pcidev_status gfx_nvidia[];
#endif

/* drkaiser.c */
#if CONFIG_DRKAISER == 1
int drkaiser_init(void);
void drkaiser_chip_writeb(uint8_t val, chipaddr addr);
uint8_t drkaiser_chip_readb(const chipaddr addr);
extern const struct pcidev_status drkaiser_pcidev[];
#endif

/* nicrealtek.c */
#if CONFIG_NICREALTEK == 1
int nicrealtek_init(void);
void nicrealtek_chip_writeb(uint8_t val, chipaddr addr);
uint8_t nicrealtek_chip_readb(const chipaddr addr);
extern const struct pcidev_status nics_realtek[];
#endif

/* nicnatsemi.c */
#if CONFIG_NICNATSEMI == 1
int nicnatsemi_init(void);
void nicnatsemi_chip_writeb(uint8_t val, chipaddr addr);
uint8_t nicnatsemi_chip_readb(const chipaddr addr);
extern const struct pcidev_status nics_natsemi[];
#endif

/* nicintel.c */
#if CONFIG_NICINTEL == 1
int nicintel_init(void);
void nicintel_chip_writeb(uint8_t val, chipaddr addr);
uint8_t nicintel_chip_readb(const chipaddr addr);
extern const struct pcidev_status nics_intel[];
#endif

/* nicintel_spi.c */
#if CONFIG_NICINTEL_SPI == 1
int nicintel_spi_init(void);
extern const struct pcidev_status nics_intel_spi[];
#endif

/* ogp_spi.c */
#if CONFIG_OGP_SPI == 1
int ogp_spi_init(void);
extern const struct pcidev_status ogp_spi[];
#endif

/* satamv.c */
#if CONFIG_SATAMV == 1
int satamv_init(void);
void satamv_chip_writeb(uint8_t val, chipaddr addr);
uint8_t satamv_chip_readb(const chipaddr addr);
extern const struct pcidev_status satas_mv[];
#endif

/* satasii.c */
#if CONFIG_SATASII == 1
int satasii_init(void);
void satasii_chip_writeb(uint8_t val, chipaddr addr);
uint8_t satasii_chip_readb(const chipaddr addr);
extern const struct pcidev_status satas_sii[];
#endif

/* atahpt.c */
#if CONFIG_ATAHPT == 1
int atahpt_init(void);
void atahpt_chip_writeb(uint8_t val, chipaddr addr);
uint8_t atahpt_chip_readb(const chipaddr addr);
extern const struct pcidev_status ata_hpt[];
#endif

/* ft2232_spi.c */
#if CONFIG_FT2232_SPI == 1
struct usbdev_status {
	uint16_t vendor_id;
	uint16_t device_id;
	int status;
	const char *vendor_name;
	const char *device_name;
};
int ft2232_spi_init(void);
extern const struct usbdev_status devs_ft2232spi[];
void print_supported_usbdevs(const struct usbdev_status *devs);
#endif

/* rayer_spi.c */
#if CONFIG_RAYER_SPI == 1
int rayer_spi_init(void);
#endif

/* bitbang_spi.c */
int bitbang_spi_init(const struct bitbang_spi_master *master, int halfperiod);
int bitbang_spi_shutdown(const struct bitbang_spi_master *master);

/* buspirate_spi.c */
#if CONFIG_BUSPIRATE_SPI == 1
int buspirate_spi_init(void);
#endif

/* linux_spi.c */
#if CONFIG_LINUX_SPI == 1
int linux_spi_init(void);
#endif

/* dediprog.c */
#if CONFIG_DEDIPROG == 1
int dediprog_init(void);
#endif

/* flashrom.c */
struct decode_sizes {
	uint32_t parallel;
	uint32_t lpc;
	uint32_t fwh;
	uint32_t spi;
};
extern struct decode_sizes max_rom_decode;
extern int programmer_may_write;
extern unsigned long flashbase;
void check_chip_supported(const struct flashchip *flash);
int check_max_decode(enum chipbustype buses, uint32_t size);
char *extract_programmer_param(const char *param_name);

/* layout.c */
int show_id(uint8_t *bios, int size, int force);

/* spi.c */
enum spi_controller {
	SPI_CONTROLLER_NONE,
#if CONFIG_INTERNAL == 1
#if defined(__i386__) || defined(__x86_64__)
	SPI_CONTROLLER_ICH7,
	SPI_CONTROLLER_ICH9,
	SPI_CONTROLLER_IT85XX,
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
#if CONFIG_OGP_SPI == 1 || CONFIG_NICINTEL_SPI == 1 || CONFIG_RAYER_SPI == 1 || (CONFIG_INTERNAL == 1 && (defined(__i386__) || defined(__x86_64__)))
	SPI_CONTROLLER_BITBANG,
#endif
#if CONFIG_LINUX_SPI == 1
	SPI_CONTROLLER_LINUX,
#endif
#if CONFIG_SERPROG == 1
	SPI_CONTROLLER_SERPROG,
#endif
};
extern const int spi_programmer_count;

#define MAX_DATA_UNSPECIFIED 0
#define MAX_DATA_READ_UNLIMITED 64 * 1024
#define MAX_DATA_WRITE_UNLIMITED 256
struct spi_programmer {
	enum spi_controller type;
	int max_data_read;
	int max_data_write;
	int (*command)(unsigned int writecnt, unsigned int readcnt,
		   const unsigned char *writearr, unsigned char *readarr);
	int (*multicommand)(struct spi_command *cmds);

	/* Optimized functions for this programmer */
	int (*read)(struct flashchip *flash, uint8_t *buf, int start, int len);
	int (*write_256)(struct flashchip *flash, uint8_t *buf, int start, int len);
};

extern const struct spi_programmer *spi_programmer;
int default_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			     const unsigned char *writearr, unsigned char *readarr);
int default_spi_send_multicommand(struct spi_command *cmds);
int default_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
int default_spi_write_256(struct flashchip *flash, uint8_t *buf, int start, int len);
void register_spi_programmer(const struct spi_programmer *programmer);

/* ichspi.c */
#if CONFIG_INTERNAL == 1
extern uint32_t ichspi_bbar;
int ich_init_spi(struct pci_dev *dev, uint32_t base, void *rcrb,
		    int ich_generation);
int via_init_spi(struct pci_dev *dev);

/* it85spi.c */
int it85xx_spi_init(struct superio s);

/* it87spi.c */
void enter_conf_mode_ite(uint16_t port);
void exit_conf_mode_ite(uint16_t port);
void probe_superio_ite(void);
int init_superio_ite(void);

/* mcp6x_spi.c */
int mcp6x_spi_init(int want_spi);

/* sb600spi.c */
int sb600_probe_spi(struct pci_dev *dev);

/* wbsio_spi.c */
int wbsio_check_for_spi(void);
#endif

/* serprog.c */
#if CONFIG_SERPROG == 1
int serprog_init(void);
void serprog_chip_writeb(uint8_t val, chipaddr addr);
uint8_t serprog_chip_readb(const chipaddr addr);
void serprog_chip_readn(uint8_t *buf, const chipaddr addr, size_t len);
void serprog_delay(int delay);
int serprog_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			const unsigned char *writearr, unsigned char *readarr);
int serprog_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len);
#endif

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
/* expose serialport_shutdown as it's currently used by buspirate */
int serialport_shutdown(void *data);
int serialport_write(unsigned char *buf, unsigned int writecnt);
int serialport_read(unsigned char *buf, unsigned int readcnt);

#endif				/* !__PROGRAMMER_H__ */
