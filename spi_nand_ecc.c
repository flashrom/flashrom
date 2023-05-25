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
