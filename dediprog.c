/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Carl-Daniel Hailfinger
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

#include <string.h>
#include <usb.h>
#include "flash.h"
#include "chipdrivers.h"
#include "spi.h"

#define DEFAULT_TIMEOUT 3000
static usb_dev_handle *dediprog_handle;

#if 0
/* Might be useful for other pieces of code as well. */
static void print_hex(void *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		msg_pdbg(" %02x", ((uint8_t *)buf)[i]);
}
#endif

/* Might be useful for other USB devices as well. static for now. */
static struct usb_device *get_device_by_vid_pid(uint16_t vid, uint16_t pid)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	for (bus = usb_get_busses(); bus; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next)
			if ((dev->descriptor.idVendor == vid) &&
			    (dev->descriptor.idProduct == pid))
				return dev;

	return NULL;
}

//int usb_control_msg(usb_dev_handle *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout);

static int dediprog_set_spi_voltage(uint16_t voltage)
{
	int ret;
	unsigned int mv;

	switch (voltage) {
	case 0x0:
		/* Admittedly this one is an assumption. */
		mv = 0;
		break;
	case 0x12:
		mv = 1800;
		break;
	case 0x11:
		mv = 2500;
		break;
	case 0x10:
		mv = 3500;
		break;
	default:
		msg_perr("Unknown voltage selector 0x%x! Aborting.\n", voltage);
		return 1;
	}
	msg_pdbg("Setting SPI voltage to %u.%03u V\n", mv / 1000, mv % 1000);

	ret = usb_control_msg(dediprog_handle, 0x42, 0x9, voltage, 0xff, NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command Set SPI Voltage 0x%x failed!\n", voltage);
		return 1;
	}
	return 0;
}

#if 0
/* After dediprog_set_spi_speed, the original app always calls
 * dediprog_set_spi_voltage(0) and then
 * dediprog_check_devicestring() four times in a row.
 * After that, dediprog_command_a() is called.
 * This looks suspiciously like the microprocessor in the SF100 has to be
 * restarted/reinitialized in case the speed changes.
 */
static int dediprog_set_spi_speed(uint16_t speed)
{
	int ret;
	unsigned int khz;

	/* Case 1 and 2 are in weird order. Probably an organically "grown"
	 * interface.
	 * Base frequency is 24000 kHz, divisors are (in order)
	 * 1, 3, 2, 8, 11, 16, 32, 64.
	 */
	switch (speed) {
	case 0x0:
		khz = 24000;
		break;
	case 0x1:
		khz = 8000;
		break;
	case 0x2:
		khz = 12000;
		break;
	case 0x3:
		khz = 3000;
		break;
	case 0x4:
		khz = 2180;
		break;
	case 0x5:
		khz = 1500;
		break;
	case 0x6:
		khz = 750;
		break;
	case 0x7:
		khz = 375;
		break;
	default:
		msg_perr("Unknown frequency selector 0x%x! Aborting.\n", speed);
		return 1;
	}
	msg_pdbg("Setting SPI speed to %u kHz\n", khz);

	ret = usb_control_msg(dediprog_handle, 0x42, 0x61, speed, 0xff, NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command Set SPI Speed 0x%x failed!\n", speed);
		return 1;
	}
	return 0;
}
#endif

int dediprog_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	msg_pspew("%s, start=0x%x, len=0x%x\n", __func__, start, len);
	/* Chosen read length is 16 bytes for now. */
	return spi_read_chunked(flash, buf, start, len, 16);
}

int dediprog_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			const unsigned char *writearr, unsigned char *readarr)
{
	int ret;

	msg_pspew("%s, writecnt=%i, readcnt=%i\n", __func__, writecnt, readcnt);
	/* Paranoid, but I don't want to be blamed if anything explodes. */
	if (writecnt > 5) {
		msg_perr("Untested writecnt=%i, aborting.\n", writecnt);
		return 1;
	}
	/* 16 byte reads should work. */
	if (readcnt > 16) {
		msg_perr("Untested readcnt=%i, aborting.\n", readcnt);
		return 1;
	}
	
	ret = usb_control_msg(dediprog_handle, 0x42, 0x1, 0xff, readcnt ? 0x1 : 0x0, (char *)writearr, writecnt, DEFAULT_TIMEOUT);
	if (ret != writecnt) {
		msg_perr("Send SPI failed, expected %i, got %i %s!\n",
			 writecnt, ret, usb_strerror());
		return 1;
	}
	if (!readcnt)
		return 0;
	memset(readarr, 0, readcnt);
	ret = usb_control_msg(dediprog_handle, 0xc2, 0x01, 0xbb8, 0x0000, (char *)readarr, readcnt, DEFAULT_TIMEOUT);
	if (ret != readcnt) {
		msg_perr("Receive SPI failed, expected %i, got %i %s!\n",
			 readcnt, ret, usb_strerror());
		return 1;
	}
	return 0;
}

static int dediprog_check_devicestring(void)
{
	int ret;
	char buf[0x11];

	/* Command Prepare Receive Device String. */
	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, 0xc3, 0x7, 0x0, 0xef03, buf, 0x1, DEFAULT_TIMEOUT);
	/* The char casting is needed to stop gcc complaining about an always true comparison. */
	if ((ret != 0x1) || (buf[0] != (char)0xff)) {
		msg_perr("Unexpected response to Command Prepare Receive Device"
			 " String!\n");
		return 1;
	}
	/* Command Receive Device String. */
	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, 0xc2, 0x8, 0xff, 0xff, buf, 0x10, DEFAULT_TIMEOUT);
	if (ret != 0x10) {
		msg_perr("Incomplete/failed Command Receive Device String!\n");
		return 1;
	}
	buf[0x10] = '\0';
	msg_pdbg("Found a %s\n", buf);
	if (memcmp(buf, "SF100", 0x5)) {
		msg_perr("Device not a SF100!\n");
		return 1;
	}
	/* Only these versions were tested. */
	if (memcmp(buf, "SF100   V:2.1.1 ", 0x10) &&
	    memcmp(buf, "SF100   V:3.1.8 ", 0x10)) {
		msg_perr("Unexpected firmware version!\n");
		return 1;
	}
	return 0;
}

/* Command A seems to be some sort of device init. It is either followed by
 * dediprog_check_devicestring (often) or Command A (often) or
 * Command F (once).
 */
static int dediprog_command_a(void)
{
	int ret;
	char buf[0x1];

	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, 0xc3, 0xb, 0x0, 0x0, buf, 0x1, DEFAULT_TIMEOUT);
	if ((ret != 0x1) || (buf[0] != 0x6f)) {
		msg_perr("Unexpected response to Command A!\n");
		return 1;
	}
	return 0;
}

/* Command C is only sent after dediprog_check_devicestring, but not after every
 * invocation of dediprog_check_devicestring. It is only sent after the first
 * dediprog_command_a(); dediprog_check_devicestring() sequence in each session.
 * I'm tempted to call this one start_SPI_engine or finish_init.
 */
static int dediprog_command_c(void)
{
	int ret;

	ret = usb_control_msg(dediprog_handle, 0x42, 0x4, 0x0, 0x0, NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Unexpected response to Command C!\n");
		return 1;
	}
	return 0;
}

#if 0
/* Very strange. Seems to be a programmer keepalive or somesuch.
 * Wait unsuccessfully for timeout ms to read one byte.
 * Is usually called after setting voltage to 0.
 */
static int dediprog_command_f(int timeout)
{
	int ret;
	char buf[0x1];

	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, 0xc2, 0x11, 0xff, 0xff, buf, 0x1, timeout);
	if (ret != 0x0) {
		msg_perr("Unexpected response to Command F!\n");
		return 1;
	}
	return 0;
}
#endif

/* URB numbers refer to the first log ever captured. */
int dediprog_init(void)
{
	struct usb_device *dev;

	msg_pspew("%s\n", __func__);

	/* Here comes the USB stuff. */
	usb_init();
	usb_find_busses();
	usb_find_devices();
	dev = get_device_by_vid_pid(0x0483, 0xdada);
	if (!dev) {
		msg_perr("Could not find a Dediprog SF100 on USB!\n");
		return 1;
	}
	msg_pdbg("Found USB device (%04x:%04x).\n",
		 dev->descriptor.idVendor,
		 dev->descriptor.idProduct);
	dediprog_handle = usb_open(dev);
	usb_set_configuration(dediprog_handle, 1);
	usb_claim_interface(dediprog_handle, 0);
	/* URB 6. Command A. */
	if (dediprog_command_a())
		return 1;
	/* URB 7. Command A. */
	if (dediprog_command_a())
		return 1;
	/* URB 8. Command Prepare Receive Device String. */
	/* URB 9. Command Receive Device String. */
	if (dediprog_check_devicestring())
		return 1;
	/* URB 10. Command C. */
	if (dediprog_command_c())
		return 1;
	/* URB 11. Command Set SPI Voltage. */
	if (dediprog_set_spi_voltage(0x10))
		return 1;

	buses_supported = CHIP_BUSTYPE_SPI;
	spi_controller = SPI_CONTROLLER_DEDIPROG;

	/* RE leftover, leave in until the driver is complete. */
#if 0
	/* Execute RDID by hand if you want to test it. */
	dediprog_do_stuff();
#endif

	return 0;
}

#if 0
/* Leftovers from reverse engineering. Keep for documentation purposes until
 * completely understood.
 */
static int dediprog_do_stuff(void)
{
	char buf[0x4];
	/* SPI command processing starts here. */

	/* URB 12. Command Send SPI. */
	/* URB 13. Command Receive SPI. */
	memset(buf, 0, sizeof(buf));
	/* JEDEC RDID */
	msg_pdbg("Sending RDID\n");
	buf[0] = JEDEC_RDID;
	if (dediprog_spi_send_command(JEDEC_RDID_OUTSIZE, JEDEC_RDID_INSIZE, (unsigned char *)buf, (unsigned char *)buf))
		return 1;
	msg_pdbg("Receiving response: ");
	print_hex(buf, JEDEC_RDID_INSIZE);
	/* URB 14-27 are more SPI commands. */
	/* URB 28. Command Set SPI Voltage. */
	if (dediprog_set_spi_voltage(0x0))
		return 1;
	/* URB 29-38. Command F, unsuccessful wait. */
	if (dediprog_command_f(544))
		return 1;
	/* URB 39. Command Set SPI Voltage. */
	if (dediprog_set_spi_voltage(0x10))
		return 1;
	/* URB 40. Command Set SPI Speed. */
	if (dediprog_set_spi_speed(0x2))
		return 1;
	/* URB 41 is just URB 28. */
	/* URB 42,44,46,48,51,53 is just URB 8. */
	/* URB 43,45,47,49,52,54 is just URB 9. */
	/* URB 50 is just URB 6/7. */
	/* URB 55-131 is just URB 29-38. (wait unsuccessfully for 4695 (maybe 4751) ms)*/
	/* URB 132,134 is just URB 6/7. */
	/* URB 133 is just URB 29-38. */
	/* URB 135 is just URB 8. */
	/* URB 136 is just URB 9. */
	/* URB 137 is just URB 11. */

	/* Command I is probably Start Bulk Read. Data is u16 blockcount, u16 blocksize. */
	/* Command J is probably Start Bulk Write. Data is u16 blockcount, u16 blocksize. */
	/* Bulk transfer sizes for Command I/J are always 512 bytes, rest is filled with 0xff. */

	return 0;
}
#endif	

int dediprog_shutdown(void)
{
	msg_pspew("%s\n", __func__);

	/* URB 28. Command Set SPI Voltage to 0. */
	if (dediprog_set_spi_voltage(0x0))
		return 1;

	if (usb_close(dediprog_handle)) {
		msg_perr("Couldn't close USB device!\n");
		return 1;
	}
	return 0;
}
