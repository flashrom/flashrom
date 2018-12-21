/* HID module interface */
#ifndef HID_H
#define HID_H

// Defined by each HID module
typedef struct hid_handle hid_handle_t;

// Initialize the HID module
// Returns: 0 -> success; -1 -> error, errno is set
int hid_init();

// Find all HID devices with the specified VID and PID
// Returns: number of devices found; -1 -> error, errno is set
ssize_t hid_find_devices(uint16_t vid, uint16_t pid, hid_handle_t **dest, size_t dest_len);

// Get a textual representation of a HID device
const char *hid_device_desc(hid_handle_t *handle);

// Write to HID device
// Returns: number of bytes written; -1 -> error, errno is set
ssize_t hid_write(hid_handle_t *handle, void *data, size_t data_len);

// Read from an HID device
// Returns: number of bytes read; -1 -> error, errno is set
ssize_t hid_read(hid_handle_t *handle, void *buffer, size_t buffer_len);

// Call when finished using a device
void hid_cleanup_device(hid_handle_t *handle);

// Call when finished using the HID module
void hid_fini();
#endif
