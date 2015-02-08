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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

/*
 * Datasheet: Intel 82580 Quad/Dual Gigabit Ethernet LAN Controller Datasheet
 * 3.3.1.4: General EEPROM Software Access
 * 4.7: Access to shared resources (FIXME: we should probably use this semaphore interface)
 * 7.4: Register Descriptions
 */

#include <stdlib.h>
#include <unistd.h>
#include "flash.h"
#include "spi.h"
#include "programmer.h"
#include "hwaccess.h"

#define PCI_VENDOR_ID_INTEL 0x8086
#define MEMMAP_SIZE (0x14 + 3) /* Only EEC and EERD are needed. */

#define EEC	0x10 /* EEPROM/Flash Control Register */
#define EERD	0x14 /* EEPROM Read Register */

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

/* EEPROM Read Register bits */
#define EERD_START 0
#define EERD_DONE 1
#define EERD_ADDR 2
#define EERD_DATA 16

#define BIT(x) (1<<x)
#define EE_PAGE_MASK 0x3f

static uint8_t *nicintel_eebar;
static struct pci_dev *nicintel_pci;

#define UNPROG_DEVICE 0x1509

const struct dev_entry nics_intel_ee[] = {
	{PCI_VENDOR_ID_INTEL, 0x150e, OK, "Intel", "82580 Quad Gigabit Ethernet Controller (Copper)"},
	{PCI_VENDOR_ID_INTEL, 0x150f, NT , "Intel", "82580 Quad Gigabit Ethernet Controller (Fiber)"},
	{PCI_VENDOR_ID_INTEL, 0x1510, NT , "Intel", "82580 Quad Gigabit Ethernet Controller (Backplane)"},
	{PCI_VENDOR_ID_INTEL, 0x1511, NT , "Intel", "82580 Quad Gigabit Ethernet Controller (Ext. PHY)"},
	{PCI_VENDOR_ID_INTEL, 0x1511, NT , "Intel", "82580 Dual Gigabit Ethernet Controller (Copper)"},
	{PCI_VENDOR_ID_INTEL, UNPROG_DEVICE, OK, "Intel", "Unprogrammed 82580 Quad/Dual Gigabit Ethernet Controller"},
	{0},
};

static int nicintel_ee_probe(struct flashctx *flash)
{
	if (nicintel_pci->device_id == UNPROG_DEVICE)
		flash->chip->total_size = 16; /* Fall back to minimum supported size. */
	else {
		uint32_t tmp = pci_mmio_readl(nicintel_eebar + EEC);
		tmp = ((tmp >> EE_SIZE) & EE_SIZE_MASK);
		switch (tmp) {
		case 7:
			flash->chip->total_size = 16;
			break;
		case 8:
			flash->chip->total_size = 32;
			break;
		default:
			msg_cerr("Unsupported chip size 0x%x\n", tmp);
			return 0;
		}
	}

	flash->chip->page_size = EE_PAGE_MASK + 1;
	flash->chip->tested = TEST_OK_PREW;
	flash->chip->gran = write_gran_1byte_implicit_erase;
	flash->chip->block_erasers->eraseblocks[0].size = (EE_PAGE_MASK + 1);
	flash->chip->block_erasers->eraseblocks[0].count = (flash->chip->total_size * 1024) / (EE_PAGE_MASK + 1);

	return 1;
}

static int nicintel_ee_read_word(unsigned int addr, uint16_t *data)
{
	uint32_t tmp = BIT(EERD_START) | (addr << EERD_ADDR);
	pci_mmio_writel(tmp, nicintel_eebar + EERD);

	/* Poll done flag. 10.000.000 cycles seem to be enough. */
	uint32_t i;
	for (i = 0; i < 10000000; i++) {
		tmp = pci_mmio_readl(nicintel_eebar + EERD);
		if (tmp & BIT(EERD_DONE)) {
			*data = (tmp >> EERD_DATA) & 0xffff;
			return 0;
		}
	}

	return -1;
}

static int nicintel_ee_read(struct flashctx *flash, uint8_t *buf, unsigned int addr, unsigned int len)
{
	uint16_t data;

	/* The NIC interface always reads 16 b words so we need to convert the address and handle odd address
	 * explicitly at the start (and also at the end in the loop below). */
	if (addr & 1) {
		if (nicintel_ee_read_word(addr / 2, &data))
			return -1;
		*buf++ = data & 0xff;
		addr++;
		len--;
	}

	while (len > 0) {
		if (nicintel_ee_read_word(addr / 2, &data))
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

static int nicintel_ee_bitset(int reg, int bit, bool val)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(nicintel_eebar + reg);
	if (val)
		tmp |= BIT(bit);
	else
		tmp &= ~BIT(bit);
	pci_mmio_writel(tmp, nicintel_eebar + reg);

	return -1;
}

/* Shifts one byte out while receiving another one by bitbanging (denoted "direct access" in the datasheet). */
static int nicintel_ee_bitbang(uint8_t mosi, uint8_t *miso)
{
	uint8_t out = 0x0;

	int i;
	for (i = 7; i >= 0; i--) {
		nicintel_ee_bitset(EEC, EE_SI, mosi & BIT(i));
		nicintel_ee_bitset(EEC, EE_SCK, 1);
		if (miso != NULL) {
			uint32_t tmp = pci_mmio_readl(nicintel_eebar + EEC);
			if (tmp & BIT(EE_SO))
				out |= BIT(i);
		}
		nicintel_ee_bitset(EEC, EE_SCK, 0);
	}

	if (miso != NULL)
		*miso = out;

	return 0;
}

/* Polls the WIP bit of the status register of the attached EEPROM via bitbanging. */
static int nicintel_ee_ready(void)
{
	unsigned int i;
	for (i = 0; i < 1000; i++) {
		nicintel_ee_bitset(EEC, EE_CS, 0);

		nicintel_ee_bitbang(JEDEC_RDSR, NULL);
		uint8_t rdsr;
		nicintel_ee_bitbang(0x00, &rdsr);

		nicintel_ee_bitset(EEC, EE_CS, 1);
		programmer_delay(1);
		if (!(rdsr & SPI_SR_WIP)) {
			return 0;
		}
	}
	return -1;
}

/* Requests direct access to the SPI pins. */
static int nicintel_ee_req(void)
{
	uint32_t tmp;
	nicintel_ee_bitset(EEC, EE_REQ, 1);

	tmp = pci_mmio_readl(nicintel_eebar + EEC);
	if (!(tmp & BIT(EE_GNT))) {
		msg_perr("Enabling eeprom access failed.\n");
		return 1;
	}

	nicintel_ee_bitset(EEC, EE_SCK, 0);
	return 0;
}

static int nicintel_ee_write(struct flashctx *flash, const uint8_t *buf, unsigned int addr, unsigned int len)
{
	if (nicintel_ee_req())
		return -1;

	int ret = -1;
	if (nicintel_ee_ready())
		goto out;

	while (len > 0) {
		/* WREN */
		nicintel_ee_bitset(EEC, EE_CS, 0);
		nicintel_ee_bitbang(JEDEC_WREN, NULL);
		nicintel_ee_bitset(EEC, EE_CS, 1);
		programmer_delay(1);

		/* data */
		nicintel_ee_bitset(EEC, EE_CS, 0);
		nicintel_ee_bitbang(JEDEC_BYTE_PROGRAM, NULL);
		nicintel_ee_bitbang((addr >> 8) & 0xff, NULL);
		nicintel_ee_bitbang(addr & 0xff, NULL);
		while (len > 0) {
			nicintel_ee_bitbang((buf) ? *buf++ : 0xff, NULL);
			len--;
			addr++;
			if (!(addr & EE_PAGE_MASK))
				break;
		}
		nicintel_ee_bitset(EEC, EE_CS, 1);
		programmer_delay(1);
		if (nicintel_ee_ready())
			goto out;
	}
	ret = 0;
out:
	nicintel_ee_bitset(EEC, EE_REQ, 0); /* Give up direct access. */
	return ret;
}

static int nicintel_ee_erase(struct flashctx *flash, unsigned int addr, unsigned int len)
{
	return nicintel_ee_write(flash, NULL, addr, len);
}

static const struct opaque_master opaque_master_nicintel_ee = {
	.probe = nicintel_ee_probe,
	.read = nicintel_ee_read,
	.write = nicintel_ee_write,
	.erase = nicintel_ee_erase,
};

static int nicintel_ee_shutdown(void *eecp)
{
	uint32_t old_eec = *(uint32_t *)eecp;
	/* Request bitbanging and unselect the chip first to be safe. */
	if (nicintel_ee_req() || nicintel_ee_bitset(EEC, EE_CS, 1))
		return -1;

	/* Try to restore individual bits we care about. */
	int ret = nicintel_ee_bitset(EEC, EE_SCK, old_eec & BIT(EE_SCK));
	ret |= nicintel_ee_bitset(EEC, EE_SI, old_eec & BIT(EE_SI));
	ret |= nicintel_ee_bitset(EEC, EE_CS, old_eec & BIT(EE_CS));
	/* REQ will be cleared by hardware anyway after 2 seconds of inactivity on the SPI pins (3.3.2.1). */
	ret |= nicintel_ee_bitset(EEC, EE_REQ, old_eec & BIT(EE_REQ));

	free(eecp);
	return ret;
}

int nicintel_ee_init(void)
{
	if (rget_io_perms())
		return 1;

	struct pci_dev *dev = pcidev_init(nics_intel_ee, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	uint32_t io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	nicintel_eebar = rphysmap("Intel Gigabit NIC w/ SPI EEPROM", io_base_addr, MEMMAP_SIZE);
	nicintel_pci = dev;
	if (dev->device_id != UNPROG_DEVICE) {
		uint32_t eec = pci_mmio_readl(nicintel_eebar + EEC);

		/* C.f. 3.3.1.5 for the detection mechanism (maybe? contradicting the EE_PRES definition),
		 * and 3.3.1.7 for possible recovery. */
		if (!(eec & BIT(EE_PRES))) {
			msg_perr("Controller reports no EEPROM is present.\n");
			return 1;
		}

		uint32_t *eecp = malloc(sizeof(uint32_t));
		if (eecp == NULL)
			return 1;
		*eecp = eec;

		if (register_shutdown(nicintel_ee_shutdown, eecp))
			return 1;
	}

	return register_opaque_master(&opaque_master_nicintel_ee);
}
