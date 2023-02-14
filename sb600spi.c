/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Wang Qingpei <Qingpei.Wang@amd.com>
 * Copyright (C) 2008 Joe Bao <Zheng.Bao@amd.com>
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 * Copyright (C) 2009, 2010, 2013 Carl-Daniel Hailfinger
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

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "spi.h"
#include "platform/pci.h"

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

enum amd_chipset {
	CHIPSET_AMD_UNKNOWN,
	CHIPSET_SB6XX,
	CHIPSET_SB7XX, /* SP5100 too */
	CHIPSET_SB89XX, /* Hudson-1 too */
	CHIPSET_HUDSON234,
	CHIPSET_BOLTON,
	CHIPSET_YANGTZE,
	CHIPSET_PROMONTORY,
};

#define FIFO_SIZE_OLD		8
#define FIFO_SIZE_YANGTZE	71

#define SPI100_CMD_CODE_REG	0x45
#define SPI100_CMD_TRIGGER_REG	0x47
#define   SPI100_EXECUTE_CMD	(1 << 7)

struct sb600spi_data {
	struct flashctx *flash;
	uint8_t *spibar;
};

static int find_smbus_dev_rev(uint16_t vendor, uint16_t device)
{
	struct pci_dev *smbus_dev = pcidev_find(vendor, device);
	if (!smbus_dev) {
		msg_pdbg("No SMBus device with ID %04X:%04X found.\n", vendor, device);
		msg_perr("ERROR: SMBus device not found. Not enabling SPI.\n");
		return -1;
	}
	return pci_read_byte(smbus_dev, PCI_REVISION_ID);
}

/* Determine the chipset's version and identify the respective SMBUS device. */
static enum amd_chipset determine_generation(struct pci_dev *dev)
{
	msg_pdbg2("Trying to determine the generation of the SPI interface... ");
	if (dev->device_id == 0x438d) {
		msg_pdbg("SB6xx detected.\n");
		return CHIPSET_SB6XX;
	} else if (dev->device_id == 0x439d) {
		int rev = find_smbus_dev_rev(0x1002, 0x4385);
		if (rev < 0)
			return CHIPSET_AMD_UNKNOWN;
		if (rev >= 0x39 && rev <= 0x3D) {
			msg_pdbg("SB7xx/SP5100 detected.\n");
			return CHIPSET_SB7XX;
		} else if (rev >= 0x40 && rev <= 0x42) {
			msg_pdbg("SB8xx/SB9xx/Hudson-1 detected.\n");
			return CHIPSET_SB89XX;
		} else {
			msg_pwarn("SB device found but SMBus revision 0x%02x does not match known values.\n"
				  "Assuming SB8xx/SB9xx/Hudson-1. Please send a log to flashrom@flashrom.org\n",
				   rev);
			return CHIPSET_SB89XX;
		}
	} else if (dev->device_id == 0x780e) {
		/* The PCI ID of the LPC bridge doesn't change between Hudson-2/3/4 and Yangtze (Kabini/Temash)
		 * although they use different SPI interfaces. */
		int rev = find_smbus_dev_rev(0x1022, 0x780B);
		if (rev < 0)
			return CHIPSET_AMD_UNKNOWN;
		if (rev >= 0x11 && rev <= 0x15) {
			msg_pdbg("Hudson-2/3/4 detected.\n");
			return CHIPSET_HUDSON234;
		} else if (rev == 0x16) {
			msg_pdbg("Bolton detected.\n");
			return CHIPSET_BOLTON;
		} else if ((rev >= 0x39 && rev <= 0x3A) || rev == 0x42) {
			msg_pdbg("Yangtze detected.\n");
			return CHIPSET_YANGTZE;
		} else {
			msg_pwarn("FCH device found but SMBus revision 0x%02x does not match known values.\n"
				  "Please report this to flashrom@flashrom.org and include this log and\n"
				  "the output of lspci -nnvx, thanks!.\n", rev);
		}
	} else if (dev->device_id == 0x790e) {
		int rev = find_smbus_dev_rev(0x1022, 0x790B);
		if (rev < 0)
			return CHIPSET_AMD_UNKNOWN;
		if (rev == 0x4a) {
			msg_pdbg("Yangtze detected.\n");
			return CHIPSET_YANGTZE;
		/**
		 * FCH chipsets called 'Promontory' are one's with the
		 * so-called SPI100 ip core that uses memory mapping and
		 * not a ring buffer for transactions. Typically this is
		 * found on both Stoney Ridge and Zen platforms.
		 *
		 * The revisions I have found by searching various lspci
		 * outputs are as follows: 0x4b, 0x59, 0x61 & 0x71.
		 */
		} else if (rev == 0x4b || rev == 0x51 || rev == 0x59 || rev == 0x61 || rev == 0x71) {
			msg_pdbg("Promontory (rev 0x%02x) detected.\n", rev);
			return CHIPSET_PROMONTORY;
		} else {
			msg_pwarn("FCH device found but SMBus revision 0x%02x does not match known values.\n"
				  "Please report this to flashrom@flashrom.org and include this log and\n"
				  "the output of lspci -nnvx, thanks!.\n", rev);
		}


	} else
		msg_pwarn("%s: Unknown LPC device %" PRIx16 ":%" PRIx16 ".\n"
			  "Please report this to flashrom@flashrom.org and include this log and\n"
			  "the output of lspci -nnvx, thanks!\n",
			  __func__, dev->vendor_id, dev->device_id);

	msg_perr("Could not determine chipset generation.");
	return CHIPSET_AMD_UNKNOWN;
}

static void reset_internal_fifo_pointer(uint8_t *sb600_spibar)
{
	mmio_writeb(mmio_readb(sb600_spibar + 2) | 0x10, sb600_spibar + 2);

	/* FIXME: This loop needs a timeout and a clearer message. */
	while (mmio_readb(sb600_spibar + 0xD) & 0x7)
		msg_pspew("reset\n");
}

static int compare_internal_fifo_pointer(uint8_t want, uint8_t *sb600_spibar)
{
	uint8_t have = mmio_readb(sb600_spibar + 0xd) & 0x07;
	want %= FIFO_SIZE_OLD;
	if (have != want) {
		msg_perr("AMD SPI FIFO pointer corruption! Pointer is %d, wanted %d\n", have, want);
		msg_perr("Something else is accessing the flash chip and causes random corruption.\n"
			 "Please stop all applications and drivers and IPMI which access the flash chip.\n");
		return 1;
	} else {
		msg_pspew("AMD SPI FIFO pointer is %d, wanted %d\n", have, want);
		return 0;
	}
}

/* Check the number of bytes to be transmitted and extract opcode. */
static int check_readwritecnt(const struct flashctx *flash, unsigned int writecnt, unsigned int readcnt)
{
	unsigned int maxwritecnt = flash->mst->spi.max_data_write + 3;
	if (writecnt > maxwritecnt) {
		msg_pinfo("%s: SPI controller can not send %d bytes, it is limited to %d bytes\n",
			  __func__, writecnt, maxwritecnt);
		return SPI_INVALID_LENGTH;
	}

	unsigned int maxreadcnt = flash->mst->spi.max_data_read;
	if (readcnt > maxreadcnt) {
		msg_pinfo("%s: SPI controller can not receive %d bytes, it is limited to %d bytes\n",
			  __func__, readcnt, maxreadcnt);
		return SPI_INVALID_LENGTH;
	}
	return 0;
}

static void execute_command(uint8_t *sb600_spibar)
{
	msg_pspew("Executing... ");
	mmio_writeb(mmio_readb(sb600_spibar + 2) | 1, sb600_spibar + 2);
	while (mmio_readb(sb600_spibar + 2) & 1)
		;
	msg_pspew("done\n");
}

static void execute_spi100_command(uint8_t *sb600_spibar)
{
	msg_pspew("Executing... ");
	mmio_writeb(mmio_readb(sb600_spibar + SPI100_CMD_TRIGGER_REG) | SPI100_EXECUTE_CMD,
			sb600_spibar + SPI100_CMD_TRIGGER_REG);
	while (mmio_readb(sb600_spibar + SPI100_CMD_TRIGGER_REG) & SPI100_CMD_TRIGGER_REG)
		;
	msg_pspew("done\n");
}

static int sb600_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *writearr,
				  unsigned char *readarr)
{
	struct sb600spi_data *sb600_data = flash->mst->spi.data;
	uint8_t *sb600_spibar = sb600_data->spibar;
	/* First byte is cmd which can not be sent through the FIFO. */
	unsigned char cmd = *writearr++;
	writecnt--;
	msg_pspew("%s, cmd=0x%02x, writecnt=%d, readcnt=%d\n", __func__, cmd, writecnt, readcnt);
	mmio_writeb(cmd, sb600_spibar + 0);

	int ret = check_readwritecnt(flash, writecnt, readcnt);
	if (ret != 0)
		return ret;

	/* This is a workaround for a bug in SPI controller. If we only send
	 * an opcode and no additional data/address, the SPI controller will
	 * read one byte too few from the chip. Basically, the last byte of
	 * the chip response is discarded and will not end up in the FIFO.
	 * It is unclear if the CS# line is set high too early as well.
	 */
	unsigned int readoffby1 = (writecnt > 0) ? 0 : 1;
	uint8_t readwrite = (readcnt + readoffby1) << 4 | (writecnt);
	mmio_writeb(readwrite, sb600_spibar + 1);

	reset_internal_fifo_pointer(sb600_spibar);
	msg_pspew("Filling FIFO: ");
	unsigned int count;
	for (count = 0; count < writecnt; count++) {
		msg_pspew("[%02x]", writearr[count]);
		mmio_writeb(writearr[count], sb600_spibar + 0xC);
	}
	msg_pspew("\n");
	if (compare_internal_fifo_pointer(writecnt, sb600_spibar))
		return SPI_PROGRAMMER_ERROR;

	/*
	 * We should send the data in sequence, which means we need to reset
	 * the FIFO pointer to the first byte we want to send.
	 */
	reset_internal_fifo_pointer(sb600_spibar);
	execute_command(sb600_spibar);
	if (compare_internal_fifo_pointer(writecnt + readcnt, sb600_spibar))
		return SPI_PROGRAMMER_ERROR;

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
	reset_internal_fifo_pointer(sb600_spibar);

	/* Skip the bytes we sent. */
	msg_pspew("Skipping: ");
	for (count = 0; count < writecnt; count++) {
		msg_pspew("[%02x]", mmio_readb(sb600_spibar + 0xC));
	}
	msg_pspew("\n");
	if (compare_internal_fifo_pointer(writecnt, sb600_spibar))
		return SPI_PROGRAMMER_ERROR;

	msg_pspew("Reading FIFO: ");
	for (count = 0; count < readcnt; count++) {
		readarr[count] = mmio_readb(sb600_spibar + 0xC);
		msg_pspew("[%02x]", readarr[count]);
	}
	msg_pspew("\n");
	if (compare_internal_fifo_pointer(writecnt+readcnt, sb600_spibar))
		return SPI_PROGRAMMER_ERROR;

	if (mmio_readb(sb600_spibar + 1) != readwrite) {
		msg_perr("Unexpected change in AMD SPI read/write count!\n");
		msg_perr("Something else is accessing the flash chip and causes random corruption.\n"
			 "Please stop all applications and drivers and IPMI which access the flash chip.\n");
		return SPI_PROGRAMMER_ERROR;
	}

	return 0;
}

static int spi100_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *writearr,
				  unsigned char *readarr)
{
	struct sb600spi_data *sb600_data = flash->mst->spi.data;
	uint8_t *sb600_spibar = sb600_data->spibar;
	/* First byte is cmd which can not be sent through the buffer. */
	unsigned char cmd = *writearr++;
	writecnt--;
	msg_pspew("%s, cmd=0x%02x, writecnt=%d, readcnt=%d\n", __func__, cmd, writecnt, readcnt);
	mmio_writeb(cmd, sb600_spibar + SPI100_CMD_CODE_REG);

	int ret = check_readwritecnt(flash, writecnt, readcnt);
	if (ret != 0)
		return ret;

	/* Use the extended TxByteCount and RxByteCount registers. */
	mmio_writeb(writecnt, sb600_spibar + 0x48);
	mmio_writeb(readcnt, sb600_spibar + 0x4b);

	msg_pspew("Filling buffer: ");
	unsigned int count;
	for (count = 0; count < writecnt; count++) {
		msg_pspew("[%02x]", writearr[count]);
		mmio_writeb(writearr[count], sb600_spibar + 0x80 + count);
	}
	msg_pspew("\n");

	execute_spi100_command(sb600_spibar);

	msg_pspew("Reading buffer: ");
	for (count = 0; count < readcnt; count++) {
		readarr[count] = mmio_readb(sb600_spibar + 0x80 + (writecnt + count) % FIFO_SIZE_YANGTZE);
		msg_pspew("[%02x]", readarr[count]);
	}
	msg_pspew("\n");

	return 0;
}

struct spispeed {
	const char *const name;
	const uint8_t speed;
};

static const char* spispeeds[] = {
	"66 MHz",
	"33 MHz",
	"22 MHz",
	"16.5 MHz",
	"100 MHz",
	"Reserved",
	"Reserved",
	"800 kHz",
};

static const char* spireadmodes[] = {
	"Normal (up to 33 MHz)",
	"Reserved",
	"Dual IO (1-1-2)",
	"Quad IO (1-1-4)",
	"Dual IO (1-2-2)",
	"Quad IO (1-4-4)",
	"Normal (up to 66 MHz)",
	"Fast Read",
};

static int set_speed(struct pci_dev *dev, enum amd_chipset amd_gen, uint8_t speed, uint8_t *sb600_spibar)
{
	bool success = false;

	msg_pdbg("Setting SPI clock to %s (%i)... ", spispeeds[speed], speed);
	if (amd_gen >= CHIPSET_YANGTZE) {
		rmmio_writew((speed << 12) | (speed << 8) | (speed << 4) | speed, sb600_spibar + 0x22);
		uint16_t tmp = mmio_readw(sb600_spibar + 0x22);
		success = (((tmp >> 12) & 0xf) == speed && ((tmp >> 8) & 0xf) == speed &&
			   ((tmp >> 4) & 0xf) == speed && ((tmp >> 0) & 0xf) == speed);
	} else {
		rmmio_writeb((mmio_readb(sb600_spibar + 0xd) & ~(0x3 << 4)) | (speed << 4), sb600_spibar + 0xd);
		success = (speed == ((mmio_readb(sb600_spibar + 0xd) >> 4) & 0x3));
	}

	if (!success) {
		msg_perr("FAILED!\n");
		return 1;
	}
	msg_pdbg("succeeded.\n");
	return 0;
}

static int set_mode(struct pci_dev *dev, uint8_t mode, uint8_t *sb600_spibar)
{
	msg_pdbg("Setting SPI read mode to %s (%i)... ", spireadmodes[mode], mode);
	uint32_t tmp = mmio_readl(sb600_spibar + 0x00);
	tmp &= ~(0x6 << 28 | 0x1 << 18); /* Clear mode bits */
	tmp |= ((mode & 0x6) << 28) | ((mode & 0x1) << 18);
	rmmio_writel(tmp, sb600_spibar + 0x00);
	if (tmp != mmio_readl(sb600_spibar + 0x00)) {
		msg_perr("FAILED!\n");
		return 1;
	}
	msg_pdbg("succeeded.\n");
	return 0;
}

static int handle_speed(const struct programmer_cfg *cfg,
		struct pci_dev *dev, enum amd_chipset amd_gen, uint8_t *sb600_spibar)
{
	uint32_t tmp;
	int16_t spispeed_idx = -1;
	int16_t spireadmode_idx = -1;
	char *param_str;

	param_str = extract_programmer_param_str(cfg, "spispeed");
	if (param_str != NULL) {
		unsigned int i;
		for (i = 0; i < ARRAY_SIZE(spispeeds); i++) {
			if (strcasecmp(spispeeds[i], param_str) == 0) {
				spispeed_idx = i;
				break;
			}
		}
		/* "reserved" is not a valid speed.
		 * Error out on speeds not present in the spispeeds array.
		 * Only Yangtze supports the second half of indices.
		 * No 66 MHz before SB8xx. */
		if ((strcasecmp(param_str, "reserved") == 0) ||
		    (i == ARRAY_SIZE(spispeeds)) ||
		    (amd_gen < CHIPSET_YANGTZE && spispeed_idx > 3) ||
		    (amd_gen < CHIPSET_SB89XX && spispeed_idx == 0)) {
			msg_perr("Error: Invalid spispeed value: '%s'.\n", param_str);
			free(param_str);
			return 1;
		}
		free(param_str);
	}

	param_str = extract_programmer_param_str(cfg, "spireadmode");
	if (param_str != NULL) {
		unsigned int i;
		for (i = 0; i < ARRAY_SIZE(spireadmodes); i++) {
			if (strcasecmp(spireadmodes[i], param_str) == 0) {
				spireadmode_idx = i;
				break;
			}
		}
		if ((strcasecmp(param_str, "reserved") == 0) ||
		    (i == ARRAY_SIZE(spireadmodes))) {
			msg_perr("Error: Invalid spireadmode value: '%s'.\n", param_str);
			free(param_str);
			return 1;
		}
		if (amd_gen < CHIPSET_BOLTON) {
			msg_perr("Warning: spireadmode not supported for this chipset.");
		}
		free(param_str);
	}

	/* See the chipset support matrix for SPI Base_Addr below for an explanation of the symbols used.
	 * bit   6xx   7xx/SP5100  8xx             9xx  hudson1  hudson234  bolton/yangtze
	 * 18    rsvd  <-          fastReadEnable  ?    <-       ?          SpiReadMode[0]
	 * 29:30 rsvd  <-          <-              ?    <-       ?          SpiReadMode[2:1]
	 */
	if (amd_gen >= CHIPSET_BOLTON) {

		tmp = mmio_readl(sb600_spibar + 0x00);
		uint8_t read_mode = ((tmp >> 28) & 0x6) | ((tmp >> 18) & 0x1);
		msg_pdbg("SPI read mode is %s (%i)\n",
			spireadmodes[read_mode], read_mode);
		if (spireadmode_idx < 0) {
			msg_pdbg("spireadmode is not set, "
				 "leaving SPI read mode unchanged.\n");
		}
		else if (set_mode(dev, spireadmode_idx, sb600_spibar) != 0) {
			return 1;
		}

		if (amd_gen >= CHIPSET_YANGTZE) {
			tmp = mmio_readb(sb600_spibar + 0x20);
			msg_pdbg("UseSpi100 is %sabled\n", (tmp & 0x1) ? "en" : "dis");
			if ((tmp & 0x1) == 0) {
				rmmio_writeb(tmp | 0x1, sb600_spibar + 0x20);
				tmp = mmio_readb(sb600_spibar + 0x20) & 0x1;
				if (tmp == 0) {
					msg_perr("Enabling Spi100 failed.\n");
					return 1;
				}
				msg_pdbg("Enabling Spi100 succeeded.\n");
			}

			tmp = mmio_readw(sb600_spibar + 0x22); /* SPI 100 Speed Config */
			msg_pdbg("NormSpeedNew is %s\n", spispeeds[(tmp >> 12) & 0xf]);
			msg_pdbg("FastSpeedNew is %s\n", spispeeds[(tmp >> 8) & 0xf]);
			msg_pdbg("AltSpeedNew is %s\n", spispeeds[(tmp >> 4) & 0xf]);
			msg_pdbg("TpmSpeedNew is %s\n", spispeeds[(tmp >> 0) & 0xf]);
		}
	} else {
		if (amd_gen >= CHIPSET_SB89XX && amd_gen <= CHIPSET_HUDSON234) {
			bool fast_read = (mmio_readl(sb600_spibar + 0x00) >> 18) & 0x1;
			msg_pdbg("Fast Reads are %sabled\n", fast_read ? "en" : "dis");
			if (fast_read) {
				msg_pdbg("Disabling them temporarily.\n");
				rmmio_writel(mmio_readl(sb600_spibar + 0x00) & ~(0x1 << 18),
					     sb600_spibar + 0x00);
			}
		}
		tmp = (mmio_readb(sb600_spibar + 0xd) >> 4) & 0x3;
		msg_pdbg("NormSpeed is %s\n", spispeeds[tmp]);
		if (spispeed_idx < 0) {
			spispeed_idx = 3; /* Default to 16.5 MHz */
		}
	}
	if (spispeed_idx < 0) {
		msg_pdbg("spispeed is not set, leaving SPI speed unchanged.\n");
		return 0;
	}
	return set_speed(dev, amd_gen, spispeed_idx, sb600_spibar);
}

static int handle_imc(const struct programmer_cfg *cfg, struct pci_dev *dev, enum amd_chipset amd_gen)
{
	/* Handle IMC everywhere but sb600 which does not have one. */
	if (amd_gen == CHIPSET_SB6XX)
		return 0;

	bool amd_imc_force = false;
	char *param_value = extract_programmer_param_str(cfg, "amd_imc_force");
	if (param_value && !strcmp(param_value, "yes")) {
		amd_imc_force = true;
		msg_pspew("amd_imc_force enabled.\n");
	} else if (param_value && !strlen(param_value)) {
		msg_perr("Missing argument for amd_imc_force.\n");
		free(param_value);
		return 1;
	} else if (param_value) {
		msg_perr("Unknown argument for amd_imc_force: \"%s\" (not \"yes\").\n", param_value);
		free(param_value);
		return 1;
	}
	free(param_value);

	/* TODO: we should not only look at IntegratedImcPresent (LPC Dev 20, Func 3, 40h) but also at
	 * IMCEnable(Strap) and Override EcEnable(Strap) (sb8xx, sb9xx?, a50, Bolton: Misc_Reg: 80h-87h;
	 * sb7xx, sp5100: PM_Reg: B0h-B1h) etc. */
	uint8_t reg = pci_read_byte(dev, 0x40);
	if ((reg & (1 << 7)) == 0) {
		msg_pdbg("IMC is not active.\n");
		return 0;
	}

	if (!amd_imc_force)
		programmer_may_write = false;
	msg_pinfo("Writes have been disabled for safety reasons because the presence of the IMC\n"
		  "was detected and it could interfere with accessing flash memory. Flashrom will\n"
		  "try to disable it temporarily but even then this might not be safe:\n"
		  "when it is re-enabled and after a reboot it expects to find working code\n"
		  "in the flash and it is unpredictable what happens if there is none.\n"
		  "\n"
		  "To be safe make sure that there is a working IMC firmware at the right\n"
		  "location in the image you intend to write and do not attempt to erase.\n"
		  "\n"
		  "You can enforce write support with the amd_imc_force programmer option.\n");
	if (amd_imc_force)
		msg_pinfo("Continuing with write support because the user forced us to!\n");

	return amd_imc_shutdown(dev);
}

static int promontory_read_memmapped(struct flashctx *flash, uint8_t *buf,
		unsigned int start, unsigned int len)
{
	struct sb600spi_data * data = (struct sb600spi_data *)flash->mst->spi.data;
	if (!data->flash) {
		map_flash(flash);
		data->flash = flash; /* keep a copy of flashctx for unmap() on tear-down. */
	}
	mmio_readn((void *)(flash->virtual_memory + start), buf, len);
	return 0;
}

static int sb600spi_shutdown(void *data)
{
	struct sb600spi_data *sb600_data = data;
	struct flashctx *flash = sb600_data->flash;
	if (flash)
		finalize_flash_access(flash);

	free(data);
	return 0;
}

static const struct spi_master spi_master_sb600 = {
	.max_data_read	= FIFO_SIZE_OLD,
	.max_data_write	= FIFO_SIZE_OLD - 3,
	.command	= sb600_spi_send_command,
	.map_flash_region	= physmap,
	.unmap_flash_region	= physunmap,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= sb600spi_shutdown,
};

static const struct spi_master spi_master_yangtze = {
	.max_data_read	= FIFO_SIZE_YANGTZE - 3, /* Apparently the big SPI 100 buffer is not a ring buffer. */
	.max_data_write	= FIFO_SIZE_YANGTZE - 3,
	.command	= spi100_spi_send_command,
	.map_flash_region	= physmap,
	.unmap_flash_region	= physunmap,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= sb600spi_shutdown,
};

static const struct spi_master spi_master_promontory = {
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= FIFO_SIZE_YANGTZE - 3,
	.command	= spi100_spi_send_command,
	.map_flash_region	= physmap,
	.unmap_flash_region	= physunmap,
	.read		= promontory_read_memmapped,
	.write_256	= default_spi_write_256,
	.shutdown	= sb600spi_shutdown,
};

int sb600_probe_spi(const struct programmer_cfg *cfg, struct pci_dev *dev)
{
	struct pci_dev *smbus_dev;
	uint32_t tmp;
	uint8_t reg;
	uint8_t *sb600_spibar = NULL;

	/* Read SPI_BaseAddr */
	tmp = pci_read_long(dev, 0xa0);
	tmp &= 0xffffffe0;	/* remove bits 4-0 (reserved) */
	msg_pdbg("SPI base address is at 0x%"PRIx32"\n", tmp);

	/* If the BAR has address 0, it is unlikely SPI is used. */
	if (!tmp)
		return 0;

	/* Physical memory has to be mapped at page (4k) boundaries. */
	sb600_spibar = rphysmap("SB600 SPI registers", tmp & 0xfffff000, 0x1000);
	if (sb600_spibar == ERROR_PTR)
		return ERROR_FLASHROM_FATAL;

	/* The low bits of the SPI base address are used as offset into
	 * the mapped page.
	 */
	sb600_spibar += tmp & 0xfff;

	enum amd_chipset amd_gen = determine_generation(dev);
	if (amd_gen == CHIPSET_AMD_UNKNOWN)
		return ERROR_FLASHROM_NONFATAL;

	/* How to read the following table and similar ones in this file:
	 * "?" means we have no datasheet for this chipset generation or it doesn't have any relevant info.
	 * "<-" means the bit/register meaning is identical to the next non-"?" chipset to the left. "<-" thus
	 *      never refers to another "?".
	 * If a "?" chipset is between two chipsets with identical meaning, we assume the meaning didn't change
	 * twice in between, i.e. the meaning is unchanged for the "?" chipset. Usually we assume that
	 * succeeding hardware supports the same functionality as its predecessor unless proven different by
	 * tests or documentation, hence "?" will often be implemented equally to "<-".
	 *
	 * Chipset support matrix for SPI Base_Addr (LPC PCI reg 0xa0)
	 * bit   6xx         7xx/SP5100       8xx       9xx  hudson1  hudson2+  yangtze
	 * 3    rsvd         <-               <-        ?    <-       ?         RouteTpm2Spi
	 * 2    rsvd         AbortEnable      rsvd      ?    <-       ?         <-
	 * 1    rsvd         SpiRomEnable     <-        ?    <-       ?         <-
	 * 0    rsvd         AltSpiCSEnable   rsvd      ?    <-       ?         <-
	 */
	if (amd_gen >= CHIPSET_SB7XX) {
		tmp = pci_read_long(dev, 0xa0);
		msg_pdbg("SpiRomEnable=%"PRIi32"", (tmp >> 1) & 0x1);
		if (amd_gen == CHIPSET_SB7XX)
			msg_pdbg(", AltSpiCSEnable=%"PRIi32", AbortEnable=%"PRIi32"", tmp & 0x1, (tmp >> 2) & 0x1);
		else if (amd_gen >= CHIPSET_YANGTZE)
			msg_pdbg(", RouteTpm2Sp=%"PRIi32"", (tmp >> 3) & 0x1);

		tmp = pci_read_byte(dev, 0xba);
		msg_pdbg(", PrefetchEnSPIFromIMC=%"PRIi32"", (tmp & 0x4) >> 2);

		tmp = pci_read_byte(dev, 0xbb);
		/* FIXME: Set bit 3,6,7 if not already set.
		 * Set bit 5, otherwise SPI accesses are pointless in LPC mode.
		 * See doc 42413 AMD SB700/710/750 RPR.
		 */
		if (amd_gen == CHIPSET_SB7XX)
			msg_pdbg(", SpiOpEnInLpcMode=%"PRIi32"", (tmp >> 5) & 0x1);
		msg_pdbg(", PrefetchEnSPIFromHost=%"PRIi32"\n", tmp & 0x1);
	}

	/* Chipset support matrix for SPI_Cntrl0 (spibar + 0x0)
	 * See the chipset support matrix for SPI Base_Addr above for an explanation of the symbols used.
	 * bit   6xx                7xx/SP5100      8xx               9xx  hudson1  hudson2+  yangtze
	 * 17    rsvd               <-              <-                ?    <-       ?         <-
	 * 18    rsvd               <-              fastReadEnable<1> ?    <-       ?         SpiReadMode[0]<1>
	 * 19    SpiArbEnable       <-              <-                ?    <-       ?         <-
	 * 20    (FifoPtrClr)       <-              <-                ?    <-       ?         <-
	 * 21    (FifoPtrInc)       <-              <-                ?    <-       ?         IllegalAccess
	 * 22    SpiAccessMacRomEn  <-              <-                ?    <-       ?         <-
	 * 23    SpiHostAccessRomEn <-              <-                ?    <-       ?         <-
	 * 24:26 ArbWaitCount       <-              <-                ?    <-       ?         <-
	 * 27    SpiBridgeDisable   <-              <-                ?    <-       ?         rsvd
	 * 28    rsvd               DropOneClkOnRd  = SPIClkGate      ?    <-       ?         <-
	 * 29:30 rsvd               <-              <-                ?    <-       ?         SpiReadMode[2:1]<1>
	 * 31    rsvd               <-              SpiBusy           ?    <-       ?         <-
	 *
	 *  <1> see handle_speed
	 */
	tmp = mmio_readl(sb600_spibar + 0x00);
	msg_pdbg("(0x%08" PRIx32 ") SpiArbEnable=%"PRIi32"", tmp, (tmp >> 19) & 0x1);
	if (amd_gen >= CHIPSET_YANGTZE)
		msg_pdbg(", IllegalAccess=%"PRIi32"", (tmp >> 21) & 0x1);

	msg_pdbg(", SpiAccessMacRomEn=%"PRIi32", SpiHostAccessRomEn=%"PRIi32", ArbWaitCount=%"PRIi32"",
		 (tmp >> 22) & 0x1, (tmp >> 23) & 0x1, (tmp >> 24) & 0x7);

	if (amd_gen < CHIPSET_YANGTZE)
		msg_pdbg(", SpiBridgeDisable=%"PRIi32"", (tmp >> 27) & 0x1);

	switch (amd_gen) {
	case CHIPSET_SB7XX:
		msg_pdbg(", DropOneClkOnRd/SpiClkGate=%"PRIi32"", (tmp >> 28) & 0x1);
		/* Fall through. */
	case CHIPSET_SB89XX:
	case CHIPSET_HUDSON234:
	case CHIPSET_YANGTZE:
	case CHIPSET_PROMONTORY:
		msg_pdbg(", SpiBusy=%"PRIi32"", (tmp >> 31) & 0x1);
	default: break;
	}
	msg_pdbg("\n");

	if (((tmp >> 22) & 0x1) == 0 || ((tmp >> 23) & 0x1) == 0) {
		msg_perr("ERROR: State of SpiAccessMacRomEn or SpiHostAccessRomEn prohibits full access.\n");
		return ERROR_FLASHROM_NONFATAL;
	}

	if (amd_gen >= CHIPSET_SB89XX) {
		tmp = mmio_readb(sb600_spibar + 0x1D);
		msg_pdbg("Using SPI_CS%"PRId32"\n", tmp & 0x3);
		/* FIXME: Handle SpiProtect* configuration on Yangtze. */
	}

	/* Look for the SMBus device. */
	smbus_dev = pcidev_find(0x1002, 0x4385);
	if (!smbus_dev)
		smbus_dev = pcidev_find(0x1022, 0x780b); /* AMD FCH */
	if (!smbus_dev)
		smbus_dev = pcidev_find(0x1022, 0x790b); /* AMD FP4 */
	if (!smbus_dev) {
		msg_perr("ERROR: SMBus device not found. Not enabling SPI.\n");
		return ERROR_FLASHROM_NONFATAL;
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

	if (handle_speed(cfg, dev, amd_gen, sb600_spibar) != 0)
		return ERROR_FLASHROM_FATAL;

	if (handle_imc(cfg, dev, amd_gen) != 0)
		return ERROR_FLASHROM_FATAL;

	struct sb600spi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for extra SPI master data.\n");
		return SPI_GENERIC_ERROR;
	}

	data->flash = NULL;
	data->spibar = sb600_spibar;

	/* Starting with Yangtze the SPI controller got a different interface with a much bigger buffer. */
	if (amd_gen < CHIPSET_YANGTZE)
		register_spi_master(&spi_master_sb600, data);
	else if (amd_gen == CHIPSET_YANGTZE)
		register_spi_master(&spi_master_yangtze, data);
	else
		register_spi_master(&spi_master_promontory, data);

	return 0;
}
