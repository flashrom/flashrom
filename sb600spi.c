/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Wang Qingpei <Qingpei.Wang@amd.com>
 * Copyright (C) 2008 Joe Bao <Zheng.Bao@amd.com>
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 * Copyright (C) 2009, 2010 Carl-Daniel Hailfinger
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#if defined(__i386__) || defined(__x86_64__)

#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"
#include "spi.h"

/* This struct is unused, but helps visualize the SB600 SPI BAR layout.
 *struct sb600_spi_controller {
 *	unsigned int spi_cntrl0;	/ * 00h * /
 *	unsigned int restrictedcmd1;	/ * 04h * /
 *	unsigned int restrictedcmd2;	/ * 08h * /
 *	unsigned int spi_cntrl1;	/ * 0ch * /
 *	unsigned int spi_cmdvalue0;	/ * 10h * /
 *	unsigned int spi_cmdvalue1;	/ * 14h * /
 *	unsigned int spi_cmdvalue2;	/ * 18h * /
 *	unsigned int spi_fakeid;	/ * 1Ch * /
 *};
 */

static uint8_t *sb600_spibar = NULL;

static void reset_internal_fifo_pointer(void)
{
	mmio_writeb(mmio_readb(sb600_spibar + 2) | 0x10, sb600_spibar + 2);

	/* FIXME: This loop makes no sense at all. */
	while (mmio_readb(sb600_spibar + 0xD) & 0x7)
		msg_pspew("reset\n");
}

static int compare_internal_fifo_pointer(uint8_t want)
{
	uint8_t tmp;

	tmp = mmio_readb(sb600_spibar + 0xd) & 0x07;
	want &= 0x7;
	if (want != tmp) {
		msg_perr("SB600 FIFO pointer corruption! Pointer is %d, wanted "
			 "%d\n", tmp, want);
		msg_perr("Something else is accessing the flash chip and "
			 "causes random corruption.\nPlease stop all "
			 "applications and drivers and IPMI which access the "
			 "flash chip.\n");
		return 1;
	} else {
		msg_pspew("SB600 FIFO pointer is %d, wanted %d\n", tmp, want);
		return 0;
	}
}

static int reset_compare_internal_fifo_pointer(uint8_t want)
{
	int ret;

	ret = compare_internal_fifo_pointer(want);
	reset_internal_fifo_pointer();
	return ret;
}

static void execute_command(void)
{
	mmio_writeb(mmio_readb(sb600_spibar + 2) | 1, sb600_spibar + 2);

	while (mmio_readb(sb600_spibar + 2) & 1)
		;
}

static int sb600_spi_send_command(struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *writearr,
				  unsigned char *readarr)
{
	int count;
	/* First byte is cmd which can not being sent through FIFO. */
	unsigned char cmd = *writearr++;
	unsigned int readoffby1;
	unsigned char readwrite;

	writecnt--;

	msg_pspew("%s, cmd=%x, writecnt=%x, readcnt=%x\n",
		  __func__, cmd, writecnt, readcnt);

	if (readcnt > 8) {
		msg_pinfo("%s, SB600 SPI controller can not receive %d bytes, "
		       "it is limited to 8 bytes\n", __func__, readcnt);
		return SPI_INVALID_LENGTH;
	}

	if (writecnt > 8) {
		msg_pinfo("%s, SB600 SPI controller can not send %d bytes, "
		       "it is limited to 8 bytes\n", __func__, writecnt);
		return SPI_INVALID_LENGTH;
	}

	/* This is a workaround for a bug in SB600 and SB700. If we only send
	 * an opcode and no additional data/address, the SPI controller will
	 * read one byte too few from the chip. Basically, the last byte of
	 * the chip response is discarded and will not end up in the FIFO.
	 * It is unclear if the CS# line is set high too early as well.
	 */
	readoffby1 = (writecnt) ? 0 : 1;
	readwrite = (readcnt + readoffby1) << 4 | (writecnt);
	mmio_writeb(readwrite, sb600_spibar + 1);
	mmio_writeb(cmd, sb600_spibar + 0);

	/* Before we use the FIFO, reset it first. */
	reset_internal_fifo_pointer();

	/* Send the write byte to FIFO. */
	msg_pspew("Writing: ");
	for (count = 0; count < writecnt; count++, writearr++) {
		msg_pspew("[%02x]", *writearr);
		mmio_writeb(*writearr, sb600_spibar + 0xC);
	}
	msg_pspew("\n");

	/*
	 * We should send the data by sequence, which means we need to reset
	 * the FIFO pointer to the first byte we want to send.
	 */
	if (reset_compare_internal_fifo_pointer(writecnt))
		return SPI_PROGRAMMER_ERROR;

	msg_pspew("Executing: \n");
	execute_command();

	/*
	 * After the command executed, we should find out the index of the
	 * received byte. Here we just reset the FIFO pointer and skip the
	 * writecnt.
	 * It would be possible to increase the FIFO pointer by one instead
	 * of reading and discarding one byte from the FIFO.
	 * The FIFO is implemented on top of an 8 byte ring buffer and the
	 * buffer is never cleared. For every byte that is shifted out after
	 * the opcode, the FIFO already stores the response from the chip.
	 * Usually, the chip will respond with 0x00 or 0xff.
	 */
	if (reset_compare_internal_fifo_pointer(writecnt + readcnt))
		return SPI_PROGRAMMER_ERROR;

	/* Skip the bytes we sent. */
	msg_pspew("Skipping: ");
	for (count = 0; count < writecnt; count++) {
		cmd = mmio_readb(sb600_spibar + 0xC);
		msg_pspew("[%02x]", cmd);
	}
	msg_pspew("\n");
	if (compare_internal_fifo_pointer(writecnt))
		return SPI_PROGRAMMER_ERROR;

	msg_pspew("Reading: ");
	for (count = 0; count < readcnt; count++, readarr++) {
		*readarr = mmio_readb(sb600_spibar + 0xC);
		msg_pspew("[%02x]", *readarr);
	}
	msg_pspew("\n");
	if (reset_compare_internal_fifo_pointer(readcnt + writecnt))
		return SPI_PROGRAMMER_ERROR;

	if (mmio_readb(sb600_spibar + 1) != readwrite) {
		msg_perr("Unexpected change in SB600 read/write count!\n");
		msg_perr("Something else is accessing the flash chip and "
			 "causes random corruption.\nPlease stop all "
			 "applications and drivers and IPMI which access the "
			 "flash chip.\n");
		return SPI_PROGRAMMER_ERROR;
	}

	return 0;
}

static const struct spi_programmer spi_programmer_sb600 = {
	.type = SPI_CONTROLLER_SB600,
	.max_data_read = 8,
	.max_data_write = 5,
	.command = sb600_spi_send_command,
	.multicommand = default_spi_send_multicommand,
	.read = default_spi_read,
	.write_256 = default_spi_write_256,
	.write_aai = default_spi_write_aai,
};

int sb600_probe_spi(struct pci_dev *dev)
{
	struct pci_dev *smbus_dev;
	uint32_t tmp;
	uint8_t reg;
	static const char *const speed_names[4] = {
		"Reserved", "33", "22", "16.5"
	};

	/* Read SPI_BaseAddr */
	tmp = pci_read_long(dev, 0xa0);
	tmp &= 0xffffffe0;	/* remove bits 4-0 (reserved) */
	msg_pdbg("SPI base address is at 0x%x\n", tmp);

	/* If the BAR has address 0, it is unlikely SPI is used. */
	if (!tmp)
		return 0;

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
	/* FIXME: Set bit 3,6,7 if not already set.
	 * Set bit 5, otherwise SPI accesses are pointless in LPC mode.
	 * See doc 42413 AMD SB700/710/750 RPR.
	 */
	msg_pdbg("PrefetchEnSPIFromHost=%i, SpiOpEnInLpcMode=%i\n",
		     tmp & 0x1, (tmp & 0x20) >> 5);
	tmp = mmio_readl(sb600_spibar);
	/* FIXME: If SpiAccessMacRomEn or SpiHostAccessRomEn are zero on
	 * SB700 or later, reads and writes will be corrupted. Abort in this
	 * case. Make sure to avoid this check on SB600.
	 */
	msg_pdbg("SpiArbEnable=%i, SpiAccessMacRomEn=%i, "
		     "SpiHostAccessRomEn=%i, ArbWaitCount=%i, "
		     "SpiBridgeDisable=%i, DropOneClkOnRd=%i\n",
		     (tmp >> 19) & 0x1, (tmp >> 22) & 0x1,
		     (tmp >> 23) & 0x1, (tmp >> 24) & 0x7,
		     (tmp >> 27) & 0x1, (tmp >> 28) & 0x1);
	tmp = (mmio_readb(sb600_spibar + 0xd) >> 4) & 0x3;
	msg_pdbg("NormSpeed is %s MHz\n", speed_names[tmp]);

	/* Look for the SMBus device. */
	smbus_dev = pci_dev_find(0x1002, 0x4385);

	if (!smbus_dev) {
		smbus_dev = pci_dev_find(0x1022, 0x780b); /* AMD Hudson */
		if (!smbus_dev) {
			msg_perr("ERROR: SMBus device not found. Not enabling SPI.\n");
			return ERROR_NONFATAL;
		}
	}

	/* Note about the bit tests below: If a bit is zero, the GPIO is SPI. */
	/* GPIO11/SPI_DO and GPIO12/SPI_DI status */
	reg = pci_read_byte(smbus_dev, 0xAB);
	reg &= 0xC0;
	msg_pdbg("GPIO11 used for %s\n", (reg & (1 << 6)) ? "GPIO" : "SPI_DO");
	msg_pdbg("GPIO12 used for %s\n", (reg & (1 << 7)) ? "GPIO" : "SPI_DI");
	if (reg != 0x00) {
		msg_pdbg("Not enabling SPI");
		return 0;
	}
	/* GPIO31/SPI_HOLD and GPIO32/SPI_CS status */
	reg = pci_read_byte(smbus_dev, 0x83);
	reg &= 0xC0;
	msg_pdbg("GPIO31 used for %s\n", (reg & (1 << 6)) ? "GPIO" : "SPI_HOLD");
	msg_pdbg("GPIO32 used for %s\n", (reg & (1 << 7)) ? "GPIO" : "SPI_CS");
	/* SPI_HOLD is not used on all boards, filter it out. */
	if ((reg & 0x80) != 0x00) {
		msg_pdbg("Not enabling SPI");
		return 0;
	}
	/* GPIO47/SPI_CLK status */
	reg = pci_read_byte(smbus_dev, 0xA7);
	reg &= 0x40;
	msg_pdbg("GPIO47 used for %s\n", (reg & (1 << 6)) ? "GPIO" : "SPI_CLK");
	if (reg != 0x00) {
		msg_pdbg("Not enabling SPI");
		return 0;
	}

	reg = pci_read_byte(dev, 0x40);
	msg_pdbg("SB700 IMC is %sactive.\n", (reg & (1 << 7)) ? "" : "not ");
	if (reg & (1 << 7)) {
		/* If we touch any region used by the IMC, the IMC and the SPI
		 * interface will lock up, and the only way to recover is a
		 * hard reset, but that is a bad choice for a half-erased or
		 * half-written flash chip.
		 * There appears to be an undocumented register which can freeze
		 * or disable the IMC, but for now we want to play it safe.
		 */
		msg_perr("The SB700 IMC is active and may interfere with SPI "
			 "commands. Disabling write.\n");
		/* FIXME: Should we only disable SPI writes, or will the lockup
		 * affect LPC/FWH chips as well?
		 */
		programmer_may_write = 0;
	}

	/* Bring the FIFO to a clean state. */
	reset_internal_fifo_pointer();

	register_spi_programmer(&spi_programmer_sb600);
	return 0;
}

#endif
