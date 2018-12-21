/* HID support code on Linux */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libudev.h>
#include "hid.h"

// HID handle
struct hid_handle {
	char *devpath;
	int fd;
};

// udev handle
static struct udev* udev = NULL;

int hid_init()
{
	if (!udev) {
		if (!(udev = udev_new()))
			return -1;
	}
	return 0;
}

ssize_t hid_find_devices(uint16_t vid, uint16_t pid, hid_handle_t **dest, size_t dest_len)
{
	if (!udev) {
		errno = EINVAL;
		return -1;
	}

	if (!dest_len) {
		errno = ENOMEM;
		return -1;
	}

	struct udev_enumerate *enumerate = udev_enumerate_new(udev);
	if (!enumerate)
		goto err1;

	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);

	struct udev_list_entry *cur, *all = udev_enumerate_get_list_entry(enumerate);
	if (!all)
		goto err2;

	size_t i = 0;
	udev_list_entry_foreach(cur, all) {
		struct udev_device *device =
		udev_device_new_from_syspath(udev, udev_list_entry_get_name(cur));
		if (!device)
			goto err2;

		struct udev_device *parent =
		udev_device_get_parent_with_subsystem_devtype(
			device, "usb", "usb_device");

		if (!parent) // Ignore non-usb HID devices
			goto loop_cleanup;

		uint16_t cur_vid = (uint16_t) strtol(udev_device_get_sysattr_value(parent, "idVendor"), NULL, 16);
		uint16_t cur_pid = (uint16_t) strtol(udev_device_get_sysattr_value(parent, "idProduct"), NULL, 16);
		if (cur_vid == vid && cur_pid == pid) {
			struct hid_handle *handle = malloc(sizeof(struct hid_handle));
			handle->devpath = strdup(udev_device_get_devnode(device));
			handle->fd = -1;
			dest[i++] = handle;
		}

		loop_cleanup:
		udev_device_unref(device);
		if (i >= dest_len) {
			errno = ENOMEM;
			goto err2;
		}
	}

	udev_enumerate_unref(enumerate);
	return (ssize_t) i;
	err2:
	udev_enumerate_unref(enumerate);
	err1:
	return -1;
}

const char *hid_device_desc(hid_handle_t *handle)
{
	return handle->devpath;
}

ssize_t hid_write(struct hid_handle *handle, void *data, size_t data_len)
{
	if (handle->fd == -1) {
		handle->fd = open(handle->devpath, O_RDWR);
		if (handle->fd == -1)
			return -1;
	}
	return write(handle->fd, data, data_len);
}

ssize_t hid_read(struct hid_handle *handle, void *buffer, size_t buffer_len)
{
	if (handle->fd == -1) {
		handle->fd = open(handle->devpath, O_RDWR);
		if (handle->fd == -1)
			return -1;
	}
	return read(handle->fd, buffer, buffer_len);
}

void hid_cleanup_device(struct hid_handle *handle)
{
	if (handle->fd != -1)
		close(handle->fd);
	free(handle->devpath);
	free(handle);
}

void hid_fini()
{
	if (udev) {
		udev_unref(udev);
		udev = NULL;
	}
}
