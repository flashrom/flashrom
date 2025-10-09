/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __PARALLEL_H__
#define __PARALLEL_H__ 1

#include "flash.h"

void chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr);
void chip_writew(const struct flashctx *flash, uint16_t val, chipaddr addr);
void chip_writel(const struct flashctx *flash, uint32_t val, chipaddr addr);
void chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len);
uint8_t chip_readb(const struct flashctx *flash, const chipaddr addr);
uint16_t chip_readw(const struct flashctx *flash, const chipaddr addr);
uint32_t chip_readl(const struct flashctx *flash, const chipaddr addr);
void chip_readn(const struct flashctx *flash, uint8_t *buf, const chipaddr addr, size_t len);

#endif /* !__PARALLEL_H__ */