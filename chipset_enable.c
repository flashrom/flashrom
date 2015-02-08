/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2005-2009 coresystems GmbH
 * Copyright (C) 2006 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2007,2008,2009 Carl-Daniel Hailfinger
 * Copyright (C) 2009 Kontron Modular Computers GmbH
 * Copyright (C) 2011, 2012 Stefan Tauner
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
#include "hwaccess.h"

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

static int enable_flash_rdc_r8610(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;

	/* enable ROMCS for writes */
	tmp = pci_read_byte(dev, 0x43);
	tmp |= 0x80;
	pci_write_byte(dev, 0x43, tmp);

	/* read the bootstrapping register */
	tmp = pci_read_byte(dev, 0x40) & 0x3;
	switch (tmp) {
	case 3:
		internal_buses_supported = BUS_FWH;
		break;
	case 2:
		internal_buses_supported = BUS_LPC;
		break;
	default:
		internal_buses_supported = BUS_PARALLEL;
		break;
	}

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
	#define SIS_MAPREG 0x40
	uint8_t new, newer;

	/* Extended BIOS enable = 1, Lower BIOS Enable = 1 */
	/* This is 0xFFF8000~0xFFFF0000 decoding on SiS 540/630. */
	new = pci_read_byte(dev, SIS_MAPREG);
	new &= (~0x04); /* No idea why we clear bit 2. */
	new |= 0xb; /* 0x3 for some chipsets, bit 7 seems to be don't care. */
	rpci_write_byte(dev, SIS_MAPREG, new);
	newer = pci_read_byte(dev, SIS_MAPREG);
	if (newer != new) { /* FIXME: share this with other code? */
		msg_pinfo("Setting register 0x%x to 0x%02x on %s failed (WARNING ONLY).\n",
			  SIS_MAPREG, new, name);
		msg_pinfo("Stuck at 0x%02x.\n", newer);
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

static int enable_flash_sis5x0(struct pci_dev *dev, const char *name, uint8_t dis_mask, uint8_t en_mask)
{
	#define SIS_REG 0x45
	uint8_t new, newer;
	int ret = 0;
	struct pci_dev *sbdev;

	sbdev = find_southbridge(dev->vendor_id, name);
	if (!sbdev)
		return -1;

	ret = enable_flash_sis_mapping(sbdev, name);

	new = pci_read_byte(sbdev, SIS_REG);
	new &= (~dis_mask);
	new |= en_mask;
	rpci_write_byte(sbdev, SIS_REG, new);
	newer = pci_read_byte(sbdev, SIS_REG);
	if (newer != new) { /* FIXME: share this with other code? */
		msg_pinfo("Setting register 0x%x to 0x%02x on %s failed (WARNING ONLY).\n", SIS_REG, new, name);
		msg_pinfo("Stuck at 0x%02x\n", newer);
		ret = -1;
	}

	return ret;
}

static int enable_flash_sis530(struct pci_dev *dev, const char *name)
{
	return enable_flash_sis5x0(dev, name, 0x20, 0x04);
}

static int enable_flash_sis540(struct pci_dev *dev, const char *name)
{
	return enable_flash_sis5x0(dev, name, 0x80, 0x40);
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

	internal_buses_supported = BUS_PARALLEL;

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

	if (pci_read_word(dev, xbcs) != new) { /* FIXME: share this with other code? */
		msg_pinfo("Setting register 0x%04x to 0x%04x on %s failed (WARNING ONLY).\n", xbcs, new, name);
		return -1;
	}

	return 0;
}

/* Handle BIOS_CNTL (aka. BCR). Disable locks and enable writes. The register can either be in PCI config space
 * at the offset given by 'bios_cntl' or at the memory-mapped address 'addr'.
 *
 * Note: the ICH0-ICH5 BIOS_CNTL register is actually 16 bit wide, in Poulsbo, Tunnel Creek and other Atom
 * chipsets/SoCs it is even 32b, but just treating it as 8 bit wide seems to work fine in practice. */
static int enable_flash_ich_bios_cntl_common(enum ich_chipset ich_generation, void *addr,
					     struct pci_dev *dev, uint8_t bios_cntl)
{
	uint8_t old, new, wanted;

	switch (ich_generation) {
	case CHIPSET_ICH_UNKNOWN:
		return ERROR_FATAL;
	/* Non-SPI-capable */
	case CHIPSET_ICH:
	case CHIPSET_ICH2345:
		break;
	/* Some Atom chipsets are special: The second byte of BIOS_CNTL (D9h) contains a prefetch bit similar to
	 * what other SPI-capable chipsets have at DCh. Others like Bay Trail use a memmapped register.
	 * The Tunnel Creek datasheet contains a lot of details about the SPI controller, among other things it
	 * mentions that the prefetching and caching does only happen for direct memory reads.
	 * Therefore - at least for Tunnel Creek - it should not matter to flashrom because we use the
	 * programmed access only and not memory mapping. */
	case CHIPSET_TUNNEL_CREEK:
	case CHIPSET_POULSBO:
	case CHIPSET_CENTERTON:
		old = pci_read_byte(dev, bios_cntl + 1);
		msg_pdbg("BIOS Prefetch Enable: %sabled, ", (old & 1) ? "en" : "dis");
		break;
	case CHIPSET_BAYTRAIL:
	case CHIPSET_ICH7:
	default: /* Future version might behave the same */
		if (ich_generation == CHIPSET_BAYTRAIL)
			old = (mmio_readl(addr) >> 2) & 0x3;
		else
			old = (pci_read_byte(dev, bios_cntl) >> 2) & 0x3;
		msg_pdbg("SPI Read Configuration: ");
		if (old == 3)
			msg_pdbg("invalid prefetching/caching settings, ");
		else
			msg_pdbg("prefetching %sabled, caching %sabled, ",
				     (old & 0x2) ? "en" : "dis",
				     (old & 0x1) ? "dis" : "en");
	}

	if (ich_generation == CHIPSET_BAYTRAIL)
		wanted = old = mmio_readl(addr);
	else
		wanted = old = pci_read_byte(dev, bios_cntl);

	/*
	 * Quote from the 6 Series datasheet (Document Number: 324645-004):
	 * "Bit 5: SMM BIOS Write Protect Disable (SMM_BWP)
	 * 1 = BIOS region SMM protection is enabled.
	 * The BIOS Region is not writable unless all processors are in SMM."
	 * In earlier chipsets this bit is reserved.
	 *
	 * Try to unset it in any case.
	 * It won't hurt and makes sense in some cases according to Stefan Reinauer.
	 *
	 * At least in Centerton aforementioned bit is located at bit 7. It is unspecified in all other Atom
	 * and Desktop chipsets before Ibex Peak/5 Series, but we reset bit 5 anyway.
	 */
	int smm_bwp_bit;
	if (ich_generation == CHIPSET_CENTERTON)
		smm_bwp_bit = 7;
	else
		smm_bwp_bit = 5;
	wanted &= ~(1 << smm_bwp_bit);

	/* Tunnel Creek has a cache disable at bit 2 of the lowest BIOS_CNTL byte. */
	if (ich_generation == CHIPSET_TUNNEL_CREEK)
		wanted |= (1 << 2);

	wanted |= (1 << 0); /* Set BIOS Write Enable */
	wanted &= ~(1 << 1); /* Disable lock (futile) */

	/* Only write the register if it's necessary */
	if (wanted != old) {
		if (ich_generation == CHIPSET_BAYTRAIL) {
			rmmio_writel(wanted, addr);
			new = mmio_readl(addr);
		} else {
			rpci_write_byte(dev, bios_cntl, wanted);
			new = pci_read_byte(dev, bios_cntl);
		}
	} else
		new = old;

	msg_pdbg("\nBIOS_CNTL = 0x%02x: ", new);
	msg_pdbg("BIOS Lock Enable: %sabled, ", (new & (1 << 1)) ? "en" : "dis");
	msg_pdbg("BIOS Write Enable: %sabled\n", (new & (1 << 0)) ? "en" : "dis");
	if (new & (1 << smm_bwp_bit))
		msg_pwarn("Warning: BIOS region SMM protection is enabled!\n");

	if (new != wanted)
		msg_pwarn("Warning: Setting Bios Control at 0x%x from 0x%02x to 0x%02x failed.\n"
			  "New value is 0x%02x.\n", bios_cntl, old, wanted, new);

	/* Return an error if we could not set the write enable only. */
	if (!(new & (1 << 0)))
		return -1;

	return 0;
}

static int enable_flash_ich_bios_cntl_config_space(struct pci_dev *dev, enum ich_chipset ich_generation,
						   uint8_t bios_cntl)
{
	return enable_flash_ich_bios_cntl_common(ich_generation, NULL, dev, bios_cntl);
}

static int enable_flash_ich_bios_cntl_memmapped(enum ich_chipset ich_generation, void *addr)
{
	return enable_flash_ich_bios_cntl_common(ich_generation, addr, NULL, 0);
}

static int enable_flash_ich_fwh_decode(struct pci_dev *dev, enum ich_chipset ich_generation)
{
	uint8_t fwh_sel1 = 0, fwh_sel2 = 0, fwh_dec_en_lo = 0, fwh_dec_en_hi = 0; /* silence compilers */
	bool implemented = 0;
	void *ilb = NULL; /* Only for Baytrail */
	switch (ich_generation) {
	case CHIPSET_ICH:
		/* FIXME: Unlike later chipsets, ICH and ICH-0 do only support mapping of the top-most 4MB
		 * and therefore do only feature FWH_DEC_EN (E3h, different default too) and FWH_SEL (E8h). */
		break;
	case CHIPSET_ICH2345:
		fwh_sel1 = 0xe8;
		fwh_sel2 = 0xee;
		fwh_dec_en_lo = 0xf0;
		fwh_dec_en_hi = 0xe3;
		implemented = 1;
		break;
	case CHIPSET_POULSBO:
	case CHIPSET_TUNNEL_CREEK:
		/* FIXME: Similar to ICH and ICH-0, Tunnel Creek and Poulsbo do only feature one register each,
		 * FWH_DEC_EN (D7h) and FWH_SEL (D0h). */
		break;
	case CHIPSET_CENTERTON:
		/* FIXME: Similar to above FWH_DEC_EN (D4h) and FWH_SEL (D0h). */
		break;
	case CHIPSET_BAYTRAIL: {
		uint32_t ilb_base = pci_read_long(dev, 0x50) & 0xfffffe00; /* bits 31:9 */
		if (ilb_base == 0) {
			msg_perr("Error: Invalid ILB_BASE_ADDRESS\n");
			return ERROR_FATAL;
		}
		ilb = rphysmap("BYT IBASE", ilb_base, 512);
		fwh_sel1 = 0x18;
		fwh_dec_en_lo = 0xd8;
		fwh_dec_en_hi = 0xd9;
		implemented = 1;
		break;
	}
	case CHIPSET_ICH6:
	case CHIPSET_ICH7:
	default: /* Future version might behave the same */
		fwh_sel1 = 0xd0;
		fwh_sel2 = 0xd4;
		fwh_dec_en_lo = 0xd8;
		fwh_dec_en_hi = 0xd9;
		implemented = 1;
		break;
	}

	char *idsel = extract_programmer_param("fwh_idsel");
	if (idsel && strlen(idsel)) {
		if (!implemented) {
			msg_perr("Error: fwh_idsel= specified, but (yet) unsupported on this chipset.\n");
			goto idsel_garbage_out;
		}
		errno = 0;
		/* Base 16, nothing else makes sense. */
		uint64_t fwh_idsel = (uint64_t)strtoull(idsel, NULL, 16);
		if (errno) {
			msg_perr("Error: fwh_idsel= specified, but value could not be converted.\n");
			goto idsel_garbage_out;
		}
		uint64_t fwh_mask = 0xffffffff;
		if (fwh_sel2 > 0)
			fwh_mask |= (0xffffULL << 32);
		if (fwh_idsel & ~fwh_mask) {
			msg_perr("Error: fwh_idsel= specified, but value had unused bits set.\n");
			goto idsel_garbage_out;
		}
		uint64_t fwh_idsel_old;
		if (ich_generation == CHIPSET_BAYTRAIL) {
			fwh_idsel_old = mmio_readl(ilb + fwh_sel1);
			rmmio_writel(fwh_idsel, ilb + fwh_sel1);
		} else {
			fwh_idsel_old = (uint64_t)pci_read_long(dev, fwh_sel1) << 16;
			rpci_write_long(dev, fwh_sel1, (fwh_idsel >> 16) & 0xffffffff);
			if (fwh_sel2 > 0) {
				fwh_idsel_old |= pci_read_word(dev, fwh_sel2);
				rpci_write_word(dev, fwh_sel2, fwh_idsel & 0xffff);
			}
		}
		msg_pdbg("Setting IDSEL from 0x%012" PRIx64 " to 0x%012" PRIx64 " for top 16 MB.\n",
			 fwh_idsel_old, fwh_idsel);
		/* FIXME: Decode settings are not changed. */
	} else if (idsel) {
		msg_perr("Error: fwh_idsel= specified, but no value given.\n");
idsel_garbage_out:
		free(idsel);
		return ERROR_FATAL;
	}
	free(idsel);

	if (!implemented) {
		msg_pdbg2("FWH IDSEL handling is not implemented on this chipset.\n");
		return 0;
	}

	/* Ignore all legacy ranges below 1 MB.
	 * We currently only support flashing the chip which responds to
	 * IDSEL=0. To support IDSEL!=0, flashbase and decode size calculations
	 * have to be adjusted.
	 */
	int max_decode_fwh_idsel = 0, max_decode_fwh_decode = 0;
	bool contiguous = 1;
	uint32_t fwh_conf;
	if (ich_generation == CHIPSET_BAYTRAIL)
		fwh_conf = mmio_readl(ilb + fwh_sel1);
	else
		fwh_conf = pci_read_long(dev, fwh_sel1);

	int i;
	/* FWH_SEL1 */
	for (i = 7; i >= 0; i--) {
		int tmp = (fwh_conf >> (i * 4)) & 0xf;
		msg_pdbg("0x%08x/0x%08x FWH IDSEL: 0x%x\n",
			 (0x1ff8 + i) * 0x80000,
			 (0x1ff0 + i) * 0x80000,
			 tmp);
		if ((tmp == 0) && contiguous) {
			max_decode_fwh_idsel = (8 - i) * 0x80000;
		} else {
			contiguous = 0;
		}
	}
	if (fwh_sel2 > 0) {
		/* FWH_SEL2 */
		fwh_conf = pci_read_word(dev, fwh_sel2);
		for (i = 3; i >= 0; i--) {
			int tmp = (fwh_conf >> (i * 4)) & 0xf;
			msg_pdbg("0x%08x/0x%08x FWH IDSEL: 0x%x\n",
				 (0xff4 + i) * 0x100000,
				 (0xff0 + i) * 0x100000,
				 tmp);
			if ((tmp == 0) && contiguous) {
				max_decode_fwh_idsel = (8 - i) * 0x100000;
			} else {
				contiguous = 0;
			}
		}
	}
	contiguous = 1;
	/* FWH_DEC_EN1 */
	fwh_conf = pci_read_byte(dev, fwh_dec_en_hi);
	fwh_conf <<= 8;
	fwh_conf |= pci_read_byte(dev, fwh_dec_en_lo);
	for (i = 7; i >= 0; i--) {
		int tmp = (fwh_conf >> (i + 0x8)) & 0x1;
		msg_pdbg("0x%08x/0x%08x FWH decode %sabled\n",
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
		int tmp = (fwh_conf >> i) & 0x1;
		msg_pdbg("0x%08x/0x%08x FWH decode %sabled\n",
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
	msg_pdbg("Maximum FWH chip size: 0x%x bytes\n", max_rom_decode.fwh);

	return 0;
}

static int enable_flash_ich_fwh(struct pci_dev *dev, enum ich_chipset ich_generation, uint8_t bios_cntl)
{
	int err;

	/* Configure FWH IDSEL decoder maps. */
	if ((err = enable_flash_ich_fwh_decode(dev, ich_generation)) != 0)
		return err;

	internal_buses_supported = BUS_FWH;
	return enable_flash_ich_bios_cntl_config_space(dev, ich_generation, bios_cntl);
}

static int enable_flash_ich0(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_fwh(dev, CHIPSET_ICH, 0x4e);
}

static int enable_flash_ich2345(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_fwh(dev, CHIPSET_ICH2345, 0x4e);
}

static int enable_flash_ich6(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_fwh(dev, CHIPSET_ICH6, 0xdc);
}

static int enable_flash_poulsbo(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_fwh(dev, CHIPSET_POULSBO, 0xd8);
}

static void enable_flash_ich_handle_gcs(struct pci_dev *dev, enum ich_chipset ich_generation, uint32_t gcs, bool top_swap)
{
	msg_pdbg("GCS = 0x%x: ", gcs);
	msg_pdbg("BIOS Interface Lock-Down: %sabled, ", (gcs & 0x1) ? "en" : "dis");

	static const char *const straps_names_EP80579[] = { "SPI", "reserved", "reserved", "LPC" };
	static const char *const straps_names_ich7_nm10[] = { "reserved", "SPI", "PCI", "LPC" };
	static const char *const straps_names_tunnel_creek[] = { "SPI", "LPC" };
	static const char *const straps_names_ich8910[] = { "SPI", "SPI", "PCI", "LPC" };
	static const char *const straps_names_pch567[] = { "LPC", "reserved", "PCI", "SPI" };
	static const char *const straps_names_pch89_baytrail[] = { "LPC", "reserved", "reserved", "SPI" };
	static const char *const straps_names_pch8_lp[] = { "SPI", "LPC" };
	static const char *const straps_names_unknown[] = { "unknown", "unknown", "unknown", "unknown" };

	const char *const *straps_names;
	switch (ich_generation) {
	case CHIPSET_ICH7:
		/* EP80579 may need further changes, but this is the least
		 * intrusive way to get correct BOOT Strap printing without
		 * changing the rest of its code path). */
		if (dev->device_id == 0x5031)
			straps_names = straps_names_EP80579;
		else
			straps_names = straps_names_ich7_nm10;
		break;
	case CHIPSET_ICH8:
	case CHIPSET_ICH9:
	case CHIPSET_ICH10:
		straps_names = straps_names_ich8910;
		break;
	case CHIPSET_TUNNEL_CREEK:
		straps_names = straps_names_tunnel_creek;
		break;
	case CHIPSET_5_SERIES_IBEX_PEAK:
	case CHIPSET_6_SERIES_COUGAR_POINT:
	case CHIPSET_7_SERIES_PANTHER_POINT:
		straps_names = straps_names_pch567;
		break;
	case CHIPSET_8_SERIES_LYNX_POINT:
	case CHIPSET_9_SERIES_WILDCAT_POINT:
	case CHIPSET_BAYTRAIL:
		straps_names = straps_names_pch89_baytrail;
		break;
	case CHIPSET_8_SERIES_LYNX_POINT_LP:
		straps_names = straps_names_pch8_lp;
		break;
	case CHIPSET_8_SERIES_WELLSBURG: // FIXME: check datasheet
	case CHIPSET_CENTERTON: // FIXME: Datasheet does not mention GCS at all
		straps_names = straps_names_unknown;
		break;
	default:
		msg_gerr("%s: unknown ICH generation. Please report!\n", __func__);
		straps_names = straps_names_unknown;
		break;
	}

	uint8_t bbs;
	switch (ich_generation) {
	case CHIPSET_TUNNEL_CREEK:
		bbs = (gcs >> 1) & 0x1;
		break;
	case CHIPSET_8_SERIES_LYNX_POINT_LP:
		/* Lynx Point LP uses a single bit for BBS */
		bbs = (gcs >> 10) & 0x1;
		break;
	default:
		/* Other chipsets use two bits for BBS */
		bbs = (gcs >> 10) & 0x3;
		break;
	}
	msg_pdbg("Boot BIOS Straps: 0x%x (%s)\n", bbs, straps_names[bbs]);

	/* Centerton has its TS bit in [GPE0BLK] + 0x30 while the exact location for Tunnel Creek is unknown. */
	if (ich_generation != CHIPSET_TUNNEL_CREEK && ich_generation != CHIPSET_CENTERTON)
		msg_pdbg("Top Swap: %s\n", (top_swap) ? "enabled (A16(+) inverted)" : "not enabled");
}

static int enable_flash_ich_spi(struct pci_dev *dev, enum ich_chipset ich_generation, uint8_t bios_cntl)
{

	/* Get physical address of Root Complex Register Block */
	uint32_t rcra = pci_read_long(dev, 0xf0) & 0xffffc000;
	msg_pdbg("Root Complex Register Block address = 0x%x\n", rcra);

	/* Map RCBA to virtual memory */
	void *rcrb = rphysmap("ICH RCRB", rcra, 0x4000);
	if (rcrb == ERROR_PTR)
		return ERROR_FATAL;

	enable_flash_ich_handle_gcs(dev, ich_generation, mmio_readl(rcrb + 0x3410), mmio_readb(rcrb + 0x3414));

	/* Handle FWH-related parameters and initialization */
	int ret_fwh = enable_flash_ich_fwh(dev, ich_generation, bios_cntl);
	if (ret_fwh == ERROR_FATAL)
		return ret_fwh;

	/* SPIBAR is at RCRB+0x3020 for ICH[78], Tunnel Creek and Centerton, and RCRB+0x3800 for ICH9. */
	uint16_t spibar_offset;
	switch (ich_generation) {
	case CHIPSET_BAYTRAIL:
	case CHIPSET_ICH_UNKNOWN:
		return ERROR_FATAL;
	case CHIPSET_ICH7:
	case CHIPSET_ICH8:
	case CHIPSET_TUNNEL_CREEK:
	case CHIPSET_CENTERTON:
		spibar_offset = 0x3020;
		break;
	case CHIPSET_ICH9:
	default:		/* Future version might behave the same */
		spibar_offset = 0x3800;
		break;
	}
	msg_pdbg("SPIBAR = 0x%0*" PRIxPTR " + 0x%04x\n", PRIxPTR_WIDTH, (uintptr_t)rcrb, spibar_offset);
	void *spibar = rcrb + spibar_offset;

	/* This adds BUS_SPI */
	int ret_spi = ich_init_spi(dev, spibar, ich_generation);
	if (ret_spi == ERROR_FATAL)
		return ret_spi;
	
	if (ret_fwh || ret_spi)
		return ERROR_NONFATAL;

	return 0;
}

static int enable_flash_tunnelcreek(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_TUNNEL_CREEK, 0xd8);
}

static int enable_flash_s12x0(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_CENTERTON, 0xd8);
}

static int enable_flash_ich7(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_ICH7, 0xdc);
}

static int enable_flash_ich8(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_ICH8, 0xdc);
}

static int enable_flash_ich9(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_ICH9, 0xdc);
}

static int enable_flash_ich10(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_ICH10, 0xdc);
}

/* Ibex Peak aka. 5 series & 3400 series */
static int enable_flash_pch5(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_5_SERIES_IBEX_PEAK, 0xdc);
}

/* Cougar Point aka. 6 series & c200 series */
static int enable_flash_pch6(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_6_SERIES_COUGAR_POINT, 0xdc);
}

/* Panther Point aka. 7 series */
static int enable_flash_pch7(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_7_SERIES_PANTHER_POINT, 0xdc);
}

/* Lynx Point aka. 8 series */
static int enable_flash_pch8(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_8_SERIES_LYNX_POINT, 0xdc);
}

/* Lynx Point LP aka. 8 series low-power */
static int enable_flash_pch8_lp(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_8_SERIES_LYNX_POINT_LP, 0xdc);
}

/* Wellsburg (for Haswell-EP Xeons) */
static int enable_flash_pch8_wb(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_8_SERIES_WELLSBURG, 0xdc);
}

/* Wildcat Point */
static int enable_flash_pch9(struct pci_dev *dev, const char *name)
{
	return enable_flash_ich_spi(dev, CHIPSET_9_SERIES_WILDCAT_POINT, 0xdc);
}

/* Silvermont architecture: Bay Trail(-T/-I), Avoton/Rangeley.
 * These have a distinctly different behavior compared to other Intel chipsets and hence are handled separately.
 *
 * Differences include:
 *	- RCBA at LPC config 0xF0 too but mapped range is only 4 B long instead of 16 kB.
 *	- GCS at [RCRB] + 0 (instead of [RCRB] + 0x3410).
 *	- TS (Top Swap) in GCS (instead of [RCRB] + 0x3414).
 *	- SPIBAR (coined SBASE) at LPC config 0x54 (instead of [RCRB] + 0x3800).
 *	- BIOS_CNTL (coined BCR) at [SPIBAR] + 0xFC (instead of LPC config 0xDC).
 */
static int enable_flash_silvermont(struct pci_dev *dev, const char *name)
{
	enum ich_chipset ich_generation = CHIPSET_BAYTRAIL;

	/* Get physical address of Root Complex Register Block */
	uint32_t rcba = pci_read_long(dev, 0xf0) & 0xfffffc00;
	msg_pdbg("Root Complex Register Block address = 0x%x\n", rcba);

	/* Handle GCS (in RCRB) */
	void *rcrb = physmap("BYT RCRB", rcba, 4);
	uint32_t gcs = mmio_readl(rcrb + 0);
	enable_flash_ich_handle_gcs(dev, ich_generation, gcs, gcs & 0x2);
	physunmap(rcrb, 4);

	/* Handle fwh_idsel parameter */
	int ret_fwh = enable_flash_ich_fwh_decode(dev, ich_generation);
	if (ret_fwh == ERROR_FATAL)
		return ret_fwh;

	internal_buses_supported = BUS_FWH;

	/* Get physical address of SPI Base Address and map it */
	uint32_t sbase = pci_read_long(dev, 0x54) & 0xfffffe00;
	msg_pdbg("SPI_BASE_ADDRESS = 0x%x\n", sbase);
	void *spibar = rphysmap("BYT SBASE", sbase, 512); /* Last defined address on Bay Trail is 0x100 */

	/* Enable Flash Writes.
	 * Silvermont-based: BCR at SBASE + 0xFC (some bits of BCR are also accessible via BC at IBASE + 0x1C).
	 */
	enable_flash_ich_bios_cntl_memmapped(ich_generation, spibar + 0xFC);

	int ret_spi = ich_init_spi(dev, spibar, ich_generation);
	if (ret_spi == ERROR_FATAL)
		return ret_spi;

	if (ret_fwh || ret_spi)
		return ERROR_NONFATAL;

	return 0;
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
		msg_pwarn("\nWarning: Failed to enable flash write on \"%s\"\n", name);
		return -1;
	}

	if (dev->device_id == 0x3227) { /* VT8237/VT8237R */
		/* All memory cycles, not just ROM ones, go to LPC. */
		val = pci_read_byte(dev, 0x59);
		val &= ~0x80;
		rpci_write_byte(dev, 0x59, val);
	}

	return 0;
}

static int enable_flash_vt_vx(struct pci_dev *dev, const char *name)
{
	struct pci_dev *south_north = pci_dev_find(0x1106, 0xa353);
	if (south_north == NULL) {
		msg_perr("Could not find South-North Module Interface Control device!\n");
		return ERROR_FATAL;
	}

	msg_pdbg("Strapped to ");
	if ((pci_read_byte(south_north, 0x56) & 0x01) == 0) {
		msg_pdbg("LPC.\n");
		return enable_flash_vt823x(dev, name);
	}
	msg_pdbg("SPI.\n");

	uint32_t mmio_base;
	void *mmio_base_physmapped;
	uint32_t spi_cntl;
	#define SPI_CNTL_LEN 0x08
	uint32_t spi0_mm_base = 0;
	switch(dev->device_id) {
		case 0x8353: /* VX800/VX820 */
			spi0_mm_base = pci_read_long(dev, 0xbc) << 8;
			break;
		case 0x8409: /* VX855/VX875 */
		case 0x8410: /* VX900 */
			mmio_base = pci_read_long(dev, 0xbc) << 8;
			mmio_base_physmapped = physmap("VIA VX MMIO register", mmio_base, SPI_CNTL_LEN);
			if (mmio_base_physmapped == ERROR_PTR)
				return ERROR_FATAL;

			/* Offset 0 - Bit 0 holds SPI Bus0 Enable Bit. */
			spi_cntl = mmio_readl(mmio_base_physmapped) + 0x00;
			if ((spi_cntl & 0x01) == 0) {
				msg_pdbg ("SPI Bus0 disabled!\n");
				physunmap(mmio_base_physmapped, SPI_CNTL_LEN);
				return ERROR_FATAL;
			}
			/* Offset 1-3 has  SPI Bus Memory Map Base Address: */
			spi0_mm_base = spi_cntl & 0xFFFFFF00;

			/* Offset 4 - Bit 0 holds SPI Bus1 Enable Bit. */
			spi_cntl = mmio_readl(mmio_base_physmapped) + 0x04;
			if ((spi_cntl & 0x01) == 1)
				msg_pdbg2("SPI Bus1 is enabled too.\n");

			physunmap(mmio_base_physmapped, SPI_CNTL_LEN);
			break;
		default:
			msg_perr("%s: Unsupported chipset %x:%x!\n", __func__, dev->vendor_id, dev->device_id);
			return ERROR_FATAL;
	}

	return via_init_spi(dev, spi0_mm_base);
}

static int enable_flash_vt8237s_spi(struct pci_dev *dev, const char *name)
{
	return via_init_spi(dev, pci_read_long(dev, 0xbc) << 8);
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

	internal_buses_supported = BUS_PARALLEL;
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
	#define SC_REG 0x52
	uint8_t new;

	rpci_write_byte(dev, SC_REG, 0xee);

	new = pci_read_byte(dev, SC_REG);

	if (new != 0xee) { /* FIXME: share this with other code? */
		msg_pinfo("Setting register 0x%x to 0x%02x on %s failed (WARNING ONLY).\n", SC_REG, new, name);
		return -1;
	}

	return 0;
}

/* Works for AMD-768, AMD-8111, VIA VT82C586A/B, VIA VT82C596, VIA VT82C686A/B.
 *
 * ROM decode control register matrix
 * 	AMD-768			AMD-8111	VT82C586A/B		VT82C596		VT82C686A/B
 * 7	FFC0_0000h–FFFF_FFFFh	<-		FFFE0000h-FFFEFFFFh	<-			<-
 * 6	FFB0_0000h–FFBF_FFFFh	<-		FFF80000h-FFFDFFFFh	<-			<-
 * 5	00E8...			<-		<-			FFF00000h-FFF7FFFFh	<-
 */
static int enable_flash_amd_via(struct pci_dev *dev, const char *name, uint8_t decode_val)
{
	#define AMD_MAPREG 0x43
	#define AMD_ENREG 0x40
	uint8_t old, new;

	old = pci_read_byte(dev, AMD_MAPREG);
	new = old | decode_val;
	if (new != old) {
		rpci_write_byte(dev, AMD_MAPREG, new);
		if (pci_read_byte(dev, AMD_MAPREG) != new) {
			msg_pwarn("Setting register 0x%x to 0x%02x on %s failed (WARNING ONLY).\n",
				  AMD_MAPREG, new, name);
		} else
			msg_pdbg("Changed ROM decode range to 0x%02x successfully.\n", new);
	}

	/* Enable 'ROM write' bit. */
	old = pci_read_byte(dev, AMD_ENREG);
	new = old | 0x01;
	if (new == old)
		return 0;
	rpci_write_byte(dev, AMD_ENREG, new);

	if (pci_read_byte(dev, AMD_ENREG) != new) {
		msg_pwarn("Setting register 0x%x to 0x%02x on %s failed (WARNING ONLY).\n",
			  AMD_ENREG, new, name);
		return ERROR_NONFATAL;
	}
	msg_pdbg2("Set ROM enable bit successfully.\n");

	return 0;
}

static int enable_flash_amd_768_8111(struct pci_dev *dev, const char *name)
{
	/* Enable decoding of 0xFFB00000 to 0xFFFFFFFF (5 MB). */
	max_rom_decode.lpc = 5 * 1024 * 1024;
	return enable_flash_amd_via(dev, name, 0xC0);
}

static int enable_flash_vt82c586(struct pci_dev *dev, const char *name)
{
	/* Enable decoding of 0xFFF80000 to 0xFFFFFFFF. (512 kB) */
	max_rom_decode.parallel = 512 * 1024;
	return enable_flash_amd_via(dev, name, 0xC0);
}

/* Works for VT82C686A/B too. */
static int enable_flash_vt82c596(struct pci_dev *dev, const char *name)
{
	/* Enable decoding of 0xFFF00000 to 0xFFFFFFFF. (1 MB) */
	max_rom_decode.parallel = 1024 * 1024;
	return enable_flash_amd_via(dev, name, 0xE0);
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
		msg_pdbg("Chipset %s%sprotected flash from 0x%08x to 0x%08x, unlocking...",
			  (prot & 0x2) ? "read " : "",
			  (prot & 0x1) ? "write " : "",
			  (prot & 0xfffff800),
			  (prot & 0xfffff800) + (((prot & 0x7fc) << 8) | 0x3ff));
		prot &= 0xfffffffc;
		rpci_write_byte(dev, reg, prot);
		prot = pci_read_long(dev, reg);
		if ((prot & 0x3) != 0) {
			msg_perr("Disabling %s%sprotection of flash addresses from 0x%08x to 0x%08x failed.\n",
				 (prot & 0x2) ? "read " : "",
				 (prot & 0x1) ? "write " : "",
				 (prot & 0xfffff800),
				 (prot & 0xfffff800) + (((prot & 0x7fc) << 8) | 0x3ff));
			continue;
		}
		msg_pdbg("done.\n");
	}

	internal_buses_supported = BUS_LPC | BUS_FWH;

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

/* sets bit 0 in 0x6d */
static int enable_flash_nvidia_common(struct pci_dev *dev, const char *name)
{
	uint8_t old, new;

	old = pci_read_byte(dev, 0x6d);
	new = old | 0x01;
	if (new == old)
		return 0;

	rpci_write_byte(dev, 0x6d, new);
	if (pci_read_byte(dev, 0x6d) != new) {
		msg_pinfo("Setting register 0x6d to 0x%02x on %s failed.\n", new, name);
		return 1;
	}
	return 0;
}

static int enable_flash_nvidia_nforce2(struct pci_dev *dev, const char *name)
{
	rpci_write_byte(dev, 0x92, 0);
	if (enable_flash_nvidia_common(dev, name))
		return ERROR_NONFATAL;
	else
		return 0;
}

static int enable_flash_ck804(struct pci_dev *dev, const char *name)
{
	uint32_t segctrl;
	uint8_t reg, old, new;
	unsigned int err = 0;

	/* 0x8A is special: it is a single byte and only one nibble is touched. */
	reg = 0x8A;
	segctrl = pci_read_byte(dev, reg);
	if ((segctrl & 0x3) != 0x0) {
		if ((segctrl & 0xC) != 0x0) {
			msg_pinfo("Can not unlock existing protection in register 0x%02x.\n", reg);
			err++;
		} else {
			msg_pdbg("Unlocking protection in register 0x%02x... ", reg);
			rpci_write_byte(dev, reg, segctrl & 0xF0);

			segctrl = pci_read_byte(dev, reg);
			if ((segctrl & 0x3) != 0x0) {
				msg_pinfo("Could not unlock protection in register 0x%02x (new value: 0x%x).\n",
					  reg, segctrl);
				err++;
			} else
				msg_pdbg("OK\n");
		}
	}

	for (reg = 0x8C; reg <= 0x94; reg += 4) {
		segctrl = pci_read_long(dev, reg);
		if ((segctrl & 0x33333333) == 0x00000000) {
			/* reads and writes are unlocked */
			continue;
		}
		if ((segctrl & 0xCCCCCCCC) != 0x00000000) {
			msg_pinfo("Can not unlock existing protection in register 0x%02x.\n", reg);
			err++;
			continue;
		}
		msg_pdbg("Unlocking protection in register 0x%02x... ", reg);
		rpci_write_long(dev, reg, 0x00000000);

		segctrl = pci_read_long(dev, reg);
		if ((segctrl & 0x33333333) != 0x00000000) {
			msg_pinfo("Could not unlock protection in register 0x%02x (new value: 0x%08x).\n",
				  reg, segctrl);
			err++;
		} else
			msg_pdbg("OK\n");
	}

	if (err > 0) {
		msg_pinfo("%d locks could not be disabled, disabling writes (reads may also fail).\n", err);
		programmer_may_write = 0;
	}

	reg = 0x88;
	old = pci_read_byte(dev, reg);
	new = old | 0xC0;
	if (new != old) {
		rpci_write_byte(dev, reg, new);
		if (pci_read_byte(dev, reg) != new) { /* FIXME: share this with other code? */
			msg_pinfo("Setting register 0x%02x to 0x%02x on %s failed.\n", reg, new, name);
			err++;
		}
	}

	if (enable_flash_nvidia_common(dev, name))
		err++;

	if (err > 0)
		return ERROR_NONFATAL;
	else
		return 0;
}

static int enable_flash_osb4(struct pci_dev *dev, const char *name)
{
	uint8_t tmp;

	internal_buses_supported = BUS_PARALLEL;

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
		return ERROR_FATAL;
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
	uint8_t val;
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

	if (enable_flash_nvidia_common(dev, name))
		return ERROR_NONFATAL;
	else
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

	/* dev is the ISA bridge. No idea what the stuff below does. */
	val = pci_read_byte(dev, 0x8a);
	msg_pdbg("ISA/LPC bridge reg 0x8a contents: 0x%02x, bit 6 is %i, bit 5 "
		 "is %i\n", val, (val >> 6) & 0x1, (val >> 5) & 0x1);

	switch ((val >> 5) & 0x3) {
	case 0x0:
		ret = enable_flash_mcp55(dev, name);
		internal_buses_supported = BUS_LPC;
		msg_pdbg("Flash bus type is LPC\n");
		break;
	case 0x2:
		want_spi = 1;
		/* SPI is added in mcp6x_spi_init if it works.
		 * Do we really want to disable LPC in this case?
		 */
		internal_buses_supported = BUS_NONE;
		msg_pdbg("Flash bus type is SPI\n");
		break;
	default:
		/* Should not happen. */
		internal_buses_supported = BUS_NONE;
		msg_pwarn("Flash bus type is unknown (none)\n");
		msg_pinfo("Please send the log files created by \"flashrom -p internal -o logfile\" to \n"
			  "flashrom@flashrom.org with \"your board name: flashrom -V\" as the subject to\n"
			  "help us finish support for your chipset. Thanks.\n");
		return ERROR_NONFATAL;
	}

	/* Force enable SPI and disable LPC? Not a good idea. */
#if 0
	val |= (1 << 6);
	val &= ~(1 << 5);
	rpci_write_byte(dev, 0x8a, val);
#endif

	if (mcp6x_spi_init(want_spi))
		ret = 1;

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
	if (mmcr == ERROR_PTR)
		return ERROR_FATAL;

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
			  "Assuming flash at 4G.\n");
	}

	/* 4. Clean up */
	physunmap(mmcr, getpagesize());
	return 0;
}

#endif

/* Please keep this list numerically sorted by vendor/device ID. */
const struct penable chipset_enables[] = {
#if defined(__i386__) || defined(__x86_64__)
	{0x1002, 0x4377, OK,  "ATI", "SB400",				enable_flash_sb400},
	{0x1002, 0x438d, OK,  "AMD", "SB600",				enable_flash_sb600},
	{0x1002, 0x439d, OK,  "AMD", "SB7x0/SB8x0/SB9x0",		enable_flash_sb600},
	{0x100b, 0x0510, NT,  "AMD", "SC1100",				enable_flash_sc1100},
	{0x1022, 0x2080, OK,  "AMD", "CS5536",				enable_flash_cs5536},
	{0x1022, 0x2090, OK,  "AMD", "CS5536",				enable_flash_cs5536},
	{0x1022, 0x3000, OK,  "AMD", "Elan SC520",			get_flashbase_sc520},
	{0x1022, 0x7440, OK,  "AMD", "AMD-768",				enable_flash_amd_768_8111},
	{0x1022, 0x7468, OK,  "AMD", "AMD-8111",			enable_flash_amd_768_8111},
	{0x1022, 0x780e, OK,  "AMD", "FCH",				enable_flash_sb600},
	{0x1039, 0x0406, NT,  "SiS", "501/5101/5501",			enable_flash_sis501},
	{0x1039, 0x0496, NT,  "SiS", "85C496+497",			enable_flash_sis85c496},
	{0x1039, 0x0530, OK,  "SiS", "530",				enable_flash_sis530},
	{0x1039, 0x0540, NT,  "SiS", "540",				enable_flash_sis540},
	{0x1039, 0x0620, NT,  "SiS", "620",				enable_flash_sis530},
	{0x1039, 0x0630, NT,  "SiS", "630",				enable_flash_sis540},
	{0x1039, 0x0635, NT,  "SiS", "635",				enable_flash_sis540},
	{0x1039, 0x0640, NT,  "SiS", "640",				enable_flash_sis540},
	{0x1039, 0x0645, NT,  "SiS", "645",				enable_flash_sis540},
	{0x1039, 0x0646, OK,  "SiS", "645DX",				enable_flash_sis540},
	{0x1039, 0x0648, OK,  "SiS", "648",				enable_flash_sis540},
	{0x1039, 0x0650, OK,  "SiS", "650",				enable_flash_sis540},
	{0x1039, 0x0651, OK,  "SiS", "651",				enable_flash_sis540},
	{0x1039, 0x0655, NT,  "SiS", "655",				enable_flash_sis540},
	{0x1039, 0x0661, OK,  "SiS", "661",				enable_flash_sis540},
	{0x1039, 0x0730, OK,  "SiS", "730",				enable_flash_sis540},
	{0x1039, 0x0733, NT,  "SiS", "733",				enable_flash_sis540},
	{0x1039, 0x0735, OK,  "SiS", "735",				enable_flash_sis540},
	{0x1039, 0x0740, NT,  "SiS", "740",				enable_flash_sis540},
	{0x1039, 0x0741, OK,  "SiS", "741",				enable_flash_sis540},
	{0x1039, 0x0745, OK,  "SiS", "745",				enable_flash_sis540},
	{0x1039, 0x0746, NT,  "SiS", "746",				enable_flash_sis540},
	{0x1039, 0x0748, NT,  "SiS", "748",				enable_flash_sis540},
	{0x1039, 0x0755, OK,  "SiS", "755",				enable_flash_sis540},
	{0x1039, 0x5511, NT,  "SiS", "5511",				enable_flash_sis5511},
	{0x1039, 0x5571, NT,  "SiS", "5571",				enable_flash_sis530},
	{0x1039, 0x5591, NT,  "SiS", "5591/5592",			enable_flash_sis530},
	{0x1039, 0x5596, NT,  "SiS", "5596",				enable_flash_sis5511},
	{0x1039, 0x5597, NT,  "SiS", "5597/5598/5581/5120",		enable_flash_sis530},
	{0x1039, 0x5600, NT,  "SiS", "600",				enable_flash_sis530},
	{0x1078, 0x0100, OK,  "AMD", "CS5530(A)",			enable_flash_cs5530},
	{0x10b9, 0x1533, OK,  "ALi", "M1533",				enable_flash_ali_m1533},
	{0x10de, 0x0030, OK,  "NVIDIA", "nForce4/MCP4",			enable_flash_nvidia_nforce2},
	{0x10de, 0x0050, OK,  "NVIDIA", "CK804",			enable_flash_ck804}, /* LPC */
	{0x10de, 0x0051, OK,  "NVIDIA", "CK804",			enable_flash_ck804}, /* Pro */
	{0x10de, 0x0060, OK,  "NVIDIA", "NForce2",			enable_flash_nvidia_nforce2},
	{0x10de, 0x00e0, OK,  "NVIDIA", "NForce3",			enable_flash_nvidia_nforce2},
	/* Slave, should not be here, to fix known bug for A01. */
	{0x10de, 0x00d3, OK,  "NVIDIA", "CK804",			enable_flash_ck804},
	{0x10de, 0x0260, OK,  "NVIDIA", "MCP51",			enable_flash_ck804},
	{0x10de, 0x0261, OK,  "NVIDIA", "MCP51",			enable_flash_ck804},
	{0x10de, 0x0262, NT,  "NVIDIA", "MCP51",			enable_flash_ck804},
	{0x10de, 0x0263, NT,  "NVIDIA", "MCP51",			enable_flash_ck804},
	{0x10de, 0x0360, OK,  "NVIDIA", "MCP55",			enable_flash_mcp55}, /* M57SLI*/
	/* 10de:0361 is present in Tyan S2915 OEM systems, but not connected to
	 * the flash chip. Instead, 10de:0364 is connected to the flash chip.
	 * Until we have PCI device class matching or some fallback mechanism,
	 * this is needed to get flashrom working on Tyan S2915 and maybe other
	 * dual-MCP55 boards.
	 */
#if 0
	{0x10de, 0x0361, NT,  "NVIDIA", "MCP55",			enable_flash_mcp55}, /* LPC */
#endif
	{0x10de, 0x0362, OK,  "NVIDIA", "MCP55",			enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0363, OK,  "NVIDIA", "MCP55",			enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0364, OK,  "NVIDIA", "MCP55",			enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0365, OK,  "NVIDIA", "MCP55",			enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0366, OK,  "NVIDIA", "MCP55",			enable_flash_mcp55}, /* LPC */
	{0x10de, 0x0367, OK,  "NVIDIA", "MCP55",			enable_flash_mcp55}, /* Pro */
	{0x10de, 0x03e0, OK,  "NVIDIA", "MCP61",			enable_flash_mcp6x_7x},
	{0x10de, 0x03e1, OK,  "NVIDIA", "MCP61",			enable_flash_mcp6x_7x},
	{0x10de, 0x03e3, NT,  "NVIDIA", "MCP61",			enable_flash_mcp6x_7x},
	{0x10de, 0x0440, NT,  "NVIDIA", "MCP65",			enable_flash_mcp6x_7x},
	{0x10de, 0x0441, NT,  "NVIDIA", "MCP65",			enable_flash_mcp6x_7x},
	{0x10de, 0x0442, NT,  "NVIDIA", "MCP65",			enable_flash_mcp6x_7x},
	{0x10de, 0x0443, NT,  "NVIDIA", "MCP65",			enable_flash_mcp6x_7x},
	{0x10de, 0x0548, OK,  "NVIDIA", "MCP67",			enable_flash_mcp6x_7x},
	{0x10de, 0x075c, OK,  "NVIDIA", "MCP78S",			enable_flash_mcp6x_7x},
	{0x10de, 0x075d, OK,  "NVIDIA", "MCP78S",			enable_flash_mcp6x_7x},
	{0x10de, 0x07d7, OK,  "NVIDIA", "MCP73",			enable_flash_mcp6x_7x},
	{0x10de, 0x0aac, OK,  "NVIDIA", "MCP79",			enable_flash_mcp6x_7x},
	{0x10de, 0x0aad, NT,  "NVIDIA", "MCP79",			enable_flash_mcp6x_7x},
	{0x10de, 0x0aae, NT,  "NVIDIA", "MCP79",			enable_flash_mcp6x_7x},
	{0x10de, 0x0aaf, NT,  "NVIDIA", "MCP79",			enable_flash_mcp6x_7x},
	{0x10de, 0x0d80, NT,  "NVIDIA", "MCP89",			enable_flash_mcp6x_7x},
	/* VIA northbridges */
	{0x1106, 0x0585, NT,  "VIA", "VT82C585VPX",			via_no_byte_merge},
	{0x1106, 0x0595, NT,  "VIA", "VT82C595",			via_no_byte_merge},
	{0x1106, 0x0597, NT,  "VIA", "VT82C597",			via_no_byte_merge},
	{0x1106, 0x0601, NT,  "VIA", "VT8601/VT8601A",			via_no_byte_merge},
	{0x1106, 0x0691, OK,  "VIA", "VT82C69x",			via_no_byte_merge},
	{0x1106, 0x8601, NT,  "VIA", "VT8601T",				via_no_byte_merge},
	/* VIA southbridges */
	{0x1106, 0x0586, OK,  "VIA", "VT82C586A/B",			enable_flash_vt82c586},
	{0x1106, 0x0596, OK,  "VIA", "VT82C596",			enable_flash_vt82c596},
	{0x1106, 0x0686, OK,  "VIA", "VT82C686A/B",			enable_flash_vt82c596},
	{0x1106, 0x3074, OK,  "VIA", "VT8233",				enable_flash_vt823x},
	{0x1106, 0x3147, OK,  "VIA", "VT8233A",				enable_flash_vt823x},
	{0x1106, 0x3177, OK,  "VIA", "VT8235",				enable_flash_vt823x},
	{0x1106, 0x3227, OK,  "VIA", "VT8237(R)",			enable_flash_vt823x},
	{0x1106, 0x3337, OK,  "VIA", "VT8237A",				enable_flash_vt823x},
	{0x1106, 0x3372, OK,  "VIA", "VT8237S",				enable_flash_vt8237s_spi},
	{0x1106, 0x8231, NT,  "VIA", "VT8231",				enable_flash_vt823x},
	{0x1106, 0x8324, OK,  "VIA", "CX700",				enable_flash_vt823x},
	{0x1106, 0x8353, NT,  "VIA", "VX800/VX820",			enable_flash_vt_vx},
	{0x1106, 0x8409, NT,  "VIA", "VX855/VX875",			enable_flash_vt_vx},
	{0x1106, 0x8410, NT,  "VIA", "VX900",				enable_flash_vt_vx},
	{0x1166, 0x0200, OK,  "Broadcom", "OSB4",			enable_flash_osb4},
	{0x1166, 0x0205, OK,  "Broadcom", "HT-1000",			enable_flash_ht1000},
	{0x17f3, 0x6030, OK,  "RDC", "R8610/R3210",			enable_flash_rdc_r8610},
	{0x8086, 0x0c60, NT,  "Intel", "S12x0",				enable_flash_s12x0},
	{0x8086, 0x0f1c, OK,  "Intel", "Bay Trail",			enable_flash_silvermont},
	{0x8086, 0x0f1d, NT,  "Intel", "Bay Trail",			enable_flash_silvermont},
	{0x8086, 0x0f1e, NT,  "Intel", "Bay Trail",			enable_flash_silvermont},
	{0x8086, 0x0f1f, NT,  "Intel", "Bay Trail",			enable_flash_silvermont},
	{0x8086, 0x122e, OK,  "Intel", "PIIX",				enable_flash_piix4},
	{0x8086, 0x1234, NT,  "Intel", "MPIIX",				enable_flash_piix4},
	{0x8086, 0x1c44, DEP, "Intel", "Z68",				enable_flash_pch6},
	{0x8086, 0x1c46, DEP, "Intel", "P67",				enable_flash_pch6},
	{0x8086, 0x1c47, NT,  "Intel", "UM67",				enable_flash_pch6},
	{0x8086, 0x1c49, NT,  "Intel", "HM65",				enable_flash_pch6},
	{0x8086, 0x1c4a, DEP, "Intel", "H67",				enable_flash_pch6},
	{0x8086, 0x1c4b, NT,  "Intel", "HM67",				enable_flash_pch6},
	{0x8086, 0x1c4c, NT,  "Intel", "Q65",				enable_flash_pch6},
	{0x8086, 0x1c4d, NT,  "Intel", "QS67",				enable_flash_pch6},
	{0x8086, 0x1c4e, NT,  "Intel", "Q67",				enable_flash_pch6},
	{0x8086, 0x1c4f, DEP, "Intel", "QM67",				enable_flash_pch6},
	{0x8086, 0x1c50, NT,  "Intel", "B65",				enable_flash_pch6},
	{0x8086, 0x1c52, NT,  "Intel", "C202",				enable_flash_pch6},
	{0x8086, 0x1c54, DEP, "Intel", "C204",				enable_flash_pch6},
	{0x8086, 0x1c56, NT,  "Intel", "C206",				enable_flash_pch6},
	{0x8086, 0x1c5c, DEP, "Intel", "H61",				enable_flash_pch6},
	{0x8086, 0x1d40, DEP, "Intel", "C60x/X79",			enable_flash_pch6},
	{0x8086, 0x1d41, DEP, "Intel", "C60x/X79",			enable_flash_pch6},
	{0x8086, 0x1e44, DEP, "Intel", "Z77",				enable_flash_pch7},
	{0x8086, 0x1e46, NT,  "Intel", "Z75",				enable_flash_pch7},
	{0x8086, 0x1e47, NT,  "Intel", "Q77",				enable_flash_pch7},
	{0x8086, 0x1e48, NT,  "Intel", "Q75",				enable_flash_pch7},
	{0x8086, 0x1e49, DEP, "Intel", "B75",				enable_flash_pch7},
	{0x8086, 0x1e4a, DEP, "Intel", "H77",				enable_flash_pch7},
	{0x8086, 0x1e53, NT,  "Intel", "C216",				enable_flash_pch7},
	{0x8086, 0x1e55, DEP, "Intel", "QM77",				enable_flash_pch7},
	{0x8086, 0x1e56, NT,  "Intel", "QS77",				enable_flash_pch7},
	{0x8086, 0x1e57, DEP, "Intel", "HM77",				enable_flash_pch7},
	{0x8086, 0x1e58, NT,  "Intel", "UM77",				enable_flash_pch7},
	{0x8086, 0x1e59, NT,  "Intel", "HM76",				enable_flash_pch7},
	{0x8086, 0x1e5d, NT,  "Intel", "HM75",				enable_flash_pch7},
	{0x8086, 0x1e5e, NT,  "Intel", "HM70",				enable_flash_pch7},
	{0x8086, 0x1e5f, DEP, "Intel", "NM70",				enable_flash_pch7},
	{0x8086, 0x1f38, NT,  "Intel", "Avoton/Rangeley",		enable_flash_silvermont},
	{0x8086, 0x1f39, NT,  "Intel", "Avoton/Rangeley",		enable_flash_silvermont},
	{0x8086, 0x1f3a, NT,  "Intel", "Avoton/Rangeley",		enable_flash_silvermont},
	{0x8086, 0x1f3b, NT,  "Intel", "Avoton/Rangeley",		enable_flash_silvermont},
	{0x8086, 0x229c, NT,  "Intel", "Braswell",			enable_flash_silvermont},
	{0x8086, 0x2310, NT,  "Intel", "DH89xxCC (Cave Creek)",		enable_flash_pch7},
	{0x8086, 0x2390, NT,  "Intel", "Coleto Creek",			enable_flash_pch7},
	{0x8086, 0x2410, OK,  "Intel", "ICH",				enable_flash_ich0},
	{0x8086, 0x2420, OK,  "Intel", "ICH0",				enable_flash_ich0},
	{0x8086, 0x2440, OK,  "Intel", "ICH2",				enable_flash_ich2345},
	{0x8086, 0x244c, OK,  "Intel", "ICH2-M",			enable_flash_ich2345},
	{0x8086, 0x2450, NT,  "Intel", "C-ICH",				enable_flash_ich2345},
	{0x8086, 0x2480, OK,  "Intel", "ICH3-S",			enable_flash_ich2345},
	{0x8086, 0x248c, OK,  "Intel", "ICH3-M",			enable_flash_ich2345},
	{0x8086, 0x24c0, OK,  "Intel", "ICH4/ICH4-L",			enable_flash_ich2345},
	{0x8086, 0x24cc, OK,  "Intel", "ICH4-M",			enable_flash_ich2345},
	{0x8086, 0x24d0, OK,  "Intel", "ICH5/ICH5R",			enable_flash_ich2345},
	{0x8086, 0x25a1, OK,  "Intel", "6300ESB",			enable_flash_ich2345},
	{0x8086, 0x2640, OK,  "Intel", "ICH6/ICH6R",			enable_flash_ich6},
	{0x8086, 0x2641, OK,  "Intel", "ICH6-M",			enable_flash_ich6},
	{0x8086, 0x2642, NT,  "Intel", "ICH6W/ICH6RW",			enable_flash_ich6},
	{0x8086, 0x2670, OK,  "Intel", "631xESB/632xESB/3100",		enable_flash_ich6},
	{0x8086, 0x27b0, OK,  "Intel", "ICH7DH",			enable_flash_ich7},
	{0x8086, 0x27b8, OK,  "Intel", "ICH7/ICH7R",			enable_flash_ich7},
	{0x8086, 0x27b9, OK,  "Intel", "ICH7M",				enable_flash_ich7},
	{0x8086, 0x27bc, OK,  "Intel", "NM10",				enable_flash_ich7},
	{0x8086, 0x27bd, OK,  "Intel", "ICH7MDH",			enable_flash_ich7},
	{0x8086, 0x2810, DEP, "Intel", "ICH8/ICH8R",			enable_flash_ich8},
	{0x8086, 0x2811, DEP, "Intel", "ICH8M-E",			enable_flash_ich8},
	{0x8086, 0x2812, DEP, "Intel", "ICH8DH",			enable_flash_ich8},
	{0x8086, 0x2814, DEP, "Intel", "ICH8DO",			enable_flash_ich8},
	{0x8086, 0x2815, DEP, "Intel", "ICH8M",				enable_flash_ich8},
	{0x8086, 0x2910, DEP, "Intel", "ICH9 Eng. Sample",		enable_flash_ich9},
	{0x8086, 0x2912, DEP, "Intel", "ICH9DH",			enable_flash_ich9},
	{0x8086, 0x2914, DEP, "Intel", "ICH9DO",			enable_flash_ich9},
	{0x8086, 0x2916, DEP, "Intel", "ICH9R",				enable_flash_ich9},
	{0x8086, 0x2917, DEP, "Intel", "ICH9M-E",			enable_flash_ich9},
	{0x8086, 0x2918, DEP, "Intel", "ICH9",				enable_flash_ich9},
	{0x8086, 0x2919, DEP, "Intel", "ICH9M",				enable_flash_ich9},
	{0x8086, 0x3a10, NT,  "Intel", "ICH10R Eng. Sample",		enable_flash_ich10},
	{0x8086, 0x3a14, DEP, "Intel", "ICH10DO",			enable_flash_ich10},
	{0x8086, 0x3a16, DEP, "Intel", "ICH10R",			enable_flash_ich10},
	{0x8086, 0x3a18, DEP, "Intel", "ICH10",				enable_flash_ich10},
	{0x8086, 0x3a1a, DEP, "Intel", "ICH10D",			enable_flash_ich10},
	{0x8086, 0x3a1e, NT,  "Intel", "ICH10 Eng. Sample",		enable_flash_ich10},
	{0x8086, 0x3b00, NT,  "Intel", "3400 Desktop",			enable_flash_pch5},
	{0x8086, 0x3b01, NT,  "Intel", "3400 Mobile",			enable_flash_pch5},
	{0x8086, 0x3b02, NT,  "Intel", "P55",				enable_flash_pch5},
	{0x8086, 0x3b03, NT,  "Intel", "PM55",				enable_flash_pch5},
	{0x8086, 0x3b06, DEP, "Intel", "H55",				enable_flash_pch5},
	{0x8086, 0x3b07, DEP, "Intel", "QM57",				enable_flash_pch5},
	{0x8086, 0x3b08, NT,  "Intel", "H57",				enable_flash_pch5},
	{0x8086, 0x3b09, NT,  "Intel", "HM55",				enable_flash_pch5},
	{0x8086, 0x3b0a, NT,  "Intel", "Q57",				enable_flash_pch5},
	{0x8086, 0x3b0b, NT,  "Intel", "HM57",				enable_flash_pch5},
	{0x8086, 0x3b0d, NT,  "Intel", "3400 Mobile SFF",		enable_flash_pch5},
	{0x8086, 0x3b0e, NT,  "Intel", "B55",				enable_flash_pch5},
	{0x8086, 0x3b0f, DEP, "Intel", "QS57",				enable_flash_pch5},
	{0x8086, 0x3b12, NT,  "Intel", "3400",				enable_flash_pch5},
	{0x8086, 0x3b14, DEP, "Intel", "3420",				enable_flash_pch5},
	{0x8086, 0x3b16, NT,  "Intel", "3450",				enable_flash_pch5},
	{0x8086, 0x3b1e, NT,  "Intel", "B55",				enable_flash_pch5},
	{0x8086, 0x5031, OK,  "Intel", "EP80579",			enable_flash_ich7},
	{0x8086, 0x7000, OK,  "Intel", "PIIX3",				enable_flash_piix4},
	{0x8086, 0x7110, OK,  "Intel", "PIIX4/4E/4M",			enable_flash_piix4},
	{0x8086, 0x7198, OK,  "Intel", "440MX",				enable_flash_piix4},
	{0x8086, 0x8119, OK,  "Intel", "SCH Poulsbo",			enable_flash_poulsbo},
	{0x8086, 0x8186, OK,  "Intel", "Atom E6xx(T) (Tunnel Creek)",	enable_flash_tunnelcreek},
	{0x8086, 0x8c40, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c41, NT,  "Intel", "Lynx Point Mobile Eng. Sample",	enable_flash_pch8},
	{0x8086, 0x8c42, NT,  "Intel", "Lynx Point Desktop Eng. Sample",enable_flash_pch8},
	{0x8086, 0x8c43, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c44, DEP, "Intel", "Z87",				enable_flash_pch8},
	{0x8086, 0x8c45, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c46, NT,  "Intel", "Z85",				enable_flash_pch8},
	{0x8086, 0x8c47, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c48, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c49, NT,  "Intel", "HM86",				enable_flash_pch8},
	{0x8086, 0x8c4a, DEP, "Intel", "H87",				enable_flash_pch8},
	{0x8086, 0x8c4b, DEP, "Intel", "HM87",				enable_flash_pch8},
	{0x8086, 0x8c4c, NT,  "Intel", "Q85",				enable_flash_pch8},
	{0x8086, 0x8c4d, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c4e, NT,  "Intel", "Q87",				enable_flash_pch8},
	{0x8086, 0x8c4f, NT,  "Intel", "QM87",				enable_flash_pch8},
	{0x8086, 0x8c50, DEP, "Intel", "B85",				enable_flash_pch8},
	{0x8086, 0x8c51, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c52, NT,  "Intel", "C222",				enable_flash_pch8},
	{0x8086, 0x8c53, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c54, NT,  "Intel", "C224",				enable_flash_pch8},
	{0x8086, 0x8c55, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c56, NT,  "Intel", "C226",				enable_flash_pch8},
	{0x8086, 0x8c57, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c58, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c59, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c5a, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c5b, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c5c, NT,  "Intel", "H81",				enable_flash_pch8},
	{0x8086, 0x8c5d, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c5e, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8c5f, NT,  "Intel", "Lynx Point",			enable_flash_pch8},
	{0x8086, 0x8cc1, NT,  "Intel", "9 Series",			enable_flash_pch9},
	{0x8086, 0x8cc2, NT,  "Intel", "9 Series Engineering Sample",	enable_flash_pch9},
	{0x8086, 0x8cc3, NT,  "Intel", "9 Series",			enable_flash_pch9},
	{0x8086, 0x8cc4, NT,  "Intel", "Z97",				enable_flash_pch9},
	{0x8086, 0x8cc6, NT,  "Intel", "H97",				enable_flash_pch9},
	{0x8086, 0x8d40, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d41, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d42, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d43, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d44, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d45, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d46, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d47, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d48, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d49, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d4a, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d4b, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d4c, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d4d, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d4e, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d4f, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d50, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d51, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d52, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d53, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d54, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d55, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d56, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d57, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d58, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d59, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d5a, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d5b, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d5c, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d5d, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d5e, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x8d5f, NT,  "Intel", "C610/X99 (Wellsburg)",		enable_flash_pch8_wb},
	{0x8086, 0x9c41, NT,  "Intel", "Lynx Point LP Eng. Sample",	enable_flash_pch8_lp},
	{0x8086, 0x9c43, NT,  "Intel", "Lynx Point LP Premium",		enable_flash_pch8_lp},
	{0x8086, 0x9c45, NT,  "Intel", "Lynx Point LP Mainstream",	enable_flash_pch8_lp},
	{0x8086, 0x9c47, NT,  "Intel", "Lynx Point LP Value",		enable_flash_pch8_lp},
	{0x8086, 0x9cc1, NT,  "Intel", "Haswell U Sample",		enable_flash_pch9},
	{0x8086, 0x9cc2, NT,  "Intel", "Broadwell U Sample",		enable_flash_pch9},
	{0x8086, 0x9cc3, NT,  "Intel", "Broadwell U Premium",		enable_flash_pch9},
	{0x8086, 0x9cc5, NT,  "Intel", "Broadwell U Base",		enable_flash_pch9},
	{0x8086, 0x9cc6, NT,  "Intel", "Broadwell Y Sample",		enable_flash_pch9},
	{0x8086, 0x9cc7, NT,  "Intel", "Broadwell Y Premium",		enable_flash_pch9},
	{0x8086, 0x9cc9, NT,  "Intel", "Broadwell Y Base",		enable_flash_pch9},
	{0x8086, 0x9ccb, NT,  "Intel", "Broadwell H",			enable_flash_pch9},
#endif
	{0},
};

int chipset_flash_enable(void)
{
	struct pci_dev *dev = NULL;
	int ret = -2;		/* Nothing! */
	int i;

	/* Now let's try to find the chipset we have... */
	for (i = 0; chipset_enables[i].vendor_name != NULL; i++) {
		dev = pci_dev_find(chipset_enables[i].vendor_id,
				   chipset_enables[i].device_id);
		if (!dev)
			continue;
		if (ret != -2) {
			msg_pwarn("Warning: unexpected second chipset match: "
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
		msg_pinfo(".\n");

		if (chipset_enables[i].status == NT) {
			msg_pinfo("This chipset is marked as untested. If "
				  "you are using an up-to-date version\nof "
				  "flashrom *and* were (not) able to "
				  "successfully update your firmware with it,\n"
				  "then please email a report to "
				  "flashrom@flashrom.org including a verbose "
				  "(-V) log.\nThank you!\n");
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
		if (ret == ERROR_FATAL) {
			msg_perr("FATAL ERROR!\n");
			return ret;
		}
	}

	return ret;
}
