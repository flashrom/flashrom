/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Abdelkader Boudih <flashrom@seuros.com>
 */

/*
 * Fault injection programmer -- a programmable fault-injection layer that
 * wraps an existing programmer and injects deterministic, reproducible faults
 * to simulate unreliable hardware behaviour.
 *
 * Usage:
 *   -p fault:backend=dummy[,bus=spi,emulate=W25Q128FV][,seed=42][,flip_prob=0.001]
 *
 * The backend parameter specifies the underlying programmer and its parameters
 * (separated by commas like any other programmer).  All fault parameters are
 * extracted first; whatever remains is forwarded to the backend.
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "flash.h"
#include "log.h"
#include "programmer.h"
#include "spi.h"

/*
 * Xoshiro256** PRNG: same seed = same faults on every platform.
 * https://en.wikipedia.org/wiki/Xorshift#xoshiro256**
 */

struct fault_rng {
	uint64_t s[4];
};

static uint64_t rng_rotl(uint64_t x, int k)
{
	return (x << k) | (x >> (64 - k));
}

static uint64_t rng_next(struct fault_rng *rng)
{
	const uint64_t result = rng_rotl(rng->s[1] * 5, 7) * 9;
	const uint64_t t = rng->s[1] << 17;

	rng->s[2] ^= rng->s[0];
	rng->s[3] ^= rng->s[1];
	rng->s[1] ^= rng->s[2];
	rng->s[0] ^= rng->s[3];
	rng->s[2] ^= t;
	rng->s[3] = rng_rotl(rng->s[3], 45);

	return result;
}

/* Return a double in [0, 1). */
static double rng_uniform(struct fault_rng *rng)
{
	return (double)(rng_next(rng) >> 11) / (double)(1ULL << 53);
}

static void rng_seed(struct fault_rng *rng, uint64_t seed)
{
	/* SplitMix64 to expand a single seed into 4 state words. */
	for (int i = 0; i < 4; i++) {
		seed += 0x9e3779b97f4a7c15ULL;
		uint64_t z = seed;
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
		z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
		rng->s[i] = z ^ (z >> 31);
	}
}

/*
 * Fault engine state
 */

struct fault_spi_master;

struct fault_data {
	struct fault_rng rng;

	/* Bit corruption */
	double flip_prob;		/* per-byte bit-flip probability */

	/* Read faults */
	double short_read_prob;		/* return truncated buffer */
	double min_read_ratio;		/* minimum fraction returned (0.5 = at least 50%) */

	/* Write faults */
	double write_fail_prob;		/* return error on write */
	double write_lie_prob;		/* return success, do nothing */
	double partial_write_prob;	/* only write a subset */

	/* Saved original SPI masters for each wrapped backend registration. */
	struct fault_spi_master *wrapped_masters;
	size_t wrapped_master_count;

	/* Stats */
	unsigned long flips_injected;
	unsigned long reads_shortened;
	unsigned long writes_failed;
	unsigned long writes_lied;
	unsigned long writes_partial;
};

struct fault_spi_master {
	struct fault_data *fault;
	int master_index;
	struct spi_master orig_spi;
};

/*
 * Fault injection helpers
 */

static void fault_corrupt_buffer(struct fault_data *fd, uint8_t *buf, unsigned int len)
{
	if (fd->flip_prob <= 0.0)
		return;

	for (unsigned int i = 0; i < len; i++) {
		if (rng_uniform(&fd->rng) < fd->flip_prob) {
			int bit = rng_next(&fd->rng) % 8;
			buf[i] ^= (1U << bit);
			fd->flips_injected++;
			msg_pdbg("[fault] flip at offset 0x%x bit %d\n", i, bit);
		}
	}
}

/*
 * SPI master wrappers
 */

/*
 * Temporarily restore the original SPI master so the backend sees its
 * own callbacks and data pointer.  This is necessary because functions
 * like default_spi_read() internally call flash->mst->spi.command().
 */
static void fault_swap_in_orig(struct flashctx *flash, struct spi_master *saved)
{
	*saved = flash->mst->spi;
	flash->mst->spi = ((struct fault_spi_master *)flash->mst->spi.data)->orig_spi;
}

static void fault_swap_in_wrapper(struct flashctx *flash,
				  const struct spi_master *saved)
{
	flash->mst->spi = *saved;
}

static struct fault_spi_master *fault_get_spi_master(const struct flashctx *flash)
{
	return flash->mst->spi.data;
}

static int fault_spi_send_command(const struct flashctx *flash,
				  unsigned int writecnt, unsigned int readcnt,
				  const unsigned char *writearr,
				  unsigned char *readarr)
{
	struct fault_spi_master *fm = fault_get_spi_master(flash);
	struct fault_data *fd = fm->fault;
	struct spi_master saved;
	int ret;

	fault_swap_in_orig((struct flashctx *)flash, &saved);

	/* Some backends only implement multicommand (e.g. ft2232_spi).
	 * Fall back to it when command is NULL. */
	if (fm->orig_spi.command) {
		ret = fm->orig_spi.command(flash, writecnt, readcnt,
					   writearr, readarr);
	} else if (fm->orig_spi.multicommand) {
		struct spi_command cmds[] = {
			{
				.writecnt = writecnt,
				.readcnt = readcnt,
				.writearr = writearr,
				.readarr = readarr,
			},
			NULL_SPI_CMD,
		};
		ret = fm->orig_spi.multicommand(flash, cmds);
	} else {
		ret = SPI_GENERIC_ERROR;
	}

	fault_swap_in_wrapper((struct flashctx *)flash, &saved);

	if (!ret && readcnt > 0)
		fault_corrupt_buffer(fd, readarr, readcnt);

	return ret;
}

static int fault_spi_multicommand(const struct flashctx *flash,
				  struct spi_command *cmds)
{
	struct fault_spi_master *fm = fault_get_spi_master(flash);
	struct fault_data *fd = fm->fault;
	struct spi_master saved;

	fault_swap_in_orig((struct flashctx *)flash, &saved);
	/* After swap, flash->mst->spi is the original master.
	 * spi_send_multicommand() dispatches to .multicommand or
	 * falls back to .command -- both are now the originals. */
	int ret = spi_send_multicommand(flash, cmds);
	fault_swap_in_wrapper((struct flashctx *)flash, &saved);

	/* Corrupt read buffers in each command. */
	if (!ret) {
		for (int i = 0; cmds[i].writecnt || cmds[i].readcnt; i++) {
			if (cmds[i].readcnt > 0)
				fault_corrupt_buffer(fd, cmds[i].readarr,
						     cmds[i].readcnt);
		}
	}

	return ret;
}

static int fault_spi_read(struct flashctx *flash, uint8_t *buf,
			  unsigned int start, unsigned int len)
{
	struct fault_spi_master *fm = fault_get_spi_master(flash);
	struct fault_data *fd = fm->fault;
	struct spi_master saved;

	/* Short read: read fewer bytes, then fill the tail with a
	 * deterministic pattern so the result depends only on the seed,
	 * not on allocator state. */
	unsigned int actual_len = len;
	if (fd->short_read_prob > 0.0 && rng_uniform(&fd->rng) < fd->short_read_prob) {
		double ratio = fd->min_read_ratio +
			       rng_uniform(&fd->rng) * (1.0 - fd->min_read_ratio);
		actual_len = (unsigned int)(len * ratio);
		if (actual_len == 0)
			actual_len = 1;
		if (actual_len > len)
			actual_len = len;
		fd->reads_shortened++;
		msg_pdbg("[fault] short read: %u/%u bytes\n", actual_len, len);
	}

	fault_swap_in_orig(flash, &saved);
	int ret = fm->orig_spi.read(flash, buf, start, actual_len);
	fault_swap_in_wrapper(flash, &saved);

	if (!ret) {
		/* Fill unread tail with 0xFF (erased-flash pattern) so
		 * the buffer content is deterministic regardless of
		 * allocator state. */
		if (actual_len < len)
			memset(buf + actual_len, 0xFF, len - actual_len);
		fault_corrupt_buffer(fd, buf, actual_len);
	}

	return ret;
}

/*
 * Decide which write fault (if any) to inject.  Returns true if the caller
 * should short-circuit and return *ret immediately (write fail or write lie);
 * otherwise returns false and sets *actual_len to the (possibly truncated)
 * length to forward to the backend.  op names the operation for debug logging
 * so write_256 and write_aai produce distinct messages.
 */
static bool fault_apply_write_faults(struct fault_data *fd, const char *op,
				     unsigned int start, unsigned int len,
				     unsigned int *actual_len, int *ret)
{
	/* Write fail: return error */
	if (fd->write_fail_prob > 0.0 && rng_uniform(&fd->rng) < fd->write_fail_prob) {
		fd->writes_failed++;
		msg_pdbg("[fault] %s failed (injected) at 0x%x len %u\n", op, start, len);
		*ret = 1;
		return true;
	}

	/* Write lie: return success but do nothing */
	if (fd->write_lie_prob > 0.0 && rng_uniform(&fd->rng) < fd->write_lie_prob) {
		fd->writes_lied++;
		msg_pdbg("[fault] %s lied (no-op) at 0x%x len %u\n", op, start, len);
		*ret = 0;
		return true;
	}

	/* Partial write */
	*actual_len = len;
	if (fd->partial_write_prob > 0.0 && rng_uniform(&fd->rng) < fd->partial_write_prob) {
		*actual_len = (unsigned int)(len * rng_uniform(&fd->rng));
		if (*actual_len == 0)
			*actual_len = 1;
		fd->writes_partial++;
		msg_pdbg("[fault] partial %s: %u/%u bytes at 0x%x\n", op, *actual_len, len, start);
	}

	return false;
}

static int fault_spi_write_256(struct flashctx *flash, const uint8_t *buf,
			       unsigned int start, unsigned int len)
{
	struct fault_spi_master *fm = fault_get_spi_master(flash);
	struct fault_data *fd = fm->fault;
	struct spi_master saved;
	unsigned int actual_len;
	int ret;

	if (fault_apply_write_faults(fd, "write", start, len, &actual_len, &ret))
		return ret;

	fault_swap_in_orig(flash, &saved);
	ret = fm->orig_spi.write_256(flash, buf, start, actual_len);
	fault_swap_in_wrapper(flash, &saved);

	return ret;
}

static int fault_spi_write_aai(struct flashctx *flash, const uint8_t *buf,
			       unsigned int start, unsigned int len)
{
	struct fault_spi_master *fm = fault_get_spi_master(flash);
	struct fault_data *fd = fm->fault;
	struct spi_master saved;
	unsigned int actual_len;
	int ret;

	if (fault_apply_write_faults(fd, "write_aai", start, len, &actual_len, &ret))
		return ret;

	fault_swap_in_orig(flash, &saved);
	if (fm->orig_spi.write_aai)
		ret = fm->orig_spi.write_aai(flash, buf, start, actual_len);
	else
		ret = default_spi_write_aai(flash, buf, start, actual_len);
	fault_swap_in_wrapper(flash, &saved);

	return ret;
}

static bool fault_spi_probe_opcode(const struct flashctx *flash, uint8_t opcode)
{
	struct fault_spi_master *fm = fault_get_spi_master(flash);
	struct spi_master saved;

	fault_swap_in_orig((struct flashctx *)flash, &saved);
	bool ret;
	if (fm->orig_spi.probe_opcode)
		ret = fm->orig_spi.probe_opcode(flash, opcode);
	else
		ret = true; /* NULL probe_opcode implies all opcodes supported. */
	fault_swap_in_wrapper((struct flashctx *)flash, &saved);

	return ret;
}

static void fault_spi_delay(const struct flashctx *flash, unsigned int usecs)
{
	struct fault_spi_master *fm = fault_get_spi_master(flash);
	struct spi_master saved;

	fault_swap_in_orig((struct flashctx *)flash, &saved);
	if (fm->orig_spi.delay)
		fm->orig_spi.delay(flash, usecs);
	fault_swap_in_wrapper((struct flashctx *)flash, &saved);
}

static void fault_spi_get_region(const struct flashctx *flash,
				 unsigned int addr,
				 struct flash_region *region)
{
	struct fault_spi_master *fm = fault_get_spi_master(flash);

	if (!fm->orig_spi.get_region)
		return;

	struct spi_master saved;
	fault_swap_in_orig((struct flashctx *)flash, &saved);
	fm->orig_spi.get_region(flash, addr, region);
	fault_swap_in_wrapper((struct flashctx *)flash, &saved);
}

static int fault_shutdown(void *data)
{
	struct fault_data *fd = data;

	for (size_t i = 0; i < fd->wrapped_master_count; i++)
		registered_masters[fd->wrapped_masters[i].master_index].spi =
			fd->wrapped_masters[i].orig_spi;

	msg_ginfo("[fault] Fault injection summary:\n");
	msg_ginfo("[fault] Bit flips injected: %lu\n", fd->flips_injected);
	msg_ginfo("[fault] Reads shortened:    %lu\n", fd->reads_shortened);
	msg_ginfo("[fault] Writes failed:      %lu\n", fd->writes_failed);
	msg_ginfo("[fault] Writes lied:        %lu\n", fd->writes_lied);
	msg_ginfo("[fault] Writes partial:     %lu\n", fd->writes_partial);

	free(fd->wrapped_masters);
	free(fd);
	return 0;
}

static double extract_double_param_with_default(const struct programmer_cfg *cfg,
						const char *name, double def)
{
	char *val = extract_programmer_param_str(cfg, name);
	if (!val)
		return def;
	double r = strtod(val, NULL);
	free(val);
	return r;
}

static uint64_t extract_uint64_param_with_default(const struct programmer_cfg *cfg,
						   const char *name, uint64_t def)
{
	char *val = extract_programmer_param_str(cfg, name);
	if (!val)
		return def;
	uint64_t r = strtoull(val, NULL, 0);
	free(val);
	return r;
}

static int fault_init(const struct programmer_cfg *cfg)
{
	int ret;

	/* Extract fault parameters before forwarding to backend. */
	char *backend_name = extract_programmer_param_str(cfg, "backend");
	if (!backend_name) {
		msg_perr("[fault] Error: 'backend' parameter is required.\n"
			 "        Usage: -p fault:backend=dummy[,bus=spi,emulate=W25Q128FV],seed=42\n");
		return 1;
	}

	uint64_t seed = extract_uint64_param_with_default(cfg, "seed", 0);
	double flip_prob = extract_double_param_with_default(cfg, "flip_prob", 0.0);
	double short_read_prob = extract_double_param_with_default(cfg, "short_read_prob", 0.0);
	double min_read_ratio = extract_double_param_with_default(cfg, "min_read_ratio", 0.5);
	if (min_read_ratio < 0.0)
		min_read_ratio = 0.0;
	if (min_read_ratio > 1.0)
		min_read_ratio = 1.0;
	double write_fail_prob = extract_double_param_with_default(cfg, "write_fail_prob", 0.0);
	double write_lie_prob = extract_double_param_with_default(cfg, "write_lie_prob", 0.0);
	double partial_write_prob = extract_double_param_with_default(cfg, "partial_write_prob", 0.0);

	/* Find and initialize the backend programmer. */
	const struct programmer_entry *backend = NULL;
	for (size_t i = 0; i < programmer_table_size; i++) {
		if (!strcmp(programmer_table[i]->name, backend_name)) {
			backend = programmer_table[i];
			break;
		}
	}

	if (!backend) {
		msg_perr("[fault] Error: Unknown backend programmer '%s'.\n", backend_name);
		free(backend_name);
		return 1;
	}
	if (backend == &programmer_fault) {
		msg_perr("[fault] Error: backend programmer cannot be 'fault'.\n");
		free(backend_name);
		return 1;
	}

	msg_ginfo("[fault] Wrapping backend: %s\n", backend_name);
	msg_ginfo("[fault] Seed: %" PRIu64 "\n", seed);
	if (flip_prob > 0.0)
		msg_ginfo("[fault] flip_prob: %g\n", flip_prob);
	if (short_read_prob > 0.0)
		msg_ginfo("[fault] short_read_prob: %g (min_ratio: %g)\n",
			  short_read_prob, min_read_ratio);
	if (write_fail_prob > 0.0)
		msg_ginfo("[fault] write_fail_prob: %g\n", write_fail_prob);
	if (write_lie_prob > 0.0)
		msg_ginfo("[fault] write_lie_prob: %g\n", write_lie_prob);
	if (partial_write_prob > 0.0)
		msg_ginfo("[fault] partial_write_prob: %g\n", partial_write_prob);

	free(backend_name);

	/* Run the backend through the normal init path so flashrom sees the
	 * wrapped backend as the active programmer for safety checks and
	 * user guidance. The remaining parameters in cfg->params belong to
	 * the backend after the fault-specific keys were extracted above. */
	ret = programmer_init(backend, cfg->params);
	if (cfg->params)
		cfg->params[0] = '\0';
	if (ret) {
		msg_perr("[fault] Backend initialization failed.\n");
		return ret;
	}

	/* Wrap the registered masters. */
	struct fault_data *fd = calloc(1, sizeof(*fd));
	if (!fd) {
		msg_perr("[fault] Memory allocation failed.\n");
		return 1;
	}

	rng_seed(&fd->rng, seed);
	fd->flip_prob = flip_prob;
	fd->short_read_prob = short_read_prob;
	fd->min_read_ratio = min_read_ratio;
	fd->write_fail_prob = write_fail_prob;
	fd->write_lie_prob = write_lie_prob;
	fd->partial_write_prob = partial_write_prob;
	fd->wrapped_masters = calloc(registered_master_count, sizeof(*fd->wrapped_masters));
	if (!fd->wrapped_masters) {
		msg_perr("[fault] Memory allocation failed.\n");
		free(fd);
		return 1;
	}

	/* Find and wrap SPI masters.  We save the entire original
	 * spi_master struct and replace ALL callbacks that receive
	 * a flashctx (and thus dereference spi.data).  Callbacks
	 * that don't receive flashctx (map/unmap) are left as-is. */
	int wrapped = 0;
	for (int i = 0; i < registered_master_count; i++) {
		if (registered_masters[i].buses_supported & BUS_SPI) {
			struct fault_spi_master *fm = &fd->wrapped_masters[wrapped];

			/* Save the original master in its entirety. */
			fm->fault = fd;
			fm->master_index = i;
			fm->orig_spi = registered_masters[i].spi;

			/* Replace every callback that uses spi.data. */
			registered_masters[i].spi.command = fault_spi_send_command;
			registered_masters[i].spi.multicommand = fault_spi_multicommand;
			registered_masters[i].spi.read = fault_spi_read;
			registered_masters[i].spi.write_256 = fault_spi_write_256;
			registered_masters[i].spi.write_aai = fault_spi_write_aai;
			registered_masters[i].spi.probe_opcode = fault_spi_probe_opcode;
			registered_masters[i].spi.delay = fault_spi_delay;
			if (fm->orig_spi.get_region)
				registered_masters[i].spi.get_region = fault_spi_get_region;
			/* map/unmap don't receive flashctx, keep originals. */
			registered_masters[i].spi.data = fm;

			wrapped++;
			msg_ginfo("[fault] Wrapped SPI master #%d\n", i);
		}
	}

	if (!wrapped) {
		msg_perr("[fault] No SPI master found to wrap. "
			 "Ensure the backend registers a SPI bus.\n");
		free(fd->wrapped_masters);
		free(fd);
		return 1;
	}
	fd->wrapped_master_count = wrapped;

	if (register_shutdown(fault_shutdown, fd)) {
		free(fd->wrapped_masters);
		free(fd);
		return 1;
	}

	return 0;
}

const struct programmer_entry programmer_fault = {
	.name		= "fault",
	.type		= OTHER,
	.devs.note	= "Fault injection wrapper for testing\n",
	.init		= fault_init,
};
