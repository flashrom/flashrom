/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2000 Ronald G. Minnich <rminnich@gmail.com>
 * Copyright (C) 2005-2007 coresystems GmbH <stepan@coresystems.de>
 * Copyright (C) 2006-2009 Carl-Daniel Hailfinger
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

#ifndef __FLASHCHIPS_H__
#define __FLASHCHIPS_H__ 1

/*
 * Please keep this list sorted alphabetically by manufacturer. The first
 * entry of each section should be the manufacturer ID, followed by the
 * list of devices from that manufacturer (sorted by device IDs).
 *
 * All LPC/FWH parts (parallel flash) have 8-bit device IDs if there is no
 * continuation code.
 * SPI parts have 16-bit device IDs if they support RDID.
 */

#define GENERIC_DEVICE_ID	0xffff	/* Only match the vendor ID */

#define ALLIANCE_ID		0x52	/* Alliance Semiconductor */

#define AMD_ID			0x01	/* AMD */
#define AM_29F010B		0x20	/* Same as Am29F010A */
#define AM_29F002BT		0xB0
#define AM_29F002BB		0x34
#define AM_29F040B		0xA4
#define AM_29F080B		0xD5
#define AM_29LV040B		0x4F
#define AM_29LV081B		0x38
#define AM_29F016D		0xAD

#define AMIC_ID			0x7F37	/* AMIC */
#define AMIC_ID_NOPREFIX	0x37	/* AMIC */
#define AMIC_A25L40P		0x2013
#define AMIC_A29002B		0x0d
#define AMIC_A29002T		0x8c
#define AMIC_A29040B		0x86
#define AMIC_A49LF040A		0x9d

/* This chip vendor/device ID is probably a misinterpreted LHA header. */
#define ASD_ID			0x25	/* ASD, not listed in JEP106W */
#define ASD_AE49F2008		0x52

#define ATMEL_ID		0x1F	/* Atmel */
#define AT_25DF021		0x4300
#define AT_25DF041A		0x4401
#define AT_25DF081		0x4502
#define AT_25DF161		0x4602
#define AT_25DF321		0x4700	/* also 26DF321 */
#define AT_25DF321A		0x4701
#define AT_25DF641		0x4800
#define AT_25F512A		0x65 /* Needs special RDID. AT25F512A_RDID 15 1d */
#define AT_25F512B		0x6500
#define AT_25FS010		0x6601
#define AT_25FS040		0x6604
#define AT_26DF041		0x4400
#define AT_26DF081		0x4500	/* guessed, no datasheet available */
#define AT_26DF081A		0x4501
#define AT_26DF161		0x4600
#define AT_26DF161A		0x4601
#define AT_26DF321		0x4700	/* also 25DF321 */
#define AT_26F004		0x0400
#define AT_29C040A		0xA4
#define AT_29C010A		0xD5	
#define AT_29C020		0xDA
#define AT_29C512		0x5D	
#define AT_45BR3214B		/* No ID available */
#define AT_45CS1282		0x2920
#define AT_45D011		/* No ID available */
#define AT_45D021A		/* No ID available */
#define AT_45D041A		/* No ID available */
#define AT_45D081A		/* No ID available */
#define AT_45D161		/* No ID available */
#define AT_45DB011		/* No ID available */
#define AT_45DB011B		/* No ID available */
#define AT_45DB011D		0x2200
#define AT_45DB021A		/* No ID available */
#define AT_45DB021B		/* No ID available */
#define AT_45DB021D		0x2300
#define AT_45DB041A		/* No ID available */
#define AT_45DB041D		0x2400
#define AT_45DB081A		/* No ID available */
#define AT_45DB081D		0x2500
#define AT_45DB161		/* No ID available */
#define AT_45DB161B		/* No ID available */
#define AT_45DB161D		0x2600
#define AT_45DB321		/* No ID available */
#define AT_45DB321B		/* No ID available */
#define AT_45DB321C		0x2700
#define AT_45DB321D		0x2701 /* Buggy data sheet */
#define AT_45DB642		/* No ID available */
#define AT_45DB642D		0x2800
#define AT_49BV512		0x03
#define AT_49F002N		0x07	/* for AT49F002(N)  */
#define AT_49F002NT		0x08	/* for AT49F002(N)T */

#define CATALYST_ID		0x31	/* Catalyst */

#define EMST_ID			0x8C	/* EMST / EFST Elite Flash Storage */
#define EMST_F49B002UA		0x00

/*
 * EN25 chips are SPI, first byte of device ID is memory type,
 * second byte of device ID is log(bitsize)-9.
 * Vendor and device ID of EN29 series are both prefixed with 0x7F, which
 * is the continuation code for IDs in bank 2.
 * Vendor ID of EN25 series is NOT prefixed with 0x7F, this results in
 * a collision with Mitsubishi. Mitsubishi once manufactured flash chips.
 * Let's hope they are not manufacturing SPI flash chips as well.
 */
#define EON_ID			0x7F1C	/* EON Silicon Devices */
#define EON_ID_NOPREFIX		0x1C	/* EON, missing 0x7F prefix */
#define EN_25B05		0x2010	/* also P05, 2^19 kbit or 2^16 kByte */
#define EN_25B10		0x2011	/* also P10 */
#define EN_25B20		0x2012	/* also P20 */
#define EN_25B40		0x2013	/* also P40 */
#define EN_25B80		0x2014	/* also P80 */
#define EN_25B16		0x2015	/* also P16 */
#define EN_25B32		0x2016	/* also P32 */
#define EN_25B64		0x2017	/* also P64 */
#define EN_25D16		0x3015
#define EN_25F05		0x3110
#define EN_25F10		0x3111
#define EN_25F20		0x3112
#define EN_25F40		0x3113
#define EN_25F80		0x3114
#define EN_25F16		0x3115
#define EN_25F32		0x3116
#define EN_29F512		0x7F21
#define EN_29F010		0x7F20
#define EN_29F040A		0x7F04
#define EN_29LV010		0x7F6E
#define EN_29LV040A		0x7F4F	/* EN_29LV040(A) */
#define EN_29F002T		0x7F92	/* Also EN29F002A */
#define EN_29F002B		0x7F97	/* Also EN29F002AN */

#define FUJITSU_ID		0x04	/* Fujitsu */
#define MBM29F400BC		0xAB
#define MBM29F400TC		0x23
#define MBM29F004BC		0x7B
#define MBM29F004TC		0x77

#define HYUNDAI_ID		0xAD	/* Hyundai */

#define IMT_ID			0x7F1F	/* Integrated Memory Technologies */
#define IM_29F004B		0xAE
#define IM_29F004T		0xAF

#define INTEL_ID		0x89	/* Intel */
#define I_82802AB			0xAD
#define I_82802AC			0xAC
#define P28F001BXT		0x94	/* 28F001BX-T */
#define P28F001BXB		0x95	/* 28F001BX-B */

#define ISSI_ID			0xD5	/* ISSI Integrated Silicon Solutions */

/*
 * MX25 chips are SPI, first byte of device ID is memory type,
 * second byte of device ID is log(bitsize)-9.
 * Generalplus SPI chips seem to be compatible with Macronix
 * and use the same set of IDs.
 */
#define MX_ID			0xC2	/* Macronix (MX) */
#define MX_25L512		0x2010	/* 2^19 kbit or 2^16 kByte */
#define MX_25L1005		0x2011
#define MX_25L2005		0x2012
#define MX_25L4005		0x2013	/* MX25L4005{,A} */
#define MX_25L8005		0x2014
#define MX_25L1605		0x2015	/* MX25L1605{,A,D} */
#define MX_25L3205		0x2016	/* MX25L3205{,A} */
#define MX_25L6405		0x2017	/* MX25L3205{,D} */
#define MX_25L12805		0x2018	/* MX25L12805 */
#define MX_25L1635D		0x2415
#define MX_25L3235D		0x5E16	/* MX25L3225D/MX25L3235D/MX25L3237D */
#define MX_29F002B		0x34
#define MX_29F002T		0xB0
#define MX_29LV002CB		0x5A
#define MX_29LV002CT		0x59
#define MX_29LV004CB		0xB6
#define MX_29LV004CT		0xB5
#define MX_29LV008CB		0x37
#define MX_29LV008CT		0x3E
#define MX_29F040C		0xA4
#define MX_29F200CB		0x57
#define MX_29F200CT		0x51
#define MX_29F400CB		0xAB
#define MX_29F400CT		0x23
#define MX_29LV040C		0x4F
#define MX_29LV128DB		0x7A
#define MX_29LV128DT		0x7E
#define MX_29LV160DB		0x49	/* Same as MX29LV161DB/MX29LV160CB */
#define MX_29LV160DT		0xC4	/* Same as MX29LV161DT/MX29LV160CT */
#define MX_29LV320DB		0xA8	/* Same as MX29LV321DB */
#define MX_29LV320DT		0xA7	/* Same as MX29LV321DT */
#define MX_29LV400CB		0xBA
#define MX_29LV400CT		0xB9
#define MX_29LV800CB		0x5B
#define MX_29LV800CT		0xDA
#define MX_29LV640DB		0xCB	/* Same as MX29LV640EB */
#define MX_29LV640DT		0xC9	/* Same as MX29LV640ET */
#define MX_29SL402CB		0xF1
#define MX_29SL402CT		0x70
#define MX_29SL800CB		0x6B	/* Same as MX29SL802CB */
#define MX_29SL800CT		0xEA	/* Same as MX29SL802CT */

/*
 * Programmable Micro Corp is listed in JEP106W in bank 2, so it should
 * have a 0x7F continuation code prefix.
 */
#define PMC_ID			0x7F9D	/* PMC */
#define PMC_ID_NOPREFIX		0x9D	/* PMC, missing 0x7F prefix */
#define PMC_25LV512		0x7B
#define PMC_25LV010		0x7C
#define PMC_25LV020		0x7D
#define PMC_25LV040		0x7E
#define PMC_25LV080B		0x13
#define PMC_25LV016B		0x14
#define PMC_29F002T		0x1D
#define PMC_29F002B		0x2D
#define PMC_39LV512		0x1B
#define PMC_39F010		0x1C	/* also Pm39LV010 */
#define PMC_39LV020		0x3D
#define PMC_39LV040		0x3E
#define PMC_39F020		0x4D
#define PMC_39F040		0x4E
#define PMC_49FL002		0x6D
#define PMC_49FL004		0x6E

#define SHARP_ID		0xB0	/* Sharp */
#define SHARP_LHF00L04		0xCF

/*
 * Spansion was previously a joint venture of AMD and Fujitsu.
 * S25 chips are SPI. The first device ID byte is memory type and
 * the second device ID byte is memory capacity.
 */
#define SPANSION_ID		0x01	/* Spansion */
#define SPANSION_S25FL016A	0x0214

/*
 * SST25 chips are SPI, first byte of device ID is memory type, second
 * byte of device ID is related to log(bitsize) at least for some chips.
 */
#define SST_ID			0xBF	/* SST */
#define SST_25WF512		0x2501
#define SST_25WF010		0x2502
#define SST_25WF020		0x2503
#define SST_25WF040		0x2504
#define SST_25VF512A_REMS	0x48	/* REMS or RES opcode */
#define SST_25VF010_REMS	0x49	/* REMS or RES opcode */
#define SST_25VF020_REMS	0x43	/* REMS or RES opcode */
#define SST_25VF040_REMS	0x44	/* REMS or RES opcode */
#define SST_25VF040B		0x258D
#define SST_25VF040B_REMS	0x8D	/* REMS or RES opcode */
#define SST_25VF080_REMS	0x80	/* REMS or RES opcode */
#define SST_25VF080B		0x258E
#define SST_25VF080B_REMS	0x8E	/* REMS or RES opcode */
#define SST_25VF016B		0x2541
#define SST_25VF032B		0x254A
#define SST_25VF032B_REMS	0x4A	/* REMS or RES opcode */
#define SST_26VF016		0x2601
#define SST_26VF032		0x2602
#define SST_27SF512		0xA4
#define SST_27SF010		0xA5
#define SST_27SF020		0xA6
#define SST_27VF010		0xA9
#define SST_27VF020		0xAA
#define SST_28SF040		0x04
#define SST_29EE512		0x5D
#define SST_29EE010		0x07
#define SST_29LE010		0x08	/* also SST29VE010 */
#define SST_29EE020A		0x10	/* also SST29EE020 */
#define SST_29LE020		0x12	/* also SST29VE020 */
#define SST_29SF020		0x24
#define SST_29VF020		0x25
#define SST_29SF040		0x13
#define SST_29VF040		0x14
#define SST_39SF010		0xB5
#define SST_39SF020		0xB6
#define SST_39SF040		0xB7
#define SST_39VF512		0xD4
#define SST_39VF010		0xD5
#define SST_39VF020		0xD6
#define SST_39VF040		0xD7
#define SST_39VF080		0xD8
#define SST_49LF040B		0x50
#define SST_49LF040		0x51
#define SST_49LF020		0x61
#define SST_49LF020A		0x52
#define SST_49LF080A		0x5B
#define SST_49LF002A		0x57
#define SST_49LF003A		0x1B
#define SST_49LF004A		0x60
#define SST_49LF008A		0x5A
#define SST_49LF004C		0x54
#define SST_49LF008C		0x59
#define SST_49LF016C		0x5C
#define SST_49LF160C		0x4C

/*
 * ST25P chips are SPI, first byte of device ID is memory type, second
 * byte of device ID is related to log(bitsize) at least for some chips.
 */
#define ST_ID			0x20	/* ST / SGS/Thomson */
#define ST_M25P05A		0x2010
#define ST_M25P10A		0x2011
#define ST_M25P20		0x2012
#define ST_M25P40		0x2013
#define ST_M25P40_RES		0x12
#define ST_M25P80		0x2014
#define ST_M25P16		0x2015
#define ST_M25P32		0x2016
#define ST_M25P64		0x2017
#define ST_M25P128		0x2018
#define ST_M25PE10		0x8011
#define ST_M25PE20		0x8012
#define ST_M25PE40		0x8013
#define ST_M25PE80		0x8014
#define ST_M25PE16		0x8015
#define ST_M50FLW040A		0x08
#define ST_M50FLW040B		0x28
#define ST_M50FLW080A		0x80
#define ST_M50FLW080B		0x81
#define ST_M50FW002		0x29
#define ST_M50FW040		0x2C
#define ST_M50FW080		0x2D
#define ST_M50FW016		0x2E
#define ST_M50LPW116		0x30
#define ST_M29F002B		0x34
#define ST_M29F002T		0xB0	/* M29F002T / M29F002NT */
#define ST_M29F400BT		0xD5
#define ST_M29F040B		0xE2
#define ST_M29W010B		0x23
#define ST_M29W040B		0xE3

#define SYNCMOS_ID		0x40	/* SyncMOS and Mosel Vitelic */
#define S29C51001T		0x01
#define S29C51002T		0x02
#define S29C51004T		0x03
#define S29C31004T		0x63

#define TI_ID			0x97	/* Texas Instruments */
#define TI_OLD_ID		0x01	/* TI chips from last century */
#define TI_TMS29F002RT		0xB0
#define TI_TMS29F002RB		0x34

/*
 * W25X chips are SPI, first byte of device ID is memory type, second
 * byte of device ID is related to log(bitsize).
 */
#define WINBOND_ID		0xDA	/* Winbond */
#define WINBOND_NEX_ID		0xEF	/* Winbond (ex Nexcom) serial flashes */
#define W_25X10			0x3011
#define W_25X20			0x3012
#define W_25X40			0x3013
#define W_25X80			0x3014
#define W_25X16			0x3015
#define W_25X32			0x3016
#define W_25X64			0x3017
#define W_29C011		0xC1
#define W_29C020C		0x45
#define W_29C040P		0x46
#define W_29EE011		0xC1
#define W_39V040FA		0x34
#define W_39V040A		0x3D
#define W_39V040B		0x54
#define W_39V040C		0x50
#define W_39V080A		0xD0
#define W_39V080FA		0xD3
#define W_39V080FA_DM		0x93
#define W_49F002U		0x0B
#define W_49V002A		0xB0
#define W_49V002FA		0x32

#endif /* !FLASHCHIPS_H */
