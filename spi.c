/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2007, 2008 Carl-Daniel Hailfinger
 * Copyright (C) 2008 coresystems GmbH
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
 * Contains the generic SPI framework
 */

#include <string.h>
#include "flash.h"
#include "spi.h"

enum spi_controller spi_controller = SPI_CONTROLLER_NONE;
void *spibar = NULL;

void spi_prettyprint_status_register(struct flashchip *flash);

int spi_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	switch (spi_controller) {
	case SPI_CONTROLLER_IT87XX:
		return it8716f_spi_command(writecnt, readcnt, writearr,
					   readarr);
	case SPI_CONTROLLER_ICH7:
	case SPI_CONTROLLER_ICH9:
	case SPI_CONTROLLER_VIA:
		return ich_spi_command(writecnt, readcnt, writearr, readarr);
	case SPI_CONTROLLER_SB600:
		return sb600_spi_command(writecnt, readcnt, writearr, readarr);
	case SPI_CONTROLLER_WBSIO:
		return wbsio_spi_command(writecnt, readcnt, writearr, readarr);
	case SPI_CONTROLLER_DUMMY:
		return dummy_spi_command(writecnt, readcnt, writearr, readarr);
	default:
		printf_debug
		    ("%s called, but no SPI chipset/strapping detected\n",
		     __FUNCTION__);
	}
	return 1;
}

static int spi_rdid(unsigned char *readarr, int bytes)
{
	const unsigned char cmd[JEDEC_RDID_OUTSIZE] = { JEDEC_RDID };
	int ret;
	int i;

	ret = spi_command(sizeof(cmd), bytes, cmd, readarr);
	if (ret)
		return ret;
	printf_debug("RDID returned");
	for (i = 0; i < bytes; i++)
		printf_debug(" 0x%02x", readarr[i]);
	printf_debug("\n");
	return 0;
}

static int spi_rems(unsigned char *readarr)
{
	unsigned char cmd[JEDEC_REMS_OUTSIZE] = { JEDEC_REMS, 0, 0, 0 };
	uint32_t readaddr;
	int ret;

	ret = spi_command(sizeof(cmd), JEDEC_REMS_INSIZE, cmd, readarr);
	if (ret == SPI_INVALID_ADDRESS) {
		/* Find the lowest even address allowed for reads. */
		readaddr = (spi_get_valid_read_addr() + 1) & ~1;
		cmd[1] = (readaddr >> 16) & 0xff,
		cmd[2] = (readaddr >> 8) & 0xff,
		cmd[3] = (readaddr >> 0) & 0xff,
		ret = spi_command(sizeof(cmd), JEDEC_REMS_INSIZE, cmd, readarr);
	}
	if (ret)
		return ret;
	printf_debug("REMS returned %02x %02x.\n", readarr[0], readarr[1]);
	return 0;
}

static int spi_res(unsigned char *readarr)
{
	unsigned char cmd[JEDEC_RES_OUTSIZE] = { JEDEC_RES, 0, 0, 0 };
	uint32_t readaddr;
	int ret;

	ret = spi_command(sizeof(cmd), JEDEC_RES_INSIZE, cmd, readarr);
	if (ret == SPI_INVALID_ADDRESS) {
		/* Find the lowest even address allowed for reads. */
		readaddr = (spi_get_valid_read_addr() + 1) & ~1;
		cmd[1] = (readaddr >> 16) & 0xff,
		cmd[2] = (readaddr >> 8) & 0xff,
		cmd[3] = (readaddr >> 0) & 0xff,
		ret = spi_command(sizeof(cmd), JEDEC_RES_INSIZE, cmd, readarr);
	}
	if (ret)
		return ret;
	printf_debug("RES returned %02x.\n", readarr[0]);
	return 0;
}

int spi_write_enable(void)
{
	const unsigned char cmd[JEDEC_WREN_OUTSIZE] = { JEDEC_WREN };
	int result;

	/* Send WREN (Write Enable) */
	result = spi_command(sizeof(cmd), 0, cmd, NULL);

	if (result)
		printf_debug("%s failed", __func__);
	if (result == SPI_INVALID_OPCODE) {
		switch (spi_controller) {
		case SPI_CONTROLLER_ICH7:
		case SPI_CONTROLLER_ICH9:
		case SPI_CONTROLLER_VIA:
			printf_debug(" due to SPI master limitation, ignoring"
				     " and hoping it will be run as PREOP\n");
			return 0;
		default:
			break;
		}
	}
	if (result)
		printf_debug("\n");

	return result;
}

int spi_write_disable(void)
{
	const unsigned char cmd[JEDEC_WRDI_OUTSIZE] = { JEDEC_WRDI };

	/* Send WRDI (Write Disable) */
	return spi_command(sizeof(cmd), 0, cmd, NULL);
}

static int probe_spi_rdid_generic(struct flashchip *flash, int bytes)
{
	unsigned char readarr[4];
	uint32_t id1;
	uint32_t id2;

	if (spi_rdid(readarr, bytes))
		return 0;

	if (!oddparity(readarr[0]))
		printf_debug("RDID byte 0 parity violation.\n");

	/* Check if this is a continuation vendor ID */
	if (readarr[0] == 0x7f) {
		if (!oddparity(readarr[1]))
			printf_debug("RDID byte 1 parity violation.\n");
		id1 = (readarr[0] << 8) | readarr[1];
		id2 = readarr[2];
		if (bytes > 3) {
			id2 <<= 8;
			id2 |= readarr[3];
		}
	} else {
		id1 = readarr[0];
		id2 = (readarr[1] << 8) | readarr[2];
	}

	printf_debug("%s: id1 0x%02x, id2 0x%02x\n", __FUNCTION__, id1, id2);

	if (id1 == flash->manufacture_id && id2 == flash->model_id) {
		/* Print the status register to tell the
		 * user about possible write protection.
		 */
		spi_prettyprint_status_register(flash);

		return 1;
	}

	/* Test if this is a pure vendor match. */
	if (id1 == flash->manufacture_id &&
	    GENERIC_DEVICE_ID == flash->model_id)
		return 1;

	return 0;
}

int probe_spi_rdid(struct flashchip *flash)
{
	return probe_spi_rdid_generic(flash, 3);
}

/* support 4 bytes flash ID */
int probe_spi_rdid4(struct flashchip *flash)
{
	/* only some SPI chipsets support 4 bytes commands */
	switch (spi_controller) {
	case SPI_CONTROLLER_ICH7:
	case SPI_CONTROLLER_ICH9:
	case SPI_CONTROLLER_VIA:
	case SPI_CONTROLLER_SB600:
	case SPI_CONTROLLER_WBSIO:
	case SPI_CONTROLLER_DUMMY:
		return probe_spi_rdid_generic(flash, 4);
	default:
		printf_debug("4b ID not supported on this SPI controller\n");
	}

	return 0;
}

int probe_spi_rems(struct flashchip *flash)
{
	unsigned char readarr[JEDEC_REMS_INSIZE];
	uint32_t id1, id2;

	if (spi_rems(readarr))
		return 0;

	id1 = readarr[0];
	id2 = readarr[1];

	printf_debug("%s: id1 0x%x, id2 0x%x\n", __FUNCTION__, id1, id2);

	if (id1 == flash->manufacture_id && id2 == flash->model_id) {
		/* Print the status register to tell the
		 * user about possible write protection.
		 */
		spi_prettyprint_status_register(flash);

		return 1;
	}

	/* Test if this is a pure vendor match. */
	if (id1 == flash->manufacture_id &&
	    GENERIC_DEVICE_ID == flash->model_id)
		return 1;

	return 0;
}

int probe_spi_res(struct flashchip *flash)
{
	unsigned char readarr[3];
	uint32_t id2;

	/* Check if RDID was successful and did not return 0xff 0xff 0xff.
	 * In that case, RES is pointless.
	 */
	if (!spi_rdid(readarr, 3) && ((readarr[0] != 0xff) ||
	    (readarr[1] != 0xff) || (readarr[2] != 0xff)))
		return 0;

	if (spi_res(readarr))
		return 0;

	id2 = readarr[0];
	printf_debug("%s: id 0x%x\n", __FUNCTION__, id2);
	if (id2 != flash->model_id)
		return 0;

	/* Print the status register to tell the
	 * user about possible write protection.
	 */
	spi_prettyprint_status_register(flash);
	return 1;
}

uint8_t spi_read_status_register(void)
{
	const unsigned char cmd[JEDEC_RDSR_OUTSIZE] = { JEDEC_RDSR };
	unsigned char readarr[2]; /* JEDEC_RDSR_INSIZE=1 but wbsio needs 2 */
	int ret;

	/* Read Status Register */
	if (spi_controller == SPI_CONTROLLER_SB600) {
		/* SB600 uses a different way to read status register. */
		return sb600_read_status_register();
	} else {
		ret = spi_command(sizeof(cmd), sizeof(readarr), cmd, readarr);
		if (ret)
			printf_debug("RDSR failed!\n");
	}

	return readarr[0];
}

/* Prettyprint the status register. Common definitions. */
void spi_prettyprint_status_register_common(uint8_t status)
{
	printf_debug("Chip status register: Bit 5 / Block Protect 3 (BP3) is "
		     "%sset\n", (status & (1 << 5)) ? "" : "not ");
	printf_debug("Chip status register: Bit 4 / Block Protect 2 (BP2) is "
		     "%sset\n", (status & (1 << 4)) ? "" : "not ");
	printf_debug("Chip status register: Bit 3 / Block Protect 1 (BP1) is "
		     "%sset\n", (status & (1 << 3)) ? "" : "not ");
	printf_debug("Chip status register: Bit 2 / Block Protect 0 (BP0) is "
		     "%sset\n", (status & (1 << 2)) ? "" : "not ");
	printf_debug("Chip status register: Write Enable Latch (WEL) is "
		     "%sset\n", (status & (1 << 1)) ? "" : "not ");
	printf_debug("Chip status register: Write In Progress (WIP/BUSY) is "
		     "%sset\n", (status & (1 << 0)) ? "" : "not ");
}

/* Prettyprint the status register. Works for
 * ST M25P series
 * MX MX25L series
 */
void spi_prettyprint_status_register_st_m25p(uint8_t status)
{
	printf_debug("Chip status register: Status Register Write Disable "
		     "(SRWD) is %sset\n", (status & (1 << 7)) ? "" : "not ");
	printf_debug("Chip status register: Bit 6 is "
		     "%sset\n", (status & (1 << 6)) ? "" : "not ");
	spi_prettyprint_status_register_common(status);
}

void spi_prettyprint_status_register_sst25(uint8_t status)
{
	printf_debug("Chip status register: Block Protect Write Disable "
		     "(BPL) is %sset\n", (status & (1 << 7)) ? "" : "not ");
	printf_debug("Chip status register: Auto Address Increment Programming "
		     "(AAI) is %sset\n", (status & (1 << 6)) ? "" : "not ");
	spi_prettyprint_status_register_common(status);
}

/* Prettyprint the status register. Works for
 * SST 25VF016
 */
void spi_prettyprint_status_register_sst25vf016(uint8_t status)
{
	const char *bpt[] = {
		"none",
		"1F0000H-1FFFFFH",
		"1E0000H-1FFFFFH",
		"1C0000H-1FFFFFH",
		"180000H-1FFFFFH",
		"100000H-1FFFFFH",
		"all", "all"
	};
	spi_prettyprint_status_register_sst25(status);
	printf_debug("Resulting block protection : %s\n",
		     bpt[(status & 0x1c) >> 2]);
}

void spi_prettyprint_status_register_sst25vf040b(uint8_t status)
{
	const char *bpt[] = {
		"none",
		"0x70000-0x7ffff",
		"0x60000-0x7ffff",
		"0x40000-0x7ffff",
		"all blocks", "all blocks", "all blocks", "all blocks"
	};
	spi_prettyprint_status_register_sst25(status);
	printf_debug("Resulting block protection : %s\n",
		bpt[(status & 0x1c) >> 2]);
}

void spi_prettyprint_status_register(struct flashchip *flash)
{
	uint8_t status;

	status = spi_read_status_register();
	printf_debug("Chip status register is %02x\n", status);
	switch (flash->manufacture_id) {
	case ST_ID:
		if (((flash->model_id & 0xff00) == 0x2000) ||
		    ((flash->model_id & 0xff00) == 0x2500))
			spi_prettyprint_status_register_st_m25p(status);
		break;
	case MX_ID:
		if ((flash->model_id & 0xff00) == 0x2000)
			spi_prettyprint_status_register_st_m25p(status);
		break;
	case SST_ID:
		switch (flash->model_id) {
		case 0x2541:
			spi_prettyprint_status_register_sst25vf016(status);
			break;
		case 0x8d:
		case 0x258d:
			spi_prettyprint_status_register_sst25vf040b(status);
			break;
		default:
			spi_prettyprint_status_register_sst25(status);
			break;
		}
		break;
	}
}

int spi_chip_erase_60(struct flashchip *flash)
{
	const unsigned char cmd[JEDEC_CE_60_OUTSIZE] = {JEDEC_CE_60};
	int result;
	
	result = spi_disable_blockprotect();
	if (result) {
		printf_debug("spi_disable_blockprotect failed\n");
		return result;
	}
	result = spi_write_enable();
	if (result)
		return result;
	/* Send CE (Chip Erase) */
	result = spi_command(sizeof(cmd), 0, cmd, NULL);
	if (result) {
		printf_debug("spi_chip_erase_60 failed sending erase\n");
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 1-85 s, so wait in 1 s steps.
	 */
	/* FIXME: We assume spi_read_status_register will never fail. */
	while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
		sleep(1);
	return 0;
}

int spi_chip_erase_c7(struct flashchip *flash)
{
	const unsigned char cmd[JEDEC_CE_C7_OUTSIZE] = { JEDEC_CE_C7 };
	int result;

	result = spi_disable_blockprotect();
	if (result) {
		printf_debug("spi_disable_blockprotect failed\n");
		return result;
	}
	result = spi_write_enable();
	if (result)
		return result;
	/* Send CE (Chip Erase) */
	result = spi_command(sizeof(cmd), 0, cmd, NULL);
	if (result) {
		printf_debug("spi_chip_erase_60 failed sending erase\n");
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 1-85 s, so wait in 1 s steps.
	 */
	/* FIXME: We assume spi_read_status_register will never fail. */
	while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
		sleep(1);
	return 0;
}

int spi_chip_erase_60_c7(struct flashchip *flash)
{
	int result;
	result = spi_chip_erase_60(flash);
	if (result) {
		printf_debug("spi_chip_erase_60 failed, trying c7\n");
		result = spi_chip_erase_c7(flash);
	}
	return result;
}

int spi_block_erase_52(const struct flashchip *flash, unsigned long addr)
{
	unsigned char cmd[JEDEC_BE_52_OUTSIZE] = {JEDEC_BE_52};
	int result;

	cmd[1] = (addr & 0x00ff0000) >> 16;
	cmd[2] = (addr & 0x0000ff00) >> 8;
	cmd[3] = (addr & 0x000000ff);
	result = spi_write_enable();
	if (result)
		return result;
	/* Send BE (Block Erase) */
	spi_command(sizeof(cmd), 0, cmd, NULL);
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 100-4000 ms, so wait in 100 ms steps.
	 */
	while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
		usleep(100 * 1000);
	return 0;
}

/* Block size is usually
 * 64k for Macronix
 * 32k for SST
 * 4-32k non-uniform for EON
 */
int spi_block_erase_d8(const struct flashchip *flash, unsigned long addr)
{
	unsigned char cmd[JEDEC_BE_D8_OUTSIZE] = { JEDEC_BE_D8 };
	int result;

	cmd[1] = (addr & 0x00ff0000) >> 16;
	cmd[2] = (addr & 0x0000ff00) >> 8;
	cmd[3] = (addr & 0x000000ff);
	result = spi_write_enable();
	if (result)
		return result;
	/* Send BE (Block Erase) */
	spi_command(sizeof(cmd), 0, cmd, NULL);
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 100-4000 ms, so wait in 100 ms steps.
	 */
	while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
		usleep(100 * 1000);
	return 0;
}

int spi_chip_erase_d8(struct flashchip *flash)
{
	int i, rc = 0;
	int total_size = flash->total_size * 1024;
	int erase_size = 64 * 1024;

	spi_disable_blockprotect();

	printf("Erasing chip: \n");

	for (i = 0; i < total_size / erase_size; i++) {
		rc = spi_block_erase_d8(flash, i * erase_size);
		if (rc) {
			printf("Error erasing block at 0x%x\n", i);
			break;
		}
	}

	printf("\n");

	return rc;
}

/* Sector size is usually 4k, though Macronix eliteflash has 64k */
int spi_sector_erase(const struct flashchip *flash, unsigned long addr)
{
	unsigned char cmd[JEDEC_SE_OUTSIZE] = { JEDEC_SE };
	int result;
	
	cmd[1] = (addr & 0x00ff0000) >> 16;
	cmd[2] = (addr & 0x0000ff00) >> 8;
	cmd[3] = (addr & 0x000000ff);

	result = spi_write_enable();
	if (result)
		return result;
	/* Send SE (Sector Erase) */
	spi_command(sizeof(cmd), 0, cmd, NULL);
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 15-800 ms, so wait in 10 ms steps.
	 */
	while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
		usleep(10 * 1000);
	return 0;
}

int spi_write_status_enable(void)
{
	const unsigned char cmd[JEDEC_EWSR_OUTSIZE] = { JEDEC_EWSR };
	int result;

	/* Send EWSR (Enable Write Status Register). */
	result = spi_command(sizeof(cmd), JEDEC_EWSR_INSIZE, cmd, NULL);

	if (result)
		printf_debug("%s failed", __func__);
	if (result == SPI_INVALID_OPCODE) {
		switch (spi_controller) {
		case SPI_CONTROLLER_ICH7:
		case SPI_CONTROLLER_ICH9:
		case SPI_CONTROLLER_VIA:
			printf_debug(" due to SPI master limitation, ignoring"
				     " and hoping it will be run as PREOP\n");
			return 0;
		default:
			break;
		}
	}
	if (result)
		printf_debug("\n");

	return result;
}

/*
 * This is according the SST25VF016 datasheet, who knows it is more
 * generic that this...
 */
int spi_write_status_register(int status)
{
	const unsigned char cmd[JEDEC_WRSR_OUTSIZE] =
	    { JEDEC_WRSR, (unsigned char)status };

	/* Send WRSR (Write Status Register) */
	return spi_command(sizeof(cmd), 0, cmd, NULL);
}

void spi_byte_program(int address, uint8_t byte)
{
	const unsigned char cmd[JEDEC_BYTE_PROGRAM_OUTSIZE] = {
		JEDEC_BYTE_PROGRAM,
		(address >> 16) & 0xff,
		(address >> 8) & 0xff,
		(address >> 0) & 0xff,
		byte
	};

	/* Send Byte-Program */
	spi_command(sizeof(cmd), 0, cmd, NULL);
}

int spi_disable_blockprotect(void)
{
	uint8_t status;
	int result;

	status = spi_read_status_register();
	/* If there is block protection in effect, unprotect it first. */
	if ((status & 0x3c) != 0) {
		printf_debug("Some block protection in effect, disabling\n");
		result = spi_write_status_enable();
		if (result) {
			printf_debug("spi_write_status_enable failed\n");
			return result;
		}
		result = spi_write_status_register(status & ~0x3c);
		if (result) {
			printf_debug("spi_write_status_register failed\n");
			return result;
		}
	}
	return 0;
}

int spi_nbyte_read(int address, uint8_t *bytes, int len)
{
	const unsigned char cmd[JEDEC_READ_OUTSIZE] = {
		JEDEC_READ,
		(address >> 16) & 0xff,
		(address >> 8) & 0xff,
		(address >> 0) & 0xff,
	};

	/* Send Read */
	return spi_command(sizeof(cmd), len, cmd, bytes);
}

int spi_chip_read(struct flashchip *flash, uint8_t *buf)
{
	switch (spi_controller) {
	case SPI_CONTROLLER_IT87XX:
		return it8716f_spi_chip_read(flash, buf);
	case SPI_CONTROLLER_SB600:
		return sb600_spi_read(flash, buf);
	case SPI_CONTROLLER_ICH7:
	case SPI_CONTROLLER_ICH9:
	case SPI_CONTROLLER_VIA:
		return ich_spi_read(flash, buf);
	case SPI_CONTROLLER_WBSIO:
		return wbsio_spi_read(flash, buf);
	default:
		printf_debug
		    ("%s called, but no SPI chipset/strapping detected\n",
		     __FUNCTION__);
	}

	return 1;
}

/*
 * Program chip using byte programming. (SLOW!)
 * This is for chips which can only handle one byte writes
 * and for chips where memory mapped programming is impossible
 * (e.g. due to size constraints in IT87* for over 512 kB)
 */
int spi_chip_write_1(struct flashchip *flash, uint8_t *buf)
{
	int total_size = 1024 * flash->total_size;
	int i;

	spi_disable_blockprotect();
	for (i = 0; i < total_size; i++) {
		spi_write_enable();
		spi_byte_program(i, buf[i]);
		while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
			myusec_delay(10);
	}

	return 0;
}

/*
 * Program chip using page (256 bytes) programming.
 * Some SPI masters can't do this, they use single byte programming instead.
 */
int spi_chip_write_256(struct flashchip *flash, uint8_t *buf)
{
	switch (spi_controller) {
	case SPI_CONTROLLER_IT87XX:
		return it8716f_spi_chip_write_256(flash, buf);
	case SPI_CONTROLLER_SB600:
		return sb600_spi_write_1(flash, buf);
	case SPI_CONTROLLER_ICH7:
	case SPI_CONTROLLER_ICH9:
	case SPI_CONTROLLER_VIA:
		return ich_spi_write_256(flash, buf);
	case SPI_CONTROLLER_WBSIO:
		return wbsio_spi_write_1(flash, buf);
	default:
		printf_debug
		    ("%s called, but no SPI chipset/strapping detected\n",
		     __FUNCTION__);
	}

	return 1;
}

uint32_t spi_get_valid_read_addr(void)
{
	/* Need to return BBAR for ICH chipsets. */
	return 0;
}

int spi_aai_write(struct flashchip *flash, uint8_t *buf)
{
	uint32_t pos = 2, size = flash->total_size * 1024;
	unsigned char w[6] = {0xad, 0, 0, 0, buf[0], buf[1]};
	int result;

	switch (spi_controller) {
	case SPI_CONTROLLER_WBSIO:
		fprintf(stderr, "%s: impossible with Winbond SPI masters,"
				" degrading to byte program\n", __func__);
		return spi_chip_write_1(flash, buf);
	default:
		break;
	}
	flash->erase(flash);
	result = spi_write_enable();
	if (result)
		return result;
	spi_command(6, 0, w, NULL);
	while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
		myusec_delay(5); /* SST25VF040B Tbp is max 10us */
	while (pos < size) {
		w[1] = buf[pos++];
		w[2] = buf[pos++];
		spi_command(3, 0, w, NULL);
		while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
			myusec_delay(5); /* SST25VF040B Tbp is max 10us */
	}
	spi_write_disable();
	return 0;
}
