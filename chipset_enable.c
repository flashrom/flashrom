/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2005-2009 coresystems GmbH
 * Copyright (C) 2006 Uwe Hermann <uwe@hermann-uwe.de>
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
 * Contains the chipset specific flash enables.
 */

#define _LARGEFILE64_SOURCE

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "flash.h"

unsigned long flashbase = 0;

/**
 * flashrom defaults to Parallel/LPC/FWH flash devices. If a known host
 * controller is found, the init routine sets the buses_supported bitfield to
 * contain the supported buses for that controller.
 */

enum chipbustype buses_supported = CHIP_BUSTYPE_NONSPI;

/**
 * Programmers supporting multiple buses can have differing size limits on
 * each bus. Store the limits for each bus in a common struct.
 */
struct decode_sizes max_rom_decode = {
	.parallel	= 0xffffffff,
	.lpc		= 0xffffffff,
	.fwh		= 0xffffffff,
	.spi		= 0xffffffff
};

extern int ichspi_lock;

static int enable_flash_ali_m1533(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;

	/*
	 * ROM Write enable, 0xFFFC0000-0xFFFDFFFF and
	 * 0xFFFE0000-0xFFFFFFFF ROM select enable.
	 */
	tmp = pci_read_byte(dev, 0x47);
	tmp |= 0x46;
	pci_write_byte(dev, 0x47, tmp);

	return 0;
}

static int enable_flash_sis85c496(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;

	tmp = pci_read_byte(dev, 0xd0);
	tmp |= 0xf8;
	pci_write_byte(dev, 0xd0, tmp);

	return 0;
}

static int enable_flash_sis_mapping(struct pci_dev *dev, const char *name)
{
	uint8_t new, newer;

	/* Extended BIOS enable = 1, Lower BIOS Enable = 1 */
	/* This is 0xFFF8000~0xFFFF0000 decoding on SiS 540/630. */
	new = pci_read_byte(dev, 0x40);
	new &= (~0x04); /* No idea why we clear bit 2. */
	new |= 0xb; /* 0x3 for some chipsets, bit 7 seems to be don't care. */
	pci_write_byte(dev, 0x40, new);
	newer = pci_read_byte(dev, 0x40);
	if (newer != new) {
		printf_debug("tried to set register 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x40, new, name);
		printf_debug("Stuck at 0x%x\n", newer);
		return -1;
	}
	return 0;
}

static struct pci_dev *find_southbridge(uint16_t vendor, const char *name)
{
	struct pci_dev *sbdev;
	
	sbdev = pci_dev_find_vendorclass(vendor, 0x0601);
	if (!sbdev)
		sbdev = pci_dev_find_vendorclass(vendor, 0x0680);
	if (!sbdev)
		sbdev = pci_dev_find_vendorclass(vendor, 0x0000);
	if (!sbdev)
		fprintf(stderr, "No southbridge found for %s!\n", name);
	if (sbdev)
		printf_debug("Found southbridge %04x:%04x at %02x:%02x:%01x\n",
			     sbdev->vendor_id, sbdev->device_id,
			     sbdev->bus, sbdev->dev, sbdev->func);
	return sbdev;
}

static int enable_flash_sis501(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;
	int ret = 0;
	struct pci_dev *sbdev;

	sbdev = find_southbridge(dev->vendor_id, name);
	if (!sbdev)
		return -1;

	ret = enable_flash_sis_mapping(sbdev, name);

	tmp = sio_read(0x22, 0x80);
	tmp &= (~0x20);
	tmp |= 0x4;
	sio_write(0x22, 0x80, tmp);

	tmp = sio_read(0x22, 0x70);
	tmp &= (~0x20);
	tmp |= 0x4;
	sio_write(0x22, 0x70, tmp);
	
	return ret;
}

static int enable_flash_sis5511(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;
	int ret = 0;
	struct pci_dev *sbdev;

	sbdev = find_southbridge(dev->vendor_id, name);
	if (!sbdev)
		return -1;

	ret = enable_flash_sis_mapping(sbdev, name);

	tmp = sio_read(0x22, 0x50);
	tmp &= (~0x20);
	tmp |= 0x4;
	sio_write(0x22, 0x50, tmp);

	return ret;
}

static int enable_flash_sis5596(struct pci_dev *dev, const char *name)
{
	int ret;

	ret = enable_flash_sis5511(dev, name);

	/* FIXME: Needs same superio handling as enable_flash_sis630 */
	return ret;
}

static int enable_flash_sis530(struct pci_dev *dev, const char *name)
{
	uint8_t new, newer;
	int ret = 0;
	struct pci_dev *sbdev;

	sbdev = find_southbridge(dev->vendor_id, name);
	if (!sbdev)
		return -1;

	ret = enable_flash_sis_mapping(sbdev, name);

	new = pci_read_byte(sbdev, 0x45);
	new &= (~0x20);
	new |= 0x4;
	pci_write_byte(sbdev, 0x45, new);
	newer = pci_read_byte(dev, 0x45);
	if (newer != new) {
		printf_debug("tried to set register 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x45, new, name);
		printf_debug("Stuck at 0x%x\n", newer);
		ret = -1;
	}

	return ret;
}

static int enable_flash_sis540(struct pci_dev *dev, const char *name)
{
	uint8_t new, newer;
	int ret = 0;
	struct pci_dev *sbdev;

	sbdev = find_southbridge(dev->vendor_id, name);
	if (!sbdev)
		return -1;

	ret = enable_flash_sis_mapping(sbdev, name);

	new = pci_read_byte(sbdev, 0x45);
	new &= (~0x80);
	new |= 0x40;
	pci_write_byte(sbdev, 0x45, new);
	newer = pci_read_byte(dev, 0x45);
	if (newer != new) {
		printf_debug("tried to set register 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x45, new, name);
		printf_debug("Stuck at 0x%x\n", newer);
		ret = -1;
	}

	return ret;
}

static int enable_flash_sis630(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;
	uint16_t siobase;
	int ret;

	ret = enable_flash_sis540(dev, name);

	/* The same thing on SiS 950 Super I/O side... */

	/* First probe for Super I/O on config port 0x2e. */
	siobase = 0x2e;
	enter_conf_mode_ite(siobase);

	if (INB(siobase + 1) != 0x87) {
		/* If that failed, try config port 0x4e. */
		siobase = 0x4e;
		enter_conf_mode_ite(siobase);
		if (INB(siobase + 1) != 0x87) {
			printf("Can not find SuperI/O.\n");
			return -1;
		}
	}

	/* Enable flash mapping. Works for most old ITE style SuperI/O. */
	tmp = sio_read(siobase, 0x24);
	tmp |= 0xfc;
	sio_write(siobase, 0x24, tmp);

	exit_conf_mode_ite(siobase);

	return ret;
}

/* Datasheet:
 *   - Name: 82371AB PCI-TO-ISA / IDE XCELERATOR (PIIX4)
 *   - URL: http://www.intel.com/design/intarch/datashts/290562.htm
 *   - PDF: http://www.intel.com/design/intarch/datashts/29056201.pdf
 *   - Order Number: 290562-001
 */
static int enable_flash_piix4(struct pci_dev *dev, const char *name)
{
	uint16_t old, new;
	uint16_t xbcs = 0x4e;	/* X-Bus Chip Select register. */

	old = pci_read_word(dev, xbcs);

	/* Set bit 9: 1-Meg Extended BIOS Enable (PCI master accesses to
	 *            FFF00000-FFF7FFFF are forwarded to ISA).
	 *            Note: This bit is reserved on PIIX/PIIX3/MPIIX.
	 * Set bit 7: Extended BIOS Enable (PCI master accesses to
	 *            FFF80000-FFFDFFFF are forwarded to ISA).
	 * Set bit 6: Lower BIOS Enable (PCI master, or ISA master accesses to
	 *            the lower 64-Kbyte BIOS block (E0000-EFFFF) at the top
	 *            of 1 Mbyte, or the aliases at the top of 4 Gbyte
	 *            (FFFE0000-FFFEFFFF) result in the generation of BIOSCS#.
	 * Note: Accesses to FFFF0000-FFFFFFFF are always forwarded to ISA.
	 * Set bit 2: BIOSCS# Write Enable (1=enable, 0=disable).
	 */
	if (dev->device_id == 0x122e || dev->device_id == 0x7000
	    || dev->device_id == 0x1234)
		new = old | 0x00c4; /* PIIX/PIIX3/MPIIX: Bit 9 is reserved. */
	else
		new = old | 0x02c4;

	if (new == old)
		return 0;

	pci_write_word(dev, xbcs, new);

	if (pci_read_word(dev, xbcs) != new) {
		printf_debug("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", xbcs, new, name);
		return -1;
	}

	return 0;
}

/*
 * See ie. page 375 of "Intel I/O Controller Hub 7 (ICH7) Family Datasheet"
 * http://download.intel.com/design/chipsets/datashts/30701303.pdf
 */
static int enable_flash_ich(struct pci_dev *dev, const char *name,
			    int bios_cntl)
{
	uint8_t old, new;

	/*
	 * Note: the ICH0-ICH5 BIOS_CNTL register is actually 16 bit wide, but
	 * just treating it as 8 bit wide seems to work fine in practice.
	 */
	old = pci_read_byte(dev, bios_cntl);

	printf_debug("\nBIOS Lock Enable: %sabled, ",
		     (old & (1 << 1)) ? "en" : "dis");
	printf_debug("BIOS Write Enable: %sabled, ",
		     (old & (1 << 0)) ? "en" : "dis");
	printf_debug("BIOS_CNTL is 0x%x\n", old);

	new = old | 1;

	if (new == old)
		return 0;

	pci_write_byte(dev, bios_cntl, new);

	if (pci_read_byte(dev, bios_cntl) != new) {
		printf_debug("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", bios_cntl, new, name);
		return -1;
	}

	return 0;
}

static int enable_flash_ich_4e(struct pci_dev *dev, const char *name)
{
	/*
	 * Note: ICH5 has registers similar to FWH_SEL1, FWH_SEL2 and
	 * FWH_DEC_EN1, but they are called FB_SEL1, FB_SEL2, FB_DEC_EN1 and
	 * FB_DEC_EN2.
	 */
	return enable_flash_ich(dev, name, 0x4e);
}

static int enable_flash_ich_dc(struct pci_dev *dev, const char *name)
{
	uint32_t fwh_conf;
	int i;
	char *idsel = NULL;

	/* Ignore all legacy ranges below 1 MB. */
	/* FWH_SEL1 */
	fwh_conf = pci_read_long(dev, 0xd0);
	for (i = 7; i >= 0; i--)
		printf_debug("\n0x%08x/0x%08x FWH IDSEL: 0x%x",
			     (0x1ff8 + i) * 0x80000,
			     (0x1ff0 + i) * 0x80000,
			     (fwh_conf >> (i * 4)) & 0xf);
	/* FWH_SEL2 */
	fwh_conf = pci_read_word(dev, 0xd4);
	for (i = 3; i >= 0; i--)
		printf_debug("\n0x%08x/0x%08x FWH IDSEL: 0x%x",
			     (0xff4 + i) * 0x100000,
			     (0xff0 + i) * 0x100000,
			     (fwh_conf >> (i * 4)) & 0xf);
	/* FWH_DEC_EN1 */
	fwh_conf = pci_read_word(dev, 0xd8);
	for (i = 7; i >= 0; i--)
		printf_debug("\n0x%08x/0x%08x FWH decode %sabled",
			     (0x1ff8 + i) * 0x80000,
			     (0x1ff0 + i) * 0x80000,
			     (fwh_conf >> (i + 0x8)) & 0x1 ? "en" : "dis");
	for (i = 3; i >= 0; i--)
		printf_debug("\n0x%08x/0x%08x FWH decode %sabled",
			     (0xff4 + i) * 0x100000,
			     (0xff0 + i) * 0x100000,
			     (fwh_conf >> i) & 0x1 ? "en" : "dis");

	if (programmer_param)
		idsel = strstr(programmer_param, "fwh_idsel=");

	if (idsel) {
		idsel += strlen("fwh_idsel=");
		fwh_conf = (uint32_t)strtoul(idsel, NULL, 0);

		/* FIXME: Need to undo this on shutdown. */
		printf("\nSetting IDSEL=0x%x for top 16 MB", fwh_conf);
		pci_write_long(dev, 0xd0, fwh_conf);
		pci_write_word(dev, 0xd4, fwh_conf);
	}

	return enable_flash_ich(dev, name, 0xdc);
}

#define ICH_STRAP_RSVD 0x00
#define ICH_STRAP_SPI  0x01
#define ICH_STRAP_PCI  0x02
#define ICH_STRAP_LPC  0x03

static int enable_flash_vt8237s_spi(struct pci_dev *dev, const char *name)
{
	uint32_t mmio_base;

	mmio_base = (pci_read_long(dev, 0xbc)) << 8;
	printf_debug("MMIO base at = 0x%x\n", mmio_base);
	spibar = physmap("VT8237S MMIO registers", mmio_base, 0x70);

	printf_debug("0x6c: 0x%04x     (CLOCK/DEBUG)\n",
		     mmio_readw(spibar + 0x6c));

	/* Not sure if it speaks all these bus protocols. */
	buses_supported = CHIP_BUSTYPE_LPC | CHIP_BUSTYPE_FWH | CHIP_BUSTYPE_SPI;
	spi_controller = SPI_CONTROLLER_VIA;
	ich_init_opcodes();

	return 0;
}

static int enable_flash_ich_dc_spi(struct pci_dev *dev, const char *name,
				   int ich_generation)
{
	int ret, i;
	uint8_t old, new, bbs, buc;
	uint16_t spibar_offset, tmp2;
	uint32_t tmp, gcs;
	void *rcrb;
	//TODO: These names are incorrect for EP80579. For that, the solution would look like the commented line
	//static const char *straps_names[] = {"SPI", "reserved", "reserved", "LPC" };
	static const char *straps_names[] = { "reserved", "SPI", "PCI", "LPC" };

	/* Enable Flash Writes */
	ret = enable_flash_ich_dc(dev, name);

	/* Get physical address of Root Complex Register Block */
	tmp = pci_read_long(dev, 0xf0) & 0xffffc000;
	printf_debug("\nRoot Complex Register Block address = 0x%x\n", tmp);

	/* Map RCBA to virtual memory */
	rcrb = physmap("ICH RCRB", tmp, 0x4000);

	gcs = mmio_readl(rcrb + 0x3410);
	printf_debug("GCS = 0x%x: ", gcs);
	printf_debug("BIOS Interface Lock-Down: %sabled, ",
		     (gcs & 0x1) ? "en" : "dis");
	bbs = (gcs >> 10) & 0x3;
	printf_debug("BOOT BIOS Straps: 0x%x (%s)\n", bbs, straps_names[bbs]);

	buc = mmio_readb(rcrb + 0x3414);
	printf_debug("Top Swap : %s\n",
		     (buc & 1) ? "enabled (A16 inverted)" : "not enabled");

	/* It seems the ICH7 does not support SPI and LPC chips at the same
	 * time. At least not with our current code. So we prevent searching
	 * on ICH7 when the southbridge is strapped to LPC
	 */

	if (ich_generation == 7 && bbs == ICH_STRAP_LPC) {
		/* Not sure if it speaks LPC as well. */
		buses_supported = CHIP_BUSTYPE_LPC | CHIP_BUSTYPE_FWH;
		/* No further SPI initialization required */
		return ret;
	}

	switch (ich_generation) {
	case 7:
		buses_supported = CHIP_BUSTYPE_SPI;
		spi_controller = SPI_CONTROLLER_ICH7;
		spibar_offset = 0x3020;
		break;
	case 8:
		/* Not sure if it speaks LPC as well. */
		buses_supported = CHIP_BUSTYPE_LPC | CHIP_BUSTYPE_FWH | CHIP_BUSTYPE_SPI;
		spi_controller = SPI_CONTROLLER_ICH9;
		spibar_offset = 0x3020;
		break;
	case 9:
	case 10:
	default:		/* Future version might behave the same */
		/* Not sure if it speaks LPC as well. */
		buses_supported = CHIP_BUSTYPE_LPC | CHIP_BUSTYPE_FWH | CHIP_BUSTYPE_SPI;
		spi_controller = SPI_CONTROLLER_ICH9;
		spibar_offset = 0x3800;
		break;
	}

	/* SPIBAR is at RCRB+0x3020 for ICH[78] and RCRB+0x3800 for ICH9. */
	printf_debug("SPIBAR = 0x%x + 0x%04x\n", tmp, spibar_offset);

	/* Assign Virtual Address */
	spibar = rcrb + spibar_offset;

	switch (spi_controller) {
	case SPI_CONTROLLER_ICH7:
		printf_debug("0x00: 0x%04x     (SPIS)\n",
			     mmio_readw(spibar + 0));
		printf_debug("0x02: 0x%04x     (SPIC)\n",
			     mmio_readw(spibar + 2));
		printf_debug("0x04: 0x%08x (SPIA)\n",
			     mmio_readl(spibar + 4));
		for (i = 0; i < 8; i++) {
			int offs;
			offs = 8 + (i * 8);
			printf_debug("0x%02x: 0x%08x (SPID%d)\n", offs,
				     mmio_readl(spibar + offs), i);
			printf_debug("0x%02x: 0x%08x (SPID%d+4)\n", offs + 4,
				     mmio_readl(spibar + offs + 4), i);
		}
		printf_debug("0x50: 0x%08x (BBAR)\n",
			     mmio_readl(spibar + 0x50));
		printf_debug("0x54: 0x%04x     (PREOP)\n",
			     mmio_readw(spibar + 0x54));
		printf_debug("0x56: 0x%04x     (OPTYPE)\n",
			     mmio_readw(spibar + 0x56));
		printf_debug("0x58: 0x%08x (OPMENU)\n",
			     mmio_readl(spibar + 0x58));
		printf_debug("0x5c: 0x%08x (OPMENU+4)\n",
			     mmio_readl(spibar + 0x5c));
		for (i = 0; i < 4; i++) {
			int offs;
			offs = 0x60 + (i * 4);
			printf_debug("0x%02x: 0x%08x (PBR%d)\n", offs,
				     mmio_readl(spibar + offs), i);
		}
		printf_debug("\n");
		if (mmio_readw(spibar) & (1 << 15)) {
			printf("WARNING: SPI Configuration Lockdown activated.\n");
			ichspi_lock = 1;
		}
		ich_init_opcodes();
		break;
	case SPI_CONTROLLER_ICH9:
		tmp2 = mmio_readw(spibar + 4);
		printf_debug("0x04: 0x%04x (HSFS)\n", tmp2);
		printf_debug("FLOCKDN %i, ", (tmp2 >> 15 & 1));
		printf_debug("FDV %i, ", (tmp2 >> 14) & 1);
		printf_debug("FDOPSS %i, ", (tmp2 >> 13) & 1);
		printf_debug("SCIP %i, ", (tmp2 >> 5) & 1);
		printf_debug("BERASE %i, ", (tmp2 >> 3) & 3);
		printf_debug("AEL %i, ", (tmp2 >> 2) & 1);
		printf_debug("FCERR %i, ", (tmp2 >> 1) & 1);
		printf_debug("FDONE %i\n", (tmp2 >> 0) & 1);

		tmp = mmio_readl(spibar + 0x50);
		printf_debug("0x50: 0x%08x (FRAP)\n", tmp);
		printf_debug("BMWAG %i, ", (tmp >> 24) & 0xff);
		printf_debug("BMRAG %i, ", (tmp >> 16) & 0xff);
		printf_debug("BRWA %i, ", (tmp >> 8) & 0xff);
		printf_debug("BRRA %i\n", (tmp >> 0) & 0xff);

		printf_debug("0x54: 0x%08x (FREG0)\n",
			     mmio_readl(spibar + 0x54));
		printf_debug("0x58: 0x%08x (FREG1)\n",
			     mmio_readl(spibar + 0x58));
		printf_debug("0x5C: 0x%08x (FREG2)\n",
			     mmio_readl(spibar + 0x5C));
		printf_debug("0x60: 0x%08x (FREG3)\n",
			     mmio_readl(spibar + 0x60));
		printf_debug("0x64: 0x%08x (FREG4)\n",
			     mmio_readl(spibar + 0x64));
		printf_debug("0x74: 0x%08x (PR0)\n",
			     mmio_readl(spibar + 0x74));
		printf_debug("0x78: 0x%08x (PR1)\n",
			     mmio_readl(spibar + 0x78));
		printf_debug("0x7C: 0x%08x (PR2)\n",
			     mmio_readl(spibar + 0x7C));
		printf_debug("0x80: 0x%08x (PR3)\n",
			     mmio_readl(spibar + 0x80));
		printf_debug("0x84: 0x%08x (PR4)\n",
			     mmio_readl(spibar + 0x84));
		printf_debug("0x90: 0x%08x (SSFS, SSFC)\n",
			     mmio_readl(spibar + 0x90));
		printf_debug("0x94: 0x%04x     (PREOP)\n",
			     mmio_readw(spibar + 0x94));
		printf_debug("0x96: 0x%04x     (OPTYPE)\n",
			     mmio_readw(spibar + 0x96));
		printf_debug("0x98: 0x%08x (OPMENU)\n",
			     mmio_readl(spibar + 0x98));
		printf_debug("0x9C: 0x%08x (OPMENU+4)\n",
			     mmio_readl(spibar + 0x9C));
		printf_debug("0xA0: 0x%08x (BBAR)\n",
			     mmio_readl(spibar + 0xA0));
		printf_debug("0xB0: 0x%08x (FDOC)\n",
			     mmio_readl(spibar + 0xB0));
		if (tmp2 & (1 << 15)) {
			printf("WARNING: SPI Configuration Lockdown activated.\n");
			ichspi_lock = 1;
		}
		ich_init_opcodes();
		break;
	default:
		/* Nothing */
		break;
	}

	old = pci_read_byte(dev, 0xdc);
	printf_debug("SPI Read Configuration: ");
	new = (old >> 2) & 0x3;
	switch (new) {
	case 0:
	case 1:
	case 2:
		printf_debug("prefetching %sabled, caching %sabled, ",
			     (new & 0x2) ? "en" : "dis",
			     (new & 0x1) ? "dis" : "en");
		break;
	default:
		printf_debug("invalid prefetching/caching settings, ");
		break;
	}

	return ret;
}

static int enable_flash_ich7(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_dc_spi(dev, name, 7);
}

static int enable_flash_ich8(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_dc_spi(dev, name, 8);
}

static int enable_flash_ich9(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_dc_spi(dev, name, 9);
}

static int enable_flash_ich10(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_dc_spi(dev, name, 10);
}

static int enable_flash_vt823x(struct pci_dev *dev, const char *name)
{
	uint8_t val;

	/* enable ROM decode range (1MB) FFC00000 - FFFFFFFF */
	pci_write_byte(dev, 0x41, 0x7f);

	/* ROM write enable */
	val = pci_read_byte(dev, 0x40);
	val |= 0x10;
	pci_write_byte(dev, 0x40, val);

	if (pci_read_byte(dev, 0x40) != val) {
		printf("\nWARNING: Failed to enable flash write on \"%s\"\n",
		       name);
		return -1;
	}

	return 0;
}

static int enable_flash_cs5530(struct pci_dev *dev, const char *name)
{
	uint8_t reg8;

#define DECODE_CONTROL_REG2		0x5b	/* F0 index 0x5b */
#define ROM_AT_LOGIC_CONTROL_REG	0x52	/* F0 index 0x52 */

#define LOWER_ROM_ADDRESS_RANGE		(1 << 0)
#define ROM_WRITE_ENABLE		(1 << 1)
#define UPPER_ROM_ADDRESS_RANGE		(1 << 2)
#define BIOS_ROM_POSITIVE_DECODE	(1 << 5)

	/* Decode 0x000E0000-0x000FFFFF (128 KB), not just 64 KB, and
	 * decode 0xFF000000-0xFFFFFFFF (16 MB), not just 256 KB.
	 * Make the configured ROM areas writable.
	 */
	reg8 = pci_read_byte(dev, ROM_AT_LOGIC_CONTROL_REG);
	reg8 |= LOWER_ROM_ADDRESS_RANGE;
	reg8 |= UPPER_ROM_ADDRESS_RANGE;
	reg8 |= ROM_WRITE_ENABLE;
	pci_write_byte(dev, ROM_AT_LOGIC_CONTROL_REG, reg8);

	/* Set positive decode on ROM. */
	reg8 = pci_read_byte(dev, DECODE_CONTROL_REG2);
	reg8 |= BIOS_ROM_POSITIVE_DECODE;
	pci_write_byte(dev, DECODE_CONTROL_REG2, reg8);

	return 0;
}

/**
 * Geode systems write protect the BIOS via RCONFs (cache settings similar
 * to MTRRs). To unlock, change MSR 0x1808 top byte to 0x22. 
 *
 * Geode systems also write protect the NOR flash chip itself via MSR_NORF_CTL.
 * To enable write to NOR Boot flash for the benefit of systems that have such
 * a setup, raise MSR 0x51400018 WE_CS3 (write enable Boot Flash Chip Select).
 */
static int enable_flash_cs5536(struct pci_dev *dev, const char *name)
{
#define MSR_RCONF_DEFAULT	0x1808
#define MSR_NORF_CTL		0x51400018

	msr_t msr;

	/* Geode only has a single core */
	if (setup_cpu_msr(0))
		return -1;

	msr = rdmsr(MSR_RCONF_DEFAULT);
	if ((msr.hi >> 24) != 0x22) {
		msr.hi &= 0xfbffffff;
		wrmsr(MSR_RCONF_DEFAULT, msr);
	}

	msr = rdmsr(MSR_NORF_CTL);
	/* Raise WE_CS3 bit. */
	msr.lo |= 0x08;
	wrmsr(MSR_NORF_CTL, msr);

	cleanup_cpu_msr();

#undef MSR_RCONF_DEFAULT
#undef MSR_NORF_CTL
	return 0;
}

static int enable_flash_sc1100(struct pci_dev *dev, const char *name)
{
	uint8_t new;

	pci_write_byte(dev, 0x52, 0xee);

	new = pci_read_byte(dev, 0x52);

	if (new != 0xee) {
		printf_debug("tried to set register 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x52, new, name);
		return -1;
	}

	return 0;
}

/* Works for AMD-8111, VIA VT82C586A/B, VIA VT82C686A/B. */
static int enable_flash_amd8111(struct pci_dev *dev, const char *name)
{
	uint8_t old, new;

	/* Enable decoding at 0xffb00000 to 0xffffffff. */
	old = pci_read_byte(dev, 0x43);
	new = old | 0xC0;
	if (new != old) {
		pci_write_byte(dev, 0x43, new);
		if (pci_read_byte(dev, 0x43) != new) {
			printf_debug("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x43, new, name);
		}
	}

	/* Enable 'ROM write' bit. */
	old = pci_read_byte(dev, 0x40);
	new = old | 0x01;
	if (new == old)
		return 0;
	pci_write_byte(dev, 0x40, new);

	if (pci_read_byte(dev, 0x40) != new) {
		printf_debug("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x40, new, name);
		return -1;
	}

	return 0;
}

static int enable_flash_sb600(struct pci_dev *dev, const char *name)
{
	uint32_t tmp, prot;
	uint8_t reg;
	struct pci_dev *smbus_dev;
	int has_spi = 1;

	/* Clear ROM protect 0-3. */
	for (reg = 0x50; reg < 0x60; reg += 4) {
		prot = pci_read_long(dev, reg);
		/* No protection flags for this region?*/
		if ((prot & 0x3) == 0)
			continue;
		printf_debug("SB600 %s%sprotected from %u to %u\n",
			(prot & 0x1) ? "write " : "",
			(prot & 0x2) ? "read " : "",
			(prot & 0xfffffc00),
			(prot & 0xfffffc00) + ((prot & 0x3ff) << 8));
		prot &= 0xfffffffc;
		pci_write_byte(dev, reg, prot);
		prot = pci_read_long(dev, reg);
		if (prot & 0x3)
			printf("SB600 %s%sunprotect failed from %u to %u\n",
				(prot & 0x1) ? "write " : "",
				(prot & 0x2) ? "read " : "",
				(prot & 0xfffffc00),
				(prot & 0xfffffc00) + ((prot & 0x3ff) << 8));
	}

	/* Read SPI_BaseAddr */
	tmp = pci_read_long(dev, 0xa0);
	tmp &= 0xffffffe0;	/* remove bits 4-0 (reserved) */
	printf_debug("SPI base address is at 0x%x\n", tmp);

	/* If the BAR has address 0, it is unlikely SPI is used. */
	if (!tmp)
		has_spi = 0;

	if (has_spi) {
		/* Physical memory has to be mapped at page (4k) boundaries. */
		sb600_spibar = physmap("SB600 SPI registers", tmp & 0xfffff000,
				       0x1000);
		/* The low bits of the SPI base address are used as offset into
		 * the mapped page.
		 */
		sb600_spibar += tmp & 0xfff;

		tmp = pci_read_long(dev, 0xa0);
		printf_debug("AltSpiCSEnable=%i, SpiRomEnable=%i, "
			     "AbortEnable=%i\n", tmp & 0x1, (tmp & 0x2) >> 1,
			     (tmp & 0x4) >> 2);
		tmp = (pci_read_byte(dev, 0xba) & 0x4) >> 2;
		printf_debug("PrefetchEnSPIFromIMC=%i, ", tmp);

		tmp = pci_read_byte(dev, 0xbb);
		printf_debug("PrefetchEnSPIFromHost=%i, SpiOpEnInLpcMode=%i\n",
			     tmp & 0x1, (tmp & 0x20) >> 5);
		tmp = mmio_readl(sb600_spibar);
		printf_debug("SpiArbEnable=%i, SpiAccessMacRomEn=%i, "
			     "SpiHostAccessRomEn=%i, ArbWaitCount=%i, "
			     "SpiBridgeDisable=%i, DropOneClkOnRd=%i\n",
			     (tmp >> 19) & 0x1, (tmp >> 22) & 0x1,
			     (tmp >> 23) & 0x1, (tmp >> 24) & 0x7,
			     (tmp >> 27) & 0x1, (tmp >> 28) & 0x1);
	}

	/* Look for the SMBus device. */
	smbus_dev = pci_dev_find(0x1002, 0x4385);

	if (has_spi && !smbus_dev) {
		fprintf(stderr, "ERROR: SMBus device not found. Not enabling SPI.\n");
		has_spi = 0;
	}
	if (has_spi) {
		/* Note about the bit tests below: If a bit is zero, the GPIO is SPI. */
		/* GPIO11/SPI_DO and GPIO12/SPI_DI status */
		reg = pci_read_byte(smbus_dev, 0xAB);
		reg &= 0xC0;
		printf_debug("GPIO11 used for %s\n", (reg & (1 << 6)) ? "GPIO" : "SPI_DO");
		printf_debug("GPIO12 used for %s\n", (reg & (1 << 7)) ? "GPIO" : "SPI_DI");
		if (reg != 0x00)
			has_spi = 0;
		/* GPIO31/SPI_HOLD and GPIO32/SPI_CS status */
		reg = pci_read_byte(smbus_dev, 0x83);
		reg &= 0xC0;
		printf_debug("GPIO31 used for %s\n", (reg & (1 << 6)) ? "GPIO" : "SPI_HOLD");
		printf_debug("GPIO32 used for %s\n", (reg & (1 << 7)) ? "GPIO" : "SPI_CS");
		/* SPI_HOLD is not used on all boards, filter it out. */
		if ((reg & 0x80) != 0x00)
			has_spi = 0;
		/* GPIO47/SPI_CLK status */
		reg = pci_read_byte(smbus_dev, 0xA7);
		reg &= 0x40;
		printf_debug("GPIO47 used for %s\n", (reg & (1 << 6)) ? "GPIO" : "SPI_CLK");
		if (reg != 0x00)
			has_spi = 0;
	}

	buses_supported = CHIP_BUSTYPE_LPC | CHIP_BUSTYPE_FWH;
	if (has_spi) {
		buses_supported |= CHIP_BUSTYPE_SPI;
		spi_controller = SPI_CONTROLLER_SB600;
	}

	/* Read ROM strap override register. */
	OUTB(0x8f, 0xcd6);
	reg = INB(0xcd7);
	reg &= 0x0e;
	printf_debug("ROM strap override is %sactive", (reg & 0x02) ? "" : "not ");
	if (reg & 0x02) {
		switch ((reg & 0x0c) >> 2) {
		case 0x00:
			printf_debug(": LPC");
			break;
		case 0x01:
			printf_debug(": PCI");
			break;
		case 0x02:
			printf_debug(": FWH");
			break;
		case 0x03:
			printf_debug(": SPI");
			break;
		}
	}
	printf_debug("\n");

	/* Force enable SPI ROM in SB600 PM register.
	 * If we enable SPI ROM here, we have to disable it after we leave.
	 * But how can we know which ROM we are going to handle? So we have
	 * to trade off. We only access LPC ROM if we boot via LPC ROM. And
	 * only SPI ROM if we boot via SPI ROM. If you want to access SPI on
	 * boards with LPC straps, you have to use the code below.
	 */
	/*
	OUTB(0x8f, 0xcd6);
	OUTB(0x0e, 0xcd7);
	*/

	return 0;
}

static int enable_flash_nvidia_nforce2(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;

	pci_write_byte(dev, 0x92, 0);

	tmp = pci_read_byte(dev, 0x6d);
	tmp |= 0x01;
	pci_write_byte(dev, 0x6d, tmp);

	return 0;
}

static int enable_flash_ck804(struct pci_dev *dev, const char *name)
{
	uint8_t old, new;

	old = pci_read_byte(dev, 0x88);
	new = old | 0xc0;
	if (new != old) {
		pci_write_byte(dev, 0x88, new);
		if (pci_read_byte(dev, 0x88) != new) {
			printf_debug("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x88, new, name);
		}
	}

	old = pci_read_byte(dev, 0x6d);
	new = old | 0x01;
	if (new == old)
		return 0;
	pci_write_byte(dev, 0x6d, new);

	if (pci_read_byte(dev, 0x6d) != new) {
		printf_debug("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x6d, new, name);
		return -1;
	}

	return 0;
}

/* ATI Technologies Inc IXP SB400 PCI-ISA Bridge (rev 80) */
static int enable_flash_sb400(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;
	struct pci_dev *smbusdev;

	/* Look for the SMBus device. */
	smbusdev = pci_dev_find(0x1002, 0x4372);

	if (!smbusdev) {
		fprintf(stderr, "ERROR: SMBus device not found. Aborting.\n");
		exit(1);
	}

	/* Enable some SMBus stuff. */
	tmp = pci_read_byte(smbusdev, 0x79);
	tmp |= 0x01;
	pci_write_byte(smbusdev, 0x79, tmp);

	/* Change southbridge. */
	tmp = pci_read_byte(dev, 0x48);
	tmp |= 0x21;
	pci_write_byte(dev, 0x48, tmp);

	/* Now become a bit silly. */
	tmp = INB(0xc6f);
	OUTB(tmp, 0xeb);
	OUTB(tmp, 0xeb);
	tmp |= 0x40;
	OUTB(tmp, 0xc6f);
	OUTB(tmp, 0xeb);
	OUTB(tmp, 0xeb);

	return 0;
}

static int enable_flash_mcp55(struct pci_dev *dev, const char *name)
{
	uint8_t old, new, byte;
	uint16_t word;

	/* Set the 0-16 MB enable bits. */
	byte = pci_read_byte(dev, 0x88);
	byte |= 0xff;		/* 256K */
	pci_write_byte(dev, 0x88, byte);
	byte = pci_read_byte(dev, 0x8c);
	byte |= 0xff;		/* 1M */
	pci_write_byte(dev, 0x8c, byte);
	word = pci_read_word(dev, 0x90);
	word |= 0x7fff;		/* 16M */
	pci_write_word(dev, 0x90, word);

	old = pci_read_byte(dev, 0x6d);
	new = old | 0x01;
	if (new == old)
		return 0;
	pci_write_byte(dev, 0x6d, new);

	if (pci_read_byte(dev, 0x6d) != new) {
		printf_debug("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x6d, new, name);
		return -1;
	}

	return 0;
}

static int enable_flash_ht1000(struct pci_dev *dev, const char *name)
{
	uint8_t byte;

	/* Set the 4MB enable bit. */
	byte = pci_read_byte(dev, 0x41);
	byte |= 0x0e;
	pci_write_byte(dev, 0x41, byte);

	byte = pci_read_byte(dev, 0x43);
	byte |= (1 << 4);
	pci_write_byte(dev, 0x43, byte);

	return 0;
}

/**
 * Usually on the x86 architectures (and on other PC-like platforms like some
 * Alphas or Itanium) the system flash is mapped right below 4G. On the AMD
 * Elan SC520 only a small piece of the system flash is mapped there, but the
 * complete flash is mapped somewhere below 1G. The position can be determined
 * by the BOOTCS PAR register.
 */
static int get_flashbase_sc520(struct pci_dev *dev, const char *name)
{
	int i, bootcs_found = 0;
	uint32_t parx = 0;
	void *mmcr;

	/* 1. Map MMCR */
	mmcr = physmap("Elan SC520 MMCR", 0xfffef000, getpagesize());

	/* 2. Scan PAR0 (0x88) - PAR15 (0xc4) for
	 *    BOOTCS region (PARx[31:29] = 100b)e
	 */
	for (i = 0x88; i <= 0xc4; i += 4) {
		parx = mmio_readl(mmcr + i);
		if ((parx >> 29) == 4) {
			bootcs_found = 1;
			break; /* BOOTCS found */
		}
	}

	/* 3. PARx[25] = 1b --> flashbase[29:16] = PARx[13:0]
	 *    PARx[25] = 0b --> flashbase[29:12] = PARx[17:0]
	 */
	if (bootcs_found) {
		if (parx & (1 << 25)) {
			parx &= (1 << 14) - 1; /* Mask [13:0] */
			flashbase = parx << 16;
		} else {
			parx &= (1 << 18) - 1; /* Mask [17:0] */
			flashbase = parx << 12;
		}
	} else {
		printf("AMD Elan SC520 detected, but no BOOTCS. Assuming flash at 4G\n");
	}

	/* 4. Clean up */
	physunmap(mmcr, getpagesize());
	return 0;
}

/* Please keep this list alphabetically sorted by vendor/device. */
const struct penable chipset_enables[] = {
	{0x10B9, 0x1533, OK, "ALi", "M1533",		enable_flash_ali_m1533},
	{0x1022, 0x7440, OK, "AMD", "AMD-768",		enable_flash_amd8111},
	{0x1022, 0x7468, OK, "AMD", "AMD8111",		enable_flash_amd8111},
	{0x1078, 0x0100, OK, "AMD", "CS5530(A)",	enable_flash_cs5530},
	{0x1022, 0x2080, OK, "AMD", "CS5536",		enable_flash_cs5536},
	{0x1022, 0x2090, OK, "AMD", "CS5536",		enable_flash_cs5536},
	{0x1022, 0x3000, OK, "AMD", "Elan SC520",	get_flashbase_sc520},
	{0x1002, 0x438D, OK, "AMD", "SB600",		enable_flash_sb600},
	{0x1002, 0x439d, OK, "AMD", "SB700/SB710/SB750", enable_flash_sb600},
	{0x100b, 0x0510, NT, "AMD", "SC1100",		enable_flash_sc1100},
	{0x1002, 0x4377, OK, "ATI", "SB400",		enable_flash_sb400},
	{0x1166, 0x0205, OK, "Broadcom", "HT-1000",	enable_flash_ht1000},
	{0x8086, 0x3b00, NT, "Intel", "3400 Desktop",	enable_flash_ich10},
	{0x8086, 0x3b01, NT, "Intel", "3400 Mobile",	enable_flash_ich10},
	{0x8086, 0x3b0d, NT, "Intel", "3400 Mobile SFF", enable_flash_ich10},
	{0x8086, 0x7198, OK, "Intel", "440MX",		enable_flash_piix4},
	{0x8086, 0x25a1, OK, "Intel", "6300ESB",	enable_flash_ich_4e},
	{0x8086, 0x2670, OK, "Intel", "631xESB/632xESB/3100", enable_flash_ich_dc},
	{0x8086, 0x5031, OK, "Intel", "EP80579",	enable_flash_ich7},
	{0x8086, 0x2420, OK, "Intel", "ICH0",		enable_flash_ich_4e},
	{0x8086, 0x3a18, OK, "Intel", "ICH10",		enable_flash_ich10},
	{0x8086, 0x3a1a, OK, "Intel", "ICH10D",		enable_flash_ich10},
	{0x8086, 0x3a14, OK, "Intel", "ICH10DO",	enable_flash_ich10},
	{0x8086, 0x3a16, OK, "Intel", "ICH10R",		enable_flash_ich10},
	{0x8086, 0x2440, OK, "Intel", "ICH2",		enable_flash_ich_4e},
	{0x8086, 0x244c, OK, "Intel", "ICH2-M",		enable_flash_ich_4e},
	{0x8086, 0x248c, OK, "Intel", "ICH3-M",		enable_flash_ich_4e},
	{0x8086, 0x2480, OK, "Intel", "ICH3-S",		enable_flash_ich_4e},
	{0x8086, 0x24c0, OK, "Intel", "ICH4/ICH4-L",	enable_flash_ich_4e},
	{0x8086, 0x24cc, OK, "Intel", "ICH4-M",		enable_flash_ich_4e},
	{0x8086, 0x24d0, OK, "Intel", "ICH5/ICH5R",	enable_flash_ich_4e},
	{0x8086, 0x2640, OK, "Intel", "ICH6/ICH6R",	enable_flash_ich_dc},
	{0x8086, 0x2641, OK, "Intel", "ICH6-M",		enable_flash_ich_dc},
	{0x8086, 0x27b0, OK, "Intel", "ICH7DH",		enable_flash_ich7},
	{0x8086, 0x27b8, OK, "Intel", "ICH7/ICH7R",	enable_flash_ich7},
	{0x8086, 0x27b9, OK, "Intel", "ICH7M",		enable_flash_ich7},
	{0x8086, 0x27bd, OK, "Intel", "ICH7MDH",	enable_flash_ich7},
	{0x8086, 0x2410, OK, "Intel", "ICH",		enable_flash_ich_4e},
	{0x8086, 0x2812, OK, "Intel", "ICH8DH",		enable_flash_ich8},
	{0x8086, 0x2814, OK, "Intel", "ICH8DO",		enable_flash_ich8},
	{0x8086, 0x2810, OK, "Intel", "ICH8/ICH8R",	enable_flash_ich8},
	{0x8086, 0x2815, OK, "Intel", "ICH8M",		enable_flash_ich8},
	{0x8086, 0x2811, OK, "Intel", "ICH8M-E",	enable_flash_ich8},
	{0x8086, 0x2918, OK, "Intel", "ICH9",		enable_flash_ich9},
	{0x8086, 0x2912, OK, "Intel", "ICH9DH",		enable_flash_ich9},
	{0x8086, 0x2914, OK, "Intel", "ICH9DO",		enable_flash_ich9},
	{0x8086, 0x2919, OK, "Intel", "ICH9M",		enable_flash_ich9},
	{0x8086, 0x2917, OK, "Intel", "ICH9M-E",	enable_flash_ich9},
	{0x8086, 0x2916, OK, "Intel", "ICH9R",		enable_flash_ich9},
	{0x8086, 0x2910, OK, "Intel", "ICH9 Engineering Sample", enable_flash_ich9},
	{0x8086, 0x1234, NT, "Intel", "MPIIX",		enable_flash_piix4},
	{0x8086, 0x7000, OK, "Intel", "PIIX3",		enable_flash_piix4},
	{0x8086, 0x7110, OK, "Intel", "PIIX4/4E/4M",	enable_flash_piix4},
	{0x8086, 0x122e, OK, "Intel", "PIIX",		enable_flash_piix4},
	{0x10de, 0x0030, OK, "NVIDIA", "nForce4/MCP4",  enable_flash_nvidia_nforce2},
	{0x10de, 0x0050, OK, "NVIDIA", "CK804",		enable_flash_ck804}, /* LPC */
	{0x10de, 0x0051, OK, "NVIDIA", "CK804",		enable_flash_ck804}, /* Pro */
	{0x10de, 0x0060, OK, "NVIDIA", "NForce2",       enable_flash_nvidia_nforce2},
	/* Slave, should not be here, to fix known bug for A01. */
	{0x10de, 0x00d3, OK, "NVIDIA", "CK804",		enable_flash_ck804},
	{0x10de, 0x0260, NT, "NVIDIA", "MCP51",		enable_flash_ck804},
	{0x10de, 0x0261, NT, "NVIDIA", "MCP51",		enable_flash_ck804},
	{0x10de, 0x0262, NT, "NVIDIA", "MCP51",		enable_flash_ck804},
	{0x10de, 0x0263, NT, "NVIDIA", "MCP51",		enable_flash_ck804},
	{0x10de, 0x0360, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* M57SLI*/
	{0x10de, 0x0361, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0362, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0363, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0364, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0365, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0366, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0367, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* Pro */
	{0x10de, 0x0548, OK, "NVIDIA", "MCP67",		enable_flash_mcp55},
	{0x1039, 0x0496, NT, "SiS", "85C496+497",	enable_flash_sis85c496},
	{0x1039, 0x0406, NT, "SiS", "501/5101/5501",	enable_flash_sis501},
	{0x1039, 0x5511, NT, "SiS", "5511",		enable_flash_sis5511},
	{0x1039, 0x5596, NT, "SiS", "5596",		enable_flash_sis5596},
	{0x1039, 0x5571, NT, "SiS", "5571",		enable_flash_sis530},
	{0x1039, 0x5591, NT, "SiS", "5591/5592",	enable_flash_sis530},
	{0x1039, 0x5597, NT, "SiS", "5597/5598/5581/5120", enable_flash_sis530},
	{0x1039, 0x0530, NT, "SiS", "530",		enable_flash_sis530},
	{0x1039, 0x5600, NT, "SiS", "600",		enable_flash_sis530},
	{0x1039, 0x0620, NT, "SiS", "620",		enable_flash_sis530},
	{0x1039, 0x0540, NT, "SiS", "540",		enable_flash_sis540},
	{0x1039, 0x0630, NT, "SiS", "630",		enable_flash_sis630},
	{0x1039, 0x0635, NT, "SiS", "635",		enable_flash_sis630},
	{0x1039, 0x0640, NT, "SiS", "640",		enable_flash_sis630},
	{0x1039, 0x0645, NT, "SiS", "645",		enable_flash_sis630},
	{0x1039, 0x0646, NT, "SiS", "645DX",		enable_flash_sis630},
	{0x1039, 0x0648, NT, "SiS", "648",		enable_flash_sis630},
	{0x1039, 0x0650, NT, "SiS", "650",		enable_flash_sis630},
	{0x1039, 0x0651, NT, "SiS", "651",		enable_flash_sis630},
	{0x1039, 0x0655, NT, "SiS", "655",		enable_flash_sis630},
	{0x1039, 0x0730, NT, "SiS", "730",		enable_flash_sis630},
	{0x1039, 0x0733, NT, "SiS", "733",		enable_flash_sis630},
	{0x1039, 0x0735, OK, "SiS", "735",		enable_flash_sis630},
	{0x1039, 0x0740, NT, "SiS", "740",		enable_flash_sis630},
	{0x1039, 0x0745, NT, "SiS", "745",		enable_flash_sis630},
	{0x1039, 0x0746, NT, "SiS", "746",		enable_flash_sis630},
	{0x1039, 0x0748, NT, "SiS", "748",		enable_flash_sis630},
	{0x1039, 0x0755, NT, "SiS", "755",		enable_flash_sis630},
	{0x1106, 0x8324, OK, "VIA", "CX700",		enable_flash_vt823x},
	{0x1106, 0x8231, NT, "VIA", "VT8231",		enable_flash_vt823x},
	{0x1106, 0x3074, NT, "VIA", "VT8233",		enable_flash_vt823x},
	{0x1106, 0x3177, OK, "VIA", "VT8235",		enable_flash_vt823x},
	{0x1106, 0x3227, OK, "VIA", "VT8237",		enable_flash_vt823x},
	{0x1106, 0x3337, OK, "VIA", "VT8237A",		enable_flash_vt823x},
	{0x1106, 0x3372, OK, "VIA", "VT8237S",		enable_flash_vt8237s_spi},
	{0x1106, 0x8353, OK, "VIA", "VX800",		enable_flash_vt8237s_spi},
	{0x1106, 0x0596, OK, "VIA", "VT82C596",		enable_flash_amd8111},
	{0x1106, 0x0586, OK, "VIA", "VT82C586A/B",	enable_flash_amd8111},
	{0x1106, 0x0686, NT, "VIA", "VT82C686A/B",	enable_flash_amd8111},

	{},
};

int chipset_flash_enable(void)
{
	struct pci_dev *dev = 0;
	int ret = -2;		/* Nothing! */
	int i;

	/* Now let's try to find the chipset we have... */
	for (i = 0; chipset_enables[i].vendor_name != NULL; i++) {
		dev = pci_dev_find(chipset_enables[i].vendor_id,
				   chipset_enables[i].device_id);
		if (dev)
			break;
	}

	if (dev) {
		printf("Found chipset \"%s %s\", enabling flash write... ",
		       chipset_enables[i].vendor_name,
		       chipset_enables[i].device_name);

		ret = chipset_enables[i].doit(dev,
					      chipset_enables[i].device_name);
		if (ret)
			printf("FAILED!\n");
		else
			printf("OK.\n");
	}
	printf("This chipset supports the following protocols: %s.\n",
	       flashbuses_to_text(buses_supported));

	return ret;
}
