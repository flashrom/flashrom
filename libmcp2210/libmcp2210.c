#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include "hid.h"
#include "libmcp2210.h"

////
// Commands
////
#define MCP2210_CMD_SET_CHIP_SETTINGS 0x21
#define MCP2210_CMD_SET_SPI_SETTINGS  0x40
#define MCP2210_CMD_TRANSFER_SPI_DATA 0x42

typedef struct mcp2210_packet {
	// Packet header
	uint8_t hdr[4];

	// Data structures
	union {
		mcp2210_spi_settings_t spi_settings;
		mcp2210_chip_settings_t chip_settings;
		uint8_t raw[60];
	} __attribute__((packed)) data;
} __attribute__((packed)) mcp2210_packet_t;

static inline int do_usb_cmd(hid_handle_t *handle,
	mcp2210_packet_t *cmd, mcp2210_packet_t *resp)
{
	if (-1 == hid_write(handle, cmd, sizeof(*cmd)))
		return -1;
	if (-1 == hid_read(handle, resp, sizeof(*resp)))
		return -1;
	return 0;
}

static inline void prepare_cmd(mcp2210_packet_t *cmd, uint8_t hdr_0, uint8_t hdr_1)
{
	bzero(cmd, sizeof(*cmd));
	cmd->hdr[0] = hdr_0;
	cmd->hdr[1] = hdr_1;
}

// Write chip settings
int mcp2210_chip_settings(hid_handle_t *handle,mcp2210_chip_settings_t *chip_settings)
{
	mcp2210_packet_t cmd;
	prepare_cmd(&cmd, MCP2210_CMD_SET_CHIP_SETTINGS, 0);
	cmd.data.chip_settings = *chip_settings;

	mcp2210_packet_t resp;
	if (-1 == do_usb_cmd(handle, &cmd, &resp))
		return -1;

	if (resp.hdr[0] != cmd.hdr[0] || resp.hdr[1] != 0) {
		errno = EACCES;
		return -1;
	}

	return 0;
}

// Write SPI settings
int mcp2210_spi_settings(hid_handle_t *handle, mcp2210_spi_settings_t *spi_settings)
{
	mcp2210_packet_t cmd;
	prepare_cmd(&cmd, MCP2210_CMD_SET_SPI_SETTINGS, 0);
	cmd.data.spi_settings = *spi_settings;

	mcp2210_packet_t resp;
	if (-1 == do_usb_cmd(handle, &cmd, &resp))
		return -1;

	if (resp.hdr[0] != cmd.hdr[0] || resp.hdr[1] != 0) {
		errno = EACCES;
		return -1;
	}

	return 0;
}

// Do an SPI transfer
int mcp2210_spi_transfer(hid_handle_t *handle,
	size_t data_len, void *data, mcp2210_spi_result_t *result)
{
	assert(data_len <= 60); // Max data bytes per HID packet

	mcp2210_packet_t cmd;
	prepare_cmd(&cmd, MCP2210_CMD_TRANSFER_SPI_DATA, data_len);
	memcpy(cmd.data.raw, data, data_len);

	mcp2210_packet_t resp;
	if (-1 == do_usb_cmd(handle, &cmd, &resp)) {
		return -1;
	}

	if (resp.hdr[0] != cmd.hdr[0]) {
		errno = EACCES;
		return -1;
	}
	if (resp.hdr[1] == 0xf7) {
		errno = EBUSY;
		return -1;
	}

	bzero(result, sizeof(mcp2210_spi_result_t));
	result->status = resp.hdr[1];
	result->spi_status = resp.hdr[3];
	result->data_len = resp.hdr[2];
	memcpy(result->data, resp.data.raw, result->data_len);

	return 0;
}
