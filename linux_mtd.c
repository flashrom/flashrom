/*
 * This file is part of the flashrom project.
 *
 * Copyright 2015 Google Inc.
 * Copyright 2018-present Facebook, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <mtd/mtd-user.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "flash.h"
#include "programmer.h"

#define LINUX_DEV_ROOT			"/dev"
#define LINUX_MTD_SYSFS_ROOT		"/sys/class/mtd"

struct linux_mtd_data {
	FILE *dev_fp;
	int device_is_writeable;
	int no_erase;
	/* Size info is presented in bytes in sysfs. */
	unsigned long int total_size;
	unsigned long int numeraseregions;
	/* only valid if numeraseregions is 0 */
	unsigned long int erasesize;
};

/* read a string from a sysfs file and sanitize it */
static int read_sysfs_string(const char *sysfs_path, const char *filename, char *buf, int len)
{
	int i;
	size_t bytes_read;
	FILE *fp;
	char path[strlen(LINUX_MTD_SYSFS_ROOT) + 32];

	snprintf(path, sizeof(path), "%s/%s", sysfs_path, filename);

	if ((fp = fopen(path, "r")) == NULL) {
		msg_perr("Cannot open %s\n", path);
		return 1;
	}

	clearerr(fp);
	bytes_read = fread(buf, 1, (size_t)len, fp);
	if (!feof(fp) && ferror(fp)) {
		msg_perr("Error occurred when reading %s\n", path);
		fclose(fp);
		return 1;
	}

	buf[bytes_read] = '\0';

	/*
	 * Files from sysfs sometimes contain a newline or other garbage that
	 * can confuse functions like strtoul() and ruin formatting in print
	 * statements. Replace the first non-printable character (space is
	 * considered printable) with a proper string terminator.
	 */
	for (i = 0; i < len; i++) {
		if (!isprint(buf[i])) {
			buf[i] = '\0';
			break;
		}
	}

	fclose(fp);
	return 0;
}

static int read_sysfs_int(const char *sysfs_path, const char *filename, unsigned long int *val)
{
	char buf[32];
	char *endptr;

	if (read_sysfs_string(sysfs_path, filename, buf, sizeof(buf)))
		return 1;

	errno = 0;
	*val = strtoul(buf, &endptr, 0);
	if (*endptr != '\0') {
		msg_perr("Error reading %s\n", filename);
		return 1;
	}

	if (errno) {
		msg_perr("Error reading %s: %s\n", filename, strerror(errno));
		return 1;
	}

	return 0;
}

static int popcnt(unsigned int u)
{
	int count = 0;

	while (u) {
		u &= u - 1;
		count++;
	}

	return count;
}

/* returns 0 to indicate success, non-zero to indicate error */
static int get_mtd_info(const char *sysfs_path, struct linux_mtd_data *data)
{
	unsigned long int tmp;
	char device_name[32];

	/* Flags */
	if (read_sysfs_int(sysfs_path, "flags", &tmp))
		return 1;
	if (tmp & MTD_WRITEABLE) {
		/* cache for later use by write function */
		data->device_is_writeable = 1;
	}
	if (tmp & MTD_NO_ERASE) {
		data->no_erase = 1;
	}

	/* Device name */
	if (read_sysfs_string(sysfs_path, "name", device_name, sizeof(device_name)))
		return 1;

	/* Total size */
	if (read_sysfs_int(sysfs_path, "size", &data->total_size))
		return 1;
	if (popcnt(data->total_size) != 1) {
		msg_perr("MTD size is not a power of 2\n");
		return 1;
	}

	/* Erase size */
	if (read_sysfs_int(sysfs_path, "erasesize", &data->erasesize))
		return 1;
	if (popcnt(data->erasesize) != 1) {
		msg_perr("MTD erase size is not a power of 2\n");
		return 1;
	}

	/* Erase regions */
	if (read_sysfs_int(sysfs_path, "numeraseregions", &data->numeraseregions))
		return 1;
	if (data->numeraseregions != 0) {
		msg_perr("Non-uniform eraseblock size is unsupported.\n");
		return 1;
	}

	msg_pdbg("%s: device_name: \"%s\", is_writeable: %d, "
		"numeraseregions: %lu, total_size: %lu, erasesize: %lu\n",
		__func__, device_name, data->device_is_writeable,
		data->numeraseregions, data->total_size, data->erasesize);

	return 0;
}

static int linux_mtd_probe(struct flashctx *flash)
{
	struct linux_mtd_data *data = flash->mst->opaque.data;

	if (data->no_erase)
		flash->chip->feature_bits |= FEATURE_NO_ERASE;
	flash->chip->tested = TEST_OK_PREW;
	flash->chip->total_size = data->total_size / 1024;	/* bytes -> kB */
	flash->chip->block_erasers[0].eraseblocks[0].size = data->erasesize;
	flash->chip->block_erasers[0].eraseblocks[0].count =
		data->total_size / data->erasesize;
	return 1;
}

static int linux_mtd_read(struct flashctx *flash, uint8_t *buf,
			  unsigned int start, unsigned int len)
{
	struct linux_mtd_data *data = flash->mst->opaque.data;
	unsigned int eb_size = flash->chip->block_erasers[0].eraseblocks[0].size;
	unsigned int i;

	if (fseek(data->dev_fp, start, SEEK_SET) != 0) {
		msg_perr("Cannot seek to 0x%06x: %s\n", start, strerror(errno));
		return 1;
	}

	for (i = 0; i < len; ) {
		/*
		 * Try to align reads to eraseblock size.
		 * FIXME: Shouldn't actually be necessary, but not all MTD
		 * drivers handle arbitrary large reads well.
		 */
		unsigned int step = eb_size - ((start + i) % eb_size);
		step = min(step, len - i);

		if (fread(buf + i, step, 1, data->dev_fp) != 1) {
			msg_perr("Cannot read 0x%06x bytes at 0x%06x: %s\n",
					step, start + i, strerror(errno));
			return 1;
		}

		i += step;
	}

	return 0;
}

/* this version assumes we must divide the write request into chunks ourselves */
static int linux_mtd_write(struct flashctx *flash, const uint8_t *buf,
				unsigned int start, unsigned int len)
{
	struct linux_mtd_data *data = flash->mst->opaque.data;
	unsigned int chunksize = flash->chip->block_erasers[0].eraseblocks[0].size;
	unsigned int i;

	if (!data->device_is_writeable)
		return 1;

	if (fseek(data->dev_fp, start, SEEK_SET) != 0) {
		msg_perr("Cannot seek to 0x%06x: %s\n", start, strerror(errno));
		return 1;
	}

	/*
	 * Try to align writes to eraseblock size. We want these large enough
	 * to give MTD room for optimizing performance.
	 * FIXME: Shouldn't need to divide this up at all, but not all MTD
	 * drivers handle arbitrary large writes well.
	 */
	for (i = 0; i < len; ) {
		unsigned int step = chunksize - ((start + i) % chunksize);
		step = min(step, len - i);

		if (fwrite(buf + i, step, 1, data->dev_fp) != 1) {
			msg_perr("Cannot write 0x%06x bytes at 0x%06x\n", step, start + i);
			return 1;
		}

		if (fflush(data->dev_fp) == EOF) {
			msg_perr("Failed to flush buffer: %s\n", strerror(errno));
			return 1;
		}

		i += step;
	}

	return 0;
}

static int linux_mtd_erase(struct flashctx *flash,
			unsigned int start, unsigned int len)
{
	struct linux_mtd_data *data = flash->mst->opaque.data;
	uint32_t u;

	if (data->no_erase) {
		msg_perr("%s: device does not support erasing. Please file a "
				"bug report at flashrom@flashrom.org\n", __func__);
		return 1;
	}

	if (data->numeraseregions != 0) {
		/* TODO: Support non-uniform eraseblock size using
		   use MEMGETREGIONCOUNT/MEMGETREGIONINFO ioctls */
		msg_perr("%s: numeraseregions must be 0\n", __func__);
		return 1;
	}

	for (u = 0; u < len; u += data->erasesize) {
		struct erase_info_user erase_info = {
			.start = start + u,
			.length = data->erasesize,
		};

		if (ioctl(fileno(data->dev_fp), MEMERASE, &erase_info) == -1) {
			msg_perr("%s: ioctl: %s\n", __func__, strerror(errno));
			return 1;
		}
	}

	return 0;
}

static int linux_mtd_shutdown(void *data)
{
	struct linux_mtd_data *mtd_data = data;
	if (mtd_data->dev_fp != NULL) {
		fclose(mtd_data->dev_fp);
	}
	free(data);

	return 0;
}

static const struct opaque_master linux_mtd_opaque_master = {
	/* max_data_{read,write} don't have any effect for this programmer */
	.max_data_read	= MAX_DATA_UNSPECIFIED,
	.max_data_write	= MAX_DATA_UNSPECIFIED,
	.probe		= linux_mtd_probe,
	.read		= linux_mtd_read,
	.write		= linux_mtd_write,
	.erase		= linux_mtd_erase,
	.shutdown	= linux_mtd_shutdown,
};

/* Returns 0 if setup is successful, non-zero to indicate error */
static int linux_mtd_setup(int dev_num, struct linux_mtd_data *data)
{
	char sysfs_path[32];
	int ret = 1;

	/* Start by checking /sys/class/mtd/mtdN/type which should be "nor" for NOR flash */
	if (snprintf(sysfs_path, sizeof(sysfs_path), "%s/mtd%d/", LINUX_MTD_SYSFS_ROOT, dev_num) < 0)
		goto linux_mtd_setup_exit;

	char buf[4] = { 0 };
	if (read_sysfs_string(sysfs_path, "type", buf, sizeof(buf)))
		return 1;

	if (strcmp(buf, "nor")) {
		msg_perr("MTD device %d type is not \"nor\"\n", dev_num);
		goto linux_mtd_setup_exit;
	}

	/* sysfs shows the correct device type, see if corresponding device node exists */
	char dev_path[32];
	struct stat s;
	snprintf(dev_path, sizeof(dev_path), "%s/mtd%d", LINUX_DEV_ROOT, dev_num);
	errno = 0;
	if (stat(dev_path, &s) < 0) {
		msg_pdbg("Cannot stat \"%s\": %s\n", dev_path, strerror(errno));
		goto linux_mtd_setup_exit;
	}

	/* so far so good, get more info from other files in this dir */
	if (snprintf(sysfs_path, sizeof(sysfs_path), "%s/mtd%d/", LINUX_MTD_SYSFS_ROOT, dev_num) < 0)
		goto linux_mtd_setup_exit;
	if (get_mtd_info(sysfs_path, data))
		goto linux_mtd_setup_exit;

	/* open file stream and go! */
	if ((data->dev_fp = fopen(dev_path, "r+")) == NULL) {
		msg_perr("Cannot open file stream for %s\n", dev_path);
		goto linux_mtd_setup_exit;
	}
	ret = setvbuf(data->dev_fp, NULL, _IONBF, 0);
	if (ret)
		msg_pwarn("Failed to set MTD device to unbuffered: %d\n", ret);

	msg_pinfo("Opened %s successfully\n", dev_path);

	ret = 0;
linux_mtd_setup_exit:
	return ret;
}

static int linux_mtd_init(void)
{
	char *param;
	int dev_num = 0;
	int ret = 1;
	struct linux_mtd_data *data = NULL;

	param = extract_programmer_param("dev");
	if (param) {
		char *endptr;

		dev_num = strtol(param, &endptr, 0);
		if ((*endptr != '\0') || (dev_num < 0)) {
			msg_perr("Invalid device number %s. Use flashrom -p "
				"linux_mtd:dev=N where N is a valid MTD\n"
				"device number.\n", param);
			goto linux_mtd_init_exit;
		}
	}

	/*
	 * If user specified the MTD device number then error out if it doesn't
	 * appear to exist. Otherwise assume the error is benign and print a
	 * debug message. Bail out in either case.
	 */
	char sysfs_path[32];
	if (snprintf(sysfs_path, sizeof(sysfs_path), "%s/mtd%d", LINUX_MTD_SYSFS_ROOT, dev_num) < 0)
		goto linux_mtd_init_exit;

	struct stat s;
	if (stat(sysfs_path, &s) < 0) {
		if (param)
			msg_perr("%s does not exist\n", sysfs_path);
		else
			msg_pdbg("%s does not exist\n", sysfs_path);
		goto linux_mtd_init_exit;
	}
	free(param);

	data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate memory for linux_mtd_data\n");
		return 1;
	}

	/* Get MTD info and store it in `data` */
	if (linux_mtd_setup(dev_num, data)) {
		free(data);
		return 1;
	}

	return register_opaque_master(&linux_mtd_opaque_master, data);

linux_mtd_init_exit:
	free(param);
	return ret;
}

const struct programmer_entry programmer_linux_mtd = {
	.name			= "linux_mtd",
	.type			= OTHER,
	.devs.note		= "Device files /dev/mtd*\n",
	.init			= linux_mtd_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
