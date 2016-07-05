/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2016 Hatim Kanchwala <hatim@hatimak.me>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "chipdrivers.h"
#include "flash.h"
#include "spi25_statusreg.h"

/*
 * struct status_register {
 *	enum status_register_bit layout[MAX_STATUS_REGISTERS + 1][8];
 *	uint8_t (*read) (struct flashctx *flash, enum status_register_num SRn);
 *	int (*write) (struct flashctx *flash, enum status_register_num SRn, uint8_t status);
 *	int (*print) (struct flashctx *flash, enum status_register_num SRn);
 *	enum wp_mode (*get_wp_mode) (struct flashctx *flash);
 *	int (*set_wp_mode) (struct flashctx *flash, enum wp_mode wp_mode);
 *	int (*print_wp_mode) (struct flashctx *flash);
 * };
 */

/* === Single status register === */
/* === AMIC === */
/* A25L080 */
struct status_register a25l080_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, RESV, RESV, SRP0 },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};
/* === Macronix === */
/* MX25L6408E, MX25L6406E */
struct status_register mx25l64xe_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, RESV, SRP0 },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* MX25L1605D, MX25L3205D, MX25L6405D, MX25L1608D, MX25L3208D,
 * MX25L6408D */
// TODO(hatim): Add support for MX25L6408D
struct status_register mx25lx5d_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, CP, SRP0 },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* MX25L6436E, MX25L6445E, MX25L6465E, MX25L12865E, MX25L12845E,
 * MX25L12835F, MX25L1673E
 * FIXME: MX25L12845E, MX25L12835F (These two chips have
 * a configuration register that behaves like a 2nd status
 * register.)
 */
struct status_register mx25lx65e_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, QE, SRP0 },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* === Double status registers === */
/* === AMIC === */
/* A25LQ16, A25LQ32A */
struct status_register a25lq16_32a_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, TB, SEC, SRP0 },
		{ SRP1, QE, APT, RESV, RESV, RESV, CMP, SUS },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* A25L032 */
struct status_register a25l032_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, TB, SEC, SRP0 },
		{ SRP1, RESV, APT, RESV, RESV, RESV, CMP, RESV },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* === GigaDevice === */
/* GD25LQ16, GD25LQ40, GD25LQ80B, GD25LQ40B, GD25LQ64C, GD25LQ80,
 * GD25LQ128C, GD25LQ32C */
// TODO(hatim): Add support for GD25LQ32C, GD25LQ80B, GD25LQ40B, GD25LQ64C, GD25LQ32C
struct status_register gd25lq_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ SRP1, QE, SUS2, LB1, LB2, LB3, CMP, SUS1 },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* GD25Q16B, GD25Q32B, GD25Q64B */
struct status_register gd25q16_32_64b_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ RESV, QE, LB1, RESV, RESV, RESV, CMP, SUS },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* GD25Q10, GD25Q16, GD25Q20, GD25Q40, GD25Q80 */
struct status_register gd25q10_20_40_80_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ SRP1, QE, RESV, RESV, RESV, RESV, RESV, RESV },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* GD25VQ16C, GD25VQ80C, GD25Q16C, GD25Q40C */
struct status_register gd25vq16_80c_q16_40c_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ SRP1, QE, LB1, RESV, RESV, HPF, CMP, SUS },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* GD25VQ21B, GD25VQ41B, GD25Q21B, GD25Q41B */
struct status_register gd25vq21_41b_q21_q41b_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ SRP1, QE, HPF, LB1, LB2, LB3, CMP, SUS },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* GD25Q80B, GD25Q128 */
struct status_register gd25q80b_128_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ SRP1, QE, LB1, RESV, RESV, RESV, CMP, SUS },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* === Winbond === */
/* W25Q80, W25Q16, W25Q32 */
struct status_register w25q80_16_32_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, TB, SEC, SRP0 },
		{ SRP1, QE, RESV, RESV, RESV, RESV, RESV, RESV },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* W25Q40BL, W25Q64FV */
struct status_register w25q40bl_64fv_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, TB, SEC, SRP0 },
		{ SRP1, QE, RESV, LB1, LB2, LB3, CMP, SUS },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* === Triple status registers === */
/* === GigaDevice === */
/* GD25LQ05B, GD25LQ10B, GD25LQ20B */
struct status_register gd25lq05_10_20b_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ SRP1, QE, SUS2, LB1, LB2, LB3, CMP, SUS1 },
		{ RESV, RESV, RESV, RESV, HPF, RESV, RESV, RESV },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* GD25Q32C, GD25Q64C */
struct status_register gd25q32_64c_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ SRP1, QE, SUS2, LB1, LB2, LB3, CMP, SUS1 },
		{ RESV, RESV, RESV, RESV, HPF, DRV0, DRV1, RESV },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* GD25Q127C */
struct status_register gd25q127c_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ SRP1, QE, SUS2, LB1, LB2, LB3, CMP, SUS1 },
		{ RESV, RESV, WPS, RESV, RESV, DRV0, DRV1, RST },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* GD25Q128C */
struct status_register gd25q128c_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, BP3, BP4, SRP0 },
		{ SRP1, QE, SUS2, LB1, LB2, LB3, CMP, SUS1 },
		{ RESV, RESV, WPS, RESV, RESV, DRV0, DRV1, RST },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};

/* === Winbond === */
/* W25Q128FW */
struct status_register w25q128fw_sr = {
	.layout = {
		{ WIP, WEL, BP0, BP1, BP2, TB, SEC, SRP0 },
		{ SRP1, QE, RESV, LB1, LB2, LB3, CMP, SUS },
		{ RESV, RESV, WPS, RESV, RESV, DRV0, DRV1, RST },
	},
	.read		= &spi_read_status_register_generic,
	.write		= &spi_write_status_register_generic,
	.print		= &spi_prettyprint_status_register_generic,
	.print_wp_mode	= &spi_prettyprint_status_register_wp_generic,
	.get_wp_mode	= &get_wp_mode_generic,
	.set_wp_mode	= &set_wp_mode_generic,
};
