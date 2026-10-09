/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2015 Paul Kocialkowski <contact@paulk.fr>
 */

#ifndef __ENE_H__
#define __ENE_H__ 1

#define ENE_XBI_EFA0			0xfea8
#define ENE_XBI_EFA1			0xfea9
#define ENE_XBI_EFA2			0xfeaa
#define ENE_XBI_EFDAT			0xfeab
#define ENE_XBI_EFCMD			0xfeac
#define ENE_XBI_EFCFG			0xfead

#define ENE_XBI_EFCFG_CMD_WE		BIT(3)
#define ENE_XBI_EFCFG_BUSY		BIT(1)

#define ENE_XBI_EFCMD_HVPL_LATCH	0x02
#define ENE_XBI_EFCMD_READ		0x03
#define ENE_XBI_EFCMD_ERASE		0x20
#define ENE_XBI_EFCMD_PROGRAM		0x70
#define ENE_XBI_EFCMD_HVPL_CLEAR	0x80

/*
 * Watchdog timer configuration register. The address is part of the ENE
 * KB9010 register map used by OLPC's Open Firmware (cpu/arm/olpc/edi.fth),
 * which shares its EDI ID and flash and 8051 registers with the KB9012.
 */
#define ENE_EC_WDTCFG			0xfe80

/*
 * Writing this value to WDTCFG disables the watchdog timer on the KB9542GF.
 * It was found by testing on hardware. Open Firmware disables the KB9010
 * watchdog differently, by writing 0x00 to WDTCFG.
 */
#define ENE_EC_WDTCFG_DISABLE		0x48

#define ENE_EC_PXCFG			0xff14

#define ENE_EC_PXCFG_8051_RESET		0x01

#define ENE_EC_HWVERSION		0xff00
#define ENE_EC_EDIID			0xff24

#define ENE_KB9012_HWVERSION		0xc3
#define ENE_KB9012_EDIID		0x04

#define ENE_KB9542GF_HWVERSION		0xf0
#define ENE_KB9542GF_EDIID		0x07

struct ene_chip {
	unsigned char hwversion;
	unsigned char ediid;
};

#endif
