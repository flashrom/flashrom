#ifndef LIBMCP2210_H
#define LIBMCP2210_H

// Byteswapping required on big-endian platforms
#ifdef LITTLE_ENDIAN
#define b16(x) x
#define b32(x) x
#elif defined BIG_ENDIAN
#define b16(x) __builtin_bswap16(x)
#define b32(x) __builtin_bswap32(x)
#else
#error Unsupported endianness
#endif

// Factory VID and PID of the MCP2210
#define MCP2210_VID 0x04d8
#define MCP2210_PID 0x00de

// Chip settings
#define MCP2210_PIN_GPIO 0
#define MCP2210_PIN_CS 1
#define MCP2210_PIN_DEDICATED 2

#define MCP2210_BUS_RELEASE_DISABLE 1

typedef struct mcp2210_chip_settings {
	uint8_t  pins[9];
	uint16_t gpio_default;
	uint16_t gpio_direction;
	uint8_t  other_settings;
	uint8_t  nvram_lock;
	uint8_t  new_password[8];
} __attribute__((packed)) mcp2210_chip_settings_t;

// Write chip settings
int mcp2210_chip_settings(hid_handle_t *handle, mcp2210_chip_settings_t *chip_settings);

// SPI settings
#define MCP2210_MIN_BITRATE 1464
#define MCP2210_MAX_BITRATE 12000000

typedef struct mcp2210_spi_settings {
	uint32_t bitrate;
	uint16_t idle_cs;
	uint16_t active_cs;
	// CS assert to first data byte delay
	uint16_t cs_to_data_delay;
	// Last data byte to CS assert delay
	uint16_t data_to_cs_delay;
	// Delay between subsequent data bytes
	uint16_t data_delay;
	uint16_t bytes_per_transaction;
	uint8_t spi_mode;
} __attribute__((packed)) mcp2210_spi_settings_t;

// Write SPI settings
int mcp2210_spi_settings(hid_handle_t *handle, mcp2210_spi_settings_t *spi_settings);

// SPI engine status
#define MCP2210_SPI_STATUS_FINISHED    0x10
#define MCP2210_SPI_STATUS_NO_DATA     0x20
#define MCP2210_SPI_STATUS_DATA_NEEDED 0x30

// Do an SPI transfer
typedef struct mcp2210_spi_result {
	int status;
	uint8_t spi_status;
	uint8_t data_len;
	uint8_t data[60];
} mcp2210_spi_result_t;

int mcp2210_spi_transfer(hid_handle_t *handle,
	size_t data_len, void *data, mcp2210_spi_result_t *result);

#endif
