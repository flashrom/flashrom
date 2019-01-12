// Flashrom programmer for the MCP2210 chip
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "programmer.h"
#include "libmcp2210/hid.h"
#include "libmcp2210/libmcp2210.h"

static hid_handle_t *mcp2210_handle;

// Chip settings
// FIXME: implement a way to use another chip select line than CS0
static mcp2210_chip_settings_t chip_settings = {
	.pins = {
		MCP2210_PIN_CS,
		MCP2210_PIN_GPIO,
		MCP2210_PIN_GPIO,
		MCP2210_PIN_GPIO,
		MCP2210_PIN_GPIO,
		MCP2210_PIN_GPIO,
		MCP2210_PIN_GPIO,
		MCP2210_PIN_GPIO,
		MCP2210_PIN_GPIO
	},
	.gpio_default = 0,
	.gpio_direction = 0,
	.other_settings = 0,
	.nvram_lock = 0,
	.new_password = { 0, 0, 0, 0, 0, 0, 0, 0 }
};

// SPI settings
static mcp2210_spi_settings_t spi_settings = {
	.bitrate				= b32(8000000),
	.idle_cs				= b16(1),
	.active_cs				= b16(0),
	.cs_to_data_delay		= b16(0),
	.data_to_cs_delay		= b16(0),
	.data_delay				= b16(0),
	.bytes_per_transaction	= b16(0), // set this for each command
	.spi_mode				= 0
};

static int mcp2210_spi_send_command(struct flashctx *flash,
	unsigned int writecnt, unsigned int readcnt,
	const unsigned char *writearr, unsigned char *readarr)
{
	//printf("\nCMD: W: %d R: %d\n", writecnt, readcnt);

	size_t transfer_total = writecnt + readcnt;

	// This is the technical transfer limit of the MCP2210
	// In theory this can be avoided by using GPIO as chip select
	// And doing multiple transfers
	assert(transfer_total <= 0xffff);

	// Setting this on every command is a MAJOR bottleneck
	if (spi_settings.bytes_per_transaction != b16(transfer_total)) {
		spi_settings.bytes_per_transaction = b16(transfer_total);
		if (-1 == mcp2210_spi_settings(mcp2210_handle, &spi_settings)) {
			perror("Failed to set MCP2210 SPI settings");
			return -1;
		}
	}

	// Write buffer
	uint8_t write_buf[transfer_total];
	uint8_t *write_ptr = write_buf;

	bzero(write_buf, transfer_total);
	memcpy(write_buf, writearr, writecnt);

	// Read buffer
	uint8_t read_buf[transfer_total];
	uint8_t *read_ptr = read_buf;

	size_t write_left = transfer_total;
	size_t read_left = transfer_total;

	// SPI result
	mcp2210_spi_result_t result;

	while (write_left || read_left) {
		size_t write_current = write_left > 60 ? 60 : write_left;

		if (-1 == mcp2210_spi_transfer(mcp2210_handle, write_current, write_ptr,
				&result)) {
			perror("SPI transfer failed");
			return -1;
		}

		if (result.status == 0xf8) // retry
			continue;

		write_left -= write_current;
		write_ptr += write_current;

		if (result.data_len) {
			if (read_left < result.data_len) {
				fprintf(stderr, "WARN: MCP2210 sent more data than expected\n");
			} else {
				memcpy(read_ptr, result.data, result.data_len);
				read_left -= result.data_len;
				read_ptr += result.data_len;
			}
		}

		if (result.spi_status == MCP2210_SPI_STATUS_FINISHED
			&& (write_left || read_left)) {
			fprintf(stderr, "ERR: MCP2210 sent less data than expected\n");
			return -1;
		}
	}

	/*printf("\n");
	for (size_t i = 0; i < transfer_total; ++i) {
		printf("%02x\n", read_buf[i]);
	}
	printf("\n");*/

	memcpy(readarr, read_buf + writecnt, readcnt);
	return 0;
}

static const struct spi_master spi_master_mcp2210_spi = {
	.type			= SPI_CONTROLLER_MCP2210_SPI,
	.features		= SPI_MASTER_4BA,
	.max_data_read	= 0x7fff,
	.max_data_write	= 0x7fff,
	.command		= mcp2210_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read			= default_spi_read,
	.write_256		= default_spi_write_256,
	.write_aai		= default_spi_write_aai,
};

static int mcp2210_spi_shutdown()
{
	hid_cleanup_device(mcp2210_handle);
	hid_fini();
	return 0;
}

int mcp2210_spi_init()
{
	if (-1 == hid_init()) {
		perror("Failed to initialize HID library");
		return -1;
	}

	// Try locating the MCP2210
	hid_handle_t *devices[10];
	ssize_t device_count =
		hid_find_devices(MCP2210_VID, MCP2210_PID, devices, 10);
	if (-1 == device_count) {
		perror("Searching for HID devices failed");
		goto err;
	}
	if (1 != device_count) {
		fprintf(stderr,
			"Please have one and only one MCP2210 device plugged in\n");
		for (size_t i = 0; i < device_count; ++i)
			hid_cleanup_device(devices[i]);
		goto err;
	}

	mcp2210_handle = devices[0];
	if (-1 == mcp2210_chip_settings(mcp2210_handle, &chip_settings)) {
		perror("Failed to configure MCP2210");
		hid_cleanup_device(mcp2210_handle);
		goto err;
	}

	register_shutdown(mcp2210_spi_shutdown, NULL);
	register_spi_master(&spi_master_mcp2210_spi);
	return 0;
err:
	hid_fini();
	return -1;
}
