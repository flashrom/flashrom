/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2022 Yangfl
 * Copyright (C) 2023 froloff
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

/*
 * Contains the SPI NAND ECC functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "chipdrivers.h"
#include "bch.h"

/***********************************************
 *                                             *
 ***********************************************/

#define BCH_T      8
#define BCH_M      14

#define NAND_PAGE_SZ 2112
#define NAND_DATA_SZ 2048
#define NAND_ECC_SZ    14


int spi_nand_ecc_encode(void *ecc_ctx, uint8_t *page_buffer, uint8_t mode)
{
	// Not implemented yet
	return 0;
}

int spi_nand_ecc_decode(void *ecc_ctx, uint8_t *page_buffer, uint8_t mode)
{
	// Not implemented yet
	return 0;
}

void *spi_nand_ecc_init(uint8_t mode)
{
	if (mode > SPI_NAND_SW_ECC2) return NULL;

	return init_bch(BCH_M, BCH_T, 0);
}

void spi_nand_ecc_done(void *ecc_ctx)
{
	if (ecc_ctx) free_bch((struct bch_control *)ecc_ctx);
}
