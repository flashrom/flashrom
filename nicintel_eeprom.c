/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2013 Ricardo Ribalda - Qtechnology A/S
 * Copyright (C) 2011, 2014 Stefan Tauner
 *
 * Based on nicinctel_spi.c and ichspi.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Datasheet: Intel 82580 Quad/Dual Gigabit Ethernet LAN Controller Datasheet
 * 3.3.1.4: General EEPROM Software Access
 * 4.7: Access to shared resources (FIXME: we should probably use this semaphore interface)
 * 7.4: Register Descriptions
 */
/*
 * Datasheet: Intel Ethernet Controller I210: Datasheet
 * 8.4.3: EEPROM-Mode Read Register
 * 8.4.6: EEPROM-Mode Write Register
 * Write process inspired on kernel e1000_i210.c
 */

#include <stdlib.h>
#include <unistd.h>
#include "flash.h"
#include "spi.h"
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "platform/pci.h"

#define PCI_VENDOR_ID_INTEL 0x8086
#define MEMMAP_SIZE 0x1c /* Only EEC, EERD and EEWR are needed. */

#define EEC	0x10 /* EEPROM/Flash Control Register */
#define EERD	0x14 /* EEPROM Read Register */
#define EEWR	0x18 /* EEPROM Write Register */

/* EPROM/Flash Control Register bits */
#define EE_SCK	0
#define EE_CS	1
#define EE_SI	2
#define EE_SO	3
#define EE_REQ	6
#define EE_GNT	7
#define EE_PRES	8
#define EE_SIZE 11
#define EE_SIZE_MASK 0xf
#define EE_FLUPD   23
#define EE_FLUDONE 26

/* EEPROM Read Register bits */
#define EERD_START 0
#define EERD_DONE 1
#define EERD_ADDR 2
#define EERD_DATA 16

/* EEPROM Write Register bits */
#define EEWR_CMDV 0
#define EEWR_DONE 1
#define EEWR_ADDR 2
#define EEWR_DATA 16

#define EE_PAGE_MASK 0x3f

#define UNPROG_DEVICE 0x1509

struct nicintel_eeprom_data {
	struct pci_dev *nicintel_pci;
	uint8_t *nicintel_eebar;

	/* Intel 82580 variable(s) */
	uint32_t eec;

	/* Intel I210 variable(s) */
	bool done_i210_write;
};

/*
 * Warning: is_i210() below makes assumptions on these PCI ids.
 *          It may have to be updated when this list is extended.
 */
static const struct dev_entry nics_intel_ee[] = {
	{PCI_VENDOR_ID_INTEL, 0x150e, OK, "Intel", "82580 Quad Gigabit Ethernet Controller (Copper)"},
	{PCI_VENDOR_ID_INTEL, 0x150f, NT , "Intel", "82580 Quad Gigabit Ethernet Controller (Fiber)"},
	{PCI_VENDOR_ID_INTEL, 0x1510, NT , "Intel", "82580 Quad Gigabit Ethernet Controller (Backplane)"},
	{PCI_VENDOR_ID_INTEL, 0x1511, NT , "Intel", "82580 Quad Gigabit Ethernet Controller (Ext. PHY)"},
	{PCI_VENDOR_ID_INTEL, 0x1511, NT , "Intel", "82580 Dual Gigabit Ethernet Controller (Copper)"},
	{PCI_VENDOR_ID_INTEL, UNPROG_DEVICE, OK, "Intel", "Unprogrammed 82580 Quad/Dual Gigabit Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x1531, OK, "Intel", "I210 Gigabit Network Connection Unprogrammed"},
	{PCI_VENDOR_ID_INTEL, 0x1532, NT, "Intel", "I211 Gigabit Network Connection Unprogrammed"},
	{PCI_VENDOR_ID_INTEL, 0x1533, OK, "Intel", "I210 Gigabit Network Connection"},
	{PCI_VENDOR_ID_INTEL, 0x1536, NT, "Intel", "I210 Gigabit Network Connection SERDES Fiber"},
	{PCI_VENDOR_ID_INTEL, 0x1537, NT, "Intel", "I210 Gigabit Network Connection SERDES Backplane"},
	{PCI_VENDOR_ID_INTEL, 0x1538, NT, "Intel", "I210 Gigabit Network Connection SGMII"},
	{PCI_VENDOR_ID_INTEL, 0x1539, NT, "Intel", "I211 Gigabit Network Connection"},
	{0},
};

static inline bool is_i210(uint16_t device_id)
{
	return (device_id & 0xfff0) == 0x1530;
}

static int nicintel_ee_probe_i210(struct flashctx *flash)
{
	/* Emulated eeprom has a fixed size of 4 KB */
	flash->chip->total_size = 4;
	flash->chip->page_size = flash->chip->total_size * 1024;
	flash->chip->tested = TEST_OK_PREWB;
	flash->chip->gran = WRITE_GRAN_1BYTE_IMPLICIT_ERASE;
	flash->chip->block_erasers->eraseblocks[0].size = flash->chip->page_size;
	flash->chip->block_erasers->eraseblocks[0].count = 1;

	return 1;
}

static int nicintel_ee_probe_82580(struct flashctx *flash)
{
	const struct nicintel_eeprom_data *data = flash->mst->opaque.data;

	if (data->nicintel_pci->device_id == UNPROG_DEVICE)
		flash->chip->total_size = 16; /* Fall back to minimum supported size. */
	else {
		uint32_t tmp = pci_mmio_readl(data->nicintel_eebar + EEC);
		tmp = ((tmp >> EE_SIZE) & EE_SIZE_MASK);
		switch (tmp) {
		case 7:
			flash->chip->total_size = 16;
			break;
		case 8:
			flash->chip->total_size = 32;
			break;
		default:
			msg_cerr("Unsupported chip size 0x%"PRIx32"\n", tmp);
			return 0;
		}
	}

	flash->chip->page_size = EE_PAGE_MASK + 1;
	flash->chip->tested = TEST_OK_PREWB;
	flash->chip->gran = WRITE_GRAN_1BYTE_IMPLICIT_ERASE;
	flash->chip->block_erasers->eraseblocks[0].size = (EE_PAGE_MASK + 1);
	flash->chip->block_erasers->eraseblocks[0].count = (flash->chip->total_size * 1024) / (EE_PAGE_MASK + 1);

	return 1;
}

#define MAX_ATTEMPTS 10000000
static int nicintel_ee_read_word(uint8_t *eebar, unsigned int addr, uint16_t *data)
{
	uint32_t tmp = BIT(EERD_START) | (addr << EERD_ADDR);
	pci_mmio_writel(tmp, eebar + EERD);

	/* Poll done flag. 10.000.000 cycles seem to be enough. */
	uint32_t i;
	for (i = 0; i < MAX_ATTEMPTS; i++) {
		tmp = pci_mmio_readl(eebar + EERD);
		if (tmp & BIT(EERD_DONE)) {
			*data = (tmp >> EERD_DATA) & 0xffff;
			return 0;
		}
	}

	return -1;
}

static int nicintel_ee_read(struct flashctx *flash, uint8_t *buf, unsigned int addr, unsigned int len)
{
	const struct nicintel_eeprom_data *opaque_data = flash->mst->opaque.data;
	uint16_t data;

	/* The NIC interface always reads 16 b words so we need to convert the address and handle odd address
	 * explicitly at the start (and also at the end in the loop below). */
	if (addr & 1) {
		if (nicintel_ee_read_word(opaque_data->nicintel_eebar, addr / 2, &data))
			return -1;
		*buf++ = data & 0xff;
		addr++;
		len--;
	}

	while (len > 0) {
		if (nicintel_ee_read_word(opaque_data->nicintel_eebar, addr / 2, &data))
			return -1;
		*buf++ = data & 0xff;
		addr++;
		len--;
		if (len > 0) {
			*buf++ = (data >> 8) & 0xff;
			addr++;
			len--;
		}
	}

	return 0;
}

static int nicintel_ee_write_word_i210(uint8_t *eebar, unsigned int addr, uint16_t data)
{
	uint32_t eewr;

	eewr = addr << EEWR_ADDR;
	eewr |= data << EEWR_DATA;
	eewr |= BIT(EEWR_CMDV);
	pci_mmio_writel(eewr, eebar + EEWR);

	default_delay(5);
	int i;
	for (i = 0; i < MAX_ATTEMPTS; i++)
		if (pci_mmio_readl(eebar + EEWR) & BIT(EEWR_DONE))
			return 0;
	return -1;
}

static int nicintel_ee_write_i210(struct flashctx *flash, const uint8_t *buf,
				  unsigned int addr, unsigned int len)
{
	struct nicintel_eeprom_data *opaque_data = flash->mst->opaque.data;
	opaque_data->done_i210_write = true;

	if (addr & 1) {
		uint16_t data;

		if (nicintel_ee_read_word(opaque_data->nicintel_eebar, addr / 2, &data)) {
			msg_perr("Timeout reading heading byte\n");
			return -1;
		}

		data &= 0xff;
		data |= (buf ? (buf[0]) : 0xff) << 8;

		if (nicintel_ee_write_word_i210(opaque_data->nicintel_eebar, addr / 2, data)) {
			msg_perr("Timeout writing heading word\n");
			return -1;
		}

		if (buf)
			buf ++;
		addr ++;
		len --;
	}

	while (len > 0) {
		uint16_t data;

		if (len == 1) {
			if (nicintel_ee_read_word(opaque_data->nicintel_eebar, addr / 2, &data)) {
				msg_perr("Timeout reading tail byte\n");
				return -1;
			}

			data &= 0xff00;
			data |= buf ? (buf[0]) : 0xff;
		} else {
			if (buf)
				data = buf[0] | (buf[1] << 8);
			else
				data = 0xffff;
		}

		if (nicintel_ee_write_word_i210(opaque_data->nicintel_eebar, addr / 2, data)) {
			msg_perr("Timeout writing Shadow RAM\n");
			return -1;
		}

		if (buf)
			buf += 2;
		if (len > 2)
			len -= 2;
		else
			len = 0;
		addr += 2;
	}

	return 0;
}

static int nicintel_ee_erase_i210(struct flashctx *flash, unsigned int addr, unsigned int len)
{
	return nicintel_ee_write_i210(flash, NULL, addr, len);
}

static int nicintel_ee_bitset(uint8_t *eebar, int reg, int bit, bool val)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(eebar + reg);
	if (val)
		tmp |= BIT(bit);
	else
		tmp &= ~BIT(bit);
	pci_mmio_writel(tmp, eebar + reg);

	return -1;
}

/* Shifts one byte out while receiving another one by bitbanging (denoted "direct access" in the datasheet). */
static int nicintel_ee_bitbang(uint8_t *eebar, uint8_t mosi, uint8_t *miso)
{
	uint8_t out = 0x0;

	int i;
	for (i = 7; i >= 0; i--) {
		nicintel_ee_bitset(eebar, EEC, EE_SI, mosi & BIT(i));
		nicintel_ee_bitset(eebar, EEC, EE_SCK, 1);
		if (miso != NULL) {
			uint32_t tmp = pci_mmio_readl(eebar + EEC);
			if (tmp & BIT(EE_SO))
				out |= BIT(i);
		}
		nicintel_ee_bitset(eebar, EEC, EE_SCK, 0);
	}

	if (miso != NULL)
		*miso = out;

	return 0;
}

/* Polls the WIP bit of the status register of the attached EEPROM via bitbanging. */
static int nicintel_ee_ready(uint8_t *eebar)
{
	unsigned int i;
	for (i = 0; i < 1000; i++) {
		nicintel_ee_bitset(eebar, EEC, EE_CS, 0);

		nicintel_ee_bitbang(eebar, JEDEC_RDSR, NULL);
		uint8_t rdsr;
		nicintel_ee_bitbang(eebar, 0x00, &rdsr);

		nicintel_ee_bitset(eebar, EEC, EE_CS, 1);
		default_delay(1);
		if (!(rdsr & SPI_SR_WIP)) {
			return 0;
		}
	}
	return -1;
}

/* Requests direct access to the SPI pins. */
static int nicintel_ee_req(uint8_t *eebar)
{
	uint32_t tmp;
	nicintel_ee_bitset(eebar, EEC, EE_REQ, 1);

	tmp = pci_mmio_readl(eebar + EEC);
	if (!(tmp & BIT(EE_GNT))) {
		msg_perr("Enabling eeprom access failed.\n");
		return 1;
	}

	nicintel_ee_bitset(eebar, EEC, EE_SCK, 0);
	return 0;
}

static int nicintel_ee_write_82580(struct flashctx *flash, const uint8_t *buf, unsigned int addr, unsigned int len)
{
	const struct nicintel_eeprom_data *opaque_data = flash->mst->opaque.data;
	uint8_t *eebar = opaque_data->nicintel_eebar;

	if (nicintel_ee_req(eebar))
		return -1;

	int ret = -1;
	if (nicintel_ee_ready(eebar))
		goto out;

	while (len > 0) {
		/* WREN */
		nicintel_ee_bitset(eebar, EEC, EE_CS, 0);
		nicintel_ee_bitbang(eebar, JEDEC_WREN, NULL);
		nicintel_ee_bitset(eebar, EEC, EE_CS, 1);
		default_delay(1);

		/* data */
		nicintel_ee_bitset(eebar, EEC, EE_CS, 0);
		nicintel_ee_bitbang(eebar, JEDEC_BYTE_PROGRAM, NULL);
		nicintel_ee_bitbang(eebar, (addr >> 8) & 0xff, NULL);
		nicintel_ee_bitbang(eebar, addr & 0xff, NULL);
		while (len > 0) {
			nicintel_ee_bitbang(eebar, (buf) ? *buf++ : 0xff, NULL);
			len--;
			addr++;
			if (!(addr & EE_PAGE_MASK))
				break;
		}
		nicintel_ee_bitset(eebar, EEC, EE_CS, 1);
		default_delay(1);
		if (nicintel_ee_ready(eebar))
			goto out;
	}
	ret = 0;
out:
	nicintel_ee_bitset(eebar, EEC, EE_REQ, 0); /* Give up direct access. */
	return ret;
}

static int nicintel_ee_erase_82580(struct flashctx *flash, unsigned int addr, unsigned int len)
{
	return nicintel_ee_write_82580(flash, NULL, addr, len);
}

static int nicintel_ee_shutdown_i210(void *opaque_data)
{
	struct nicintel_eeprom_data *data = opaque_data;
	int ret = 0;

	if (!data->done_i210_write)
		goto out;

	uint32_t flup = pci_mmio_readl(data->nicintel_eebar + EEC);

	flup |= BIT(EE_FLUPD);
	pci_mmio_writel(flup, data->nicintel_eebar + EEC);

	int i;
	for (i = 0; i < MAX_ATTEMPTS; i++)
		if (pci_mmio_readl(data->nicintel_eebar + EEC) & BIT(EE_FLUDONE))
			goto out;

	ret = -1;
	msg_perr("Flash update failed\n");

out:
	free(data);
	return ret;
}

static int nicintel_ee_shutdown_82580(void *opaque_data)
{
	struct nicintel_eeprom_data *data = opaque_data;
	uint8_t *eebar = data->nicintel_eebar;
	int ret = 0;

	if (data->nicintel_pci->device_id != UNPROG_DEVICE) {
		uint32_t old_eec = data->eec;
		/* Request bitbanging and unselect the chip first to be safe. */
		if (nicintel_ee_req(eebar) || nicintel_ee_bitset(eebar, EEC, EE_CS, 1)) {
			ret = -1;
			goto out;
		}

		/* Try to restore individual bits we care about. */
		ret = nicintel_ee_bitset(eebar, EEC, EE_SCK, old_eec & BIT(EE_SCK));
		ret |= nicintel_ee_bitset(eebar, EEC, EE_SI, old_eec & BIT(EE_SI));
		ret |= nicintel_ee_bitset(eebar, EEC, EE_CS, old_eec & BIT(EE_CS));
		/* REQ will be cleared by hardware anyway after 2 seconds of inactivity
		 * on the SPI pins (3.3.2.1). */
		ret |= nicintel_ee_bitset(eebar, EEC, EE_REQ, old_eec & BIT(EE_REQ));
	}

out:
	free(data);
	return ret;
}

static const struct opaque_master opaque_master_nicintel_ee_82580 = {
	.probe		= nicintel_ee_probe_82580,
	.read		= nicintel_ee_read,
	.write		= nicintel_ee_write_82580,
	.erase		= nicintel_ee_erase_82580,
	.shutdown	= nicintel_ee_shutdown_82580,
};

static const struct opaque_master opaque_master_nicintel_ee_i210 = {
	.probe		= nicintel_ee_probe_i210,
	.read		= nicintel_ee_read,
	.write		= nicintel_ee_write_i210,
	.erase		= nicintel_ee_erase_i210,
	.shutdown	= nicintel_ee_shutdown_i210,
};

static int nicintel_ee_init(const struct programmer_cfg *cfg)
{
	const struct opaque_master *mst;
	uint32_t eec = 0;
	uint8_t *eebar;

	struct pci_dev *dev = pcidev_init(cfg, nics_intel_ee, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	uint32_t io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	if (!is_i210(dev->device_id)) {
		eebar = rphysmap("Intel Gigabit NIC w/ SPI EEPROM", io_base_addr, MEMMAP_SIZE);
		if (!eebar)
			return 1;

		if (dev->device_id != UNPROG_DEVICE) {
			eec = pci_mmio_readl(eebar + EEC);

			/* C.f. 3.3.1.5 for the detection mechanism (maybe? contradicting
			                the EE_PRES definition),
			    and 3.3.1.7 for possible recovery. */
			if (!(eec & BIT(EE_PRES))) {
				msg_perr("Controller reports no EEPROM is present.\n");
				return 1;
			}
		}

		mst = &opaque_master_nicintel_ee_82580;
	} else {
		eebar = rphysmap("Intel i210 NIC w/ emulated EEPROM",
					  io_base_addr + 0x12000, MEMMAP_SIZE);
		if (!eebar)
			return 1;

		mst = &opaque_master_nicintel_ee_i210;
	}

	struct nicintel_eeprom_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for OPAQUE master data\n");
		return 1;
	}
	data->nicintel_pci = dev;
	data->nicintel_eebar = eebar;
	data->eec = eec;
	data->done_i210_write = false;

	return register_opaque_master(mst, data);
}

const struct programmer_entry programmer_nicintel_eeprom = {
	.name			= "nicintel_eeprom",
	.type			= PCI,
	.devs.dev		= nics_intel_ee,
	.init			= nicintel_ee_init,
};
