/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2005-2007 coresystems GmbH <stepan@coresystems.de>
 * Copyright (C) 2006 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2007-2009 Luc Verhaegen <libv@skynet.be>
 * Copyright (C) 2007 Carl-Daniel Hailfinger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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

/*
 * Contains the board specific flash enables.
 */

#include <string.h>
#include <fcntl.h>
#include "flash.h"

/*
 * Helper functions for many Winbond Super I/Os of the W836xx range.
 */
/* Enter extended functions */
void w836xx_ext_enter(uint16_t port)
{
	OUTB(0x87, port);
	OUTB(0x87, port);
}

/* Leave extended functions */
void w836xx_ext_leave(uint16_t port)
{
	OUTB(0xAA, port);
}

/* Generic Super I/O helper functions */
uint8_t sio_read(uint16_t port, uint8_t reg)
{
	OUTB(reg, port);
	return INB(port + 1);
}

void sio_write(uint16_t port, uint8_t reg, uint8_t data)
{
	OUTB(reg, port);
	OUTB(data, port + 1);
}

void sio_mask(uint16_t port, uint8_t reg, uint8_t data, uint8_t mask)
{
	uint8_t tmp;

	OUTB(reg, port);
	tmp = INB(port + 1) & ~mask;
	OUTB(tmp | (data & mask), port + 1);
}

/**
 * Winbond W83627HF: Raise GPIO24.
 *
 * Suited for:
 *  - Agami Aruma
 *  - IWILL DK8-HTX
 */
static int w83627hf_gpio24_raise(uint16_t port, const char *name)
{
	w836xx_ext_enter(port);

	/* Is this the W83627HF? */
	if (sio_read(port, 0x20) != 0x52) {	/* Super I/O device ID reg. */
		fprintf(stderr, "\nERROR: %s: W83627HF: Wrong ID: 0x%02X.\n",
			name, sio_read(port, 0x20));
		w836xx_ext_leave(port);
		return -1;
	}

	/* PIN89S: WDTO/GP24 multiplex -> GPIO24 */
	sio_mask(port, 0x2B, 0x10, 0x10);

	/* Select logical device 8: GPIO port 2 */
	sio_write(port, 0x07, 0x08);

	sio_mask(port, 0x30, 0x01, 0x01);	/* Activate logical device. */
	sio_mask(port, 0xF0, 0x00, 0x10);	/* GPIO24 -> output */
	sio_mask(port, 0xF2, 0x00, 0x10);	/* Clear GPIO24 inversion */
	sio_mask(port, 0xF1, 0x10, 0x10);	/* Raise GPIO24 */

	w836xx_ext_leave(port);

	return 0;
}

static int w83627hf_gpio24_raise_2e(const char *name)
{
	return w83627hf_gpio24_raise(0x2e, name);
}

/**
 * Winbond W83627THF: GPIO 4, bit 4
 *
 * Suited for:
 *  - MSI K8T Neo2-F
 *  - MSI K8N-NEO3
 */
static int w83627thf_gpio4_4_raise(uint16_t port, const char *name)
{
	w836xx_ext_enter(port);

	/* Is this the W83627THF? */
	if (sio_read(port, 0x20) != 0x82) {	/* Super I/O device ID reg. */
		fprintf(stderr, "\nERROR: %s: W83627THF: Wrong ID: 0x%02X.\n",
			name, sio_read(port, 0x20));
		w836xx_ext_leave(port);
		return -1;
	}

	/* PINxxxxS: GPIO4/bit 4 multiplex -> GPIOXXX */

	sio_write(port, 0x07, 0x09);      /* Select LDN 9: GPIO port 4 */
	sio_mask(port, 0x30, 0x02, 0x02); /* Activate logical device. */
	sio_mask(port, 0xF4, 0x00, 0x10); /* GPIO4 bit 4 -> output */
	sio_mask(port, 0xF6, 0x00, 0x10); /* Clear GPIO4 bit 4 inversion */
	sio_mask(port, 0xF5, 0x10, 0x10); /* Raise GPIO4 bit 4 */

	w836xx_ext_leave(port);

	return 0;
}

static int w83627thf_gpio4_4_raise_2e(const char *name)
{
	return w83627thf_gpio4_4_raise(0x2e, name);
}

static int w83627thf_gpio4_4_raise_4e(const char *name)
{
	return w83627thf_gpio4_4_raise(0x4e, name);
}

/**
 * w83627: Enable MEMW# and set ROM size to max.
 */
static void w836xx_memw_enable(uint16_t port)
{
	w836xx_ext_enter(port);
	if (!(sio_read(port, 0x24) & 0x02)) {	/* Flash ROM enabled? */
		/* Enable MEMW# and set ROM size select to max. (4M). */
		sio_mask(port, 0x24, 0x28, 0x28);
	}
	w836xx_ext_leave(port);
}

/**
 * Common routine for several VT823x based boards.
 */
static void vt823x_set_all_writes_to_lpc(struct pci_dev *dev)
{
	uint8_t val;

	/* All memory cycles, not just ROM ones, go to LPC. */
	val = pci_read_byte(dev, 0x59);
	val &= ~0x80;
	pci_write_byte(dev, 0x59, val);
}

/**
 * VT823x: Set one of the GPIO pins.
 */
static void vt823x_gpio_set(struct pci_dev *dev, uint8_t gpio, int raise)
{
	uint16_t base;
	uint8_t val, bit;

	if ((gpio >= 12) && (gpio <= 15)) {
		/* GPIO12-15 -> output */
		val = pci_read_byte(dev, 0xE4);
		val |= 0x10;
		pci_write_byte(dev, 0xE4, val);
	} else if (gpio == 9) {
		/* GPIO9 -> Output */
		val = pci_read_byte(dev, 0xE4);
		val |= 0x20;
		pci_write_byte(dev, 0xE4, val);
	} else {
		fprintf(stderr, "\nERROR: "
			"VT823x GPIO%02d is not implemented.\n", gpio);
		return;
	}

	/* Now raise/drop the GPIO line itself. */
	bit = 0x01 << (gpio - 8);

	/* We need the I/O Base Address for this board's flash enable. */
	base = pci_read_word(dev, 0x88) & 0xff80;

	val = INB(base + 0x4D);
	if (raise)
		val |= bit;
	else
		val &= ~bit;
	OUTB(val, base + 0x4D);
}

/**
 * Suited for VIAs EPIA M and MII, and maybe other CLE266 based EPIAs.
 *
 * We don't need to do this when using coreboot, GPIO15 is never lowered there.
 */
static int board_via_epia_m(const char *name)
{
	struct pci_dev *dev;

	dev = pci_dev_find(0x1106, 0x3177);	/* VT8235 ISA bridge */
	if (!dev) {
		fprintf(stderr, "\nERROR: VT8235 ISA bridge not found.\n");
		return -1;
	}

	/* GPIO15 is connected to write protect. */
	vt823x_gpio_set(dev, 15, 1);

	return 0;
}

/**
 * Suited for:
 *   - ASUS A7V8X-MX SE and A7V400-MX: AMD K7 + VIA KM400A + VT8235
 *   - Tyan S2498 (Tomcat K7M): AMD Geode NX + VIA KM400 + VT8237.
 */
static int board_asus_a7v8x_mx(const char *name)
{
	struct pci_dev *dev;

	dev = pci_dev_find(0x1106, 0x3177);	/* VT8235 ISA bridge */
	if (!dev)
		dev = pci_dev_find(0x1106, 0x3227);	/* VT8237 ISA bridge */
	if (!dev) {
		fprintf(stderr, "\nERROR: VT823x ISA bridge not found.\n");
		return -1;
	}

	vt823x_set_all_writes_to_lpc(dev);
	w836xx_memw_enable(0x2E);

	return 0;
}

/**
 * Suited for VIAs EPIA SP and EPIA CN.
 */
static int board_via_epia_sp(const char *name)
{
	struct pci_dev *dev;

	dev = pci_dev_find(0x1106, 0x3227);	/* VT8237R ISA bridge */
	if (!dev) {
		fprintf(stderr, "\nERROR: VT8237R ISA bridge not found.\n");
		return -1;
	}

	vt823x_set_all_writes_to_lpc(dev);

	return 0;
}

/**
 * Suited for VIAs EPIA N & NL.
 */
static int board_via_epia_n(const char *name)
{
	struct pci_dev *dev;

	dev = pci_dev_find(0x1106, 0x3227);	/* VT8237R ISA bridge */
	if (!dev) {
		fprintf(stderr, "\nERROR: VT8237R ISA bridge not found.\n");
		return -1;
	}

	/* All memory cycles, not just ROM ones, go to LPC */
	vt823x_set_all_writes_to_lpc(dev);

	/* GPIO9 -> output */
	vt823x_gpio_set(dev, 9, 1);

	return 0;
}

/**
 * Suited for:
 *   - EPoX EP-8K5A2: VIA KT333 + VT8235.
 *   - Albatron PM266A Pro: VIA P4M266A + VT8235.
 *   - Shuttle AK31 (all versions): VIA KT266 + VT8233.
 */
static int w836xx_memw_enable_2e(const char *name)
{
	w836xx_memw_enable(0x2E);

	return 0;
}

/**
 * Suited for ASUS P5A.
 *
 * This is rather nasty code, but there's no way to do this cleanly.
 * We're basically talking to some unknown device on SMBus, my guess
 * is that it is the Winbond W83781D that lives near the DIP BIOS.
 */
static int board_asus_p5a(const char *name)
{
	uint8_t tmp;
	int i;

#define ASUSP5A_LOOP 5000

	OUTB(0x00, 0xE807);
	OUTB(0xEF, 0xE803);

	OUTB(0xFF, 0xE800);

	for (i = 0; i < ASUSP5A_LOOP; i++) {
		OUTB(0xE1, 0xFF);
		if (INB(0xE800) & 0x04)
			break;
	}

	if (i == ASUSP5A_LOOP) {
		printf("%s: Unable to contact device.\n", name);
		return -1;
	}

	OUTB(0x20, 0xE801);
	OUTB(0x20, 0xE1);

	OUTB(0xFF, 0xE802);

	for (i = 0; i < ASUSP5A_LOOP; i++) {
		tmp = INB(0xE800);
		if (tmp & 0x70)
			break;
	}

	if ((i == ASUSP5A_LOOP) || !(tmp & 0x10)) {
		printf("%s: failed to read device.\n", name);
		return -1;
	}

	tmp = INB(0xE804);
	tmp &= ~0x02;

	OUTB(0x00, 0xE807);
	OUTB(0xEE, 0xE803);

	OUTB(tmp, 0xE804);

	OUTB(0xFF, 0xE800);
	OUTB(0xE1, 0xFF);

	OUTB(0x20, 0xE801);
	OUTB(0x20, 0xE1);

	OUTB(0xFF, 0xE802);

	for (i = 0; i < ASUSP5A_LOOP; i++) {
		tmp = INB(0xE800);
		if (tmp & 0x70)
			break;
	}

	if ((i == ASUSP5A_LOOP) || !(tmp & 0x10)) {
		printf("%s: failed to write to device.\n", name);
		return -1;
	}

	return 0;
}

static int board_ibm_x3455(const char *name)
{
	/* Set GPIO lines in the Broadcom HT-1000 southbridge. */
	/* It's not a Super I/O but it uses the same index/data port method. */
	sio_mask(0xcd6, 0x45, 0x20, 0x20);

	return 0;
}

/**
 * Suited for Shuttle FN25 (SN25P): AMD S939 + Nvidia CK804 (nForce4).
 */
static int board_shuttle_fn25(const char *name)
{
	struct pci_dev *dev;

	dev = pci_dev_find(0x10DE, 0x0050);	/* NVIDIA CK804 ISA Bridge. */
	if (!dev) {
		fprintf(stderr,
			"\nERROR: NVIDIA nForce4 ISA bridge not found.\n");
		return -1;
	}

	/* one of those bits seems to be connected to TBL#, but -ENOINFO. */
	pci_write_byte(dev, 0x92, 0);

	return 0;
}

/**
 * Very similar to AMD 8111 IO Hub.
 */
static int nvidia_mcp_gpio_set(int gpio, int raise)
{
	struct pci_dev *dev;
	uint16_t base;
	uint8_t tmp;

	if ((gpio < 0) || (gpio > 31)) {
		fprintf(stderr, "\nERROR: unsupported GPIO: %d.\n", gpio);
		return -1;
	}

	dev = pci_dev_find_vendorclass(0x10DE, 0x0C05);
	switch (dev->device_id) {
	case 0x0030: /* CK804 */
	case 0x0050: /* MCP04 */
	case 0x0060: /* MCP2 */
		break;
	default:
		fprintf(stderr, "\nERROR: no nVidia SMBus controller found.\n");
		return -1;
	}

	base = pci_read_long(dev, 0x64) & 0x0000FF00; /* System control area */
	base += 0xC0;

	tmp = INB(base + gpio);
	tmp &= ~0x0F; /* null lower nibble */
	tmp |= 0x04; /* gpio -> output. */
	if (raise)
		tmp |= 0x01;
	OUTB(tmp, base + gpio);

	return 0;
}

/**
 * Suited for ASUS P5ND2-SLI Deluxe: LGA775 + nForce4 SLI + MCP04.
 */
static int nvidia_mcp_gpio10_raise(const char *name)
{
	return nvidia_mcp_gpio_set(0x10, 1);
}

/**
 * Suited for the Gigabyte GA-K8N-SLI: CK804 southbridge.
 */
static int nvidia_mcp_gpio21_raise(const char *name)
{
	return nvidia_mcp_gpio_set(0x21, 0x01);
}

/**
 * Suited for EPoX EP-8RDA3+: Socket A + nForce2 Ultra 400 + MCP2.
 */
static int nvidia_mcp_gpio31_raise(const char *name)
{
	return nvidia_mcp_gpio_set(0x31, 0x01);
}

static int board_hp_dl145_g3_enable(const char *name)
{
	/* Set GPIO lines in the Broadcom HT-1000 southbridge. */
	/* GPIO 0 reg from PM regs */
	/* Set GPIO 2 and 5 high, connected to flash WP# and TBL# pins. */
	/* It's not a Super I/O but it uses the same index/data port method. */
	sio_mask(0xcd6, 0x44, 0x24, 0x24);

	return 0;
}

/**
 * Suited for EPoX EP-BX3, and maybe some other Intel 440BX based boards.
 */
static int board_epox_ep_bx3(const char *name)
{
	uint8_t tmp;

	/* Raise GPIO22. */
	tmp = INB(0x4036);
	OUTB(tmp, 0xEB);

	tmp |= 0x40;

	OUTB(tmp, 0x4036);
	OUTB(tmp, 0xEB);

	return 0;
}

/**
 * Suited for Artec Group DBE61 and DBE62.
 */
static int board_artecgroup_dbe6x(const char *name)
{
#define DBE6x_MSR_DIVIL_BALL_OPTS	0x51400015
#define DBE6x_PRI_BOOT_LOC_SHIFT	(2)
#define DBE6x_BOOT_OP_LATCHED_SHIFT	(8)
#define DBE6x_SEC_BOOT_LOC_SHIFT	(10)
#define DBE6x_PRI_BOOT_LOC		(3 << DBE6x_PRI_BOOT_LOC_SHIFT)
#define DBE6x_BOOT_OP_LATCHED		(3 << DBE6x_BOOT_OP_LATCHED_SHIFT)
#define DBE6x_SEC_BOOT_LOC		(3 << DBE6x_SEC_BOOT_LOC_SHIFT)
#define DBE6x_BOOT_LOC_FLASH		(2)
#define DBE6x_BOOT_LOC_FWHUB		(3)

	msr_t msr;
	unsigned long boot_loc;

	/* Geode only has a single core */
	if (setup_cpu_msr(0))
		return -1;

	msr = rdmsr(DBE6x_MSR_DIVIL_BALL_OPTS);

	if ((msr.lo & (DBE6x_BOOT_OP_LATCHED)) ==
	    (DBE6x_BOOT_LOC_FWHUB << DBE6x_BOOT_OP_LATCHED_SHIFT))
		boot_loc = DBE6x_BOOT_LOC_FWHUB;
	else
		boot_loc = DBE6x_BOOT_LOC_FLASH;

	msr.lo &= ~(DBE6x_PRI_BOOT_LOC | DBE6x_SEC_BOOT_LOC);
	msr.lo |= ((boot_loc << DBE6x_PRI_BOOT_LOC_SHIFT) |
		   (boot_loc << DBE6x_SEC_BOOT_LOC_SHIFT));

	wrmsr(DBE6x_MSR_DIVIL_BALL_OPTS, msr);

	cleanup_cpu_msr();

	return 0;
}

/**
 * Set a GPIO line on a given intel ICH LPC controller.
 */
static int intel_ich_gpio_set(int gpio, int raise)
{
	/* table mapping the different intel ICH LPC chipsets. */
	static struct {
		uint16_t id;
		uint8_t base_reg;
		uint32_t bank0;
		uint32_t bank1;
		uint32_t bank2;
	} intel_ich_gpio_table[] = {
		{0x2410, 0x58, 0x0FE30000,          0,          0}, /* 82801AA (ICH) */
		{0x2420, 0x58, 0x0FE30000,          0,          0}, /* 82801AB (ICH0) */
		{0x2440, 0x58, 0x1BFF391B,          0,          0}, /* 82801BA (ICH2) */
		{0x244C, 0x58, 0x1A23399B,          0,          0}, /* 82801BAM (ICH2M) */
		{0x2450, 0x58, 0x1BFF0000,          0,          0}, /* 82801E (C-ICH) */
		{0x2480, 0x58, 0x1BFF0000, 0x00000FFF,          0}, /* 82801CA (ICH3-S) */
		{0x248C, 0x58, 0x1A230000, 0x00000FFF,          0}, /* 82801CAM (ICH3-M) */
		{0x24C0, 0x58, 0x1BFF0000, 0x00000FFF,          0}, /* 82801DB/DBL (ICH4/ICH4-L) */
		{0x24CC, 0x58, 0x1A030000, 0x00000FFF,          0}, /* 82801DBM (ICH4-M) */
		{0x24D0, 0x58, 0x1BFF0000, 0x00030305,          0}, /* 82801EB/ER (ICH5/ICH5R) */
		{0x2640, 0x48, 0x1BFF0000, 0x00030307,          0}, /* 82801FB/FR (ICH6/ICH6R) */
		{0x2641, 0x48, 0x1BFF0000, 0x00030307,          0}, /* 82801FBM (ICH6M) */
		{0x27B8, 0x48, 0xFFFFFFFF, 0x000300FF,          0}, /* 82801GB/GR (ICH7 Family) */
		{0x27B9, 0x48, 0xFFEBFFFE, 0x000300FE,          0}, /* 82801GBM (ICH7-M) */
		{0x27BD, 0x48, 0xFFEBFFFE, 0x000300FE,          0}, /* 82801GHM (ICH7-M DH) */
		{0x2810, 0x48, 0xFFFFFFFF, 0x00FF0FFF,          0}, /* 82801HB/HR (ICH8/R) */
		{0x2811, 0x48, 0xFFFFFFFF, 0x00FF0FFF,          0}, /* 82801HBM (ICH8M-E) */
		{0x2812, 0x48, 0xFFFFFFFF, 0x00FF0FFF,          0}, /* 82801HH (ICH8DH) */
		{0x2814, 0x48, 0xFFFFFFFF, 0x00FF0FFF,          0}, /* 82801HO (ICH8DO) */
		{0x2815, 0x48, 0xFFFFFFFF, 0x00FF0FFF,          0}, /* 82801HEM (ICH8M) */
		{0x2912, 0x48, 0xFFFFFFFF, 0x00FFFFFF,          0}, /* 82801IH (ICH9DH) */
		{0x2914, 0x48, 0xFFFFFFFF, 0x00FFFFFF,          0}, /* 82801IO (ICH9DO) */
		{0x2916, 0x48, 0xFFFFFFFF, 0x00FFFFFF,          0}, /* 82801IR (ICH9R) */
		{0x2917, 0x48, 0xFFFFFFFF, 0x00FFFFFF,          0}, /* 82801IEM (ICH9M-E) */
		{0x2918, 0x48, 0xFFFFFFFF, 0x00FFFFFF,          0}, /* 82801IB (ICH9) */
		{0x2919, 0x48, 0xFFFFFFFF, 0x00FFFFFF,          0}, /* 82801IBM (ICH9M) */
		{0x3A14, 0x48, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000100}, /* 82801JDO (ICH10DO) */
		{0x3A16, 0x48, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000100}, /* 82801JIR (ICH10R) */
		{0x3A18, 0x48, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000100}, /* 82801JIB (ICH10) */
		{0x3A1A, 0x48, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000100}, /* 82801JD (ICH10D) */
		{0, 0, 0, 0, 0} /* end marker */
	};

	struct pci_dev *dev;
	uint16_t base;
	uint32_t tmp;
	int i, allowed;

	/* First, look for a known LPC bridge */
	for (dev = pacc->devices; dev; dev = dev->next)
		if ((dev->vendor_id == 0x8086) &&
		    (dev->device_class == 0x0601)) { /* ISA Bridge */
			/* Is this device in our list? */
			for (i = 0; intel_ich_gpio_table[i].id; i++)
				if (dev->device_id == intel_ich_gpio_table[i].id)
					break;

			if (intel_ich_gpio_table[i].id)
				break;
		}

	if (!dev) {
		fprintf(stderr, "\nERROR: No Known Intel LPC Bridge found.\n");
		return -1;
	}

	/* According to the datasheets, all intel ICHs have the gpio bar 5:1
	   strapped to zero. From some mobile ich9 version on, this becomes
	   6:1. The mask below catches all. */
	base = pci_read_word(dev, intel_ich_gpio_table[i].base_reg) & 0xFFC0;

	/* check whether the line is allowed */
	if (gpio < 32)
		allowed = (intel_ich_gpio_table[i].bank0 >> gpio) & 0x01;
	else if (gpio < 64)
		allowed = (intel_ich_gpio_table[i].bank1 >> (gpio - 32)) & 0x01;
	else
		allowed = (intel_ich_gpio_table[i].bank2 >> (gpio - 64)) & 0x01;

	if (!allowed) {
		fprintf(stderr, "\nERROR: This Intel LPC Bridge does not allow"
			" setting GPIO%02d\n", gpio);
		return -1;
	}

	printf("\nIntel ICH LPC Bridge: %sing GPIO%02d.\n",
	       raise ? "Rais" : "Dropp", gpio);

	if (gpio < 32) {
		/* Set line to GPIO */
		tmp = INL(base);
		/* ICH/ICH0 multiplexes 27/28 on the line set. */
		if ((gpio == 28) &&
		    ((dev->device_id == 0x2410) || (dev->device_id == 0x2420)))
			tmp |= 1 << 27;
		else
			tmp |= 1 << gpio;
		OUTL(tmp, base);

		/* As soon as we are talking to ICH8 and above, this register
		   decides whether we can set the gpio or not. */
		if (dev->device_id > 0x2800) {
			tmp = INL(base);
			if (!(tmp & (1 << gpio))) {
				fprintf(stderr, "\nERROR: This Intel LPC Bridge"
					" does not allow setting GPIO%02d\n",
					gpio);
				return -1;
			}
		}

		/* Set GPIO to OUTPUT */
		tmp = INL(base + 0x04);
		tmp &= ~(1 << gpio);
		OUTL(tmp, base + 0x04);

		/* Raise GPIO line */
		tmp = INL(base + 0x0C);
		if (raise)
			tmp |= 1 << gpio;
		else
			tmp &= ~(1 << gpio);
		OUTL(tmp, base + 0x0C);
	} else if (gpio < 64) {
		gpio -= 32;

		/* Set line to GPIO */
		tmp = INL(base + 0x30);
		tmp |= 1 << gpio;
		OUTL(tmp, base + 0x30);

		/* As soon as we are talking to ICH8 and above, this register
		   decides whether we can set the gpio or not. */
		if (dev->device_id > 0x2800) {
			tmp = INL(base + 30);
			if (!(tmp & (1 << gpio))) {
				fprintf(stderr, "\nERROR: This Intel LPC Bridge"
					" does not allow setting GPIO%02d\n",
					gpio + 32);
				return -1;
			}
		}

		/* Set GPIO to OUTPUT */
		tmp = INL(base + 0x34);
		tmp &= ~(1 << gpio);
		OUTL(tmp, base + 0x34);

		/* Raise GPIO line */
		tmp = INL(base + 0x38);
		if (raise)
			tmp |= 1 << gpio;
		else
			tmp &= ~(1 << gpio);
		OUTL(tmp, base + 0x38);
	} else {
		gpio -= 64;

		/* Set line to GPIO */
		tmp = INL(base + 0x40);
		tmp |= 1 << gpio;
		OUTL(tmp, base + 0x40);

		tmp = INL(base + 40);
		if (!(tmp & (1 << gpio))) {
			fprintf(stderr, "\nERROR: This Intel LPC Bridge does "
				"not allow setting GPIO%02d\n", gpio + 64);
			return -1;
		}

		/* Set GPIO to OUTPUT */
		tmp = INL(base + 0x44);
		tmp &= ~(1 << gpio);
		OUTL(tmp, base + 0x44);

		/* Raise GPIO line */
		tmp = INL(base + 0x48);
		if (raise)
			tmp |= 1 << gpio;
		else
			tmp &= ~(1 << gpio);
		OUTL(tmp, base + 0x48);
	}

	return 0;
}

/**
 * Suited for Abit IP35: Intel P35 + ICH9R.
 */
static int intel_ich_gpio16_raise(const char *name)
{
	return intel_ich_gpio_set(16, 1);
}

/**
 * Suited for MSI MS-7046: LGA775 + 915P + ICH6.
 */
static int intel_ich_gpio19_raise(const char *name)
{
	return intel_ich_gpio_set(19, 1);
}

/**
 * Suited for:
 * - Asus P4B266LM (Sony Vaio PCV-RX650): socket478 + 845D + ICH2.
 * - Asus P4P800-E Deluxe: Intel socket478 + 865PE + ICH5R.
 */
static int intel_ich_gpio21_raise(const char *name)
{
	return intel_ich_gpio_set(21, 1);
}

/**
 * Suited for ASUS P4B266: socket478 + intel 845D + ICH2.
 */
static int intel_ich_gpio22_raise(const char *name)
{
	return intel_ich_gpio_set(22, 1);
}

/**
 * Suited for:
 * - Dell Poweredge 1850: Intel PPGA604 + E7520 + ICH5R.
 * - ASRock P4i65GV: Intel Socket478 + 865GV + ICH5R.
 */
static int intel_ich_gpio23_raise(const char *name)
{
	return intel_ich_gpio_set(23, 1);
}

/**
 * Suited for Acorp 6A815EPD: socket 370 + intel 815 + ICH2.
 */
static int board_acorp_6a815epd(const char *name)
{
	int ret;

	/* Lower Blocks Lock -- pin 7 of PLCC32 */
	ret = intel_ich_gpio_set(22, 1);
	if (!ret) /* Top Block Lock -- pin 8 of PLCC32 */
		ret = intel_ich_gpio_set(23, 1);

	return ret;
}

/**
 * Suited for Kontron 986LCD-M: socket478 + 915GM + ICH7R.
 */
static int board_kontron_986lcd_m(const char *name)
{
	int ret;

	ret = intel_ich_gpio_set(34, 1); /* #TBL */
	if (!ret)
		ret = intel_ich_gpio_set(35, 1); /* #WP */

	return ret;
}

/**
 * Suited for:
 *   - Biostar P4M80-M4: VIA P4M800 + VT8237 + IT8705AF
 *   - GIGABYTE GA-7VT600: VIA KT600 + VT8237 + IT8705
 */
static int it8705_rom_write_enable(const char *name)
{
	/* enter IT87xx conf mode */
	enter_conf_mode_ite(0x2e);

	/* select right flash chip */
	sio_mask(0x2e, 0x22, 0x80, 0x80);

	/* bit 3: flash chip write enable
	 * bit 7: map flash chip at 1MB-128K (why though? ignoring this.)
	 */
	sio_mask(0x2e, 0x24, 0x04, 0x04);

	/* exit IT87xx conf mode */
	exit_conf_mode_ite(0x2e);

	return 0;
}

/**
 * Suited for AOpen vKM400Am-S: VIA KM400 + VT8237 + IT8705F.
 */
static int board_aopen_vkm400(const char *name)
{
	struct pci_dev *dev;

	dev = pci_dev_find(0x1106, 0x3227);	/* VT8237 ISA bridge */
	if (!dev) {
		fprintf(stderr, "\nERROR: VT8237 ISA bridge not found.\n");
		return -1;
	}

	vt823x_set_all_writes_to_lpc(dev);

	return it8705_rom_write_enable(name);
}

/**
 * Winbond W83697HF Super I/O + VIA VT8235 southbridge
 *
 * Suited for:
 *   - MSI KT4V and KT4V-L: AMD K7 + VIA KT400 + VT8235
 *   - MSI KT4 Ultra: AMD K7 + VIA KT400 + VT8235
 */
static int board_msi_kt4v(const char *name)
{
	struct pci_dev *dev;

	dev = pci_dev_find(0x1106, 0x3177);	/* VT8235 ISA bridge */
	if (!dev) {
		fprintf(stderr, "\nERROR: VT823x ISA bridge not found.\n");
		return -1;
	}

	vt823x_set_all_writes_to_lpc(dev);

	vt823x_gpio_set(dev, 12, 1);
	w836xx_memw_enable(0x2E);

	return 0;
}

/**
 * Suited for Soyo SY-7VCA: Pro133A + VT82C686.
 */
static int board_soyo_sy_7vca(const char *name)
{
    	struct pci_dev *dev;
	uint32_t base;
	uint8_t tmp;

	/* VT82C686 Power management */
	dev = pci_dev_find(0x1106, 0x3057);
	if (!dev) {
		fprintf(stderr, "\nERROR: VT82C686 PM device not found.\n");
		return -1;
	}

	/* GPO0 output from PM IO base + 0x4C */
	tmp = pci_read_byte(dev, 0x54);
	tmp &= ~0x03;
	pci_write_byte(dev, 0x54, tmp);

	/* PM IO base */
	base = pci_read_long(dev, 0x48) & 0x0000FF00;

	/* Drop GPO0 */
	tmp = INB(base + 0x4C);
	tmp &= ~0x01;
	OUTB(tmp, base + 0x4C);

	return 0;
}

static int it8705f_write_enable(uint8_t port, const char *name)
{
	enter_conf_mode_ite(port);
	sio_mask(port, 0x24, 0x04, 0x04); /* Flash ROM I/F Writes Enable */
	exit_conf_mode_ite(port);

	return 0;
}

/**
 * Suited for:
 *   - Shuttle AK38N: VIA KT333CF + VIA VT8235 + ITE IT8705F
 *   - Elitegroup K7VTA3: VIA Apollo KT266/A/333 + VIA VT8235 + ITE IT8705F
 */
static int it8705f_write_enable_2e(const char *name)
{
	return it8705f_write_enable(0x2e, name);
}

/**
 * Find the runtime registers of an SMSC Super I/O, after verifying its
 * chip ID.
 *
 * Returns the base port of the runtime register block, or 0 on error.
 */
static uint16_t smsc_find_runtime(uint16_t sio_port, uint16_t chip_id,
                                  uint8_t logical_device)
{
	uint16_t rt_port = 0;

	/* Verify the chip ID. */
	OUTB(0x55, sio_port);  /* Enable configuration. */
	if (sio_read(sio_port, 0x20) != chip_id) {
		fprintf(stderr, "\nERROR: SMSC Super I/O not found.\n");
		goto out;
	}

	/* If the runtime block is active, get its address. */
	sio_write(sio_port, 0x07, logical_device);
	if (sio_read(sio_port, 0x30) & 1) {
		rt_port = (sio_read(sio_port, 0x60) << 8)
		          | sio_read(sio_port, 0x61);
	}

	if (rt_port == 0) {
		fprintf(stderr, "\nERROR: "
			"Super I/O runtime interface not available.\n");
	}
out:
	OUTB(0xaa, sio_port);  /* Disable configuration. */
	return rt_port;
}

/**
 * Disable write protection on the Mitac 6513WU.  WP# on the FWH is
 * connected to GP30 on the Super I/O, and TBL# is always high.
 */
static int board_mitac_6513wu(const char *name)
{
	struct pci_dev *dev;
	uint16_t rt_port;
	uint8_t val;

	dev = pci_dev_find(0x8086, 0x2410);	/* Intel 82801AA ISA bridge */
	if (!dev) {
		fprintf(stderr, "\nERROR: Intel 82801AA ISA bridge not found.\n");
		return -1;
	}

	rt_port = smsc_find_runtime(0x4e, 0x54 /* LPC47U33x */, 0xa);
	if (rt_port == 0)
		return -1;

	/* Configure the GPIO pin. */
	val = INB(rt_port + 0x33);  /* GP30 config */
	val &= ~0x87;               /* Output, non-inverted, GPIO, push/pull */
	OUTB(val, rt_port + 0x33);

	/* Disable write protection. */
	val = INB(rt_port + 0x4d);  /* GP3 values */
	val |= 0x01;                /* Set GP30 high. */
	OUTB(val, rt_port + 0x4d);

	return 0;
}

/**
 * Suited for Asus A7V8X: VIA KT400 + VT8235 + IT8703F-A
 */
static int board_asus_a7v8x(const char *name)
{
	uint16_t id, base;
	uint8_t tmp;

	/* find the IT8703F */
	w836xx_ext_enter(0x2E);
	id = (sio_read(0x2E, 0x20) << 8) | sio_read(0x2E, 0x21);
	w836xx_ext_leave(0x2E);

	if (id != 0x8701) {
		fprintf(stderr, "\nERROR: IT8703F SuperIO not found.\n");
		return -1;
	}

	/* Get the GP567 IO base */
	w836xx_ext_enter(0x2E);
	sio_write(0x2E, 0x07, 0x0C);
	base = (sio_read(0x2E, 0x60) << 8) | sio_read(0x2E, 0x61);
	w836xx_ext_leave(0x2E);

	if (!base) {
		fprintf(stderr, "\nERROR: Failed to read IT8703F SuperIO GPIO"
			" Base.\n");
		return -1;
	}

	/* Raise GP51. */
	tmp = INB(base);
	tmp |= 0x02;
	OUTB(tmp, base);

	return 0;
}

/*
 * General routine for raising/dropping GPIO lines on the ITE IT8712F.
 * There is only some limited checking on the port numbers.
 */
static int
it8712f_gpio_set(unsigned int line, int raise)
{
	unsigned int port;
	uint16_t id, base;
	uint8_t tmp;

	port = line / 10;
	port--;
	line %= 10;

	/* Check line */
	if ((port > 4) || /* also catches unsigned -1 */
	    ((port < 4) && (line > 7)) || ((port == 4) && (line > 5))) {
	    fprintf(stderr,
		    "\nERROR: Unsupported IT8712F GPIO Line %02d.\n", line);
	    return -1;
	}

	/* find the IT8712F */
	enter_conf_mode_ite(0x2E);
	id = (sio_read(0x2E, 0x20) << 8) | sio_read(0x2E, 0x21);
	exit_conf_mode_ite(0x2E);

	if (id != 0x8712) {
		fprintf(stderr, "\nERROR: IT8712F SuperIO not found.\n");
		return -1;
	}

	/* Get the GPIO base */
	enter_conf_mode_ite(0x2E);
	sio_write(0x2E, 0x07, 0x07);
	base = (sio_read(0x2E, 0x62) << 8) | sio_read(0x2E, 0x63);
	exit_conf_mode_ite(0x2E);

	if (!base) {
		fprintf(stderr, "\nERROR: Failed to read IT8712F SuperIO GPIO"
			" Base.\n");
		return -1;
	}

	/* set GPIO. */
	tmp = INB(base + port);
	if (raise)
	    tmp |= 1 << line;
	else
	    tmp &= ~(1 << line);
	OUTB(tmp, base + port);

	return 0;
}

/**
 * Suited for Asus A7V600-X: VIA KT600 + VT8237 + IT8712F
 */
static int board_asus_a7v600x(const char *name)
{
	return it8712f_gpio_set(32, 1);
}

/**
 * Below is the list of boards which need a special "board enable" code in
 * flashrom before their ROM chip can be accessed/written to.
 *
 * NOTE: Please add boards that _don't_ need such enables or don't work yet
 *       to the respective tables in print.c. Thanks!
 *
 * We use 2 sets of IDs here, you're free to choose which is which. This
 * is to provide a very high degree of certainty when matching a board on
 * the basis of subsystem/card IDs. As not every vendor handles
 * subsystem/card IDs in a sane manner.
 *
 * Keep the second set NULLed if it should be ignored. Keep the subsystem IDs
 * NULLed if they don't identify the board fully. But please take care to
 * provide an as complete set of pci ids as possible; autodetection is the
 * preferred behaviour and we would like to make sure that matches are unique.
 *
 * The coreboot ids are used two fold. When running with a coreboot firmware,
 * the ids uniquely matches the coreboot board identification string. When a
 * legacy bios is installed and when autodetection is not possible, these ids
 * can be used to identify the board through the -m command line argument.
 *
 * When a board is identified through its coreboot ids (in both cases), the
 * main pci ids are still required to match, as a safeguard.
 */

/* Please keep this list alphabetically ordered by vendor/board name. */
struct board_pciid_enable board_pciid_enables[] = {
	/* first pci-id set [4],          second pci-id set [4],          coreboot id [2],             vendor name    board name            flash enable */
	{0x8086, 0x2926, 0x147b, 0x1084,  0x11ab, 0x4364, 0x147b, 0x1084, NULL,         NULL,          "Abit",        "IP35",               intel_ich_gpio16_raise},
	{0x8086, 0x1130,      0,      0,  0x105a, 0x0d30, 0x105a, 0x4d33, "acorp",      "6a815epd",    "Acorp",       "6A815EPD",           board_acorp_6a815epd},
	{0x8086, 0x24D4, 0x1849, 0x24D0,  0x8086, 0x24D5, 0x1849, 0x9739, NULL,         NULL,          "ASRock",      "P4i65GV",            intel_ich_gpio23_raise},
	{0x1022, 0x746B, 0x1022, 0x36C0,       0,      0,      0,      0, "AGAMI",      "ARUMA",       "agami",       "Aruma",              w83627hf_gpio24_raise_2e},
	{0x1106, 0x3177, 0x17F2, 0x3177,  0x1106, 0x3148, 0x17F2, 0x3148, NULL,         NULL,          "Albatron",    "PM266A",             w836xx_memw_enable_2e},
	{0x1106, 0x3205, 0x1106, 0x3205,  0x10EC, 0x8139, 0xA0A0, 0x0477, NULL,         NULL,          "AOpen",       "vKM400Am-S",         board_aopen_vkm400},
	{0x1022, 0x2090,      0,      0,  0x1022, 0x2080,      0,      0, "artecgroup", "dbe61",       "Artec Group", "DBE61",              board_artecgroup_dbe6x},
	{0x1022, 0x2090,      0,      0,  0x1022, 0x2080,      0,      0, "artecgroup", "dbe62",       "Artec Group", "DBE62",              board_artecgroup_dbe6x},
	{0x1106, 0x3189, 0x1043, 0x807F,  0x1106, 0x3065, 0x1043, 0x80ED, NULL,         NULL,          "ASUS",        "A7V600-X",           board_asus_a7v600x},
	{0x1106, 0x3189, 0x1043, 0x807F,  0x1106, 0x3177, 0x1043, 0x808C, NULL,         NULL,          "ASUS",        "A7V8X",              board_asus_a7v8x},
	{0x1106, 0x3177, 0x1043, 0x80A1,  0x1106, 0x3205, 0x1043, 0x8118, NULL,         NULL,          "ASUS",        "A7V8X-MX SE",        board_asus_a7v8x_mx},
	{0x8086, 0x1a30, 0x1043, 0x8070,  0x8086, 0x244b, 0x1043, 0x8028, NULL,         NULL,          "ASUS",        "P4B266",             intel_ich_gpio22_raise},
	{0x8086, 0x1A30, 0x1043, 0x8025,  0x8086, 0x244B, 0x104D, 0x80F0, NULL,         NULL,          "ASUS",        "P4B266-LM",          intel_ich_gpio21_raise},
	{0x8086, 0x2570, 0x1043, 0x80F2,  0x105A, 0x3373, 0x1043, 0x80F5, NULL,         NULL,          "ASUS",        "P4P800-E Deluxe",    intel_ich_gpio21_raise},
	{0x10B9, 0x1541,      0,      0,  0x10B9, 0x1533,      0,      0, "asus",       "p5a",         "ASUS",        "P5A",                board_asus_p5a},
	{0x10DE, 0x0030, 0x1043, 0x818a,  0x8086, 0x100E, 0x1043, 0x80EE, NULL,         NULL,          "ASUS",        "P5ND2-SLI Deluxe",   nvidia_mcp_gpio10_raise},
	{0x1106, 0x3149, 0x1565, 0x3206,  0x1106, 0x3344, 0x1565, 0x1202, NULL,         NULL,          "Biostar",     "P4M80-M4",           it8705_rom_write_enable},
	{0x8086, 0x3590, 0x1028, 0x016c,  0x1000, 0x0030, 0x1028, 0x016c, NULL,         NULL,          "Dell",        "PowerEdge 1850",     intel_ich_gpio23_raise},
	{0x1106, 0x3038, 0x1019, 0x0996,  0x1106, 0x3177, 0x1019, 0x0996, NULL,         NULL,          "Elitegroup",  "K7VTA3",             it8705f_write_enable_2e},
	{0x1106, 0x3177, 0x1106, 0x3177,  0x1106, 0x3059, 0x1695, 0x3005, NULL,         NULL,          "EPoX",        "EP-8K5A2",           w836xx_memw_enable_2e},
	{0x10EC, 0x8139, 0x1695, 0x9001,  0x11C1, 0x5811, 0x1695, 0x9015, NULL,         NULL,          "EPoX",        "EP-8RDA3+",          nvidia_mcp_gpio31_raise},
	{0x8086, 0x7110,      0,      0,  0x8086, 0x7190,      0,      0, "epox",       "ep-bx3",      "EPoX",        "EP-BX3",             board_epox_ep_bx3},
	{0x1039, 0x0761,      0,      0,       0,      0,      0,      0, "gigabyte",   "2761gxdk",    "GIGABYTE",    "GA-2761GXDK",        it87xx_probe_spi_flash},
	{0x1106, 0x3227, 0x1458, 0x5001,  0x10ec, 0x8139, 0x1458, 0xe000, NULL,         NULL,          "GIGABYTE",    "GA-7VT600",          it8705_rom_write_enable},
	{0x10DE, 0x0050, 0x1458, 0x0C11,  0x10DE, 0x005e, 0x1458, 0x5000, NULL,         NULL,          "GIGABYTE",    "GA-K8N-SLI",         nvidia_mcp_gpio21_raise},
	{0x10de, 0x0360,      0,      0,       0,      0,      0,      0, "gigabyte",   "m57sli",      "GIGABYTE",    "GA-M57SLI-S4",       it87xx_probe_spi_flash},
	{0x10de, 0x03e0,      0,      0,       0,      0,      0,      0, "gigabyte",   "m61p",        "GIGABYTE",    "GA-M61P-S3",         it87xx_probe_spi_flash},
	{0x1002, 0x4398, 0x1458, 0x5004,  0x1002, 0x4391, 0x1458, 0xb000, NULL,         NULL,          "GIGABYTE",    "GA-MA78G-DS3H",      it87xx_probe_spi_flash},
	{0x1002, 0x4398, 0x1458, 0x5004,  0x1002, 0x4391, 0x1458, 0xb002, NULL,         NULL,          "GIGABYTE",    "GA-MA78GM-S2H",      it87xx_probe_spi_flash},
	/* SB600 LPC, RD790 North. Neither are specific to the GA-MA790FX-DQ6. The coreboot ID is here to be able to trigger the board enable more easily. */
	{0x1002, 0x438d, 0x1458, 0x5001,  0x1002, 0x5956, 0x1002, 0x5956, "gigabyte",   "ma790fx-dq6", "GIGABYTE",    "GA-MA790FX-DQ6",     it87xx_probe_spi_flash},
	{0x1166, 0x0223, 0x103c, 0x320d,  0x102b, 0x0522, 0x103c, 0x31fa, "hp",         "dl145_g3",    "HP",          "DL145 G3",           board_hp_dl145_g3_enable},
	{0x1166, 0x0205, 0x1014, 0x0347,       0,      0,      0,      0, "ibm",        "x3455",       "IBM",         "x3455",              board_ibm_x3455},
	{0x1039, 0x5513, 0x8086, 0xd61f,  0x1039, 0x6330, 0x8086, 0xd61f, NULL,         NULL,          "Intel",       "D201GLY",            wbsio_check_for_spi},
	{0x1022, 0x7468,      0,      0,       0,      0,      0,      0, "iwill",      "dk8_htx",     "IWILL",       "DK8-HTX",            w83627hf_gpio24_raise_2e},
	/* Note: There are >= 2 version of the Kontron 986LCD-M/mITX! */
	{0x8086, 0x27b8,      0,      0,       0,      0,      0,      0, "kontron",    "986lcd-m",    "Kontron",     "986LCD-M",           board_kontron_986lcd_m},
	{0x10ec, 0x8168, 0x10ec, 0x8168,  0x104c, 0x8023, 0x104c, 0x8019, "kontron",    "986lcd-m",    "Kontron",     "986LCD-M",           board_kontron_986lcd_m},
	{0x8086, 0x2411, 0x8086, 0x2411,  0x8086, 0x7125, 0x0e11, 0xb165, NULL,         NULL,          "Mitac",       "6513WU",             board_mitac_6513wu},
	{0x13f6, 0x0111, 0x1462, 0x5900,  0x1106, 0x3177, 0x1106,      0, "msi",        "kt4ultra",    "MSI",         "MS-6590 (KT4 Ultra)",board_msi_kt4v},
	{0x1106, 0x3149, 0x1462, 0x7094,  0x10ec, 0x8167, 0x1462, 0x094c, NULL,         NULL,          "MSI",         "MS-6702E (K8T Neo2-F)",w83627thf_gpio4_4_raise_2e},
	{0x1106, 0x0571, 0x1462, 0x7120,  0x1106, 0x3065, 0x1462, 0x7120, NULL,         NULL,          "MSI",         "MS-6712 (KT4V)",     board_msi_kt4v},
	{0x8086, 0x2658, 0x1462, 0x7046,  0x1106, 0x3044, 0x1462, 0x046d, NULL,         NULL,          "MSI",         "MS-7046",            intel_ich_gpio19_raise},
	{0x10de, 0x005e,      0,      0,       0,      0,      0,      0, "msi",        "k8n-neo3",    "MSI",         "MS-7135 (K8N Neo3)", w83627thf_gpio4_4_raise_4e},
	{0x1106, 0x3099,      0,      0,  0x1106, 0x3074,      0,      0, "shuttle",    "ak31",        "Shuttle",     "AK31",               w836xx_memw_enable_2e},
	{0x1106, 0x3104, 0x1297, 0xa238,  0x1106, 0x3059, 0x1297, 0xc063, NULL,         NULL,          "Shuttle",     "AK38N",              it8705f_write_enable_2e},
	{0x10DE, 0x0050, 0x1297, 0x5036,  0x1412, 0x1724, 0x1297, 0x5036, NULL,         NULL,          "Shuttle",     "FN25",               board_shuttle_fn25},
	{0x1106, 0x3038, 0x0925, 0x1234,  0x1106, 0x3058, 0x15DD, 0x7609, NULL,         NULL,          "Soyo",        "SY-7VCA",            board_soyo_sy_7vca},
	{0x8086, 0x1076, 0x8086, 0x1176,  0x1106, 0x3059, 0x10f1, 0x2498, NULL,         NULL,          "Tyan",        "S2498 (Tomcat K7M)", board_asus_a7v8x_mx},
	{0x1106, 0x0314, 0x1106, 0xaa08,  0x1106, 0x3227, 0x1106, 0xAA08, NULL,         NULL,          "VIA",         "EPIA-CN",            board_via_epia_sp},
	{0x1106, 0x3177, 0x1106, 0xAA01,  0x1106, 0x3123, 0x1106, 0xAA01, NULL,         NULL,          "VIA",         "EPIA M/MII/...",     board_via_epia_m},
	{0x1106, 0x0259, 0x1106, 0x3227,  0x1106, 0x3065, 0x1106, 0x3149, "via",        "epia-n",      "VIA",         "EPIA-N/NL",          board_via_epia_n}, /* TODO: remove coreboot ids */
	{0x1106, 0x3227, 0x1106, 0xAA01,  0x1106, 0x0259, 0x1106, 0xAA01, NULL,         NULL,          "VIA",         "EPIA SP",            board_via_epia_sp},
	{0x1106, 0x5337, 0x1458, 0xb003,  0x1106, 0x287e, 0x1106, 0x337e, "via",        "pc3500g",     "VIA",         "PC3500G",            it87xx_probe_spi_flash},

	{     0,      0,      0,      0,       0,      0,      0,      0, NULL,         NULL,          NULL,          NULL,                 NULL}, /* end marker */
};

/**
 * Match boards on coreboot table gathered vendor and part name.
 * Require main PCI IDs to match too as extra safety.
 */
static struct board_pciid_enable *board_match_coreboot_name(const char *vendor,
							    const char *part)
{
	struct board_pciid_enable *board = board_pciid_enables;
	struct board_pciid_enable *partmatch = NULL;

	for (; board->vendor_name; board++) {
		if (vendor && (!board->lb_vendor
			       || strcasecmp(board->lb_vendor, vendor)))
			continue;

		if (!board->lb_part || strcasecmp(board->lb_part, part))
			continue;

		if (!pci_dev_find(board->first_vendor, board->first_device))
			continue;

		if (board->second_vendor &&
		    !pci_dev_find(board->second_vendor, board->second_device))
			continue;

		if (vendor)
			return board;

		if (partmatch) {
			/* a second entry has a matching part name */
			printf("AMBIGUOUS BOARD NAME: %s\n", part);
			printf("At least vendors '%s' and '%s' match.\n",
			       partmatch->lb_vendor, board->lb_vendor);
			printf("Please use the full -m vendor:part syntax.\n");
			return NULL;
		}
		partmatch = board;
	}

	if (partmatch)
		return partmatch;

	if (!partvendor_from_cbtable) {
		/* Only warn if the mainboard type was not gathered from the
		 * coreboot table. If it was, the coreboot implementor is
		 * expected to fix flashrom, too.
		 */
		printf("\nUnknown vendor:board from -m option: %s:%s\n\n",
		       vendor, part);
	}
	return NULL;
}

/**
 * Match boards on PCI IDs and subsystem IDs.
 * Second set of IDs can be main only or missing completely.
 */
static struct board_pciid_enable *board_match_pci_card_ids(void)
{
	struct board_pciid_enable *board = board_pciid_enables;

	for (; board->vendor_name; board++) {
		if (!board->first_card_vendor || !board->first_card_device)
			continue;

		if (!pci_card_find(board->first_vendor, board->first_device,
				   board->first_card_vendor,
				   board->first_card_device))
			continue;

		if (board->second_vendor) {
			if (board->second_card_vendor) {
				if (!pci_card_find(board->second_vendor,
						   board->second_device,
						   board->second_card_vendor,
						   board->second_card_device))
					continue;
			} else {
				if (!pci_dev_find(board->second_vendor,
						  board->second_device))
					continue;
			}
		}

		return board;
	}

	return NULL;
}

int board_flash_enable(const char *vendor, const char *part)
{
	struct board_pciid_enable *board = NULL;
	int ret = 0;

	if (part)
		board = board_match_coreboot_name(vendor, part);

	if (!board)
		board = board_match_pci_card_ids();

	if (board) {
		printf("Disabling flash write protection for board \"%s %s\"... ",
		       board->vendor_name, board->board_name);

		ret = board->enable(board->vendor_name);
		if (ret)
			printf("FAILED!\n");
		else
			printf("OK.\n");
	}

	return ret;
}
