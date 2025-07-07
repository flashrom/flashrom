/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2025 NVIDIA CORPORATION
 */

#include <string.h>
#include <stdlib.h>
#include <libusb.h>
#include <errno.h>
#include "platform.h"
#include "programmer.h"
#include "flash.h"
#include "usb_device.h"

/* This is common flashrom timeout for usb for 1 second. It works for erasing and programming 256 bytes */
#define USB_TIMEOUT		1000

#define NV_SMA_HEADER_LEN		4 /* channel_id(1byte) + cmd(1byte) + len(2bytes) */
#define NV_SMA_CH_OFFSET		0
#define NV_SMA_CMD_OFFSET		1
#define NV_SMA_LEN_OFFSET		2

/* External commands */
#define NV_SMA_CMD_CONFIG		0x00
#define NV_SMA_CMD_READ			0x01
#define NV_SMA_CMD_WRITE		0x02
#define NV_SMA_CMD_WRITE_READ		0x03
#define NV_SMA_CMD_POSTED_WRITE		0x04

#define NV_SMA_CMD_WRITE_RESP_LEN		5
#define NV_SMA_CMD_WRITE_RESP_STATUS_OFFSET	4

#define NV_SMA_CS_ASSERT		0x20
#define NV_SMA_CS_DEASSERT		0x10
#define NV_SMA_CS0			0x00
#define NV_SMA_CS1			0x40
#define NV_SMA_CS2			0x80
#define NV_SMA_CS3			0xC0

/* USB interface class/subclass/protocol for NV SMA SPI */
#define NV_SMA_INTERFACE_CLASS     0xFF  /* Vendor Specific */
#define NV_SMA_INTERFACE_SUBCLASS  0x3F	 /* Nvidia assigned class */
#define NV_SMA_INTERFACE_PROTOCOL  0x01  /* Protocol v1 */



/* The USB descriptor says the max transfer size is 512 bytes,
 * leaving 508 bytes for data as the channel + command + length take up 4 bytes
 */
#define NV_SMA_PACKET_SIZE 512
#define NV_SMA_MAX_DATA_LEN (NV_SMA_PACKET_SIZE - NV_SMA_HEADER_LEN)

struct nv_sma_spi_data {
	struct libusb_device_handle *handle;
	int interface;
	uint8_t cs_bits;
	uint8_t write_ep;
	uint8_t read_ep;
};


static const struct dev_entry devs_nv_sma_spi[] = {
	{0x0955, 0xcf11, OK, "Nvidia SMA", "USB To SPI"},
	{0}
};

static int nv_sma_spi_shutdown(void *data)
{
	struct nv_sma_spi_data *nv_sma_data = data;
	int spi_interface = nv_sma_data->interface;
	libusb_release_interface(nv_sma_data->handle, spi_interface);
	libusb_attach_kernel_driver(nv_sma_data->handle, spi_interface);
	libusb_close(nv_sma_data->handle);
	libusb_exit(NULL);
	free(data);
	return 0;
}

static int nv_sma_write(struct nv_sma_spi_data *nv_sma_data, unsigned int writecnt,
			const uint8_t *writearr, uint8_t cs_ctrl)
{
	unsigned int data_len;
	int packet_len;
	int transferred;
	int ret;

	uint8_t resp_buf[NV_SMA_CMD_WRITE_RESP_LEN] = {0};
	uint8_t buffer[NV_SMA_PACKET_SIZE] = {0};
	unsigned int bytes_written = 0;
	bool asserted = false;

	while (bytes_written < writecnt) {
		data_len = min(NV_SMA_MAX_DATA_LEN, writecnt - bytes_written );
		packet_len = data_len + NV_SMA_HEADER_LEN;

		buffer[NV_SMA_CMD_OFFSET] = NV_SMA_CMD_WRITE;
		buffer[NV_SMA_CMD_OFFSET] |= nv_sma_data->cs_bits;
		if (cs_ctrl & NV_SMA_CS_ASSERT && !asserted) {
			buffer[NV_SMA_CMD_OFFSET] |= NV_SMA_CS_ASSERT;
			asserted = true;
		}
		if (cs_ctrl & NV_SMA_CS_DEASSERT && (bytes_written + data_len) >=  writecnt)
			buffer[NV_SMA_CMD_OFFSET] |= NV_SMA_CS_DEASSERT;

		buffer[NV_SMA_LEN_OFFSET] = (data_len) & 0xFF;
		buffer[NV_SMA_LEN_OFFSET+1] = ((data_len) & 0xFF00) >> 8;
		memcpy(buffer + NV_SMA_HEADER_LEN, writearr + bytes_written, data_len);

		ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->write_ep, buffer,
					packet_len, &transferred, USB_TIMEOUT);
		if (ret < 0 || transferred != packet_len) {
			msg_perr("Could not send write command\n");
			return -1;
		}

		ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->read_ep, resp_buf,
					NV_SMA_CMD_WRITE_RESP_LEN, &transferred, USB_TIMEOUT);
		if (ret < 0 || transferred < NV_SMA_CMD_WRITE_RESP_LEN) {
			msg_perr("Could not receive write command response\n");
			return -1;
		}

		if (resp_buf[NV_SMA_CMD_WRITE_RESP_STATUS_OFFSET] != 0) {
			msg_perr("recv error status=%d\n", resp_buf[NV_SMA_CMD_WRITE_RESP_STATUS_OFFSET]);
			return -1;
		}

		bytes_written += data_len;
	}
	return 0;
}

static int nv_sma_read(struct nv_sma_spi_data *nv_sma_data, unsigned int readcnt,
			uint8_t *readarr, uint8_t cs_ctrl)
{
	uint8_t *read_ptr = readarr;
	int ret;
	int transferred;
	unsigned int bytes_read = 0;
	uint8_t buffer[NV_SMA_PACKET_SIZE] = {0};
	uint8_t command_buf[8] = {
		[0] = 0x01, /* reserved field for channel id, fixed to 0x01 */
		[1] = NV_SMA_CMD_READ,
		[2] = 4,
		[3] = 0,
		[4] = readcnt & 0xFF,
		[5] = (readcnt & 0xFF00) >> 8,
		[6] = (readcnt & 0xFF0000) >> 16,
		[7] = (readcnt & 0xFF000000) >> 24
	};

	command_buf[NV_SMA_CMD_OFFSET] |= nv_sma_data->cs_bits;
	if (cs_ctrl & NV_SMA_CS_ASSERT)
		command_buf[NV_SMA_CMD_OFFSET] |=  NV_SMA_CS_ASSERT;

	if (cs_ctrl & NV_SMA_CS_DEASSERT)
		command_buf[NV_SMA_CMD_OFFSET] |=  NV_SMA_CS_DEASSERT;

	ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->write_ep, command_buf,
				sizeof(command_buf), &transferred, USB_TIMEOUT);
	if (ret < 0 || transferred != sizeof(command_buf)) {
		msg_perr("Could not send read command\n");
		return -1;
	}

	while (bytes_read < readcnt) {
		ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->read_ep, buffer,
					NV_SMA_PACKET_SIZE, &transferred, USB_TIMEOUT);
		if (ret < 0) {
			msg_perr("Could not read data\n");
			return -1;
		}
		if (transferred > NV_SMA_PACKET_SIZE) {
			msg_perr("libusb bug: bytes received overflowed buffer\n");
			return -1;
		}
		/* Response: u8 channel, u8 command, u16 data length, then the data that was read */
		if (transferred < NV_SMA_HEADER_LEN) {
			msg_perr("NV_SMA returned an invalid response to read command\n");
			return -1;
		}
		int nv_sma_data_length = read_le16(buffer, NV_SMA_LEN_OFFSET);
		if (transferred - NV_SMA_HEADER_LEN < nv_sma_data_length) {
			msg_perr("NV_SMA returned less data than data length header indicates\n");
			return -1;
		}
		bytes_read += nv_sma_data_length;
		if (bytes_read > readcnt) {
			msg_perr("NV_SMA returned more bytes than requested\n");
			return -1;
		}
		memcpy(read_ptr, buffer + NV_SMA_HEADER_LEN, nv_sma_data_length);
		read_ptr += nv_sma_data_length;
	}
	return 0;
}

static int nv_sma_posted_write(struct nv_sma_spi_data *nv_sma_data, unsigned int writecnt,
				const unsigned char *writearr)
{
	unsigned int data_len;
	int packet_len;
	int transferred;
	int ret;

	uint8_t buffer[NV_SMA_PACKET_SIZE] = {0};
	int bytes_written = 0;

	if (writecnt > NV_SMA_MAX_DATA_LEN) {
		/* the API cannot handle such long msg */
		msg_pspew("%s: invalid msg len: %i (max:%i)", __func__, writecnt, NV_SMA_MAX_DATA_LEN);
		return -1;
	}

	data_len = writecnt;
	packet_len = data_len + NV_SMA_HEADER_LEN;

	buffer[NV_SMA_CMD_OFFSET] = NV_SMA_CS_ASSERT | NV_SMA_CS_DEASSERT | NV_SMA_CMD_POSTED_WRITE;
	buffer[NV_SMA_CMD_OFFSET] |= nv_sma_data->cs_bits;

	buffer[NV_SMA_LEN_OFFSET] = (data_len) & 0xFF;
	buffer[NV_SMA_LEN_OFFSET + 1] = ((data_len) & 0xFF00) >> 8;
	memcpy(buffer + NV_SMA_HEADER_LEN, writearr, writecnt);
	bytes_written = packet_len;

	ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->write_ep, buffer,
				packet_len, &transferred, USB_TIMEOUT);
	if (ret < 0 || transferred != bytes_written) {
		msg_perr("Could not send write read command\n");
		return -1;
	}

	return 0;
}

static int nv_sma_write_read(struct nv_sma_spi_data *nv_sma_data, unsigned int writecnt,
				const unsigned char *writearr,
				unsigned int readcnt, unsigned char *readarr)
{
	unsigned int data_len;
	int packet_len;
	int transferred;
	int ret;

	uint8_t resp_buf[NV_SMA_PACKET_SIZE] = {0};
	uint8_t buffer[NV_SMA_PACKET_SIZE] = {0};
	int bytes_written = 0;

	if (writecnt + readcnt > NV_SMA_MAX_DATA_LEN) {
		/* the API cannot handle such long msg */
		msg_pspew("%s: invalid msg len: %i (max:%i)", __func__, writecnt, NV_SMA_MAX_DATA_LEN);
		return -1;
	}
	data_len = writecnt + readcnt;
	packet_len = data_len + NV_SMA_HEADER_LEN;

	buffer[NV_SMA_CMD_OFFSET] = NV_SMA_CS_ASSERT | NV_SMA_CS_DEASSERT | NV_SMA_CMD_WRITE_READ;
	buffer[NV_SMA_CMD_OFFSET] |= nv_sma_data->cs_bits;

	buffer[NV_SMA_LEN_OFFSET] = (data_len) & 0xFF;
	buffer[NV_SMA_LEN_OFFSET + 1] = ((data_len) & 0xFF00) >> 8;
	memcpy(buffer + NV_SMA_HEADER_LEN, writearr, writecnt);
	bytes_written = packet_len;

	ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->write_ep, buffer,
				packet_len, &transferred, USB_TIMEOUT);
	if (ret < 0 || transferred != bytes_written) {
		msg_perr("Could not send write read command\n");
		return -1;
	}

	ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->read_ep, resp_buf,
				sizeof(resp_buf), NULL, USB_TIMEOUT);
	if (ret < 0) {
		msg_perr("Could not receive write read command response\n");
		return -1;
	}

	memcpy(readarr, resp_buf + writecnt + NV_SMA_HEADER_LEN, readcnt);
	return 0;
}

static int nv_sma_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
		unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr)
{
	struct nv_sma_spi_data *nv_sma_data = flash->mst->spi.data;
	int ret = 0;
	uint8_t cs_ctrl = 0;

	if (writecnt + readcnt < NV_SMA_MAX_DATA_LEN) {
		if (readcnt > 0) {
			/* use OUT_IN commands for the length of DO and DI data can fit in single USB URB */
			ret = nv_sma_write_read(nv_sma_data, writecnt, writearr, readcnt, readarr);
			if (ret < 0) {
				msg_perr("NV_SMA write/read error\n");
				return -1;
			}
		}
		else {
			/* use posted write command for the length of DO can fit in single USB URB */
			ret = nv_sma_posted_write(nv_sma_data, writecnt, writearr);
			if (ret < 0) {
				msg_perr("NV_SMA posted write error\n");
				return -1;
			}
		}
	}
	else {
		if (writecnt) {
			cs_ctrl = NV_SMA_CS_ASSERT; /* assert cs before write */
			if (readcnt == 0)
				cs_ctrl |= NV_SMA_CS_DEASSERT;

			ret = nv_sma_write(nv_sma_data, writecnt, writearr, cs_ctrl);
			if (ret < 0) {
				msg_perr("NV_SMA write error\n");
				return -1;
			}
		}
		if (readcnt) {
			cs_ctrl = NV_SMA_CS_DEASSERT; /* de-assert cs after read */
			if (writecnt == 0)
				cs_ctrl |= NV_SMA_CS_ASSERT;

			ret = nv_sma_read(nv_sma_data, readcnt, readarr, cs_ctrl);
			if (ret < 0) {
				msg_perr("NV_SMA read error\n");
				return -1;
			}
		}
	}
	msg_pspew("%s: write %i, read %i ", __func__, writecnt, readcnt);
	return 0;
}

static int32_t nv_sma_spi_config(struct nv_sma_spi_data *nv_sma_data, uint32_t spispeed_hz)
{
	int32_t ret;
	int transferred;
	uint8_t buffer[16] = {
		[0] = 0x0,
		[1] = NV_SMA_CMD_CONFIG,
		[2] = (sizeof(buffer) - 4) & 0xFF,
		[3] = ((sizeof(buffer) - 4) & 0xFF00) >> 8,
		/* Store frequency as 32-bit little endian at bytes 4-7 */
		[4] = (spispeed_hz & 0xFF),           /* LSB */
		[5] = ((spispeed_hz >> 8) & 0xFF),
		[6] = ((spispeed_hz >> 16) & 0xFF),
		[7] = ((spispeed_hz >> 24) & 0xFF)    /* MSB */
	};

	uint8_t response[NV_SMA_PACKET_SIZE] = {0}; /* Response buffer */

	/* flush out IN EP */
	do {
		ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->read_ep, response,
					sizeof(response), &transferred, USB_TIMEOUT);
		if (ret < 0) {
			break;
		}
	} while (transferred > 0);

	msg_pdbg("Requesting SPI frequency: %u Hz\n", spispeed_hz);

	ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->write_ep, buffer,
				sizeof(buffer), NULL, USB_TIMEOUT);
	if (ret < 0) {
		msg_perr("Could not configure SPI interface\n");
		return ret;
	}

	/* Read the configuration response */
	ret = libusb_bulk_transfer(nv_sma_data->handle, nv_sma_data->read_ep, response,
				sizeof(response), &transferred, USB_TIMEOUT);
	if (ret < 0) {
		msg_perr("Could not receive configure SPI command response\n");
		return ret;
	}

	/* Extract actual frequency from response bytes 4-7 (little-endian) */
	if (transferred >= 8) {
		uint32_t actual_freq = response[4] |
					(response[5] << 8) |
					(response[6] << 16) |
					(response[7] << 24);

		if (spispeed_hz == 0) {
			/* No frequency specified, just show the actual device default */
			msg_pinfo("SPI frequency using device default: %u Hz\n", actual_freq);
		} else {
			msg_pinfo("SPI frequency configured: requested=%u Hz, actual=%u Hz\n",
						spispeed_hz, actual_freq);

			/* Warn if actual frequency differs significantly from requested */
			if (actual_freq != spispeed_hz) {
				int32_t diff_percent = ((int64_t)(actual_freq - spispeed_hz) * 100) / spispeed_hz;
				if (diff_percent < 0) diff_percent = -diff_percent;

				if (diff_percent > 10)
					msg_pwarn("Note: Actual frequency differs by %d%% from requested\n", diff_percent);
			}
		}
	} else {
		msg_pdbg("Response too short to extract frequency (received %d bytes)\n", transferred);
	}

	return ret;
}

static const struct spi_master spi_master_nv_sma_spi = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_WRITE_UNLIMITED,
	.command	= nv_sma_spi_send_command,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
	.shutdown	= nv_sma_spi_shutdown,
};

/* Function to discover interface by class/subclass/protocol and endpoints */
static int discover_interface_and_endpoints(struct libusb_device_handle *handle,
						int *interface_num,
						uint8_t *write_ep, uint8_t *read_ep)
{
	struct libusb_device *dev = libusb_get_device(handle);
	struct libusb_config_descriptor *config;
	int ret;

	ret = libusb_get_active_config_descriptor(dev, &config);
	if (ret != 0) {
		msg_perr("Failed to get config descriptor: %s\n", libusb_error_name(ret));
		return -1;
	}

	*interface_num = -1;
	*write_ep = 0;
	*read_ep = 0;

	/* Search for interface with matching class/subclass/protocol */
	for (int i = 0; i < config->bNumInterfaces; i++) {
		const struct libusb_interface *interface = &config->interface[i];
		for (int j = 0; j < interface->num_altsetting; j++) {
			const struct libusb_interface_descriptor *altsetting = &interface->altsetting[j];

			/* Check if this interface matches our class/subclass/protocol */
			if (altsetting->bInterfaceClass != NV_SMA_INTERFACE_CLASS ||
				altsetting->bInterfaceSubClass != NV_SMA_INTERFACE_SUBCLASS ||
				altsetting->bInterfaceProtocol != NV_SMA_INTERFACE_PROTOCOL) {
				continue;
			}

			msg_pdbg("Found NV SMA SPI interface: %d (class=0x%02x, subclass=0x%02x, protocol=0x%02x)\n",
						altsetting->bInterfaceNumber,
						altsetting->bInterfaceClass,
						altsetting->bInterfaceSubClass,
						altsetting->bInterfaceProtocol);

			*interface_num = altsetting->bInterfaceNumber;

			/* Scan endpoints in this interface */
			for (int k = 0; k < altsetting->bNumEndpoints; k++) {
				const struct libusb_endpoint_descriptor *endpoint = &altsetting->endpoint[k];
				uint8_t ep_addr = endpoint->bEndpointAddress;
				uint8_t ep_type = endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK;

				/* We're looking for bulk endpoints */
				if (ep_type != LIBUSB_TRANSFER_TYPE_BULK)
					continue;

				/* Check direction: bit 7 set = IN (device to host) */
				if (ep_addr & LIBUSB_ENDPOINT_IN) {
					if (*read_ep == 0) {
						*read_ep = ep_addr;
						msg_pdbg("Found bulk IN endpoint: 0x%02x\n", ep_addr);
					}
				} else {
					if (*write_ep == 0) {
						*write_ep = ep_addr;
						msg_pdbg("Found bulk OUT endpoint: 0x%02x\n", ep_addr);
					}
				}
			}

			/* If we found the interface and both endpoints, we're done */
			if (*write_ep != 0 && *read_ep != 0) {
				libusb_free_config_descriptor(config);
				return 0;
			}
		}
	}

	libusb_free_config_descriptor(config);

	if (*interface_num == -1) {
		msg_perr("Failed to find NV SMA SPI interface (class=0x%02x, subclass=0x%02x, protocol=0x%02x)\n",
				NV_SMA_INTERFACE_CLASS, NV_SMA_INTERFACE_SUBCLASS, NV_SMA_INTERFACE_PROTOCOL);
		return -1;
	}

	if (*write_ep == 0 || *read_ep == 0) {
		msg_perr("Failed to find required bulk endpoints on interface %d\n", *interface_num);
		return -1;
	}

	return 0;
}

/* Largely copied from ch341a_spi.c */
static int nv_sma_spi_init(const struct programmer_cfg *cfg)
{
	char *arg;
	uint16_t vid = devs_nv_sma_spi[0].vendor_id;
	uint16_t pid = 0;
	int index = 0;
	uint32_t freq_hz = 0; /* Use device default frequency */
	struct nv_sma_spi_data *nv_sma_data = calloc(1, sizeof(*nv_sma_data));
	if (!nv_sma_data) {
		msg_perr("Could not allocate space for SPI data\n");
		return 1;
	}

	int32_t ret = libusb_init(NULL);
	if (ret < 0) {
		msg_perr("Could not initialize libusb!\n");
		free(nv_sma_data);
		return 1;
	}
	/* Enable information, warning, and error messages (only). */
#if LIBUSB_API_VERSION < 0x01000106
	libusb_set_debug(NULL, 3);
#else
	libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
#endif

	char *bus_str = extract_programmer_param_str(cfg, "bus");
	char *devnum_str = extract_programmer_param_str(cfg, "devnum");
	if ((bus_str && !devnum_str) || (!bus_str && devnum_str)) {
		msg_perr("Error: Both 'bus' and 'devnum' parameters must be specified together.\n");
		free(bus_str);
		free(devnum_str);
		free(nv_sma_data);
		return 1;
	}

	long bus_num = 0;
	long dev_num = 0;
	if (bus_str && devnum_str) {
		char *endptr;
		errno = 0;
		bus_num = strtol(bus_str, &endptr, 10);
		if (errno != 0 || bus_str == endptr || *endptr != '\0' || bus_num < 0) {
			msg_perr("Error: Invalid bus number: '%s'.\n", bus_str);
			free(bus_str);
			free(devnum_str);
			free(nv_sma_data);
			return 1;
		}

		errno = 0;
		dev_num = strtol(devnum_str, &endptr, 10);
		if (errno != 0 || devnum_str == endptr || *endptr != '\0' || dev_num < 0) {
			msg_perr("Error: Invalid device number: '%s'.\n", devnum_str);
			free(bus_str);
			free(devnum_str);
			free(nv_sma_data);
			return 1;
		}

		msg_pinfo("Looking for Nvidia SMA at bus %ld, device %ld.\n", bus_num, dev_num);
		free(bus_str);
		free(devnum_str);
	}

	while (devs_nv_sma_spi[index].vendor_id != 0) {
		vid = devs_nv_sma_spi[index].vendor_id;
		/* fixme: remove pid check if subclass is accepted globaly for NVIDIA CORPORATION */
		pid = devs_nv_sma_spi[index].device_id;

		if (bus_str && devnum_str) {
			/* Select by bus and devnum */
			struct usb_match match;
			struct usb_device *found_device = NULL;

			usb_match_init(cfg, &match);
			usb_match_value_default(&match.vid, vid);
			usb_match_value_default(&match.pid, pid);
			usb_match_value_default(&match.bus, bus_num);
			usb_match_value_default(&match.address, dev_num);

			ret = usb_device_find(&match, &found_device);
			if (ret != 0) {
				msg_perr("Failed to find devices\n");
				free(bus_str);
				free(devnum_str);
				free(nv_sma_data);
				return ret;
			}

			ret = LIBUSB(libusb_open(found_device->device, &nv_sma_data->handle));
			usb_device_free(found_device);
			if (ret != 0) {
				msg_perr("Failed to open device\n");
				free(bus_str);
				free(devnum_str);
				free(nv_sma_data);
				return ret;
			}
		}
		else
			/* Default behavior - open first device found */
			nv_sma_data->handle = libusb_open_device_with_vid_pid(NULL, vid, pid);

		if (nv_sma_data->handle)
			break;

		index++;
	}
	if (!nv_sma_data->handle) {
		msg_perr("Couldn't find Nvidia System Management Agent.\n");
		free(nv_sma_data);
		return 1;
	}

	/* Discover interface and endpoints by class/subclass/protocol */
	ret = discover_interface_and_endpoints(nv_sma_data->handle,
						&nv_sma_data->interface,
						&nv_sma_data->write_ep,
						&nv_sma_data->read_ep);
	if (ret != 0) {
		msg_perr("Failed to discover NV SMA SPI interface and endpoints\n");
		goto error_exit;
	}
	msg_pinfo("Using interface %d with endpoints: write=0x%02x, read=0x%02x\n",
				nv_sma_data->interface, nv_sma_data->write_ep, nv_sma_data->read_ep);

	ret = libusb_detach_kernel_driver(nv_sma_data->handle, nv_sma_data->interface);
	if (ret != 0 && ret != LIBUSB_ERROR_NOT_FOUND)
		msg_pwarn("Cannot detach the existing USB driver. Claiming the interface may fail. %s\n",
			libusb_error_name(ret));

	ret = libusb_claim_interface(nv_sma_data->handle, nv_sma_data->interface);
	if (ret != 0) {
		msg_perr("Failed to claim interface %d: '%s'\n", nv_sma_data->interface, libusb_error_name(ret));
		goto error_exit;
	}

	struct libusb_device *dev;
	if (!(dev = libusb_get_device(nv_sma_data->handle))) {
		msg_perr("Failed to get device from device handle.\n");
		goto error_exit;
	}

	struct libusb_device_descriptor desc;
	ret = libusb_get_device_descriptor(dev, &desc);
	if (ret < 0) {
		msg_perr("Failed to get device descriptor: '%s'\n", libusb_error_name(ret));
		goto error_exit;
	}

	msg_pdbg("Device revision is %d.%01d.%01d\n",
		(desc.bcdDevice >> 8) & 0x00FF,
		(desc.bcdDevice >> 4) & 0x000F,
		(desc.bcdDevice >> 0) & 0x000F);

	/* select CS pin - default to CS0 if not specified */
	nv_sma_data->cs_bits = NV_SMA_CS0;  /* Default to CS0 bits */
	arg = extract_programmer_param_str(cfg, "cs");
	if (arg) {
		if (!strcasecmp(arg, "0")) {
			nv_sma_data->cs_bits = NV_SMA_CS0;
			msg_pdbg("Using chip select CS0\n");
		} else if (!strcasecmp(arg, "1")) {
			nv_sma_data->cs_bits = NV_SMA_CS1;
			msg_pdbg("Using chip select CS1\n");
		} else if (!strcasecmp(arg, "2")) {
			nv_sma_data->cs_bits = NV_SMA_CS2;
			msg_pdbg("Using chip select CS2\n");
		} else if (!strcasecmp(arg, "3")) {
			nv_sma_data->cs_bits = NV_SMA_CS3;
			msg_pdbg("Using chip select CS3\n");
		} else {
			msg_perr("Invalid chip select pin specified: '%s'. Valid values are 0, 1, 2, or 3.\n", arg);
			free(arg);
			return 1;
		}
		free(arg);
	} else {
		msg_pdbg("No CS specified, defaulting to CS0\n");
	}

	/* set NV_SMA SPI frequency */
	arg = extract_programmer_param_str(cfg, "spispeed");
	if (arg) {
		char *endptr;
		errno = 0;
		long freq_input = strtol(arg, &endptr, 10);

		if (errno != 0 || arg == endptr || *endptr != '\0' || freq_input <= 0) {
			msg_perr("Error: Invalid frequency value: '%s'. "
				"Please specify frequency in Hz (e.g., 15000000 for 15MHz).\n", arg);
			free(arg);
			goto error_exit;
		}

		freq_hz = (uint32_t)freq_input;

		/* Validate frequency range - typical SPI range */
		if (freq_hz > 60000000)
			msg_pwarn("Warning: Frequency %u Hz exceeds typical maximum 60MHz.\n", freq_hz);

		free(arg);
	}

	if (nv_sma_spi_config(nv_sma_data, freq_hz) < 0)
		goto error_exit;

	return register_spi_master(&spi_master_nv_sma_spi, nv_sma_data);

error_exit:
	nv_sma_spi_shutdown(nv_sma_data);
	return 1;
}

const struct programmer_entry programmer_nv_sma_spi = {
	.name		= "nv_sma_spi",
	.type		= USB,
	.devs.dev	= devs_nv_sma_spi,
	.init		= nv_sma_spi_init,
};
