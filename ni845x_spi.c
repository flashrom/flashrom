/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2018 Miklós Márton martonmiklosqdev@gmail.com
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
#if CONFIG_NI845X_SPI == 1


#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ni845x.h>
#include <unistd.h>

#include "flash.h"
#include "programmer.h"
#include "spi.h"


enum USB845xType {
    USB8451 = 0x7166,
    USB8452 = 0x7514,
    Unknown_NI845X_Device
};

static const struct spi_master spi_programmer_ni845x;

static unsigned char CS_number = 0; // use Chip select 0 as default
static char *serial_number = NULL; // by default open the first connected device
static enum USB845xType device_pid = Unknown_NI845X_Device;

static uInt32 device_handle = 0;
static NiHandle configuration_handle = 0;
static char error_string_buffer[1024];

// forward declaration
static int ni845x_spi_shutdown(void *data);
static int32 ni845x_spi_open_resource(char *resource_handle, uInt32 *opened_handle);

// USB-8452 supported voltages, keep this array in descending order!
static uint8_t usb8452_io_voltages_in_100mV[5] = {
    kNi845x33Volts,
    kNi845x25Volts,
    kNi845x18Volts,
    kNi845x15Volts,
    kNi845x12Volts
};

/* Copied from dediprog.c */
/* Might be useful for other USB devices as well. static for now. */
static int parse_voltage(char *voltage)
{
    char *tmp = NULL;
    int i;
    int millivolt = 0, fraction = 0;

    if (!voltage || !strlen(voltage)) {
        msg_perr("Empty voltage= specified.\n");
        return -1;
    }
    millivolt = (int)strtol(voltage, &tmp, 0);
    voltage = tmp;
    /* Handle "," and "." as decimal point. Everything after it is assumed
     * to be in decimal notation.
     */
    if ((*voltage == '.') || (*voltage == ',')) {
        voltage++;
        for (i = 0; i < 3; i++) {
            fraction *= 10;
            /* Don't advance if the current character is invalid,
             * but continue multiplying.
             */
            if ((*voltage < '0') || (*voltage > '9'))
                continue;
            fraction += *voltage - '0';
            voltage++;
        }
        /* Throw away remaining digits. */
        voltage += strspn(voltage, "0123456789");
    }
    /* The remaining string must be empty or "mV" or "V". */
    tolower_string(voltage);

    /* No unit or "V". */
    if ((*voltage == '\0') || !strncmp(voltage, "v", 1)) {
        millivolt *= 1000;
        millivolt += fraction;
    } else if (!strncmp(voltage, "mv", 2) ||
           !strncmp(voltage, "millivolt", 9)) {
        /* No adjustment. fraction is discarded. */
    } else {
        /* Garbage at the end of the string. */
        msg_perr("Garbage voltage= specified.\n");
        return -1;
    }
    return millivolt;
}

/**
 * @brief ni845x_spi_open
 * @param serial a null terminated string containing the serial number of the specific device or NULL
 * @return the 0 on successful competition, negative error code on failure
 */
static int ni845x_spi_open(char *serial, uInt32 *returnHandle)
{
    char resource_handle[256];
    NiHandle device_find_handle;
    uInt32 found_devices_count = 0;
    int32 tmp = 0;
    uint32_t vid, usb_bus, serial_as_number;
    int ret = -1;

    tmp = ni845xFindDevice(resource_handle, &device_find_handle, &found_devices_count);
    if (tmp == 0) {
        if (found_devices_count) {
            // Read the serial number and the PID here
            // VISA resource string format example:
            // USB0::0x3923::0x7514::DEADBEEF::RAW
            // where the 0x7514 is the PID
            // and the DEADBEEF is the serial of the device
            sscanf(resource_handle, "USB%d::0x%04X::0x%04X::%08X::RAW",
                   &usb_bus, &vid, &device_pid, &serial_as_number);
            if (serial_number == NULL) {
                // if no serial number passed -> open the first device
                if (ni845x_spi_open_resource(resource_handle, returnHandle) == 0) {
                    ret = 0;
                }
            } else {
                // serial number parameter specified -> check the resource_handle
                // and if it is not the device what we are looking for
                // loop on the available devices
                do {
                    if (strtol(serial_number, NULL, 16) == serial_as_number) {
                        if (ni845x_spi_open_resource(resource_handle, returnHandle) == 0)
                            ret = 0;
                        break;
                    }

                    // serial does not match the requested -> continue with the next one
                    found_devices_count--;
                    if (found_devices_count) {
                        if (ni845xFindDeviceNext(device_find_handle, resource_handle) == 0) {
                            sscanf(resource_handle, "USB%d::0x%04X::0x%04X::%08X::RAW",
                                   &usb_bus, &vid, &device_pid, &serial_as_number);
                        }
                    }
                } while (found_devices_count);
            }
        }
    } else {
        ni845xStatusToString(tmp, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_perr("ni845xFindDevice failed with: %s (%ld)\n", error_string_buffer, tmp);
        return -1;
    }

    tmp = ni845xCloseFindDeviceHandle(device_find_handle);
    if (tmp) {
        ni845xStatusToString(tmp, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_perr("ni845xCloseFindDeviceHandle failed with: %s (%ld)\n", error_string_buffer, tmp);
        return -1;
    }
    return ret;
}

/**
 * @brief ni845x_spi_open_resource
 * @param resource_handle the resource handle returned by the ni845xFindDevice or ni845xFindDeviceNext
 * @param opened_handle the opened handle from the ni845xOpen
 * @return the 0 on successful competition, negative error code on failure positive code on warning
 */
static int32 ni845x_spi_open_resource(char *resource_handle, uInt32 *opened_handle)
{
    int32 ret = ni845xOpen(resource_handle, opened_handle);
    if (ret < 0) {
        ni845xStatusToString(ret, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_perr("ni845xOpen(%s) failed with: %s (%ld)\n", resource_handle, error_string_buffer, ret);
    } else if (ret > 0) {
        ni845xStatusToString(ret, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_pwarn("ni845xOpen(%s) failed with: %s (%ld)\n", resource_handle, error_string_buffer, ret);
    }
    return ret;
}

/**
 * @brief usb8452_spi_set_io_voltage sets the IO voltage for the USB-8452 devices
 * @param IOVoltage_mV the desired IO voltage in mVolts
 * @return 0 on success, negative on error, positive on warning
 */
static int usb8452_spi_set_io_voltage(uint16_t IOVoltage_mV)
{
    int i = 0;
    uint8_t selected_voltage_100mV = 0;
    uint8_t IO_voltage_100mV = 0;

    if (device_pid == USB8451) {
        msg_pwarn("USB-8451 does not supports the changing of the SPI IO voltage\n");
        return 0;
    }

    // limit the IO voltage to 3.3V
    if (IOVoltage_mV > 3300) {
        msg_pinfo("USB-8452 maximum IO voltage is 3.3V\n");
        return -1;
    } else {
        IO_voltage_100mV = ((float)IOVoltage_mV / 100.0f);
    }

    // usb8452_io_voltages_in_100mV is a decreasing list of supported voltages
    for (i = 0; i < ARRAY_SIZE(usb8452_io_voltages_in_100mV) - 1; i++) {
        if (usb8452_io_voltages_in_100mV[i] >= IO_voltage_100mV &&
            IO_voltage_100mV > usb8452_io_voltages_in_100mV[i+1]) {
            selected_voltage_100mV = usb8452_io_voltages_in_100mV[i];
            if (IO_voltage_100mV != usb8452_io_voltages_in_100mV[i]) {
                msg_pwarn("USB-8452 IO Voltage forced to: %.1f V\n", (float)selected_voltage_100mV / 10.0f);
			} else {
                msg_pwarn("USB-8452 IO Voltage set to: %.1f V\n", (float)selected_voltage_100mV / 10.0f);
			}
            break;
        }
    }

    if (selected_voltage_100mV == 0) {
        msg_perr("The USB-8452 does not supports the %.1f V IO voltage\n", IOVoltage_mV/1000.0f);
        msg_perr("Supported IO voltages: ");
        for (i = 0; i < ARRAY_SIZE(usb8452_io_voltages_in_100mV); i++) {
            msg_perr("%.1fV", (double)usb8452_io_voltages_in_100mV[i]/1000.0f);
            if (i != ARRAY_SIZE(usb8452_io_voltages_in_100mV) - 1)
                msg_perr(", ");
        }
        msg_perr("\n");
        return -1;
    }

    i = ni845xSetIoVoltageLevel(device_handle, selected_voltage_100mV);
    if (i != 0) {
        ni845xStatusToString(i, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_perr("ni845xSetIoVoltageLevel failed with: %s (%d)\n", error_string_buffer, i);
        return -1;
    }
    return 0;
}

/**
 * @brief ni845x_spi_set_speed sets the SPI SCK speed
 * @param clockToSetInkHz SCK speed in KHz
 * @return
 */
static int ni845x_spi_set_speed(uint16_t clockToSetInkHz)
{
    int32 i = ni845xSpiConfigurationSetClockRate(configuration_handle, clockToSetInkHz);
    uInt16 clock_freq_read_KHz;
    if (i != 0) {
        ni845xStatusToString(i, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_perr("ni845xSpiConfigurationSetClockRate(%d) failed with: %s (%ld)\n",
                 clockToSetInkHz,
                 error_string_buffer,
                 i);
        return -1;
    }

    // read back the clock frequency and notify the user if it is not the same as it was requested
    i = ni845xSpiConfigurationGetClockRate(configuration_handle, &clock_freq_read_KHz);
    if (i != 0) {
        ni845xStatusToString(i, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_perr("ni845xSpiConfigurationSetClockRate failed with: %s (%ld)\n", error_string_buffer, i);
        return -1;
    } else {
        if (clock_freq_read_KHz != clockToSetInkHz) {
            msg_pinfo("SPI clock frequency forced to: %d KHz (requested: %d KHz)\n",
                      (int)clock_freq_read_KHz, (int)clockToSetInkHz);
        } else {
            msg_pinfo("SPI clock frequency set to: %d KHz\n",
                      (int)clockToSetInkHz);
        }
        return 0;
    }
}

/**
 * @brief ni845x_spi_print_available_devices prints a list of the available devices
 */
void ni845x_spi_print_available_devices(void)
{
    char resource_handle[256], device_type_string[16];
    NiHandle device_find_handle;
    uInt32 found_devices_count = 0;
    int32 tmp = 0;
    uint32_t pid, vid, usb_bus, serial_as_number;

    tmp = ni845xFindDevice(resource_handle, &device_find_handle, &found_devices_count);
    if (tmp != 0) {
        ni845xStatusToString(tmp, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_perr("ni845xFindDevice() failed with: %s (%ld)\n",
                 error_string_buffer,
                 tmp);
        return;
    }

    if (found_devices_count) {
        msg_pinfo("Available devices:\n");
        do {
            sscanf(resource_handle, "USB%d::0x%04X::0x%04X::%08X::RAW",
                   &usb_bus, &vid, &pid, &serial_as_number);
            switch (pid) {
            case USB8451:
                snprintf(device_type_string, ARRAY_SIZE(device_type_string), "USB-8451");
                break;
            case USB8452:
                snprintf(device_type_string, ARRAY_SIZE(device_type_string), "USB-8452");
                break;
            default:
                snprintf(device_type_string, ARRAY_SIZE(device_type_string), "Unknown device");
                break;
            }
            msg_pinfo("- %X (%s)\n", serial_as_number, device_type_string);

            found_devices_count--;
            if (found_devices_count) {
                ni845xFindDeviceNext(device_find_handle, resource_handle);
            }
        } while (found_devices_count);
    }

    tmp = ni845xCloseFindDeviceHandle(device_find_handle);
    if (tmp) {
        ni845xStatusToString(tmp, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_perr("ni845xCloseFindDeviceHandle failed with: %s (%ld)\n", error_string_buffer, tmp);
    }
}

int ni845x_spi_init(void)
{
    char *speed_str = NULL;
    char *CS_str = NULL;
    char *voltage = NULL;
    int IO_voltage_mV = 3300;
    int spi_speed_KHz = 1000; // selecting 1 MHz SCK is a good bet
    int32 tmp = 0;

    // read the cs parameter (which Chip select should we use)
    CS_str = extract_programmer_param("cs");
    if (CS_str) {
        CS_number = atoi(CS_str);
        if (CS_number < 0 || 7 < CS_number) {
            msg_perr("Only CS 0-7 supported\n");
            return 1;
        }
        free(CS_str);
    }

    voltage = extract_programmer_param("voltage");
    if (voltage != NULL) {
        IO_voltage_mV = parse_voltage(voltage);
        free(voltage);
        if (IO_voltage_mV < 0)
            return 1;
    }

    serial_number = extract_programmer_param("serial");
    if (ni845x_spi_open(serial_number, &device_handle)) {
        if (serial_number) {
            msg_pinfo("Could not find any NI USB-8451/8452 with serialnumber: %s!\n", serial_number);
            ni845x_spi_print_available_devices();
            msg_pinfo("Check the S/N field on the bottom of the device,\n"
                     "or use 'lsusb -v -d 3923:7166 | grep Serial' for USB-8451\n"
                     "or 'lsusb -v -d 3923:7514 | grep Serial' for USB-8452\n");
            free(serial_number);
        } else {
            msg_pinfo("Could not find any NI USB-845x device!\n");
        }
        return 1;
    } else {
        // open the SPI config handle
        tmp = ni845xSpiConfigurationOpen(&configuration_handle);
        if (tmp != 0) {
            ni845xStatusToString(tmp, ARRAY_SIZE(error_string_buffer), error_string_buffer);
            msg_perr("ni845xSpiConfigurationOpen() failed with: %s (%ld)\n",
                     error_string_buffer,
                     tmp);
        }
    }

    if (serial_number)
        free(serial_number);

    if (usb8452_spi_set_io_voltage(IO_voltage_mV) < 0) {
        return 1; // no alert here usb8452_spi_set_io_voltage already printed that
    }

    speed_str = extract_programmer_param("spispeed");
    if (speed_str) {
        spi_speed_KHz = atoi(speed_str);
        free(speed_str);
    }

    if (ni845x_spi_set_speed(spi_speed_KHz)) {
        msg_perr("Unable to set SPI speed\n");
        return 1;
    }

    if (register_shutdown(ni845x_spi_shutdown, NULL))
        return 1;
    register_spi_master(&spi_programmer_ni845x);

    return 0;
}

static int ni845x_spi_shutdown(void *data)
{
    int32 ret = 0;
    if (configuration_handle != 0) {
        ret = ni845xSpiConfigurationClose(configuration_handle);
        if (ret) {
            ni845xStatusToString(ret, ARRAY_SIZE(error_string_buffer), error_string_buffer);
            msg_perr("ni845xSpiConfigurationClose failed with: %s (%ld)\n", error_string_buffer, ret);
        }
    }

    if (device_handle != 0) {
        ret = ni845xClose(device_handle);
        if (ret) {
            ni845xStatusToString(ret, ARRAY_SIZE(error_string_buffer), error_string_buffer);
            msg_perr("ni845xSpiConfigurationClose failed with: %s (%ld)\n", error_string_buffer, ret);
        }
    }
    return 0;
}

static int ni845x_spi_transmit(struct flashctx *flash,
                                    unsigned int write_cnt,
                                    unsigned int read_cnt,
                                    const unsigned char *write_arr,
                                    unsigned char *read_arr)
{
    uInt32 read_size = 0;
    uInt8 *transfer_buffer = NULL;
    int32 ret = 0;

    transfer_buffer = calloc(write_cnt + read_cnt, sizeof(uInt8));
    if (transfer_buffer == NULL) {
        msg_gerr("Memory allocation failed!\n");
        msg_cinfo("FAILED.\n");
        return -1;
    }

    memcpy(transfer_buffer, write_arr, write_cnt);

    ret = ni845xSpiWriteRead(device_handle,
                                   configuration_handle,
                                   (uInt32)(write_cnt + read_cnt),
                                   transfer_buffer,
                                   &read_size,
                                   transfer_buffer);
    if (ret < 0) {
        // Negative specifies an error, meaning the function did not perform the expected behavior.
        ni845xStatusToString(ret, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_perr("ni845xSpiWriteRead failed with: %s (%ld)\n", error_string_buffer, ret);
        free(transfer_buffer);
        return -1;
    } else if (ret > 0) {
        // Positive specifies a warning, meaning the function performed as expected,
        // but a condition arose that might require attention.
        ni845xStatusToString(ret, ARRAY_SIZE(error_string_buffer), error_string_buffer);
        msg_pwarn("ni845xSpiWriteRead returned with: %s (%ld)\n", error_string_buffer, ret);
    }

    if (read_cnt != 0 && read_arr != NULL) {
        if ((read_cnt + write_cnt) != read_size) {
            msg_perr("ni845x_spi_transmit: expected and returned read count mismatch: %u expected, %ld recieved\n",
                     read_cnt, read_size);
            free(transfer_buffer);
            return -1;
        }
        memcpy(read_arr, &transfer_buffer[write_cnt], read_cnt);
    }
    free(transfer_buffer);
    return 0;
}


static const struct spi_master spi_programmer_ni845x = {
    .type		= SPI_CONTROLLER_NI845X,
    .max_data_read	= MAX_DATA_READ_UNLIMITED,
    .max_data_write	= MAX_DATA_WRITE_UNLIMITED,
    .command	= ni845x_spi_transmit,
    .multicommand	= default_spi_send_multicommand,
    .read		= default_spi_read,
    .write_256	= default_spi_write_256,
    .write_aai	= default_spi_write_aai,
};


#endif // endif CONFIG_NI845X
