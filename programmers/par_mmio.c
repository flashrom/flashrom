/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "par_mmio.h"
#include "flash.h"
#include "programmer.h"
#include "hwaccess_physmap.h"

/*
 * par.data points at the driver's own data struct, not directly at a
 * struct par_mmio_data. The cast below is only valid
 * because par_mmio_data is embedded as the first member of every driver data
 * struct that uses these accessors, so the addresses coincide.
 */
void par_mmio_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	const struct par_mmio_data *data = flash->mst->par.data;

	pci_mmio_writeb(val, data->bar + (addr & data->mask));
}

uint8_t par_mmio_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	const struct par_mmio_data *data = flash->mst->par.data;

	return pci_mmio_readb(data->bar + (addr & data->mask));
}
