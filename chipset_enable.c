/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2005-2009 coresystems GmbH
 * Copyright (C) 2006 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2007,2008,2009 Carl-Daniel Hailfinger
 * Copyright (C) 2009 Kontron Modular Computers GmbH
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
#include <unistd.h>
#include "flash.h"

#if defined(__i386__) || defined(__x86_64__)

#define NOT_DONE_YET 1

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
		msg_pinfo("tried to set register 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x40, new, name);
		msg_pinfo("Stuck at 0x%x\n", newer);
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
		msg_perr("No southbridge found for %s!\n", name);
	if (sbdev)
		msg_pdbg("Found southbridge %04x:%04x at %02x:%02x:%01x\n",
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
	newer = pci_read_byte(sbdev, 0x45);
	if (newer != new) {
		msg_pinfo("tried to set register 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x45, new, name);
		msg_pinfo("Stuck at 0x%x\n", newer);
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
	newer = pci_read_byte(sbdev, 0x45);
	if (newer != new) {
		msg_pinfo("tried to set register 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x45, new, name);
		msg_pinfo("Stuck at 0x%x\n", newer);
		ret = -1;
	}

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

	buses_supported = CHIP_BUSTYPE_PARALLEL;

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
		msg_pinfo("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", xbcs, new, name);
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

	msg_pdbg("\nBIOS Lock Enable: %sabled, ",
		     (old & (1 << 1)) ? "en" : "dis");
	msg_pdbg("BIOS Write Enable: %sabled, ",
		     (old & (1 << 0)) ? "en" : "dis");
	msg_pdbg("BIOS_CNTL is 0x%x\n", old);

	new = old | 1;

	if (new == old)
		return 0;

	pci_write_byte(dev, bios_cntl, new);

	if (pci_read_byte(dev, bios_cntl) != new) {
		msg_pinfo("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", bios_cntl, new, name);
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
	buses_supported = CHIP_BUSTYPE_FWH;
	return enable_flash_ich(dev, name, 0x4e);
}

static int enable_flash_ich_dc(struct pci_dev *dev, const char *name)
{
	uint32_t fwh_conf;
	int i;
	char *idsel = NULL;
	int tmp;
	int max_decode_fwh_idsel = 0;
	int max_decode_fwh_decode = 0;
	int contiguous = 1;

	idsel = extract_programmer_param("fwh_idsel");
	if (idsel && strlen(idsel)) {
		fwh_conf = (uint32_t)strtoul(idsel, NULL, 0);

		/* FIXME: Need to undo this on shutdown. */
		msg_pinfo("\nSetting IDSEL=0x%x for top 16 MB", fwh_conf);
		pci_write_long(dev, 0xd0, fwh_conf);
		pci_write_word(dev, 0xd4, fwh_conf);
		/* FIXME: Decode settings are not changed. */
	} else if (idsel) {
		msg_perr("Error: idsel= specified, but no number given.\n");
		free(idsel);
		/* FIXME: Return failure here once internal_init() starts
		 * to care about the return value of the chipset enable.
		 */
		exit(1);
	}
	free(idsel);

	/* Ignore all legacy ranges below 1 MB.
	 * We currently only support flashing the chip which responds to
	 * IDSEL=0. To support IDSEL!=0, flashbase and decode size calculations
	 * have to be adjusted.
	 */
	/* FWH_SEL1 */
	fwh_conf = pci_read_long(dev, 0xd0);
	for (i = 7; i >= 0; i--) {
		tmp = (fwh_conf >> (i * 4)) & 0xf;
		msg_pdbg("\n0x%08x/0x%08x FWH IDSEL: 0x%x",
			     (0x1ff8 + i) * 0x80000,
			     (0x1ff0 + i) * 0x80000,
			     tmp);
		if ((tmp == 0) && contiguous) {
			max_decode_fwh_idsel = (8 - i) * 0x80000;
		} else {
			contiguous = 0;
		}
	}
	/* FWH_SEL2 */
	fwh_conf = pci_read_word(dev, 0xd4);
	for (i = 3; i >= 0; i--) {
		tmp = (fwh_conf >> (i * 4)) & 0xf;
		msg_pdbg("\n0x%08x/0x%08x FWH IDSEL: 0x%x",
			     (0xff4 + i) * 0x100000,
			     (0xff0 + i) * 0x100000,
			     tmp);
		if ((tmp == 0) && contiguous) {
			max_decode_fwh_idsel = (8 - i) * 0x100000;
		} else {
			contiguous = 0;
		}
	}
	contiguous = 1;
	/* FWH_DEC_EN1 */
	fwh_conf = pci_read_word(dev, 0xd8);
	for (i = 7; i >= 0; i--) {
		tmp = (fwh_conf >> (i + 0x8)) & 0x1;
		msg_pdbg("\n0x%08x/0x%08x FWH decode %sabled",
			     (0x1ff8 + i) * 0x80000,
			     (0x1ff0 + i) * 0x80000,
			     tmp ? "en" : "dis");
		if ((tmp == 1) && contiguous) {
			max_decode_fwh_decode = (8 - i) * 0x80000;
		} else {
			contiguous = 0;
		}
	}
	for (i = 3; i >= 0; i--) {
		tmp = (fwh_conf >> i) & 0x1;
		msg_pdbg("\n0x%08x/0x%08x FWH decode %sabled",
			     (0xff4 + i) * 0x100000,
			     (0xff0 + i) * 0x100000,
			     tmp ? "en" : "dis");
		if ((tmp == 1) && contiguous) {
			max_decode_fwh_decode = (8 - i) * 0x100000;
		} else {
			contiguous = 0;
		}
	}
	max_rom_decode.fwh = min(max_decode_fwh_idsel, max_decode_fwh_decode);
	msg_pdbg("\nMaximum FWH chip size: 0x%x bytes", max_rom_decode.fwh);

	/* If we're called by enable_flash_ich_dc_spi, it will override
	 * buses_supported anyway.
	 */
	buses_supported = CHIP_BUSTYPE_FWH;
	return enable_flash_ich(dev, name, 0xdc);
}

static int enable_flash_poulsbo(struct pci_dev *dev, const char *name)
{
       uint16_t old, new;
       int err;

       if ((err = enable_flash_ich(dev, name, 0xd8)) != 0)
               return err;

       old = pci_read_byte(dev, 0xd9);
       msg_pdbg("BIOS Prefetch Enable: %sabled, ",
                    (old & 1) ? "en" : "dis");
       new = old & ~1;

       if (new != old)
               pci_write_byte(dev, 0xd9, new);

	buses_supported = CHIP_BUSTYPE_FWH;
       return 0;
}


#define ICH_STRAP_RSVD 0x00
#define ICH_STRAP_SPI  0x01
#define ICH_STRAP_PCI  0x02
#define ICH_STRAP_LPC  0x03

static int enable_flash_vt8237s_spi(struct pci_dev *dev, const char *name)
{
	uint32_t mmio_base;

	/* Do we really need no write enable? */
	mmio_base = (pci_read_long(dev, 0xbc)) << 8;
	msg_pdbg("MMIO base at = 0x%x\n", mmio_base);
	ich_spibar = physmap("VT8237S MMIO registers", mmio_base, 0x70);

	msg_pdbg("0x6c: 0x%04x     (CLOCK/DEBUG)\n",
		     mmio_readw(ich_spibar + 0x6c));

	/* Not sure if it speaks all these bus protocols. */
	buses_supported = CHIP_BUSTYPE_LPC | CHIP_BUSTYPE_FWH | CHIP_BUSTYPE_SPI;
	spi_controller = SPI_CONTROLLER_VIA;
	ich_init_opcodes();

	return 0;
}

#define ICH_BMWAG(x) ((x >> 24) & 0xff)
#define ICH_BMRAG(x) ((x >> 16) & 0xff)
#define ICH_BRWA(x)  ((x >>  8) & 0xff)
#define ICH_BRRA(x)  ((x >>  0) & 0xff)

#define ICH_FREG_BASE(x)  ((x >>  0) & 0x1fff)
#define ICH_FREG_LIMIT(x) ((x >> 16) & 0x1fff)

static void do_ich9_spi_frap(uint32_t frap, int i)
{
	const char *access_names[4] = {
		"locked", "read-only", "write-only", "read-write"
	};
	const char *region_names[5] = {
		"Flash Descriptor", "BIOS", "Management Engine",
		"Gigabit Ethernet", "Platform Data"
	};
	int rwperms = ((ICH_BRWA(frap) & (1 << i)) << 1) |
		      ((ICH_BRRA(frap) & (1 << i)) << 0);
	int offset = 0x54 + i * 4;
	uint32_t freg = mmio_readl(ich_spibar + offset), base, limit;

	msg_pdbg("0x%02X: 0x%08x (FREG%i: %s)\n",
		     offset, freg, i, region_names[i]);

	base  = ICH_FREG_BASE(freg);
	limit = ICH_FREG_LIMIT(freg);
	if (base == 0x1fff && limit == 0) {
		/* this FREG is disabled */
		msg_pdbg("%s region is unused.\n", region_names[i]);
		return;
	}

	msg_pdbg("0x%08x-0x%08x is %s\n",
		    (base << 12), (limit << 12) | 0x0fff,
		    access_names[rwperms]);
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
	msg_pdbg("\nRoot Complex Register Block address = 0x%x\n", tmp);

	/* Map RCBA to virtual memory */
	rcrb = physmap("ICH RCRB", tmp, 0x4000);

	gcs = mmio_readl(rcrb + 0x3410);
	msg_pdbg("GCS = 0x%x: ", gcs);
	msg_pdbg("BIOS Interface Lock-Down: %sabled, ",
		     (gcs & 0x1) ? "en" : "dis");
	bbs = (gcs >> 10) & 0x3;
	msg_pdbg("BOOT BIOS Straps: 0x%x (%s)\n", bbs, straps_names[bbs]);

	buc = mmio_readb(rcrb + 0x3414);
	msg_pdbg("Top Swap : %s\n",
		     (buc & 1) ? "enabled (A16 inverted)" : "not enabled");

	/* It seems the ICH7 does not support SPI and LPC chips at the same
	 * time. At least not with our current code. So we prevent searching
	 * on ICH7 when the southbridge is strapped to LPC
	 */

	if (ich_generation == 7 && bbs == ICH_STRAP_LPC) {
		buses_supported = CHIP_BUSTYPE_FWH;
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
		buses_supported = CHIP_BUSTYPE_FWH | CHIP_BUSTYPE_SPI;
		spi_controller = SPI_CONTROLLER_ICH9;
		spibar_offset = 0x3020;
		break;
	case 9:
	case 10:
	default:		/* Future version might behave the same */
		buses_supported = CHIP_BUSTYPE_FWH | CHIP_BUSTYPE_SPI;
		spi_controller = SPI_CONTROLLER_ICH9;
		spibar_offset = 0x3800;
		break;
	}

	/* SPIBAR is at RCRB+0x3020 for ICH[78] and RCRB+0x3800 for ICH9. */
	msg_pdbg("SPIBAR = 0x%x + 0x%04x\n", tmp, spibar_offset);

	/* Assign Virtual Address */
	ich_spibar = rcrb + spibar_offset;

	switch (spi_controller) {
	case SPI_CONTROLLER_ICH7:
		msg_pdbg("0x00: 0x%04x     (SPIS)\n",
			     mmio_readw(ich_spibar + 0));
		msg_pdbg("0x02: 0x%04x     (SPIC)\n",
			     mmio_readw(ich_spibar + 2));
		msg_pdbg("0x04: 0x%08x (SPIA)\n",
			     mmio_readl(ich_spibar + 4));
		for (i = 0; i < 8; i++) {
			int offs;
			offs = 8 + (i * 8);
			msg_pdbg("0x%02x: 0x%08x (SPID%d)\n", offs,
				     mmio_readl(ich_spibar + offs), i);
			msg_pdbg("0x%02x: 0x%08x (SPID%d+4)\n", offs + 4,
				     mmio_readl(ich_spibar + offs + 4), i);
		}
		ichspi_bbar = mmio_readl(ich_spibar + 0x50);
		msg_pdbg("0x50: 0x%08x (BBAR)\n",
			     ichspi_bbar);
		msg_pdbg("0x54: 0x%04x     (PREOP)\n",
			     mmio_readw(ich_spibar + 0x54));
		msg_pdbg("0x56: 0x%04x     (OPTYPE)\n",
			     mmio_readw(ich_spibar + 0x56));
		msg_pdbg("0x58: 0x%08x (OPMENU)\n",
			     mmio_readl(ich_spibar + 0x58));
		msg_pdbg("0x5c: 0x%08x (OPMENU+4)\n",
			     mmio_readl(ich_spibar + 0x5c));
		for (i = 0; i < 4; i++) {
			int offs;
			offs = 0x60 + (i * 4);
			msg_pdbg("0x%02x: 0x%08x (PBR%d)\n", offs,
				     mmio_readl(ich_spibar + offs), i);
		}
		msg_pdbg("\n");
		if (mmio_readw(ich_spibar) & (1 << 15)) {
			msg_pinfo("WARNING: SPI Configuration Lockdown activated.\n");
			ichspi_lock = 1;
		}
		ich_init_opcodes();
		break;
	case SPI_CONTROLLER_ICH9:
		tmp2 = mmio_readw(ich_spibar + 4);
		msg_pdbg("0x04: 0x%04x (HSFS)\n", tmp2);
		msg_pdbg("FLOCKDN %i, ", (tmp2 >> 15 & 1));
		msg_pdbg("FDV %i, ", (tmp2 >> 14) & 1);
		msg_pdbg("FDOPSS %i, ", (tmp2 >> 13) & 1);
		msg_pdbg("SCIP %i, ", (tmp2 >> 5) & 1);
		msg_pdbg("BERASE %i, ", (tmp2 >> 3) & 3);
		msg_pdbg("AEL %i, ", (tmp2 >> 2) & 1);
		msg_pdbg("FCERR %i, ", (tmp2 >> 1) & 1);
		msg_pdbg("FDONE %i\n", (tmp2 >> 0) & 1);

		tmp = mmio_readl(ich_spibar + 0x50);
		msg_pdbg("0x50: 0x%08x (FRAP)\n", tmp);
		msg_pdbg("BMWAG 0x%02x, ", ICH_BMWAG(tmp));
		msg_pdbg("BMRAG 0x%02x, ", ICH_BMRAG(tmp));
		msg_pdbg("BRWA 0x%02x, ", ICH_BRWA(tmp));
		msg_pdbg("BRRA 0x%02x\n", ICH_BRRA(tmp));

		/* print out the FREGx registers along with FRAP access bits */
		for(i = 0; i < 5; i++)
			do_ich9_spi_frap(tmp, i);

		msg_pdbg("0x74: 0x%08x (PR0)\n",
			     mmio_readl(ich_spibar + 0x74));
		msg_pdbg("0x78: 0x%08x (PR1)\n",
			     mmio_readl(ich_spibar + 0x78));
		msg_pdbg("0x7C: 0x%08x (PR2)\n",
			     mmio_readl(ich_spibar + 0x7C));
		msg_pdbg("0x80: 0x%08x (PR3)\n",
			     mmio_readl(ich_spibar + 0x80));
		msg_pdbg("0x84: 0x%08x (PR4)\n",
			     mmio_readl(ich_spibar + 0x84));
		msg_pdbg("0x90: 0x%08x (SSFS, SSFC)\n",
			     mmio_readl(ich_spibar + 0x90));
		msg_pdbg("0x94: 0x%04x     (PREOP)\n",
			     mmio_readw(ich_spibar + 0x94));
		msg_pdbg("0x96: 0x%04x     (OPTYPE)\n",
			     mmio_readw(ich_spibar + 0x96));
		msg_pdbg("0x98: 0x%08x (OPMENU)\n",
			     mmio_readl(ich_spibar + 0x98));
		msg_pdbg("0x9C: 0x%08x (OPMENU+4)\n",
			     mmio_readl(ich_spibar + 0x9C));
		ichspi_bbar = mmio_readl(ich_spibar + 0xA0);
		msg_pdbg("0xA0: 0x%08x (BBAR)\n",
			     ichspi_bbar);
		msg_pdbg("0xB0: 0x%08x (FDOC)\n",
			     mmio_readl(ich_spibar + 0xB0));
		if (tmp2 & (1 << 15)) {
			msg_pinfo("WARNING: SPI Configuration Lockdown activated.\n");
			ichspi_lock = 1;
		}
		ich_init_opcodes();
		break;
	default:
		/* Nothing */
		break;
	}

	old = pci_read_byte(dev, 0xdc);
	msg_pdbg("SPI Read Configuration: ");
	new = (old >> 2) & 0x3;
	switch (new) {
	case 0:
	case 1:
	case 2:
		msg_pdbg("prefetching %sabled, caching %sabled, ",
			     (new & 0x2) ? "en" : "dis",
			     (new & 0x1) ? "dis" : "en");
		break;
	default:
		msg_pdbg("invalid prefetching/caching settings, ");
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

static void via_do_byte_merge(void * arg)
{
	struct pci_dev * dev = arg;
	uint8_t val;

	msg_pdbg("Re-enabling byte merging\n");
	val = pci_read_byte(dev, 0x71);
	val |= 0x40;
	pci_write_byte(dev, 0x71, val);
}

static int via_no_byte_merge(struct pci_dev *dev, const char *name)
{
	uint8_t val;

	val = pci_read_byte(dev, 0x71);
	if (val & 0x40)
	{
		msg_pdbg("Disabling byte merging\n");
		val &= ~0x40;
		pci_write_byte(dev, 0x71, val);
		register_shutdown(via_do_byte_merge, dev);
	}
	return NOT_DONE_YET;	/* need to find south bridge, too */
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
		msg_pinfo("\nWARNING: Failed to enable flash write on \"%s\"\n",
		       name);
		return -1;
	}

	if (dev->device_id == 0x3227) { /* VT8237R */
	    /* All memory cycles, not just ROM ones, go to LPC. */
	    val = pci_read_byte(dev, 0x59);
	    val &= ~0x80;
	    pci_write_byte(dev, 0x59, val);
	}

	return 0;
}

static int enable_flash_cs5530(struct pci_dev *dev, const char *name)
{
	uint8_t reg8;

#define DECODE_CONTROL_REG2		0x5b	/* F0 index 0x5b */
#define ROM_AT_LOGIC_CONTROL_REG	0x52	/* F0 index 0x52 */
#define CS5530_RESET_CONTROL_REG	0x44	/* F0 index 0x44 */
#define CS5530_USB_SHADOW_REG		0x43	/* F0 index 0x43 */

#define LOWER_ROM_ADDRESS_RANGE		(1 << 0)
#define ROM_WRITE_ENABLE		(1 << 1)
#define UPPER_ROM_ADDRESS_RANGE		(1 << 2)
#define BIOS_ROM_POSITIVE_DECODE	(1 << 5)
#define CS5530_ISA_MASTER		(1 << 7)
#define CS5530_ENABLE_SA2320		(1 << 2)
#define CS5530_ENABLE_SA20		(1 << 6)

	buses_supported = CHIP_BUSTYPE_PARALLEL;
	/* Decode 0x000E0000-0x000FFFFF (128 KB), not just 64 KB, and
	 * decode 0xFF000000-0xFFFFFFFF (16 MB), not just 256 KB.
	 * FIXME: Should we really touch the low mapping below 1 MB? Flashrom
	 * ignores that region completely.
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

	reg8 = pci_read_byte(dev, CS5530_RESET_CONTROL_REG);
	if (reg8 & CS5530_ISA_MASTER) {
		/* We have A0-A23 available. */
		max_rom_decode.parallel = 16 * 1024 * 1024;
	} else {
		reg8 = pci_read_byte(dev, CS5530_USB_SHADOW_REG);
		if (reg8 & CS5530_ENABLE_SA2320) {
			/* We have A0-19, A20-A23 available. */
			max_rom_decode.parallel = 16 * 1024 * 1024;
		} else if (reg8 & CS5530_ENABLE_SA20) {
			/* We have A0-19, A20 available. */
			max_rom_decode.parallel = 2 * 1024 * 1024;
		} else {
			/* A20 and above are not active. */
			max_rom_decode.parallel = 1024 * 1024;
		}
	}

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
		msg_pinfo("tried to set register 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x52, new, name);
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
			msg_pinfo("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x43, new, name);
		}
	}

	/* Enable 'ROM write' bit. */
	old = pci_read_byte(dev, 0x40);
	new = old | 0x01;
	if (new == old)
		return 0;
	pci_write_byte(dev, 0x40, new);

	if (pci_read_byte(dev, 0x40) != new) {
		msg_pinfo("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x40, new, name);
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
		msg_pinfo("SB600 %s%sprotected from %u to %u\n",
			(prot & 0x1) ? "write " : "",
			(prot & 0x2) ? "read " : "",
			(prot & 0xfffffc00),
			(prot & 0xfffffc00) + ((prot & 0x3ff) << 8));
		prot &= 0xfffffffc;
		pci_write_byte(dev, reg, prot);
		prot = pci_read_long(dev, reg);
		if (prot & 0x3)
			msg_perr("SB600 %s%sunprotect failed from %u to %u\n",
				(prot & 0x1) ? "write " : "",
				(prot & 0x2) ? "read " : "",
				(prot & 0xfffffc00),
				(prot & 0xfffffc00) + ((prot & 0x3ff) << 8));
	}

	/* Read SPI_BaseAddr */
	tmp = pci_read_long(dev, 0xa0);
	tmp &= 0xffffffe0;	/* remove bits 4-0 (reserved) */
	msg_pdbg("SPI base address is at 0x%x\n", tmp);

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
		msg_pdbg("AltSpiCSEnable=%i, SpiRomEnable=%i, "
			     "AbortEnable=%i\n", tmp & 0x1, (tmp & 0x2) >> 1,
			     (tmp & 0x4) >> 2);
		tmp = (pci_read_byte(dev, 0xba) & 0x4) >> 2;
		msg_pdbg("PrefetchEnSPIFromIMC=%i, ", tmp);

		tmp = pci_read_byte(dev, 0xbb);
		msg_pdbg("PrefetchEnSPIFromHost=%i, SpiOpEnInLpcMode=%i\n",
			     tmp & 0x1, (tmp & 0x20) >> 5);
		tmp = mmio_readl(sb600_spibar);
		msg_pdbg("SpiArbEnable=%i, SpiAccessMacRomEn=%i, "
			     "SpiHostAccessRomEn=%i, ArbWaitCount=%i, "
			     "SpiBridgeDisable=%i, DropOneClkOnRd=%i\n",
			     (tmp >> 19) & 0x1, (tmp >> 22) & 0x1,
			     (tmp >> 23) & 0x1, (tmp >> 24) & 0x7,
			     (tmp >> 27) & 0x1, (tmp >> 28) & 0x1);
	}

	/* Look for the SMBus device. */
	smbus_dev = pci_dev_find(0x1002, 0x4385);

	if (has_spi && !smbus_dev) {
		msg_perr("ERROR: SMBus device not found. Not enabling SPI.\n");
		has_spi = 0;
	}
	if (has_spi) {
		/* Note about the bit tests below: If a bit is zero, the GPIO is SPI. */
		/* GPIO11/SPI_DO and GPIO12/SPI_DI status */
		reg = pci_read_byte(smbus_dev, 0xAB);
		reg &= 0xC0;
		msg_pdbg("GPIO11 used for %s\n", (reg & (1 << 6)) ? "GPIO" : "SPI_DO");
		msg_pdbg("GPIO12 used for %s\n", (reg & (1 << 7)) ? "GPIO" : "SPI_DI");
		if (reg != 0x00)
			has_spi = 0;
		/* GPIO31/SPI_HOLD and GPIO32/SPI_CS status */
		reg = pci_read_byte(smbus_dev, 0x83);
		reg &= 0xC0;
		msg_pdbg("GPIO31 used for %s\n", (reg & (1 << 6)) ? "GPIO" : "SPI_HOLD");
		msg_pdbg("GPIO32 used for %s\n", (reg & (1 << 7)) ? "GPIO" : "SPI_CS");
		/* SPI_HOLD is not used on all boards, filter it out. */
		if ((reg & 0x80) != 0x00)
			has_spi = 0;
		/* GPIO47/SPI_CLK status */
		reg = pci_read_byte(smbus_dev, 0xA7);
		reg &= 0x40;
		msg_pdbg("GPIO47 used for %s\n", (reg & (1 << 6)) ? "GPIO" : "SPI_CLK");
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
	msg_pdbg("ROM strap override is %sactive", (reg & 0x02) ? "" : "not ");
	if (reg & 0x02) {
		switch ((reg & 0x0c) >> 2) {
		case 0x00:
			msg_pdbg(": LPC");
			break;
		case 0x01:
			msg_pdbg(": PCI");
			break;
		case 0x02:
			msg_pdbg(": FWH");
			break;
		case 0x03:
			msg_pdbg(": SPI");
			break;
		}
	}
	msg_pdbg("\n");

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
			msg_pinfo("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x88, new, name);
		}
	}

	old = pci_read_byte(dev, 0x6d);
	new = old | 0x01;
	if (new == old)
		return 0;
	pci_write_byte(dev, 0x6d, new);

	if (pci_read_byte(dev, 0x6d) != new) {
		msg_pinfo("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x6d, new, name);
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
		msg_perr("ERROR: SMBus device not found. Aborting.\n");
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
	uint8_t old, new, val;
	uint16_t wordval;

	/* Set the 0-16 MB enable bits. */
	val = pci_read_byte(dev, 0x88);
	val |= 0xff;		/* 256K */
	pci_write_byte(dev, 0x88, val);
	val = pci_read_byte(dev, 0x8c);
	val |= 0xff;		/* 1M */
	pci_write_byte(dev, 0x8c, val);
	wordval = pci_read_word(dev, 0x90);
	wordval |= 0x7fff;	/* 16M */
	pci_write_word(dev, 0x90, wordval);

	old = pci_read_byte(dev, 0x6d);
	new = old | 0x01;
	if (new == old)
		return 0;
	pci_write_byte(dev, 0x6d, new);

	if (pci_read_byte(dev, 0x6d) != new) {
		msg_pinfo("tried to set 0x%x to 0x%x on %s failed (WARNING ONLY)\n", 0x6d, new, name);
		return -1;
	}

	return 0;
}

/* This is a shot in the dark. Even if the code is totally bogus for some
 * chipsets, users will at least start to send in reports.
 */
static int enable_flash_mcp6x_7x_common(struct pci_dev *dev, const char *name)
{
	int ret = 0;
	uint8_t val;
	uint16_t status;
	char *busname;
	uint32_t mcp_spibaraddr;
	void *mcp_spibar;
	struct pci_dev *smbusdev;

	msg_pinfo("This chipset is not really supported yet. Guesswork...\n");

	/* dev is the ISA bridge. No idea what the stuff below does. */
	val = pci_read_byte(dev, 0x8a);
	msg_pdbg("ISA/LPC bridge reg 0x8a contents: 0x%02x, bit 6 is %i, bit 5 "
		 "is %i\n", val, (val >> 6) & 0x1, (val >> 5) & 0x1);
	switch ((val >> 5) & 0x3) {
	case 0x0:
		buses_supported = CHIP_BUSTYPE_LPC;
		break;
	case 0x2:
		buses_supported = CHIP_BUSTYPE_SPI;
		break;
	default:
		buses_supported = CHIP_BUSTYPE_UNKNOWN;
		break;
	}
	busname = flashbuses_to_text(buses_supported);
	msg_pdbg("Guessed flash bus type is %s\n", busname);
	free(busname);

	/* Force enable SPI and disable LPC? Not a good idea. */
#if 0
	val |= (1 << 6);
	val &= ~(1 << 5);
	pci_write_byte(dev, 0x8a, val);
#endif

	/* Look for the SMBus device (SMBus PCI class) */
	smbusdev = pci_dev_find_vendorclass(0x10de, 0x0c05);
	if (!smbusdev) {
		if (buses_supported & CHIP_BUSTYPE_SPI) {
			msg_perr("ERROR: SMBus device not found. Not enabling "
				 "SPI.\n");
			buses_supported &= ~CHIP_BUSTYPE_SPI;
			ret = 1;
		} else {
			msg_pinfo("Odd. SMBus device not found.\n");
		}
		goto out_msg;
	}
	msg_pdbg("Found SMBus device %04x:%04x at %02x:%02x:%01x\n",
		smbusdev->vendor_id, smbusdev->device_id,
		smbusdev->bus, smbusdev->dev, smbusdev->func);

	/* Locate the BAR where the SPI interface lives. */
	mcp_spibaraddr = pci_read_long(smbusdev, 0x74);
	msg_pdbg("SPI BAR is at 0x%08x, ", mcp_spibaraddr);
	/* We hope this has native alignment. We know the SPI interface (well,
	 * a set of GPIOs that is connected to SPI flash) is at offset 0x530,
	 * so we expect a size of at least 0x800. Clear the lower bits.
	 * It is entirely possible that the BAR is 64k big and the low bits are
	 * reserved for an entirely different purpose.
	 */
	mcp_spibaraddr &= ~0x7ff;
	msg_pdbg("after clearing low bits BAR is at 0x%08x\n", mcp_spibaraddr);

	/* Accessing a NULL pointer BAR is evil. Don't do it. */
	if (mcp_spibaraddr && (buses_supported == CHIP_BUSTYPE_SPI)) {
		/* Map the BAR. Bytewise/wordwise access at 0x530 and 0x540. */
		mcp_spibar = physmap("MCP67 SPI", mcp_spibaraddr, 0x544);

/* Guessed. If this is correct, migrate to a separate MCP67 SPI driver. */
#define MCP67_SPI_CS		(1 << 1)
#define MCP67_SPI_SCK		(1 << 2)
#define MCP67_SPI_MOSI		(1 << 3)
#define MCP67_SPI_MISO		(1 << 4)
#define MCP67_SPI_ENABLE	(1 << 0)
#define MCP67_SPI_IDLE		(1 << 8)

		status = mmio_readw(mcp_spibar + 0x530);
		msg_pdbg("SPI control is 0x%04x, enable=%i, idle=%i\n",
			 status, status & 0x1, (status >> 8) & 0x1);
		/* FIXME: Remove the physunmap once the SPI driver exists. */
		physunmap(mcp_spibar, 0x544);
	} else if (!mcp_spibaraddr && (buses_supported & CHIP_BUSTYPE_SPI)) {
		msg_pdbg("Strange. MCP SPI BAR is invalid.\n");
		buses_supported &= ~CHIP_BUSTYPE_SPI;
		ret = 1;
	} else if (mcp_spibaraddr && !(buses_supported & CHIP_BUSTYPE_SPI)) {
		msg_pdbg("Strange. MCP SPI BAR is valid, but chipset apparently"
			 " doesn't have SPI enabled.\n");
	} else {
		msg_pdbg("MCP SPI is not used.\n");
	}
out_msg:
	msg_pinfo("Please send the output of \"flashrom -V\" to "
		  "flashrom@flashrom.org to help us finish support for your "
		  "chipset. Thanks.\n");

	return ret;
}

/**
 * The MCP61/MCP67 code is guesswork based on cleanroom reverse engineering.
 * Due to that, it only reads info and doesn't change any settings.
 * It is assumed that LPC chips need the MCP55 code and SPI chips need the
 * code provided in enable_flash_mcp6x_7x_common. Until we know for sure, call
 * enable_flash_mcp55 from this function only if enable_flash_mcp6x_7x_common
 * indicates the flash chip is LPC. Warning: enable_flash_mcp55
 * might make SPI flash inaccessible. The same caveat applies to SPI init
 * for LPC flash.
 */
static int enable_flash_mcp67(struct pci_dev *dev, const char *name)
{
	int result = 0;

	result = enable_flash_mcp6x_7x_common(dev, name);
	if (result)
		return result;

	/* Not sure if this is correct. No docs as usual. */
	switch (buses_supported) {
	case CHIP_BUSTYPE_LPC:
		result = enable_flash_mcp55(dev, name);
		break;
	case CHIP_BUSTYPE_SPI:
		msg_pinfo("SPI on this chipset is not supported yet.\n");
		buses_supported = CHIP_BUSTYPE_NONE;
		break;
	default:
		msg_pinfo("Something went wrong with bus type detection.\n");
		buses_supported = CHIP_BUSTYPE_NONE;
		break;
	}

	return result;
}

static int enable_flash_mcp7x(struct pci_dev *dev, const char *name)
{
	int result = 0;

	result = enable_flash_mcp6x_7x_common(dev, name);
	if (result)
		return result;

	/* Not sure if this is correct. No docs as usual. */
	switch (buses_supported) {
	case CHIP_BUSTYPE_LPC:
		msg_pinfo("LPC on this chipset is not supported yet.\n");
		break;
	case CHIP_BUSTYPE_SPI:
		msg_pinfo("SPI on this chipset is not supported yet.\n");
		buses_supported = CHIP_BUSTYPE_NONE;
		break;
	default:
		msg_pinfo("Something went wrong with bus type detection.\n");
		buses_supported = CHIP_BUSTYPE_NONE;
		break;
	}

	return result;
}

static int enable_flash_ht1000(struct pci_dev *dev, const char *name)
{
	uint8_t val;

	/* Set the 4MB enable bit. */
	val = pci_read_byte(dev, 0x41);
	val |= 0x0e;
	pci_write_byte(dev, 0x41, val);

	val = pci_read_byte(dev, 0x43);
	val |= (1 << 4);
	pci_write_byte(dev, 0x43, val);

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
		msg_pinfo("AMD Elan SC520 detected, but no BOOTCS. Assuming flash at 4G\n");
	}

	/* 4. Clean up */
	physunmap(mmcr, getpagesize());
	return 0;
}

#endif

/* Please keep this list alphabetically sorted by vendor/device. */
const struct penable chipset_enables[] = {
#if defined(__i386__) || defined(__x86_64__)
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
	{0x8086, 0x27bc, OK, "Intel", "NM10",		enable_flash_ich7},
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
	{0x8086, 0x8119, OK, "Intel", "Poulsbo",        enable_flash_poulsbo},
	{0x10de, 0x0030, OK, "NVIDIA", "nForce4/MCP4",  enable_flash_nvidia_nforce2},
	{0x10de, 0x0050, OK, "NVIDIA", "CK804",		enable_flash_ck804}, /* LPC */
	{0x10de, 0x0051, OK, "NVIDIA", "CK804",		enable_flash_ck804}, /* Pro */
	{0x10de, 0x0060, OK, "NVIDIA", "NForce2",       enable_flash_nvidia_nforce2},
	{0x10de, 0x00e0, OK, "NVIDIA", "NForce3",       enable_flash_nvidia_nforce2},
	/* Slave, should not be here, to fix known bug for A01. */
	{0x10de, 0x00d3, OK, "NVIDIA", "CK804",		enable_flash_ck804},
	{0x10de, 0x0260, NT, "NVIDIA", "MCP51",		enable_flash_ck804},
	{0x10de, 0x0261, NT, "NVIDIA", "MCP51",		enable_flash_ck804},
	{0x10de, 0x0262, NT, "NVIDIA", "MCP51",		enable_flash_ck804},
	{0x10de, 0x0263, NT, "NVIDIA", "MCP51",		enable_flash_ck804},
	{0x10de, 0x0360, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* M57SLI*/
	/* 10de:0361 is present in Tyan S2915 OEM systems, but not connected to
	 * the flash chip. Instead, 10de:0364 is connected to the flash chip.
	 * Until we have PCI device class matching or some fallback mechanism,
	 * this is needed to get flashrom working on Tyan S2915 and maybe other
	 * dual-MCP55 boards.
	 */
#if 0
	{0x10de, 0x0361, NT, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
#endif
	{0x10de, 0x0362, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0363, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0364, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0365, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0366, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0367, OK, "NVIDIA", "MCP55",		enable_flash_mcp55}, /* Pro */
	{0x10de, 0x03e0, NT, "NVIDIA", "MCP61",		enable_flash_mcp67},
	{0x10de, 0x03e1, NT, "NVIDIA", "MCP61",		enable_flash_mcp67},
	{0x10de, 0x03e2, NT, "NVIDIA", "MCP61",		enable_flash_mcp67},
	{0x10de, 0x03e3, NT, "NVIDIA", "MCP61",		enable_flash_mcp67},
	{0x10de, 0x0440, NT, "NVIDIA", "MCP65",		enable_flash_mcp7x},
	{0x10de, 0x0441, NT, "NVIDIA", "MCP65",		enable_flash_mcp7x},
	{0x10de, 0x0442, NT, "NVIDIA", "MCP65",		enable_flash_mcp7x},
	{0x10de, 0x0443, NT, "NVIDIA", "MCP65",		enable_flash_mcp7x},
	{0x10de, 0x0548, OK, "NVIDIA", "MCP67",		enable_flash_mcp67},
	{0x10de, 0x075c, NT, "NVIDIA", "MCP78S",	enable_flash_mcp7x},
	{0x10de, 0x075d, NT, "NVIDIA", "MCP78S",	enable_flash_mcp7x},
	{0x10de, 0x07d7, NT, "NVIDIA", "MCP73",		enable_flash_mcp7x},
	{0x10de, 0x0aac, NT, "NVIDIA", "MCP79",		enable_flash_mcp7x},
	{0x10de, 0x0aad, NT, "NVIDIA", "MCP79",		enable_flash_mcp7x},
	{0x10de, 0x0aae, NT, "NVIDIA", "MCP79",		enable_flash_mcp7x},
	{0x10de, 0x0aaf, NT, "NVIDIA", "MCP79",		enable_flash_mcp7x},
	{0x1039, 0x0496, NT, "SiS", "85C496+497",	enable_flash_sis85c496},
	{0x1039, 0x0406, NT, "SiS", "501/5101/5501",	enable_flash_sis501},
	{0x1039, 0x5511, NT, "SiS", "5511",		enable_flash_sis5511},
	{0x1039, 0x5596, NT, "SiS", "5596",		enable_flash_sis5511},
	{0x1039, 0x5571, NT, "SiS", "5571",		enable_flash_sis530},
	{0x1039, 0x5591, NT, "SiS", "5591/5592",	enable_flash_sis530},
	{0x1039, 0x5597, NT, "SiS", "5597/5598/5581/5120", enable_flash_sis530},
	{0x1039, 0x0530, NT, "SiS", "530",		enable_flash_sis530},
	{0x1039, 0x5600, NT, "SiS", "600",		enable_flash_sis530},
	{0x1039, 0x0620, NT, "SiS", "620",		enable_flash_sis530},
	{0x1039, 0x0540, NT, "SiS", "540",		enable_flash_sis540},
	{0x1039, 0x0630, NT, "SiS", "630",		enable_flash_sis540},
	{0x1039, 0x0635, NT, "SiS", "635",		enable_flash_sis540},
	{0x1039, 0x0640, NT, "SiS", "640",		enable_flash_sis540},
	{0x1039, 0x0645, NT, "SiS", "645",		enable_flash_sis540},
	{0x1039, 0x0646, NT, "SiS", "645DX",		enable_flash_sis540},
	{0x1039, 0x0648, NT, "SiS", "648",		enable_flash_sis540},
	{0x1039, 0x0650, NT, "SiS", "650",		enable_flash_sis540},
	{0x1039, 0x0651, NT, "SiS", "651",		enable_flash_sis540},
	{0x1039, 0x0655, NT, "SiS", "655",		enable_flash_sis540},
	{0x1039, 0x0730, NT, "SiS", "730",		enable_flash_sis540},
	{0x1039, 0x0733, NT, "SiS", "733",		enable_flash_sis540},
	{0x1039, 0x0735, OK, "SiS", "735",		enable_flash_sis540},
	{0x1039, 0x0740, NT, "SiS", "740",		enable_flash_sis540},
	{0x1039, 0x0745, NT, "SiS", "745",		enable_flash_sis540},
	{0x1039, 0x0746, NT, "SiS", "746",		enable_flash_sis540},
	{0x1039, 0x0748, NT, "SiS", "748",		enable_flash_sis540},
	{0x1039, 0x0755, NT, "SiS", "755",		enable_flash_sis540},
	/* VIA northbridges */
	{0x1106, 0x0585, NT, "VIA", "VT82C585VPX",	via_no_byte_merge},
	{0x1106, 0x0595, NT, "VIA", "VT82C595",		via_no_byte_merge},
	{0x1106, 0x0597, NT, "VIA", "VT82C597",		via_no_byte_merge},
	{0x1106, 0x0691, NT, "VIA", "VT82C69x",		via_no_byte_merge}, /* 691, 693a, 694t, 694x checked */
	{0x1106, 0x0601, NT, "VIA", "VT8601/VT8601A",	via_no_byte_merge},
	{0x1106, 0x8601, NT, "VIA", "VT8601T",		via_no_byte_merge},
	/* VIA southbridges */
	{0x1106, 0x8324, OK, "VIA", "CX700",		enable_flash_vt823x},
	{0x1106, 0x8231, NT, "VIA", "VT8231",		enable_flash_vt823x},
	{0x1106, 0x3074, NT, "VIA", "VT8233",		enable_flash_vt823x},
	{0x1106, 0x3147, OK, "VIA", "VT8233A",		enable_flash_vt823x},
	{0x1106, 0x3177, OK, "VIA", "VT8235",		enable_flash_vt823x},
	{0x1106, 0x3227, OK, "VIA", "VT8237",		enable_flash_vt823x},
	{0x1106, 0x3337, OK, "VIA", "VT8237A",		enable_flash_vt823x},
	{0x1106, 0x3372, OK, "VIA", "VT8237S",		enable_flash_vt8237s_spi},
	{0x1106, 0x8353, OK, "VIA", "VX800",		enable_flash_vt8237s_spi},
	{0x1106, 0x0596, OK, "VIA", "VT82C596",		enable_flash_amd8111},
	{0x1106, 0x0586, OK, "VIA", "VT82C586A/B",	enable_flash_amd8111},
	{0x1106, 0x0686, NT, "VIA", "VT82C686A/B",	enable_flash_amd8111},
#endif
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
		if (!dev)
			continue;
		if (ret != -2) {
			msg_pinfo("WARNING: unexpected second chipset match: "
			       "\"%s %s\"\nignoring, please report lspci and "
			       "board URL to flashrom@flashrom.org!\n",
				chipset_enables[i].vendor_name,
					chipset_enables[i].device_name);
			continue;
		}
		msg_pinfo("Found chipset \"%s %s\", enabling flash write... ",
		       chipset_enables[i].vendor_name,
		       chipset_enables[i].device_name);
		msg_pdbg("chipset PCI ID is %04x:%04x, ",
			 chipset_enables[i].vendor_id,
			 chipset_enables[i].device_id);

		ret = chipset_enables[i].doit(dev,
					      chipset_enables[i].device_name);
		if (ret == NOT_DONE_YET) {
			ret = -2;
			msg_pinfo("OK - searching further chips.\n");
		} else if (ret < 0)
			msg_pinfo("FAILED!\n");
		else if(ret == 0)
			msg_pinfo("OK.\n");
	}

	msg_pinfo("This chipset supports the following protocols: %s.\n",
	       flashbuses_to_text(buses_supported));

	return ret;
}
