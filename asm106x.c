/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2023 Alex Badea <vamposdecampos@gmail.com>
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

#include <stdlib.h>
#include "programmer.h"
#include "platform/pci.h"

#define PCI_VENDOR_ID_ASMEDIA	0x1b21

#define ASM106X_REG_DATA	0xf0
#define ASM106X_REG_CTRL	0xf4
#define ASM106X_CTRL_RUN	0x20	/* SPI master is running */
#define ASM106X_CTRL_CSN	0x10	/* CS_n pin output */
#define ASM106X_CTRL_WRITE	0x08	/* 0=read, 1=write */
#define ASM106X_CTRL_MASK	0xc0	/* unknown, untouched */

struct asm106x_data {
	struct pci_dev *pci;
};

static const struct dev_entry asm106x_devs[] = {
	{PCI_VENDOR_ID_ASMEDIA, 0x0612, OK, "ASMedia", "ASM106x"},

	{0},
};

static int asm106x_wait_ready(struct pci_dev *pci, uint8_t *pval)
{
	unsigned int timeout = 100;
	uint8_t val;

	while (timeout) {
		val = pci_read_byte(pci, ASM106X_REG_CTRL);
		msg_pdbg2("asm106x status %#02"PRIx8" tries %d\n", val, timeout);
		if (!(val & ASM106X_CTRL_RUN)) {
			if (pval)
				*pval = val;
			return 0;
		}
		default_delay(10);
		timeout--;
	}

	msg_pdbg("asm106x timed out, ctrl %#02"PRIx8"\n", val);
	return 1;
}


static int asm106x_command(const struct flashctx *flash,
	unsigned int writecnt, unsigned int readcnt,
	const unsigned char *writearr, unsigned char *readarr)
{
	struct asm106x_data *data = flash->mst->spi.data;
	uint8_t ctrl;
	int rv = 1;

	msg_pdbg2("asm106x command: wr %d rd %d\n", writecnt, readcnt);
	if (asm106x_wait_ready(data->pci, &ctrl))
		return 1;
	ctrl &= ASM106X_CTRL_MASK;

	while (writecnt) {
		const unsigned int chunk = min(writecnt, 4);
		uint32_t val = 0;

		for (int k = chunk-1; k >= 0; k--)
			val = (val << 8) | writearr[k];
		msg_pdbg2("asm106x write %#08"PRIx32" chunk %u\n", val, chunk);
		pci_write_long(data->pci, ASM106X_REG_DATA, val);
		pci_write_byte(data->pci, ASM106X_REG_CTRL,
			ctrl | ASM106X_CTRL_RUN | ASM106X_CTRL_WRITE | chunk);
		if (asm106x_wait_ready(data->pci, NULL))
			goto out;
		writecnt -= chunk;
		writearr += chunk;
	}
	while (readcnt) {
		const unsigned int chunk = min(readcnt, 4);

		pci_write_byte(data->pci, ASM106X_REG_CTRL,
			ctrl | ASM106X_CTRL_RUN | chunk);
		if (asm106x_wait_ready(data->pci, NULL))
			goto out;

		uint32_t val = pci_read_long(data->pci, ASM106X_REG_DATA);
		msg_pdbg2("asm106x read %#08"PRIx32" chunk %u\n", val, chunk);
		for (unsigned int k = 0; k < chunk; k++) {
			readarr[k] = val & 0xff;
			val >>= 8;
		}
		readcnt -= chunk;
		readarr += chunk;
	}

	rv = 0;
out:
	pci_write_byte(data->pci, ASM106X_REG_CTRL, ctrl | ASM106X_CTRL_CSN);
	return rv;
}

static int asm106x_shutdown(void *data)
{
	free(data);
	return 0;
}

static const struct spi_master asm106x_spi_master = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_WRITE_UNLIMITED,
	.command	= asm106x_command,
	.shutdown	= asm106x_shutdown,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
};

static int asm106x_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *pci;
	struct asm106x_data *data;

	/* TODO: no BAR required (just config space) */
	pci = pcidev_init(cfg, asm106x_devs, PCI_ROM_ADDRESS);
	if (!pci)
		return 1;

	data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("cannot allocate memory for asm106x_data\n");
		return 1;
	}
	data->pci = pci;
	return register_spi_master(&asm106x_spi_master, data);
}

const struct programmer_entry programmer_asm106x = {
	.name			= "asm106x",
	.type			= PCI,
	.devs.dev		= asm106x_devs,
	.init			= asm106x_init,
};
