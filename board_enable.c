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
#include "flash.h"

#if defined(__i386__) || defined(__x86_64__)
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

/* Not used yet. */
#if 0
static int enable_flash_decode_superio(void)
{
	int ret;
	uint8_t tmp;

	switch (superio.vendor) {
	case SUPERIO_VENDOR_NONE:
		ret = -1;
		break;
	case SUPERIO_VENDOR_ITE:
		enter_conf_mode_ite(superio.port);
		/* Enable flash mapping. Works for most old ITE style Super I/O. */
		tmp = sio_read(superio.port, 0x24);
		tmp |= 0xfc;
		sio_write(superio.port, 0x24, tmp);
		exit_conf_mode_ite(superio.port);
		ret = 0;
		break;
	default:
		msg_pdbg("Unhandled Super I/O type!\n");
		ret = -1;
		break;
	}
	return ret;
}
#endif

/**
 * SMSC FDC37B787: Raise GPIO50
 */

static int fdc37b787_gpio50_raise(uint16_t port, const char * name)
{
	uint8_t id, val;

	OUTB(0x55, port);	/* enter conf mode */
	id = sio_read(port, 0x20);
	if (id != 0x44) {
		msg_perr("\nERROR: %s: FDC37B787: Wrong ID 0x%02X.\n",
			 name, id);
		OUTB(0xAA, port); /* leave conf mode */
		return -1;
	}

	sio_write(port, 0x07, 0x08);	/* Select Aux I/O subdevice */

	val = sio_read(port, 0xC8);	/* GP50 */
	if ((val & 0x1B) != 0x10)	/* output, no invert, GPIO */
	{
		msg_perr("\nERROR: %s: GPIO50 mode 0x%02X unexpected.\n",
			 name, val);
		OUTB(0xAA, port);
		return -1;
	}

	sio_mask(port, 0xF9, 0x01, 0x01);

	OUTB(0xAA, port);		/* Leave conf mode */
	return 0;
}

/**
 * Suited for Nokia IP530: Intel 440BX + PIIX4 + FDC37B787
 */
static int fdc37b787_gpio50_raise_3f0(const char *name)
{
	return fdc37b787_gpio50_raise(0x3f0, name);
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
		msg_perr("\nERROR: %s: W83627HF: Wrong ID: 0x%02X.\n",
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
		msg_perr("\nERROR: %s: W83627THF: Wrong ID: 0x%02X.\n",
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
 * Suited for:
 *   - EPoX EP-8K5A2: VIA KT333 + VT8235.
 *   - Albatron PM266A Pro: VIA P4M266A + VT8235.
 *   - Shuttle AK31 (all versions): VIA KT266 + VT8233.
 *   - ASUS A7V8X-MX SE and A7V400-MX: AMD K7 + VIA KM400A + VT8235
 *   - Tyan S2498 (Tomcat K7M): AMD Geode NX + VIA KM400 + VT8237.
 */
static int w836xx_memw_enable_2e(const char *name)
{
	w836xx_memw_enable(0x2E);

	return 0;
}

/**
 * Suited for:
 *   - Termtek TK-3370 (rev. 2.5b)
 */
static int w836xx_memw_enable_4e(const char *name)
{
	w836xx_memw_enable(0x4E);

	return 0;
}

/**
 *
 */
static int it8705f_write_enable(uint8_t port, const char *name)
{
	enter_conf_mode_ite(port);
	sio_mask(port, 0x24, 0x04, 0x04); /* Flash ROM I/F Writes Enable */
	exit_conf_mode_ite(port);

	return 0;
}

/**
 * Suited for:
 *  - AOpen vKM400Am-S: VIA KM400 + VT8237 + IT8705F.
 *  - Biostar P4M80-M4: VIA P4M800 + VT8237 + IT8705AF
 *  - Elitegroup K7S6A: SiS745 + ITE IT8705F
 *  - Elitegroup K7VTA3: VIA Apollo KT266/A/333 + VIA VT8235 + ITE IT8705F
 *  - GIGABYTE GA-7VT600: VIA KT600 + VT8237 + IT8705
 *  - Shuttle AK38N: VIA KT333CF + VIA VT8235 + ITE IT8705F
 *
 * The SIS950 Super I/O probably requires the same flash write enable.
 */
static int it8705f_write_enable_2e(const char *name)
{
	return it8705f_write_enable(0x2e, name);
}

static int pc87360_gpio_set(uint8_t gpio, int raise)
{
        static const int bankbase[] = {0, 4, 8, 10, 12};
        int gpio_bank = gpio / 8;
        int gpio_pin = gpio % 8;
        uint16_t baseport;
        uint8_t id, val;

        if (gpio_bank > 4) {
                msg_perr("PC87360: Invalid GPIO %d\n", gpio);
                return -1;
        }

        id = sio_read(0x2E, 0x20);
        if (id != 0xE1) {
                msg_perr("PC87360: unexpected ID %02x\n", id);
                return -1;
        }

        sio_write(0x2E, 0x07, 0x07);		/* Select GPIO device */
        baseport = (sio_read(0x2E, 0x60) << 8) | sio_read(0x2E, 0x61);
        if ((baseport & 0xFFF0) == 0xFFF0 || baseport == 0) {
                msg_perr("PC87360: invalid GPIO base address %04x\n",
                         baseport);
                return -1;
        }
        sio_mask (0x2E, 0x30, 0x01, 0x01);	/* Enable logical device */
        sio_write(0x2E, 0xF0, gpio_bank * 16 + gpio_pin);
        sio_mask (0x2E, 0xF1, 0x01, 0x01);	/* Make pin output */

        val = INB(baseport + bankbase[gpio_bank]);
        if (raise)
                val |= 1 << gpio_pin;
        else
                val &= ~(1 << gpio_pin);
        OUTB(val, baseport + bankbase[gpio_bank]);

        return 0;
}

/**
 * VT823x: Set one of the GPIO pins.
 */
static int via_vt823x_gpio_set(uint8_t gpio, int raise)
{
	struct pci_dev *dev;
	uint16_t base;
	uint8_t val, bit, offset;

	dev = pci_dev_find_vendorclass(0x1106, 0x0601);
	switch (dev->device_id) {
	case 0x3177:	/* VT8235 */
	case 0x3227:	/* VT8237R */
	case 0x3337:	/* VT8237A */
		break;
	default:
		msg_perr("\nERROR: VT823x ISA bridge not found.\n");
		return -1;
	}

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
	} else if (gpio == 5) {
		val = pci_read_byte(dev, 0xE4);
		val |= 0x01;
		pci_write_byte(dev, 0xE4, val);
	} else {
		msg_perr("\nERROR: "
			"VT823x GPIO%02d is not implemented.\n", gpio);
		return -1;
	}

	/* We need the I/O Base Address for this board's flash enable. */
	base = pci_read_word(dev, 0x88) & 0xff80;

	offset = 0x4C + gpio / 8;
	bit = 0x01 << (gpio % 8);

	val = INB(base + offset);
	if (raise)
		val |= bit;
	else
		val &= ~bit;
	OUTB(val, base + offset);

	return 0;
}

/**
 * Suited for ASUS M2V-MX: VIA K8M890 + VT8237A + IT8716F
 */
static int via_vt823x_gpio5_raise(const char *name)
{
	/* On M2V-MX: GPO5 is connected to WP# and TBL#. */
	return via_vt823x_gpio_set(5, 1);
}

/**
 * Suited for VIA EPIA N & NL.
 */
static int via_vt823x_gpio9_raise(const char *name)
{
	return via_vt823x_gpio_set(9, 1);
}

/**
 * Suited for VIA EPIA M and MII, and maybe other CLE266 based EPIAs.
 *
 * We don't need to do this for EPIA M when using coreboot, GPIO15 is never
 * lowered there.
 */
static int via_vt823x_gpio15_raise(const char *name)
{
	return via_vt823x_gpio_set(15, 1);
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
	int ret;

	ret = via_vt823x_gpio_set(12, 1);
	w836xx_memw_enable(0x2E);

	return ret;
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
		msg_perr("%s: Unable to contact device.\n", name);
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
		msg_perr("%s: failed to read device.\n", name);
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
		msg_perr("%s: failed to write to device.\n", name);
		return -1;
	}

	return 0;
}

/*
 * Set GPIO lines in the Broadcom HT-1000 southbridge.
 *
 *  It's not a Super I/O but it uses the same index/data port method.
 */
static int board_hp_dl145_g3_enable(const char *name)
{
	/* GPIO 0 reg from PM regs */
	/* Set GPIO 2 and 5 high, connected to flash WP# and TBL# pins. */
	sio_mask(0xcd6, 0x44, 0x24, 0x24);

	return 0;
}

static int board_ibm_x3455(const char *name)
{
	/* raise gpio13 */
	sio_mask(0xcd6, 0x45, 0x20, 0x20);

	return 0;
}

/**
 * Suited for Shuttle FN25 (SN25P): AMD S939 + NVIDIA CK804 (nForce4).
 */
static int board_shuttle_fn25(const char *name)
{
	struct pci_dev *dev;

	dev = pci_dev_find(0x10DE, 0x0050);	/* NVIDIA CK804 ISA Bridge. */
	if (!dev) {
		msg_perr("\nERROR: NVIDIA nForce4 ISA bridge not found.\n");
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
	uint16_t devclass;
	uint8_t tmp;

	if ((gpio < 0) || (gpio >= 0x40)) {
		msg_perr("\nERROR: unsupported GPIO: %d.\n", gpio);
		return -1;
	}

	/* First, check the ISA Bridge */
	dev = pci_dev_find_vendorclass(0x10DE, 0x0601);
	switch (dev->device_id) {
	case 0x0030: /* CK804 */
	case 0x0050: /* MCP04 */
	case 0x0060: /* MCP2 */
		break;
	case 0x0260: /* MCP51 */
	case 0x0364: /* MCP55 */
		/* find SMBus controller on *this* southbridge */
		/* The infamous Tyan S2915-E has two south bridges; they are
		   easily told apart from each other by the class of the 
		   LPC bridge, but have the same SMBus bridge IDs */
		if (dev->func != 0) {
			msg_perr("MCP LPC bridge at unexpected function"
			         " number %d\n", dev->func);
			return -1;
		}

		dev = pci_get_dev(pacc, dev->domain, dev->bus, dev->dev, 1);
		if (!dev) {
			msg_perr("MCP SMBus controller could not be found\n");
			return -1;
		}
		devclass = pci_read_word(dev, PCI_CLASS_DEVICE);
		if (devclass != 0x0C05) {
			msg_perr("Unexpected device class %04x for SMBus"
			         " controller\n", devclass);
			return -1;
		}
		break;
	default:
		msg_perr("\nERROR: no NVIDIA LPC/SMBus controller found.\n");
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
 * Suited for ASUS A8N-LA: nVidia MCP51.
 * Suited for ASUS M2NBP-VM CSM: NVIDIA MCP51.
 */
static int nvidia_mcp_gpio0_raise(const char *name)
{
	return nvidia_mcp_gpio_set(0x00, 1);
}

/**
 * Suited for Abit KN8 Ultra: nVidia CK804.
 */
static int nvidia_mcp_gpio2_lower(const char *name)
{
	return nvidia_mcp_gpio_set(0x02, 0);
}

/**
 * Suited for MSI K8N Neo4: NVIDIA CK804.
 * Suited for MSI K8N GM2-L: NVIDIA MCP51.
 */
static int nvidia_mcp_gpio2_raise(const char *name)
{
	return nvidia_mcp_gpio_set(0x02, 1);
}


/**
 * Suited for HP xw9400 (Tyan S2915-E OEM): Dual(!) nVidia MCP55.
 *   Notes: a) There are two MCP55 chips, so also two SMBus bridges on that
 *             board. We can't tell the SMBus logical devices apart, but we
 *             can tell the LPC bridge functions apart.
 *             We need to choose the SMBus bridge next to the LPC bridge with
 *             ID 0x364 and the "LPC bridge" class.
 *          b) #TBL is hardwired on that board to a pull-down. It can be
 *             overridden by connecting the two solder points next to F2.
 */
static int nvidia_mcp_gpio5_raise(const char *name)
{
	return nvidia_mcp_gpio_set(0x05, 1);
}

/**
 * Suited for Abit NF7-S: NVIDIA CK804.
 */
static int nvidia_mcp_gpio8_raise(const char *name)
{
	return nvidia_mcp_gpio_set(0x08, 1);
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
 * Helper function to raise/drop a given gpo line on Intel PIIX4{,E,M}.
 */
static int intel_piix4_gpo_set(unsigned int gpo, int raise)
{
	unsigned int gpo_byte, gpo_bit;
	struct pci_dev *dev;
	uint32_t tmp, base;

	dev = pci_dev_find(0x8086, 0x7110);	/* Intel PIIX4 ISA bridge */
	if (!dev) {
		msg_perr("\nERROR: Intel PIIX4 ISA bridge not found.\n");
		return -1;
	}

	/* sanity check */
	if (gpo > 30) {
		msg_perr("\nERROR: Intel PIIX4 has no GPO%d.\n", gpo);
		return -1;
	}

	/* these are dual function pins which are most likely in use already */
	if (((gpo >= 1) && (gpo <= 7)) ||
	    ((gpo >= 9) && (gpo <= 21)) || (gpo == 29)) {
		msg_perr("\nERROR: Unsupported PIIX4 GPO%d.\n", gpo);
		return -1;
	}

	/* dual function that need special enable. */
	if ((gpo >= 22) && (gpo <= 26)) {
		tmp = pci_read_long(dev, 0xB0); /* GENCFG */
		switch (gpo) {
		case 22: /* XBUS: XDIR#/GPO22 */
		case 23: /* XBUS: XOE#/GPO23 */
			tmp |= 1 << 28;
			break;
		case 24: /* RTCSS#/GPO24 */
			tmp |= 1 << 29;
			break;
		case 25: /* RTCALE/GPO25 */
			tmp |= 1 << 30;
			break;
		case 26: /* KBCSS#/GPO26 */
			tmp |= 1 << 31;
			break;
		}
		pci_write_long(dev, 0xB0, tmp);
	}

	/* GPO {0,8,27,28,30} are always available. */

	dev = pci_dev_find(0x8086, 0x7113);	/* Intel PIIX4 PM */
	if (!dev) {
		msg_perr("\nERROR: Intel PIIX4 PM not found.\n");
		return -1;
	}

	/* PM IO base */
	base = pci_read_long(dev, 0x40) & 0x0000FFC0;

	gpo_byte = gpo >> 3;
	gpo_bit = gpo & 7;
	tmp = INB(base + 0x34 + gpo_byte); /* GPO register */
	if (raise)
		tmp |= 0x01 << gpo_bit;
	else
		tmp &= ~(0x01 << gpo_bit);
	OUTB(tmp, base + 0x34 + gpo_byte);

	return 0;
}

/**
 * Suited for EPoX EP-BX3, and maybe some other Intel 440BX based boards.
 */
static int board_epox_ep_bx3(const char *name)
{
	return intel_piix4_gpo_set(22, 1);
}

/**
 * Suited for Intel SE440BX-2
 */
static int intel_piix4_gpo27_lower(const char *name)
{
        return intel_piix4_gpo_set(27, 0);
}

/**
 * Set a GPIO line on a given Intel ICH LPC controller.
 */
static int intel_ich_gpio_set(int gpio, int raise)
{
	/* Table mapping the different Intel ICH LPC chipsets. */
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
	for (dev = pacc->devices; dev; dev = dev->next) {
		uint16_t device_class;
		/* libpci before version 2.2.4 does not store class info. */
		device_class = pci_read_word(dev, PCI_CLASS_DEVICE);
		if ((dev->vendor_id == 0x8086) &&
		    (device_class == 0x0601)) { /* ISA Bridge */
			/* Is this device in our list? */
			for (i = 0; intel_ich_gpio_table[i].id; i++)
				if (dev->device_id == intel_ich_gpio_table[i].id)
					break;

			if (intel_ich_gpio_table[i].id)
				break;
		}
	}

	if (!dev) {
		msg_perr("\nERROR: No Known Intel LPC Bridge found.\n");
		return -1;
	}

	/* According to the datasheets, all Intel ICHs have the GPIO bar 5:1
	   strapped to zero. From some mobile ICH9 version on, this becomes
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
		msg_perr("\nERROR: This Intel LPC Bridge does not allow"
			" setting GPIO%02d\n", gpio);
		return -1;
	}

	msg_pdbg("\nIntel ICH LPC Bridge: %sing GPIO%02d.\n",
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
				msg_perr("\nERROR: This Intel LPC Bridge"
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
				msg_perr("\nERROR: This Intel LPC Bridge"
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
			msg_perr("\nERROR: This Intel LPC Bridge does "
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
 * Suited for Abit IP35 Pro: Intel P35 + ICH9R.
 */
static int intel_ich_gpio16_raise(const char *name)
{
	return intel_ich_gpio_set(16, 1);
}

/**
 * Suited for ASUS A8JM: Intel 945 + ICH7
 */
static int intel_ich_gpio34_raise(const char *name)
{
	return intel_ich_gpio_set(34, 1);
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
 * - ASUS P4B266LM (Sony Vaio PCV-RX650): socket478 + 845D + ICH2.
 * - ASUS P4C800-E Deluxe: socket478 + 875P + ICH5.
 * - ASUS P4P800-E Deluxe: Intel socket478 + 865PE + ICH5R.
 */
static int intel_ich_gpio21_raise(const char *name)
{
	return intel_ich_gpio_set(21, 1);
}

/**
 * Suited for:
 *  - ASUS P4B266: socket478 + Intel 845D + ICH2.
 *  - ASUS P4B533-E: socket478 + 845E + ICH4
 *  - ASUS P4B-MX variant in HP Vectra VL420 SFF: socket478 + 845D + ICH2
 */
static int intel_ich_gpio22_raise(const char *name)
{
	return intel_ich_gpio_set(22, 1);
}

/**
 * Suited for HP Vectra VL400: 815 + ICH + PC87360.
 */

static int board_hp_vl400(const char *name)
{
        int ret;
        ret = intel_ich_gpio_set(25, 1);	/* Master write enable ? */
        if (!ret)
                ret = pc87360_gpio_set(0x09, 1);	/* #WP ? */
        if (!ret)
                ret = pc87360_gpio_set(0x27, 1);	/* #TBL */
        return ret;
}

/**
 * Suited for:
 * - Dell PowerEdge 1850: Intel PPGA604 + E7520 + ICH5R.
 * - ASRock P4i65GV: Intel Socket478 + 865GV + ICH5R.
 */
static int intel_ich_gpio23_raise(const char *name)
{
	return intel_ich_gpio_set(23, 1);
}

/**
 * Suited for IBase MB899: i945GM + ICH7.
 */
static int intel_ich_gpio26_raise(const char *name)
{
	return intel_ich_gpio_set(26, 1);
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
 * Suited for Soyo SY-7VCA: Pro133A + VT82C686.
 */
static int via_apollo_gpo_set(int gpio, int raise)
{
	struct pci_dev *dev;
	uint32_t base;
	uint32_t tmp;

	/* VT82C686 Power management */
	dev = pci_dev_find(0x1106, 0x3057);
	if (!dev) {
		msg_perr("\nERROR: VT82C686 PM device not found.\n");
		return -1;
	}

	msg_pdbg("\nVIA Apollo ACPI: %sing GPIO%02d.\n",
	       raise ? "Rais" : "Dropp", gpio);

	/* select GPO function on multiplexed pins */
	tmp = pci_read_byte(dev, 0x54);
	switch(gpio)
	{
		case 0:
			tmp &= ~0x03;
			break;
		case 1:
			tmp |= 0x04;
			break;
		case 2:
			tmp |= 0x08;
			break;
		case 3:
			tmp |= 0x10;
			break;
	}
	pci_write_byte(dev, 0x54, tmp);

	/* PM IO base */
	base = pci_read_long(dev, 0x48) & 0x0000FF00;

	/* Drop GPO0 */
	tmp = INL(base + 0x4C);
	if (raise)
		tmp |= 1U << gpio;
	else
		tmp &= ~(1U << gpio);
	OUTL(tmp, base + 0x4C);

	return 0;
}

/**
 * Suited for Abit VT6X4: Pro133x + VT82C686A
 */
static int via_apollo_gpo4_lower(const char *name)
{
	return via_apollo_gpo_set(4, 0);
}

/**
 * Suited for Soyo SY-7VCA: Pro133A + VT82C686.
 */
static int via_apollo_gpo0_lower(const char *name)
{
	return via_apollo_gpo_set(0, 0);
}

/**
 * Enable some GPIO pin on SiS southbridge.
 * Suited for MSI 651M-L: SiS651 / SiS962
 */
static int board_msi_651ml(const char *name)
{
    	struct pci_dev *dev;
	uint16_t base, temp;

	dev = pci_dev_find(0x1039, 0x0962);
	if (!dev) {
		msg_perr("Expected south bridge not found\n");
		return 1;
	}

	/* Registers 68 and 64 seem like bitmaps */
	base = pci_read_word(dev, 0x74);
	temp = INW(base + 0x68);
	temp &= ~(1 << 0);		/* Make pin output? */
	OUTW(temp, base + 0x68);

	temp = INW(base + 0x64);
	temp |= (1 << 0);		/* Raise output? */
	OUTW(temp, base + 0x64);

	w836xx_memw_enable(0x2E);

	return 0;
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
		msg_perr("\nERROR: SMSC Super I/O not found.\n");
		goto out;
	}

	/* If the runtime block is active, get its address. */
	sio_write(sio_port, 0x07, logical_device);
	if (sio_read(sio_port, 0x30) & 1) {
		rt_port = (sio_read(sio_port, 0x60) << 8)
		          | sio_read(sio_port, 0x61);
	}

	if (rt_port == 0) {
		msg_perr("\nERROR: "
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
		msg_perr("\nERROR: Intel 82801AA ISA bridge not found.\n");
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
 * Suited for ASUS A7V8X: VIA KT400 + VT8235 + IT8703F-A
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
		msg_perr("\nERROR: IT8703F Super I/O not found.\n");
		return -1;
	}

	/* Get the GP567 IO base */
	w836xx_ext_enter(0x2E);
	sio_write(0x2E, 0x07, 0x0C);
	base = (sio_read(0x2E, 0x60) << 8) | sio_read(0x2E, 0x61);
	w836xx_ext_leave(0x2E);

	if (!base) {
		msg_perr("\nERROR: Failed to read IT8703F Super I/O GPIO"
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
static int it8712f_gpio_set(unsigned int line, int raise)
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
	    msg_perr("\nERROR: Unsupported IT8712F GPIO Line %02d.\n", line);
	    return -1;
	}

	/* find the IT8712F */
	enter_conf_mode_ite(0x2E);
	id = (sio_read(0x2E, 0x20) << 8) | sio_read(0x2E, 0x21);
	exit_conf_mode_ite(0x2E);

	if (id != 0x8712) {
		msg_perr("\nERROR: IT8712F Super I/O not found.\n");
		return -1;
	}

	/* Get the GPIO base */
	enter_conf_mode_ite(0x2E);
	sio_write(0x2E, 0x07, 0x07);
	base = (sio_read(0x2E, 0x62) << 8) | sio_read(0x2E, 0x63);
	exit_conf_mode_ite(0x2E);

	if (!base) {
		msg_perr("\nERROR: Failed to read IT8712F Super I/O GPIO"
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
 * Suited for:
 * - ASUS A7V600-X: VIA KT600 + VT8237 + IT8712F
 * - ASUS A7V8X-X: VIA KT400 + VT8235 + IT8712F
 */
static int it8712f_gpio3_1_raise(const char *name)
{
	return it8712f_gpio_set(32, 1);
}

#endif

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
 * NULLed if they don't identify the board fully and if you can't use DMI.
 * But please take care to provide an as complete set of pci ids as possible;
 * autodetection is the preferred behaviour and we would like to make sure that
 * matches are unique.
 *
 * If PCI IDs are not sufficient for board matching, the match can be further
 * constrained by a string that has to be present in the DMI database for
 * the baseboard or the system entry. The pattern is matched by case sensitive
 * substring match, unless it is anchored to the beginning (with a ^ in front)
 * or the end (with a $ at the end). Both anchors may be specified at the
 * same time to match the full field.
 *
 * When a board is matched through DMI, the first and second main PCI IDs
 * and the first subsystem PCI ID have to match as well. If you specify the
 * first subsystem ID as 0x0:0x0, the DMI matching code expects that the
 * subsystem ID of that device is indeed zero.
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

	/* first pci-id set [4],          second pci-id set [4],          dmi identifier coreboot id [2],             vendor name    board name       max_rom_...  OK? flash enable */
#if defined(__i386__) || defined(__x86_64__)
	{0x10DE, 0x0547, 0x147B, 0x1C2F,  0x10DE, 0x0548, 0x147B, 0x1C2F, NULL,          NULL,         NULL,          "Abit",        "AN-M2",                 0,   NT, nvidia_mcp_gpio2_raise},
	{0x8086, 0x2926, 0x147b, 0x1084,  0x11ab, 0x4364, 0x147b, 0x1084, NULL,          NULL,         NULL,          "Abit",        "IP35",                  0,   OK, intel_ich_gpio16_raise},
	{0x8086, 0x2930, 0x147b, 0x1083,  0x10ec, 0x8167, 0x147b, 0x1083, NULL,          NULL,         NULL,          "Abit",        "IP35 Pro",              0,   OK, intel_ich_gpio16_raise},
	{0x10de, 0x0050, 0x147b, 0x1c1a,       0,      0,      0,      0, NULL,          NULL,         NULL,          "Abit",        "KN8 Ultra",             0,   NT, nvidia_mcp_gpio2_lower},
	{0x10de, 0x01e0, 0x147b, 0x1c00,  0x10de, 0x0060, 0x147B, 0x1c00, NULL,          NULL,         NULL,          "Abit",        "NF7-S",                 0,   OK, nvidia_mcp_gpio8_raise},
	{0x1106, 0x0691,      0,      0,  0x1106, 0x3057,      0,      0, NULL,          "abit",       "vt6x4",       "Abit",        "VT6X4",                 0,   OK, via_apollo_gpo4_lower},
	{0x105a, 0x0d30, 0x105a, 0x4d33,  0x8086, 0x1130, 0x8086,      0, NULL,          NULL,         NULL,          "Acorp",       "6A815EPD",              0,   OK, board_acorp_6a815epd},
	{0x1022, 0x746B,      0,      0,       0,      0,      0,      0, NULL,          "AGAMI",      "ARUMA",       "agami",       "Aruma",                 0,   OK, w83627hf_gpio24_raise_2e},
	{0x1106, 0x3177, 0x17F2, 0x3177,  0x1106, 0x3148, 0x17F2, 0x3148, NULL,          NULL,         NULL,          "Albatron",    "PM266A Pro",            0,   OK, w836xx_memw_enable_2e},
	{0x1106, 0x3205, 0x1106, 0x3205,  0x10EC, 0x8139, 0xA0A0, 0x0477, NULL,          NULL,         NULL,          "AOpen",       "vKM400Am-S",            0,   OK, it8705f_write_enable_2e},
	{0x1022, 0x2090,      0,      0,  0x1022, 0x2080,      0,      0, NULL,          "artecgroup", "dbe61",       "Artec Group", "DBE61",                 0,   OK, board_artecgroup_dbe6x},
	{0x1022, 0x2090,      0,      0,  0x1022, 0x2080,      0,      0, NULL,          "artecgroup", "dbe62",       "Artec Group", "DBE62",                 0,   OK, board_artecgroup_dbe6x},
	{0x8086, 0x24D4, 0x1849, 0x24D0,  0x8086, 0x24D5, 0x1849, 0x9739, NULL,          NULL,         NULL,          "ASRock",      "P4i65GV",               0,   OK, intel_ich_gpio23_raise},
	{0x1106, 0x3189, 0x1043, 0x807F,  0x1106, 0x3065, 0x1043, 0x80ED, NULL,          NULL,         NULL,          "ASUS",        "A7V600-X",              0,   OK, it8712f_gpio3_1_raise},
	{0x1106, 0x3177, 0x1043, 0x80A1,  0x1106, 0x3205, 0x1043, 0x8118, NULL,          NULL,         NULL,          "ASUS",        "A7V8X-MX SE",           0,   OK, w836xx_memw_enable_2e},
	{0x1106, 0x3189, 0x1043, 0x807F,  0x1106, 0x3177, 0x1043, 0x808C, NULL,          NULL,         NULL,          "ASUS",        "A7V8X",                 0,   OK, board_asus_a7v8x},
	{0x1106, 0x3189, 0x1043, 0x807F,  0x1106, 0x3177, 0x1043, 0x80A1, NULL,          NULL,         NULL,          "ASUS",        "A7V8X-X",               0,   OK, it8712f_gpio3_1_raise},
	{0x8086, 0x27A0, 0x1043, 0x1287,  0x8086, 0x27DF, 0x1043, 0x1287, "^A8J",        NULL,         NULL,          "ASUS",        "A8JM",                  0,   NT, intel_ich_gpio34_raise},
	{0x10DE, 0x0260, 0x103c, 0x2a3e,  0x10DE, 0x0264, 0x103c, 0x2a3e, "NAGAMI",      NULL,         NULL,          "ASUS",        "A8N-LA",                0,   NT, nvidia_mcp_gpio0_raise},
	{0x10DE, 0x005E, 0x1043, 0x815A,  0x10DE, 0x0054, 0x1043, 0x815A, NULL,          NULL,         NULL,          "ASUS",        "A8N",                   0,   NT, board_shuttle_fn25},
	{0x10DE, 0x0264, 0x1043, 0x81C0,  0x10DE, 0x0260, 0x1043, 0x81C0, NULL,          NULL,         NULL,          "ASUS",        "M2NBP-VM CSM",          0,   OK, nvidia_mcp_gpio0_raise},
	{0x1106, 0x1336, 0x1043, 0x80ed,  0x1106, 0x3288, 0x1043, 0x8249, NULL,          NULL,         NULL,          "ASUS",        "M2V-MX",                0,   OK, via_vt823x_gpio5_raise},
	{0x8086, 0x1A30, 0x1043, 0x8025,  0x8086, 0x244B, 0x104D, 0x80F0, NULL,          NULL,         NULL,          "ASUS",        "P4B266-LM",             0,   OK, intel_ich_gpio21_raise},
	{0x8086, 0x1a30, 0x1043, 0x8070,  0x8086, 0x244b, 0x1043, 0x8028, NULL,          NULL,         NULL,          "ASUS",        "P4B266",                0,   OK, intel_ich_gpio22_raise},
	{0x8086, 0x1A30, 0x1043, 0x8088,  0x8086, 0x24C3, 0x1043, 0x8089, NULL,          NULL,         NULL,          "ASUS",        "P4B533-E",              0,   NT, intel_ich_gpio22_raise},
	{0x8086, 0x24D3, 0x1043, 0x80A6,  0x8086, 0x2578, 0x1043, 0x80F6, NULL,          NULL,         NULL,          "ASUS",        "P4C800-E Deluxe",       0,   OK, intel_ich_gpio21_raise},
	{0x8086, 0x2570, 0x1043, 0x80F2,  0x105A, 0x3373, 0x1043, 0x80F5, NULL,          NULL,         NULL,          "ASUS",        "P4P800-E Deluxe",       0,   OK, intel_ich_gpio21_raise},
	{0x10B9, 0x1541,      0,      0,  0x10B9, 0x1533,      0,      0, "^P5A$",       "asus",       "p5a",         "ASUS",        "P5A",                   0,   OK, board_asus_p5a},
	{0x10DE, 0x0030, 0x1043, 0x818a,  0x8086, 0x100E, 0x1043, 0x80EE, NULL,          NULL,         NULL,          "ASUS",        "P5ND2-SLI Deluxe",      0,   OK, nvidia_mcp_gpio10_raise},
	{0x1106, 0x3149, 0x1565, 0x3206,  0x1106, 0x3344, 0x1565, 0x1202, NULL,          NULL,         NULL,          "Biostar",     "P4M80-M4",              0,   OK, it8705f_write_enable_2e},
	{0x8086, 0x3590, 0x1028, 0x016c,  0x1000, 0x0030, 0x1028, 0x016c, NULL,          NULL,         NULL,          "Dell",        "PowerEdge 1850",        0,   OK, intel_ich_gpio23_raise},
	{0x1039, 0x5513, 0x1019, 0x0A41,  0x1039, 0x0018,      0,      0, NULL,          NULL,         NULL,          "Elitegroup",  "K7S6A",                 0,   OK, it8705f_write_enable_2e},
	{0x1106, 0x3038, 0x1019, 0x0996,  0x1106, 0x3177, 0x1019, 0x0996, NULL,          NULL,         NULL,          "Elitegroup",  "K7VTA3",                256, OK, it8705f_write_enable_2e},
	{0x1106, 0x3177, 0x1106, 0x3177,  0x1106, 0x3059, 0x1695, 0x3005, NULL,          NULL,         NULL,          "EPoX",        "EP-8K5A2",              0,   OK, w836xx_memw_enable_2e},
	{0x10EC, 0x8139, 0x1695, 0x9001,  0x11C1, 0x5811, 0x1695, 0x9015, NULL,          NULL,         NULL,          "EPoX",        "EP-8RDA3+",             0,   OK, nvidia_mcp_gpio31_raise},
	{0x8086, 0x7110,      0,      0,  0x8086, 0x7190,      0,      0, NULL,          "epox",       "ep-bx3",      "EPoX",        "EP-BX3",                0,   OK, board_epox_ep_bx3},
	{0x1106, 0x3227, 0x1458, 0x5001,  0x10ec, 0x8139, 0x1458, 0xe000, NULL,          NULL,         NULL,          "GIGABYTE",    "GA-7VT600",             0,   OK, it8705f_write_enable_2e},
	{0x1106, 0x0686, 0x1106, 0x0686,  0x1106, 0x3058, 0x1458, 0xa000, NULL,          NULL,         NULL,          "GIGABYTE",    "GA-7ZM",                512, OK, NULL},
	{0x10DE, 0x0050, 0x1458, 0x0C11,  0x10DE, 0x005e, 0x1458, 0x5000, NULL,          NULL,         NULL,          "GIGABYTE",    "GA-K8N-SLI",            0,   OK, nvidia_mcp_gpio21_raise},
	{0x1166, 0x0223, 0x103c, 0x320d,  0x102b, 0x0522, 0x103c, 0x31fa, NULL,          "hp",         "dl145_g3",    "HP",          "DL145 G3",              0,   OK, board_hp_dl145_g3_enable},
	{0x8086, 0x2415, 0x103c, 0x1249,  0x10b7, 0x9200, 0x103c, 0x1246, NULL,          NULL,         NULL,          "HP",          "Vectra VL400",          0,   OK, board_hp_vl400}, 
	{0x8086, 0x1a30, 0x103c, 0x1a30,  0x8086, 0x2443, 0x103c, 0x2440, "^VL420$",     NULL,         NULL,          "HP",          "VL420 SFF",             0,   OK, intel_ich_gpio22_raise},
	{0x10de, 0x0369, 0x103c, 0x12fe,  0x10de, 0x0364, 0x103c, 0x12fe, NULL,          NULL,         NULL,          "HP",          "xw9400",                0,   OK, nvidia_mcp_gpio5_raise},
	{0x8086, 0x27A0,      0,      0,  0x8086, 0x27B9,      0,      0, NULL,          "ibase",      "mb899",       "iBASE",       "MB899",                 0,   NT, intel_ich_gpio26_raise},
	{0x1166, 0x0205, 0x1014, 0x0347,  0x1002, 0x515E, 0x1014, 0x0325, NULL,          NULL,         NULL,          "IBM",         "x3455",                 0,   OK, board_ibm_x3455},
	{0x1039, 0x5513, 0x8086, 0xd61f,  0x1039, 0x6330, 0x8086, 0xd61f, NULL,          NULL,         NULL,          "Intel",       "D201GLY",               0,   OK, wbsio_check_for_spi},
	{0x8086, 0x7190,      0,      0,  0x8086, 0x7110,      0,      0, "^SE440BX-2$", NULL,         NULL,          "Intel",       "SE440BX-2",             0,   NT, intel_piix4_gpo27_lower},
	{0x1022, 0x7468,      0,      0,       0,      0,      0,      0, NULL,          "iwill",      "dk8_htx",     "IWILL",       "DK8-HTX",               0,   OK, w83627hf_gpio24_raise_2e},
	{0x8086, 0x27A0, 0x8086, 0x27a0,  0x8086, 0x27b8, 0x8086, 0x27b8, NULL,          "kontron",    "986lcd-m",    "Kontron",     "986LCD-M",              0,   OK, board_kontron_986lcd_m},
	{0x8086, 0x2411, 0x8086, 0x2411,  0x8086, 0x7125, 0x0e11, 0xb165, NULL,          NULL,         NULL,          "Mitac",       "6513WU",                0,   OK, board_mitac_6513wu},
	{0x10DE, 0x005E, 0x1462, 0x7125,  0x10DE, 0x0052, 0x1462, 0x7125, NULL,          NULL,         NULL,          "MSI",         "K8N Neo4-F",            0,   OK, nvidia_mcp_gpio2_raise},
	{0x13f6, 0x0111, 0x1462, 0x5900,  0x1106, 0x3177, 0x1106,      0, NULL,          NULL,         NULL,          "MSI",         "MS-6590 (KT4 Ultra)",   0,   OK, board_msi_kt4v},
	{0x1106, 0x3149, 0x1462, 0x7094,  0x10ec, 0x8167, 0x1462, 0x094c, NULL,          NULL,         NULL,          "MSI",         "MS-6702E (K8T Neo2-F)", 0,   OK, w83627thf_gpio4_4_raise_2e},
	{0x1106, 0x0571, 0x1462, 0x7120,  0x1106, 0x3065, 0x1462, 0x7120, NULL,          NULL,         NULL,          "MSI",         "MS-6712 (KT4V)",        0,   OK, board_msi_kt4v},
	{0x1039, 0x7012, 0x1462, 0x0050,  0x1039, 0x6325, 0x1462, 0x0058, NULL,          NULL,         NULL,          "MSI",         "MS-7005 (651M-L)",      0,   OK, board_msi_651ml},
	{0x8086, 0x2658, 0x1462, 0x7046,  0x1106, 0x3044, 0x1462, 0x046d, NULL,          NULL,         NULL,          "MSI",         "MS-7046",               0,   OK, intel_ich_gpio19_raise},
	{0x10DE, 0x005E, 0x1462, 0x7135,  0x10DE, 0x0050, 0x1462, 0x7135, NULL,          "msi",        "k8n-neo3",    "MSI",         "MS-7135 (K8N Neo3)",    0,   OK, w83627thf_gpio4_4_raise_4e},
	{0x10DE, 0x0270, 0x1462, 0x7207,  0x10DE, 0x0264, 0x1462, 0x7207, NULL,          NULL,         NULL,          "MSI",         "MS-7207 (K8N GM2-L)",   0,   NT, nvidia_mcp_gpio2_raise},
	{0x1011, 0x0019, 0xaa55, 0xaa55,  0x8086, 0x7190,      0,      0, NULL,          NULL,         NULL,          "Nokia",       "IP530",                 0,   OK, fdc37b787_gpio50_raise_3f0},
	{0x1106, 0x3099,      0,      0,  0x1106, 0x3074,      0,      0, NULL,          "shuttle",    "ak31",        "Shuttle",     "AK31",                  0,   OK, w836xx_memw_enable_2e},
	{0x1106, 0x3104, 0x1297, 0xa238,  0x1106, 0x3059, 0x1297, 0xc063, NULL,          NULL,         NULL,          "Shuttle",     "AK38N",                 256, OK, it8705f_write_enable_2e},
	{0x10DE, 0x0050, 0x1297, 0x5036,  0x1412, 0x1724, 0x1297, 0x5036, NULL,          NULL,         NULL,          "Shuttle",     "FN25",                  0,   OK, board_shuttle_fn25},
	{0x1106, 0x3038, 0x0925, 0x1234,  0x1106, 0x3058, 0x15DD, 0x7609, NULL,          NULL,         NULL,          "Soyo",        "SY-7VCA",               0,   OK, via_apollo_gpo0_lower},
	{0x1106, 0x3038, 0x0925, 0x1234,  0x1106, 0x0596, 0x1106,      0, NULL,          NULL,         NULL,          "Tekram",      "P6Pro-A5",              256, OK, NULL},
	{0x1106, 0x3123, 0x1106, 0x3123,  0x1106, 0x3059, 0x1106, 0x4161, NULL,          NULL,         NULL,          "Termtek",     "TK-3370 (Rev:2.5B)",    0,   OK, w836xx_memw_enable_4e},
	{0x8086, 0x1076, 0x8086, 0x1176,  0x1106, 0x3059, 0x10f1, 0x2498, NULL,          NULL,         NULL,          "Tyan",        "S2498 (Tomcat K7M)",    0,   OK, w836xx_memw_enable_2e},
	{0x1106, 0x3177, 0x1106, 0xAA01,  0x1106, 0x3123, 0x1106, 0xAA01, NULL,          NULL,         NULL,          "VIA",         "EPIA M/MII/...",        0,   OK, via_vt823x_gpio15_raise},
	{0x1106, 0x0259, 0x1106, 0x3227,  0x1106, 0x3065, 0x1106, 0x3149, NULL,          NULL,         NULL,          "VIA",         "EPIA-N/NL",             0,   OK, via_vt823x_gpio9_raise},
#endif
	{     0,      0,      0,      0,       0,      0,      0,      0, NULL,          NULL,         NULL,          NULL,          NULL,                    0,   NT, NULL}, /* end marker */
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
			msg_pinfo("AMBIGUOUS BOARD NAME: %s\n", part);
			msg_pinfo("At least vendors '%s' and '%s' match.\n",
			       partmatch->lb_vendor, board->lb_vendor);
			msg_perr("Please use the full -m vendor:part syntax.\n");
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
		msg_perr("\nUnknown vendor:board from -m option: %s:%s\n\n",
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
		if ((!board->first_card_vendor || !board->first_card_device) &&
		      !board->dmi_pattern)
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

		if (board->dmi_pattern) {
			if (!has_dmi_support) {
				msg_perr("WARNING: Can't autodetect %s %s,"
				       " DMI info unavailable.\n",
				       board->vendor_name, board->board_name);
				continue;
			} else {
				if (!dmi_match(board->dmi_pattern))
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

        if (board && board->status == NT) {
                if (!force_boardenable) {
                        msg_pinfo("WARNING: Your mainboard is %s %s, but the mainboard-specific\n"
                               "code has not been tested, and thus will not not be executed by default.\n"
                               "Depending on your hardware environment, erasing, writing or even probing\n"
                               "can fail without running the board specific code.\n\n"
                               "Please see the man page (section PROGRAMMER SPECIFIC INFO, subsection\n"
                               "\"internal programmer\") for details.\n",
                               board->vendor_name, board->board_name);
                        board = NULL;
                } else {
                        msg_pinfo("NOTE: Running an untested board enable procedure.\n"
                               "Please report success/failure to flashrom@flashrom.org.\n");
		}
        }

	if (board) {
		if (board->max_rom_decode_parallel)
			max_rom_decode.parallel =
				board->max_rom_decode_parallel * 1024;

		if (board->enable != NULL) {
			msg_pinfo("Disabling flash write protection for "
			       "board \"%s %s\"... ", board->vendor_name,
			       board->board_name);

			ret = board->enable(board->vendor_name);
			if (ret)
				msg_pinfo("FAILED!\n");
			else
				msg_pinfo("OK.\n");
		}
	}

	return ret;
}
