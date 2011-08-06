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
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include "flash.h"
#include "programmer.h"

#define NOT_DONE_YET 1

#if defined(__i386__) || defined(__x86_64__)

static int enable_flash_ali_m1533(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;

	/*
	 * ROM Write enable, 0xFFFC0000-0xFFFDFFFF and
	 * 0xFFFE0000-0xFFFFFFFF ROM select enable.
	 */
	tmp = pci_read_byte(dev, 0x47);
	tmp |= 0x46;
	rpci_write_byte(dev, 0x47, tmp);

	return 0;
}

static int enable_flash_sis85c496(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;

	tmp = pci_read_byte(dev, 0xd0);
	tmp |= 0xf8;
	rpci_write_byte(dev, 0xd0, tmp);

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
	rpci_write_byte(dev, 0x40, new);
	newer = pci_read_byte(dev, 0x40);
	if (newer != new) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", 0x40, new, name);
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
	rpci_write_byte(sbdev, 0x45, new);
	newer = pci_read_byte(sbdev, 0x45);
	if (newer != new) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", 0x45, new, name);
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
	rpci_write_byte(sbdev, 0x45, new);
	newer = pci_read_byte(sbdev, 0x45);
	if (newer != new) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", 0x45, new, name);
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

	buses_supported = BUS_PARALLEL;

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

	rpci_write_word(dev, xbcs, new);

	if (pci_read_word(dev, xbcs) != new) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", xbcs, new, name);
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

	/*
	 * Quote from the 6 Series datasheet (Document Number: 324645-004):
	 * "Bit 5: SMM BIOS Write Protect Disable (SMM_BWP)
	 * 1 = BIOS region SMM protection is enabled.
	 * The BIOS Region is not writable unless all processors are in SMM."
	 * In earlier chipsets this bit is reserved.
	 */
	if (old & (1 << 5))
		msg_pinfo("WARNING: BIOS region SMM protection is enabled!\n");

	new = old | 1;
	if (new == old)
		return 0;

	rpci_write_byte(dev, bios_cntl, new);

	if (pci_read_byte(dev, bios_cntl) != new) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", bios_cntl, new, name);
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
	buses_supported = BUS_FWH;
	return enable_flash_ich(dev, name, 0x4e);
}

static int enable_flash_ich_dc(struct pci_dev *dev, const char *name)
{
	uint32_t fwh_conf;
	char *idsel = NULL;
	int i, tmp, max_decode_fwh_idsel = 0, max_decode_fwh_decode = 0;
	int contiguous = 1;

	idsel = extract_programmer_param("fwh_idsel");
	if (idsel && strlen(idsel)) {
		uint64_t fwh_idsel_old, fwh_idsel;
		errno = 0;
		/* Base 16, nothing else makes sense. */
		fwh_idsel = (uint64_t)strtoull(idsel, NULL, 16);
		if (errno) {
			msg_perr("Error: fwh_idsel= specified, but value could "
				 "not be converted.\n");
			goto idsel_garbage_out;
		}
		if (fwh_idsel & 0xffff000000000000ULL) {
			msg_perr("Error: fwh_idsel= specified, but value had "
				 "unusued bits set.\n");
			goto idsel_garbage_out;
		}
		fwh_idsel_old = pci_read_long(dev, 0xd0);
		fwh_idsel_old <<= 16;
		fwh_idsel_old |= pci_read_word(dev, 0xd4);
		msg_pdbg("\nSetting IDSEL from 0x%012" PRIx64 " to "
			 "0x%012" PRIx64 " for top 16 MB.", fwh_idsel_old,
			 fwh_idsel);
		rpci_write_long(dev, 0xd0, (fwh_idsel >> 16) & 0xffffffff);
		rpci_write_word(dev, 0xd4, fwh_idsel & 0xffff);
		/* FIXME: Decode settings are not changed. */
	} else if (idsel) {
		msg_perr("Error: fwh_idsel= specified, but no value given.\n");
idsel_garbage_out:	
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
	buses_supported = BUS_FWH;
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
		rpci_write_byte(dev, 0xd9, new);

	buses_supported = BUS_FWH;
	return 0;
}

#define ICH_STRAP_RSVD 0x00
#define ICH_STRAP_SPI  0x01
#define ICH_STRAP_PCI  0x02
#define ICH_STRAP_LPC  0x03

static int enable_flash_vt8237s_spi(struct pci_dev *dev, const char *name)
{
	/* Do we really need no write enable? */
	return via_init_spi(dev);
}

static int enable_flash_ich_dc_spi(struct pci_dev *dev, const char *name,
				   int ich_generation)
{
	int ret;
	uint8_t bbs, buc;
	uint32_t tmp, gcs;
	void *rcrb;

	/*
	 * TODO: These names are incorrect for EP80579. For that, the solution
	 * would look like the commented line below.
	 */
	// static const char *straps_names[] = {"SPI", "reserved", "reserved", "LPC" };
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

	buses_supported = BUS_FWH;
	if (ich_generation == 7) {
		if (bbs == ICH_STRAP_LPC) {
			/* No further SPI initialization required */
			return ret;
		} else {
			/* Disable LPC/FWH if strapped to PCI or SPI */
			buses_supported = 0;
		}
	}

	/* This adds BUS_SPI */
	if (ich_init_spi(dev, tmp, rcrb, ich_generation) != 0) {
		if (!ret)
			ret = ERROR_NONFATAL;
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

static int via_no_byte_merge(struct pci_dev *dev, const char *name)
{
	uint8_t val;

	val = pci_read_byte(dev, 0x71);
	if (val & 0x40) {
		msg_pdbg("Disabling byte merging\n");
		val &= ~0x40;
		rpci_write_byte(dev, 0x71, val);
	}
	return NOT_DONE_YET;	/* need to find south bridge, too */
}

static int enable_flash_vt823x(struct pci_dev *dev, const char *name)
{
	uint8_t val;

	/* Enable ROM decode range (1MB) FFC00000 - FFFFFFFF. */
	rpci_write_byte(dev, 0x41, 0x7f);

	/* ROM write enable */
	val = pci_read_byte(dev, 0x40);
	val |= 0x10;
	rpci_write_byte(dev, 0x40, val);

	if (pci_read_byte(dev, 0x40) != val) {
		msg_pinfo("\nWARNING: Failed to enable flash write on \"%s\"\n",
			  name);
		return -1;
	}

	if (dev->device_id == 0x3227) { /* VT8237R */
		/* All memory cycles, not just ROM ones, go to LPC. */
		val = pci_read_byte(dev, 0x59);
		val &= ~0x80;
		rpci_write_byte(dev, 0x59, val);
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

	buses_supported = BUS_PARALLEL;
	/* Decode 0x000E0000-0x000FFFFF (128 kB), not just 64 kB, and
	 * decode 0xFF000000-0xFFFFFFFF (16 MB), not just 256 kB.
	 * FIXME: Should we really touch the low mapping below 1 MB? Flashrom
	 * ignores that region completely.
	 * Make the configured ROM areas writable.
	 */
	reg8 = pci_read_byte(dev, ROM_AT_LOGIC_CONTROL_REG);
	reg8 |= LOWER_ROM_ADDRESS_RANGE;
	reg8 |= UPPER_ROM_ADDRESS_RANGE;
	reg8 |= ROM_WRITE_ENABLE;
	rpci_write_byte(dev, ROM_AT_LOGIC_CONTROL_REG, reg8);

	/* Set positive decode on ROM. */
	reg8 = pci_read_byte(dev, DECODE_CONTROL_REG2);
	reg8 |= BIOS_ROM_POSITIVE_DECODE;
	rpci_write_byte(dev, DECODE_CONTROL_REG2, reg8);

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

/*
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

	rpci_write_byte(dev, 0x52, 0xee);

	new = pci_read_byte(dev, 0x52);

	if (new != 0xee) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", 0x52, new, name);
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
		rpci_write_byte(dev, 0x43, new);
		if (pci_read_byte(dev, 0x43) != new) {
			msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
				  "(WARNING ONLY).\n", 0x43, new, name);
		}
	}

	/* Enable 'ROM write' bit. */
	old = pci_read_byte(dev, 0x40);
	new = old | 0x01;
	if (new == old)
		return 0;
	rpci_write_byte(dev, 0x40, new);

	if (pci_read_byte(dev, 0x40) != new) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", 0x40, new, name);
		return -1;
	}

	return 0;
}

static int enable_flash_sb600(struct pci_dev *dev, const char *name)
{
	uint32_t prot;
	uint8_t reg;
	int ret;

	/* Clear ROM protect 0-3. */
	for (reg = 0x50; reg < 0x60; reg += 4) {
		prot = pci_read_long(dev, reg);
		/* No protection flags for this region?*/
		if ((prot & 0x3) == 0)
			continue;
		msg_pinfo("SB600 %s%sprotected from 0x%08x to 0x%08x\n",
			  (prot & 0x1) ? "write " : "",
			  (prot & 0x2) ? "read " : "",
			  (prot & 0xfffff800),
			  (prot & 0xfffff800) + (((prot & 0x7fc) << 8) | 0x3ff));
		prot &= 0xfffffffc;
		rpci_write_byte(dev, reg, prot);
		prot = pci_read_long(dev, reg);
		if (prot & 0x3)
			msg_perr("SB600 %s%sunprotect failed from 0x%08x to 0x%08x\n",
				 (prot & 0x1) ? "write " : "",
				 (prot & 0x2) ? "read " : "",
				 (prot & 0xfffff800),
				 (prot & 0xfffff800) + (((prot & 0x7fc) << 8) | 0x3ff));
	}

	buses_supported = BUS_LPC | BUS_FWH;

	ret = sb600_probe_spi(dev);

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

	return ret;
}

static int enable_flash_nvidia_nforce2(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;

	rpci_write_byte(dev, 0x92, 0);

	tmp = pci_read_byte(dev, 0x6d);
	tmp |= 0x01;
	rpci_write_byte(dev, 0x6d, tmp);

	return 0;
}

static int enable_flash_ck804(struct pci_dev *dev, const char *name)
{
	uint8_t old, new;

	pci_write_byte(dev, 0x92, 0x00);
	if (pci_read_byte(dev, 0x92) != 0x00) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", 0x92, 0x00, name);
	}

	old = pci_read_byte(dev, 0x88);
	new = old | 0xc0;
	if (new != old) {
		rpci_write_byte(dev, 0x88, new);
		if (pci_read_byte(dev, 0x88) != new) {
			msg_pinfo("Setting register to set 0x%x to 0x%x on %s "
				  "failed (WARNING ONLY).\n", 0x88, new, name);
		}
	}

	old = pci_read_byte(dev, 0x6d);
	new = old | 0x01;
	if (new == old)
		return 0;
	rpci_write_byte(dev, 0x6d, new);

	if (pci_read_byte(dev, 0x6d) != new) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", 0x6d, new, name);
		return -1;
	}

	return 0;
}

static int enable_flash_osb4(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;

	buses_supported = BUS_PARALLEL;

	tmp = INB(0xc06);
	tmp |= 0x1;
	OUTB(tmp, 0xc06);

	tmp = INB(0xc6f);
	tmp |= 0x40;
	OUTB(tmp, 0xc6f);

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
	rpci_write_byte(smbusdev, 0x79, tmp);

	/* Change southbridge. */
	tmp = pci_read_byte(dev, 0x48);
	tmp |= 0x21;
	rpci_write_byte(dev, 0x48, tmp);

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
	rpci_write_byte(dev, 0x88, val);
	val = pci_read_byte(dev, 0x8c);
	val |= 0xff;		/* 1M */
	rpci_write_byte(dev, 0x8c, val);
	wordval = pci_read_word(dev, 0x90);
	wordval |= 0x7fff;	/* 16M */
	rpci_write_word(dev, 0x90, wordval);

	old = pci_read_byte(dev, 0x6d);
	new = old | 0x01;
	if (new == old)
		return 0;
	rpci_write_byte(dev, 0x6d, new);

	if (pci_read_byte(dev, 0x6d) != new) {
		msg_pinfo("Setting register 0x%x to 0x%x on %s failed "
			  "(WARNING ONLY).\n", 0x6d, new, name);
		return -1;
	}

	return 0;
}

/*
 * The MCP6x/MCP7x code is based on cleanroom reverse engineering.
 * It is assumed that LPC chips need the MCP55 code and SPI chips need the
 * code provided in enable_flash_mcp6x_7x_common.
 */
static int enable_flash_mcp6x_7x(struct pci_dev *dev, const char *name)
{
	int ret = 0, want_spi = 0;
	uint8_t val;

	msg_pinfo("This chipset is not really supported yet. Guesswork...\n");

	/* dev is the ISA bridge. No idea what the stuff below does. */
	val = pci_read_byte(dev, 0x8a);
	msg_pdbg("ISA/LPC bridge reg 0x8a contents: 0x%02x, bit 6 is %i, bit 5 "
		 "is %i\n", val, (val >> 6) & 0x1, (val >> 5) & 0x1);

	switch ((val >> 5) & 0x3) {
	case 0x0:
		ret = enable_flash_mcp55(dev, name);
		buses_supported = BUS_LPC;
		msg_pdbg("Flash bus type is LPC\n");
		break;
	case 0x2:
		want_spi = 1;
		/* SPI is added in mcp6x_spi_init if it works.
		 * Do we really want to disable LPC in this case?
		 */
		buses_supported = BUS_NONE;
		msg_pdbg("Flash bus type is SPI\n");
		msg_pinfo("SPI on this chipset is WIP. Please report any "
			  "success or failure by mailing us the verbose "
			  "output to flashrom@flashrom.org, thanks!\n");
		break;
	default:
		/* Should not happen. */
		buses_supported = BUS_NONE;
		msg_pdbg("Flash bus type is unknown (none)\n");
		msg_pinfo("Something went wrong with bus type detection.\n");
		goto out_msg;
		break;
	}

	/* Force enable SPI and disable LPC? Not a good idea. */
#if 0
	val |= (1 << 6);
	val &= ~(1 << 5);
	rpci_write_byte(dev, 0x8a, val);
#endif

	if (mcp6x_spi_init(want_spi))
		ret = 1;

out_msg:
	msg_pinfo("Please send the output of \"flashrom -V\" to "
		  "flashrom@flashrom.org with\n"
		  "your board name: flashrom -V as the subject to help us "
		  "finish support for your\n"
		  "chipset. Thanks.\n");

	return ret;
}

static int enable_flash_ht1000(struct pci_dev *dev, const char *name)
{
	uint8_t val;

	/* Set the 4MB enable bit. */
	val = pci_read_byte(dev, 0x41);
	val |= 0x0e;
	rpci_write_byte(dev, 0x41, val);

	val = pci_read_byte(dev, 0x43);
	val |= (1 << 4);
	rpci_write_byte(dev, 0x43, val);

	return 0;
}

/*
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
		msg_pinfo("AMD Elan SC520 detected, but no BOOTCS. "
			  "Assuming flash at 4G\n");
	}

	/* 4. Clean up */
	physunmap(mmcr, getpagesize());
	return 0;
}

#endif

/* Please keep this list numerically sorted by vendor/device ID. */
const struct penable chipset_enables[] = {
#if defined(__i386__) || defined(__x86_64__)
	{0x1002, 0x4377, OK, "ATI", "SB400",		enable_flash_sb400},
	{0x1002, 0x438d, OK, "AMD", "SB600",		enable_flash_sb600},
	{0x1002, 0x439d, OK, "AMD", "SB700/SB710/SB750/SB850", enable_flash_sb600},
	{0x100b, 0x0510, NT, "AMD", "SC1100",		enable_flash_sc1100},
	{0x1022, 0x2080, OK, "AMD", "CS5536",		enable_flash_cs5536},
	{0x1022, 0x2090, OK, "AMD", "CS5536",		enable_flash_cs5536},
	{0x1022, 0x3000, OK, "AMD", "Elan SC520",	get_flashbase_sc520},
	{0x1022, 0x7440, OK, "AMD", "AMD-768",		enable_flash_amd8111},
	{0x1022, 0x7468, OK, "AMD", "AMD8111",		enable_flash_amd8111},
	{0x1039, 0x0406, NT, "SiS", "501/5101/5501",	enable_flash_sis501},
	{0x1039, 0x0496, NT, "SiS", "85C496+497",	enable_flash_sis85c496},
	{0x1039, 0x0530, NT, "SiS", "530",		enable_flash_sis530},
	{0x1039, 0x0540, NT, "SiS", "540",		enable_flash_sis540},
	{0x1039, 0x0620, NT, "SiS", "620",		enable_flash_sis530},
	{0x1039, 0x0630, NT, "SiS", "630",		enable_flash_sis540},
	{0x1039, 0x0635, NT, "SiS", "635",		enable_flash_sis540},
	{0x1039, 0x0640, NT, "SiS", "640",		enable_flash_sis540},
	{0x1039, 0x0645, NT, "SiS", "645",		enable_flash_sis540},
	{0x1039, 0x0646, OK, "SiS", "645DX",		enable_flash_sis540},
	{0x1039, 0x0648, NT, "SiS", "648",		enable_flash_sis540},
	{0x1039, 0x0650, NT, "SiS", "650",		enable_flash_sis540},
	{0x1039, 0x0651, OK, "SiS", "651",		enable_flash_sis540},
	{0x1039, 0x0655, NT, "SiS", "655",		enable_flash_sis540},
	{0x1039, 0x0661, OK, "SiS", "661",		enable_flash_sis540},
	{0x1039, 0x0730, NT, "SiS", "730",		enable_flash_sis540},
	{0x1039, 0x0733, NT, "SiS", "733",		enable_flash_sis540},
	{0x1039, 0x0735, OK, "SiS", "735",		enable_flash_sis540},
	{0x1039, 0x0740, NT, "SiS", "740",		enable_flash_sis540},
	{0x1039, 0x0741, OK, "SiS", "741",		enable_flash_sis540},
	{0x1039, 0x0745, OK, "SiS", "745",		enable_flash_sis540},
	{0x1039, 0x0746, NT, "SiS", "746",		enable_flash_sis540},
	{0x1039, 0x0748, NT, "SiS", "748",		enable_flash_sis540},
	{0x1039, 0x0755, NT, "SiS", "755",		enable_flash_sis540},
	{0x1039, 0x5511, NT, "SiS", "5511",		enable_flash_sis5511},
	{0x1039, 0x5571, NT, "SiS", "5571",		enable_flash_sis530},
	{0x1039, 0x5591, NT, "SiS", "5591/5592",	enable_flash_sis530},
	{0x1039, 0x5596, NT, "SiS", "5596",		enable_flash_sis5511},
	{0x1039, 0x5597, NT, "SiS", "5597/5598/5581/5120", enable_flash_sis530},
	{0x1039, 0x5600, NT, "SiS", "600",		enable_flash_sis530},
	{0x1078, 0x0100, OK, "AMD", "CS5530(A)",	enable_flash_cs5530},
	{0x10b9, 0x1533, OK, "ALi", "M1533",		enable_flash_ali_m1533},
	{0x10de, 0x0030, OK, "NVIDIA", "nForce4/MCP4",	enable_flash_nvidia_nforce2},
	{0x10de, 0x0050, OK, "NVIDIA", "CK804",		enable_flash_ck804}, /* LPC */
	{0x10de, 0x0051, OK, "NVIDIA", "CK804",		enable_flash_ck804}, /* Pro */
	{0x10de, 0x0060, OK, "NVIDIA", "NForce2",	enable_flash_nvidia_nforce2},
	{0x10de, 0x00e0, OK, "NVIDIA", "NForce3",	enable_flash_nvidia_nforce2},
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
	{0x10de, 0x03e0, NT, "NVIDIA", "MCP61",		enable_flash_mcp6x_7x},
	{0x10de, 0x03e1, NT, "NVIDIA", "MCP61",		enable_flash_mcp6x_7x},
	{0x10de, 0x03e2, NT, "NVIDIA", "MCP61",		enable_flash_mcp6x_7x},
	{0x10de, 0x03e3, NT, "NVIDIA", "MCP61",		enable_flash_mcp6x_7x},
	{0x10de, 0x0440, NT, "NVIDIA", "MCP65",		enable_flash_mcp6x_7x},
	{0x10de, 0x0441, NT, "NVIDIA", "MCP65",		enable_flash_mcp6x_7x},
	{0x10de, 0x0442, NT, "NVIDIA", "MCP65",		enable_flash_mcp6x_7x},
	{0x10de, 0x0443, NT, "NVIDIA", "MCP65",		enable_flash_mcp6x_7x},
	{0x10de, 0x0548, OK, "NVIDIA", "MCP67",		enable_flash_mcp6x_7x},
	{0x10de, 0x075c, NT, "NVIDIA", "MCP78S",	enable_flash_mcp6x_7x},
	{0x10de, 0x075d, NT, "NVIDIA", "MCP78S",	enable_flash_mcp6x_7x},
	{0x10de, 0x07d7, NT, "NVIDIA", "MCP73",		enable_flash_mcp6x_7x},
	{0x10de, 0x0aac, NT, "NVIDIA", "MCP79",		enable_flash_mcp6x_7x},
	{0x10de, 0x0aad, NT, "NVIDIA", "MCP79",		enable_flash_mcp6x_7x},
	{0x10de, 0x0aae, NT, "NVIDIA", "MCP79",		enable_flash_mcp6x_7x},
	{0x10de, 0x0aaf, NT, "NVIDIA", "MCP79",		enable_flash_mcp6x_7x},
	/* VIA northbridges */
	{0x1106, 0x0585, NT, "VIA", "VT82C585VPX",	via_no_byte_merge},
	{0x1106, 0x0595, NT, "VIA", "VT82C595",		via_no_byte_merge},
	{0x1106, 0x0597, NT, "VIA", "VT82C597",		via_no_byte_merge},
	{0x1106, 0x0601, NT, "VIA", "VT8601/VT8601A",	via_no_byte_merge},
	{0x1106, 0x0691, NT, "VIA", "VT82C69x",		via_no_byte_merge}, /* 691, 693a, 694t, 694x checked */
	{0x1106, 0x8601, NT, "VIA", "VT8601T",		via_no_byte_merge},
	/* VIA southbridges */
	{0x1106, 0x0586, OK, "VIA", "VT82C586A/B",	enable_flash_amd8111},
	{0x1106, 0x0596, OK, "VIA", "VT82C596",		enable_flash_amd8111},
	{0x1106, 0x0686, NT, "VIA", "VT82C686A/B",	enable_flash_amd8111},
	{0x1106, 0x3074, NT, "VIA", "VT8233",		enable_flash_vt823x},
	{0x1106, 0x3147, OK, "VIA", "VT8233A",		enable_flash_vt823x},
	{0x1106, 0x3177, OK, "VIA", "VT8235",		enable_flash_vt823x},
	{0x1106, 0x3227, OK, "VIA", "VT8237",		enable_flash_vt823x},
	{0x1106, 0x3337, OK, "VIA", "VT8237A",		enable_flash_vt823x},
	{0x1106, 0x3372, OK, "VIA", "VT8237S",		enable_flash_vt8237s_spi},
	{0x1106, 0x8231, NT, "VIA", "VT8231",		enable_flash_vt823x},
	{0x1106, 0x8324, OK, "VIA", "CX700",		enable_flash_vt823x},
	{0x1106, 0x8353, OK, "VIA", "VX800/VX820",	enable_flash_vt8237s_spi},
	{0x1106, 0x8409, OK, "VIA", "VX855/VX875",	enable_flash_vt823x},
	{0x1166, 0x0200, OK, "Broadcom", "OSB4",	enable_flash_osb4},
	{0x1166, 0x0205, OK, "Broadcom", "HT-1000",	enable_flash_ht1000},
	{0x8086, 0x122e, OK, "Intel", "PIIX",		enable_flash_piix4},
	{0x8086, 0x1234, NT, "Intel", "MPIIX",		enable_flash_piix4},
	{0x8086, 0x1c44, NT, "Intel", "Z68",		enable_flash_ich10},
	{0x8086, 0x1c46, NT, "Intel", "P67",		enable_flash_ich10},
	{0x8086, 0x1c47, NT, "Intel", "UM67",		enable_flash_ich10},
	{0x8086, 0x1c49, NT, "Intel", "HM65",		enable_flash_ich10},
	{0x8086, 0x1c4a, NT, "Intel", "H67",		enable_flash_ich10},
	{0x8086, 0x1c4b, NT, "Intel", "HM67",		enable_flash_ich10},
	{0x8086, 0x1c4c, NT, "Intel", "Q65",		enable_flash_ich10},
	{0x8086, 0x1c4d, NT, "Intel", "QS67",		enable_flash_ich10},
	{0x8086, 0x1c4e, NT, "Intel", "Q67",		enable_flash_ich10},
	{0x8086, 0x1c4f, NT, "Intel", "QM67",		enable_flash_ich10},
	{0x8086, 0x1c50, NT, "Intel", "B65",		enable_flash_ich10},
	{0x8086, 0x1c52, NT, "Intel", "C202",		enable_flash_ich10},
	{0x8086, 0x1c54, NT, "Intel", "C204",		enable_flash_ich10},
	{0x8086, 0x1c56, NT, "Intel", "C206",		enable_flash_ich10},
	{0x8086, 0x1c5c, NT, "Intel", "H61",		enable_flash_ich10},
	{0x8086, 0x2410, OK, "Intel", "ICH",		enable_flash_ich_4e},
	{0x8086, 0x2420, OK, "Intel", "ICH0",		enable_flash_ich_4e},
	{0x8086, 0x2440, OK, "Intel", "ICH2",		enable_flash_ich_4e},
	{0x8086, 0x244c, OK, "Intel", "ICH2-M",		enable_flash_ich_4e},
	{0x8086, 0x2450, NT, "Intel", "C-ICH",		enable_flash_ich_4e},
	{0x8086, 0x2480, OK, "Intel", "ICH3-S",		enable_flash_ich_4e},
	{0x8086, 0x248c, OK, "Intel", "ICH3-M",		enable_flash_ich_4e},
	{0x8086, 0x24c0, OK, "Intel", "ICH4/ICH4-L",	enable_flash_ich_4e},
	{0x8086, 0x24cc, OK, "Intel", "ICH4-M",		enable_flash_ich_4e},
	{0x8086, 0x24d0, OK, "Intel", "ICH5/ICH5R",	enable_flash_ich_4e},
	{0x8086, 0x25a1, OK, "Intel", "6300ESB",	enable_flash_ich_4e},
	{0x8086, 0x2640, OK, "Intel", "ICH6/ICH6R",	enable_flash_ich_dc},
	{0x8086, 0x2641, OK, "Intel", "ICH6-M",		enable_flash_ich_dc},
	{0x8086, 0x2642, NT, "Intel", "ICH6W/ICH6RW",	enable_flash_ich_dc},
	{0x8086, 0x2670, OK, "Intel", "631xESB/632xESB/3100", enable_flash_ich_dc},
	{0x8086, 0x27b0, OK, "Intel", "ICH7DH",		enable_flash_ich7},
	{0x8086, 0x27b8, OK, "Intel", "ICH7/ICH7R",	enable_flash_ich7},
	{0x8086, 0x27b9, OK, "Intel", "ICH7M",		enable_flash_ich7},
	{0x8086, 0x27bc, OK, "Intel", "NM10",		enable_flash_ich7},
	{0x8086, 0x27bd, OK, "Intel", "ICH7MDH",	enable_flash_ich7},
	{0x8086, 0x2810, OK, "Intel", "ICH8/ICH8R",	enable_flash_ich8},
	{0x8086, 0x2811, OK, "Intel", "ICH8M-E",	enable_flash_ich8},
	{0x8086, 0x2812, OK, "Intel", "ICH8DH",		enable_flash_ich8},
	{0x8086, 0x2814, OK, "Intel", "ICH8DO",		enable_flash_ich8},
	{0x8086, 0x2815, OK, "Intel", "ICH8M",		enable_flash_ich8},
	{0x8086, 0x2910, OK, "Intel", "ICH9 Engineering Sample", enable_flash_ich9},
	{0x8086, 0x2912, OK, "Intel", "ICH9DH",		enable_flash_ich9},
	{0x8086, 0x2914, OK, "Intel", "ICH9DO",		enable_flash_ich9},
	{0x8086, 0x2916, OK, "Intel", "ICH9R",		enable_flash_ich9},
	{0x8086, 0x2917, OK, "Intel", "ICH9M-E",	enable_flash_ich9},
	{0x8086, 0x2918, OK, "Intel", "ICH9",		enable_flash_ich9},
	{0x8086, 0x2919, OK, "Intel", "ICH9M",		enable_flash_ich9},
	{0x8086, 0x3a10, NT, "Intel", "ICH10R Engineering Sample", enable_flash_ich10},
	{0x8086, 0x3a14, OK, "Intel", "ICH10DO",	enable_flash_ich10},
	{0x8086, 0x3a16, OK, "Intel", "ICH10R",		enable_flash_ich10},
	{0x8086, 0x3a18, OK, "Intel", "ICH10",		enable_flash_ich10},
	{0x8086, 0x3a1a, OK, "Intel", "ICH10D",		enable_flash_ich10},
	{0x8086, 0x3a1e, NT, "Intel", "ICH10 Engineering Sample", enable_flash_ich10},
	{0x8086, 0x3b00, NT, "Intel", "3400 Desktop",	enable_flash_ich10},
	{0x8086, 0x3b01, NT, "Intel", "3400 Mobile",	enable_flash_ich10},
	{0x8086, 0x3b02, NT, "Intel", "P55",		enable_flash_ich10},
	{0x8086, 0x3b03, NT, "Intel", "PM55",		enable_flash_ich10},
	{0x8086, 0x3b06, NT, "Intel", "H55",		enable_flash_ich10},
	{0x8086, 0x3b07, OK, "Intel", "QM57",		enable_flash_ich10},
	{0x8086, 0x3b08, NT, "Intel", "H57",		enable_flash_ich10},
	{0x8086, 0x3b09, NT, "Intel", "HM55",		enable_flash_ich10},
	{0x8086, 0x3b0a, NT, "Intel", "Q57",		enable_flash_ich10},
	{0x8086, 0x3b0b, NT, "Intel", "HM57",		enable_flash_ich10},
	{0x8086, 0x3b0d, NT, "Intel", "3400 Mobile SFF", enable_flash_ich10},
	{0x8086, 0x3b0e, NT, "Intel", "B55",		enable_flash_ich10},
	{0x8086, 0x3b0f, OK, "Intel", "QS57",		enable_flash_ich10},
	{0x8086, 0x3b12, NT, "Intel", "3400",		enable_flash_ich10},
	{0x8086, 0x3b14, NT, "Intel", "3420",		enable_flash_ich10},
	{0x8086, 0x3b16, NT, "Intel", "3450",		enable_flash_ich10},
	{0x8086, 0x3b1e, NT, "Intel", "B55",		enable_flash_ich10},
	{0x8086, 0x5031, OK, "Intel", "EP80579",	enable_flash_ich7},
	{0x8086, 0x7000, OK, "Intel", "PIIX3",		enable_flash_piix4},
	{0x8086, 0x7110, OK, "Intel", "PIIX4/4E/4M",	enable_flash_piix4},
	{0x8086, 0x7198, OK, "Intel", "440MX",		enable_flash_piix4},
	{0x8086, 0x8119, OK, "Intel", "SCH Poulsbo",	enable_flash_poulsbo},
	{0x8086, 0x8186, NT, "Intel", "Atom E6xx(T)/Tunnel Creek", enable_flash_poulsbo},
#endif
	{},
};

int chipset_flash_enable(void)
{
	struct pci_dev *dev = NULL;
	int ret = -2;		/* Nothing! */
	int i;
	char *s;

	/* Now let's try to find the chipset we have... */
	for (i = 0; chipset_enables[i].vendor_name != NULL; i++) {
		dev = pci_dev_find(chipset_enables[i].vendor_id,
				   chipset_enables[i].device_id);
		if (!dev)
			continue;
		if (ret != -2) {
			msg_pinfo("WARNING: unexpected second chipset match: "
				    "\"%s %s\"\n"
				  "ignoring, please report lspci and board URL "
				    "to flashrom@flashrom.org\n"
				  "with \'CHIPSET: your board name\' in the "
				    "subject line.\n",
				chipset_enables[i].vendor_name,
					chipset_enables[i].device_name);
			continue;
		}
		msg_pinfo("Found chipset \"%s %s\"",
			  chipset_enables[i].vendor_name,
			  chipset_enables[i].device_name);
		msg_pdbg(" with PCI ID %04x:%04x",
			 chipset_enables[i].vendor_id,
			 chipset_enables[i].device_id);
		msg_pinfo(". ");

		if (chipset_enables[i].status == NT) {
			msg_pinfo("\nThis chipset is marked as untested. If "
				  "you are using an up-to-date version\nof "
				  "flashrom please email a report to "
				  "flashrom@flashrom.org including a\nverbose "
				  "(-V) log. Thank you!\n");
		}
		msg_pinfo("Enabling flash write... ");
		ret = chipset_enables[i].doit(dev,
					      chipset_enables[i].device_name);
		if (ret == NOT_DONE_YET) {
			ret = -2;
			msg_pinfo("OK - searching further chips.\n");
		} else if (ret < 0)
			msg_pinfo("FAILED!\n");
		else if (ret == 0)
			msg_pinfo("OK.\n");
		else if (ret == ERROR_NONFATAL)
			msg_pinfo("PROBLEMS, continuing anyway\n");
	}

	s = flashbuses_to_text(buses_supported);
	msg_pinfo("This chipset supports the following protocols: %s.\n", s);
	free(s);

	return ret;
}
