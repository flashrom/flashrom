/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2013 Rudolf Marek <r.marek@assembler.cz>
 * Copyright (C) 2013 Stefan Tauner
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

#include "flash.h"
#include "programmer.h"
#include "hwaccess_x86_io.h"
#include "spi.h"
#include "platform/pci.h"

/* same as serverengines */
static void enter_conf_mode_ec(uint16_t port)
{
	OUTB(0x5a, port);
}

static void exit_conf_mode_ec(uint16_t port)
{
	OUTB(0xa5, port);
}

static uint16_t get_sio_port(struct pci_dev *dev)
{
	uint16_t ec_port;

	if (!dev) {
		return 0;
	}

	ec_port = pci_read_word(dev, 0xa4);

	/* EcPortActive? */
	if (!(ec_port & 0x1))
		return 0;

	ec_port &= ~0x1;

	return ec_port;
}

/* Wait for up to 10 ms for a response. */
static int mbox_wait_ack(uint16_t mbox_port)
{
	int i = 10;
	while (sio_read(mbox_port, 0x82) != 0xfa) {
		if (--i == 0) {
			msg_pwarn("IMC MBOX: Timeout!\n");
			return 1;
		}
		default_delay(1000);
	}
	return 0;
}

static uint16_t mbox_get_port(uint16_t sio_port)
{
	uint16_t mbox_port;

	enter_conf_mode_ec(sio_port);

	/* Go to LDN 9, mailbox */
	sio_write(sio_port, 7, 9);

	/* MBOX inactive? */
	if ((sio_read(sio_port, 0x30) & 1) == 0) {
		exit_conf_mode_ec(sio_port);
		return 0;
	}

	mbox_port = sio_read(sio_port, 0x60) << 8;
	mbox_port |= sio_read(sio_port, 0x61);

	exit_conf_mode_ec(sio_port);
	return mbox_port;
}

/* Returns negative values when IMC is inactive, positive values on errors */
static int imc_send_cmd(struct pci_dev *dev, uint8_t cmd)
{
	uint16_t sio_port;
	uint16_t mbox_port;

	/* IntegratedEcPresent? */
	if (!(pci_read_byte(dev, 0x40) & (1 << 7)))
		return -1;

	sio_port = get_sio_port(dev);
	if (!sio_port)
		return -1;

	msg_pdbg2("IMC SIO is at 0x%x.\n", sio_port);
	mbox_port = mbox_get_port(sio_port);
	if (!mbox_port)
		return -1;
	msg_pdbg2("IMC MBOX is at 0x%x.\n", mbox_port);

	sio_write(mbox_port, 0x82, 0x0);
	sio_write(mbox_port, 0x83, cmd);
	sio_write(mbox_port, 0x84, 0x0);
	/* trigger transfer 0x96 with subcommand cmd */
	sio_write(mbox_port, 0x80, 0x96);

	return mbox_wait_ack(mbox_port);
}

static int imc_resume(void *data)
{
	struct pci_dev *dev = data;
	int ret = imc_send_cmd(dev, 0xb5);

	if (ret != 0)
		msg_pinfo("Resuming IMC failed.\n");
	else
		msg_pdbg2("IMC resumed.\n");
	return ret;
}

int amd_imc_shutdown(struct pci_dev *dev)
{
	/* Try to put IMC to sleep */
	int ret = imc_send_cmd(dev, 0xb4);

	/* No IMC activity detectable, assume we are fine */
	if (ret < 0) {
		msg_pdbg2("No IMC found.\n");
		return 0;
	}

	if (ret != 0) {
		msg_perr("Shutting down IMC failed.\n");
		return ret;
	}
	msg_pdbg2("Shutting down IMC successful.\n");

	if (register_shutdown(imc_resume, dev))
		return 1;

	return ret;
}
