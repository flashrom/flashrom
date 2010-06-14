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

#define GENERIC_MANUF_ID	0xffff	/* Check if there is a vendor ID */
#define GENERIC_DEVICE_ID	0xffff	/* Only match the vendor ID */

#define ALLIANCE_ID		0x52	/* Alliance Semiconductor */

#define AMD_ID			0x01	/* AMD */
#define AM_29DL400BT		0x0C
#define AM_29DL400BB		0x0F
#define AM_29DL800BT		0x4A
#define AM_29DL800BB		0xCB
#define AM_29F002BB		0x34	/* Same as Am29F002NBB */
#define AM_29F002BT		0xB0	/* Same as Am29F002NBT */
#define AM_29F004BB		0x7B
#define AM_29F004BT		0x77
#define AM_29F016D		0xAD
#define AM_29F010B		0x20	/* Same as Am29F010A */
#define AM_29F040B		0xA4
#define AM_29F080B		0xD5
#define AM_29F200BB		0x57
#define AM_29F200BT		0x51
#define AM_29F400BB		0xAB
#define AM_29F400BT		0x23
#define AM_29F800BB		0x58
#define AM_29F800BT		0xD6
#define AM_29LV002BB		0xC2
#define AM_29LV002BT		0x40
#define AM_29LV004BB		0xB6
#define AM_29LV004BT		0xB5
#define AM_29LV008BB		0x37
#define AM_29LV008BT		0x3E
#define AM_29LV040B		0x4F
#define AM_29LV080B		0x38	/* Same as Am29LV081B */
#define AM_29LV200BB		0xBF
#define AM_29LV200BT		0x3B
#define AM_29LV800BB		0x5B	/* Same as Am29LV800DB */
#define AM_29LV400BT		0xB9
#define AM_29LV400BB		0xBA
#define AM_29LV800BT		0xDA	/* Same as Am29LV800DT */

#define AMIC_ID			0x7F37	/* AMIC */
#define AMIC_ID_NOPREFIX	0x37	/* AMIC */
#define AMIC_A25L40P		0x2013
#define AMIC_A29002B		0x0d
#define AMIC_A29002T		0x8C	/* Same as A290021T */
#define AMIC_A29040B		0x86
#define AMIC_A29400T		0xB0	/* Same as 294001T */
#define AMIC_A29400U		0x31	/* Same as A294001U */
#define AMIC_A29800T		0x0E
#define AMIC_A29800U		0x8F
#define AMIC_A29L004T		0x34	/* Same as A29L400T */
#define AMIC_A29L004U		0xB5	/* Same as A29L400U */
#define AMIC_A29L008T		0x1A	/* Same as A29L800T */
#define AMIC_A29L008U		0x9B	/* Same as A29L800U */
#define AMIC_A29L040		0x92
#define AMIC_A49LF040A		0x9d

/* This chip vendor/device ID is probably a misinterpreted LHA header. */
#define ASD_ID			0x25	/* ASD, not listed in JEP106W */
#define ASD_AE49F2008		0x52

#define ATMEL_ID		0x1F	/* Atmel */
#define AT_25DF021		0x4300
#define AT_25DF041A		0x4401
#define AT_25DF081		0x4502
#define AT_25DF161		0x4602
#define AT_25DF321		0x4700	/* Same as 26DF321 */
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
#define AT_26DF321		0x4700	/* Same as 25DF321 */
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
#define AT_49F020		0x0B
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
#define EN_25B05		0x2010	/* Same as P05, 2^19 kbit or 2^16 kByte */
#define EN_25B05T		0x25
#define EN_25B05B		0x95
#define EN_25B10		0x2011	/* Same as P10 */
#define EN_25B10T		0x40
#define EN_25B10B		0x30
#define EN_25B20		0x2012	/* Same as P20 */
#define EN_25B20T		0x41
#define EN_25B20B		0x31
#define EN_25B40		0x2013	/* Same as P40 */
#define EN_25B40T		0x42
#define EN_25B40B		0x32
#define EN_25B80		0x2014	/* Same as P80 */
#define EN_25B80T		0x43
#define EN_25B80B		0x33
#define EN_25B16		0x2015	/* Same as P16 */
#define EN_25B16T		0x44
#define EN_25B16B		0x34
#define EN_25B32		0x2016	/* Same as P32 */
#define EN_25B32T		0x45
#define EN_25B32B		0x35
#define EN_25B64		0x2017	/* Same as P64 */
#define EN_25B64T		0x46
#define EN_25B64B		0x36
#define EN_25D16		0x3015
#define EN_25F05		0x3110
#define EN_25F10		0x3111
#define EN_25F20		0x3112
#define EN_25F40		0x3113
#define EN_25F80		0x3114
#define EN_25F16		0x3115
#define EN_25F32		0x3116
#define EN_29F512		0x7F21
#define EN_29F010		0x20
#define EN_29F040A		0x7F04
#define EN_29LV010		0x7F6E
#define EN_29LV040A		0x7F4F	/* EN_29LV040(A) */
#define EN_29F002T		0x7F92	/* Same as EN29F002A */
#define EN_29F002B		0x7F97	/* Same as EN29F002AN */

#define FUJITSU_ID		0x04	/* Fujitsu */
#define MBM29DL400BC		0x0F
#define MBM29DL400TC		0x0C
#define MBM29DL800BA		0xCB
#define MBM29DL800TA		0x4A
#define MBM29F002BC		0x34
#define MBM29F002TC		0xB0
#define MBM29F004BC		0x7B
#define MBM29F004TC		0x77
#define MBM29F040C		0xA4
#define MBM29F080A		0xD5
#define MBM29F200BC		0x57
#define MBM29F200TC		0x51
#define MBM29F400BC		0xAB
#define MBM29F400TC		0x23
#define MBM29F800BA		0x58
#define MBM29F800TA		0xD6
#define MBM29LV002BC		0xC2
#define MBM29LV002TC		0x40
#define MBM29LV004BC		0xB6
#define MBM29LV004TC		0xB5
#define MBM29LV008BA		0x37
#define MBM29LV008TA		0x3E
#define MBM29LV080A		0x38
#define MBM29LV200BC		0xBF
#define MBM29LV200TC		0x3B
#define MBM29LV400BC		0xBA
#define MBM29LV400TC		0xB9
#define MBM29LV800BA		0x5B	/* Same as MBM29LV800BE */
#define MBM29LV800TA		0xDA	/* Same as MBM29LV800TE */

#define HYUNDAI_ID		0xAD	/* Hyundai */
#define HY_29F400T		0x23	/* Same as HY_29F400AT */
#define HY_29F800B		0x58	/* Same as HY_29F800AB */
#define HY_29LV800B		0x5B
#define HY_29F040A		0xA4
#define HY_29F400B		0xAB	/* Same as HY_29F400AB */
#define HY_29F002		0xB0
#define HY_29LV400T		0xB9
#define HY_29LV400B		0xBA
#define HY_29F080		0xD5
#define HY_29F800T		0xD6	/* Same as HY_29F800AT */
#define HY_29LV800T		0xDA

#define IMT_ID			0x7F1F	/* Integrated Memory Technologies */
#define IM_29F004B		0xAE
#define IM_29F004T		0xAF

#define INTEL_ID		0x89	/* Intel */
#define I_82802AB		0xAD
#define I_82802AC		0xAC
#define E_28F004S5		0xA7
#define E_28F008S5		0xA6
#define E_28F016S5		0xAA
#define P28F001BXT		0x94	/* 28F001BX-T */
#define P28F001BXB		0x95	/* 28F001BX-B */
#define P28F004BT		0x78	/* 28F004BV/BE-T */
#define P28F004BB		0x79	/* 28F004BV/BE-B */
#define P28F400BT		0x70	/* 28F400BV/CV/CE-T */
#define P28F400BB		0x71	/* 28F400BV/CV/CE-B */
#define SHARP_LH28F008SA	0xA2	/* Sharp chip, Intel Vendor ID */
#define SHARP_LH28F008SC	0xA6	/* Sharp chip, Intel Vendor ID */

#define ISSI_ID			0xD5	/* ISSI Integrated Silicon Solutions */

/*
 * MX25 chips are SPI, first byte of device ID is memory type,
 * second byte of device ID is log(bitsize)-9.
 * Generalplus SPI chips seem to be compatible with Macronix
 * and use the same set of IDs.
 */
#define MX_ID			0xC2	/* Macronix (MX) */
#define MX_25L512		0x2010	/* Same as MX25V512 */
#define MX_25L1005		0x2011
#define MX_25L2005		0x2012
#define MX_25L4005		0x2013	/* MX25L4005{,A} */
#define MX_25L8005		0x2014	/* Same as MX25V8005 */
#define MX_25L1605		0x2015	/* MX25L1605{,A,D} */
#define MX_25L3205		0x2016	/* MX25L3205{,A} */
#define MX_25L6405		0x2017	/* MX25L3205{,D} */
#define MX_25L12805		0x2018	/* MX25L12805 */
#define MX_25L1635D		0x2415
#define MX_25L3235D		0x5E16	/* MX25L3225D/MX25L3235D/MX25L3237D */
#define MX_29F001B		0x19
#define MX_29F001T		0x18
#define MX_29F002B		0x34	/* Same as MX29F002NB */
#define MX_29F002T		0xB0	/* Same as MX29F002NT */
#define MX_29F004B		0x46
#define MX_29F004T		0x45
#define MX_29F022T		0x36	/* Same as MX29F022NT */
#define MX_29F040		0xA4	/* Same as MX29F040C */
#define MX_29F080		0xD5
#define MX_29F200B		0x57	/* Same as MX29F200CB */
#define MX_29F200T		0x51	/* Same as MX29F200CT */
#define MX_29F400B		0xAB	/* Same as MX29F400CB */
#define MX_29F400T		0x23	/* Same as MX29F400CT */
#define MX_29F800B		0x58
#define MX_29F800T		0xD6
#define MX_29LV002CB		0x5A
#define MX_29LV002CT		0x59
#define MX_29LV004B		0xB6	/* Same as MX29LV004CB */
#define MX_29LV004T		0xB5	/* Same as MX29LV004CT */
#define MX_29LV008B		0x37	/* Same as MX29LV008CB */
#define MX_29LV008T		0x3E	/* Same as MX29LV008CT */
#define MX_29LV040		0x4F	/* Same as MX29LV040C */
#define MX_29LV081		0x38
#define MX_29LV128DB		0x7A
#define MX_29LV128DT		0x7E
#define MX_29LV160DB		0x49	/* Same as MX29LV161DB/MX29LV160CB */
#define MX_29LV160DT		0xC4	/* Same as MX29LV161DT/MX29LV160CT */
#define MX_29LV320DB		0xA8	/* Same as MX29LV321DB */
#define MX_29LV320DT		0xA7	/* Same as MX29LV321DT */
#define MX_29LV400B		0xBA	/* Same as MX29LV400CB */
#define MX_29LV400T		0xB9	/* Same as MX29LV400CT */
#define MX_29LV640DB		0xCB	/* Same as MX29LV640EB */
#define MX_29LV640DT		0xC9	/* Same as MX29LV640ET */
#define MX_29LV800B		0x5B	/* Same as MX29LV800CB */
#define MX_29LV800T		0xDA	/* Same as MX29LV800CT */
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
#define PMC_39F010		0x1C	/* Same as Pm39LV010 */
#define PMC_39LV020		0x3D
#define PMC_39LV040		0x3E
#define PMC_39F020		0x4D
#define PMC_39F040		0x4E
#define PMC_49FL002		0x6D
#define PMC_49FL004		0x6E

/* 
 * The Sanyo chip found so far uses SPI, first byte is manufacture code,
 * second byte is the device code,
 * third byte is a dummy byte.
 */
#define SANYO_ID		0x62
#define SANYO_LE25FW203A	0x1600

#define SHARP_ID		0xB0	/* Sharp */
#define SHARP_LH28F008BJxxPT	0xEC
#define SHARP_LH28F008BJxxPB	0xED
#define SHARP_LH28F800BVxxBTL	0x4B
#define SHARP_LH28F800BVxxBV	0x4D
#define SHARP_LH28F800BVxxTV	0x4C
#define SHARP_LHF00L02		0xC9	/* Same as LHF00L06/LHF00L07 */
#define SHARP_LHF00L04		0xCF	/* Same as LHF00L03/LHF00L05 */

/*
 * Spansion was previously a joint venture of AMD and Fujitsu.
 * S25 chips are SPI. The first device ID byte is memory type and
 * the second device ID byte is memory capacity.
 */
#define SPANSION_ID		0x01	/* Spansion, same ID as AMD */
#define SPANSION_S25FL008A	0x0213
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
#define SST_25VF040_REMS	0x44	/* REMS or RES opcode, same as SST25LF040A */
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
#define SST_29LE010		0x08	/* Same as SST29VE010 */
#define SST_29EE020A		0x10	/* Same as SST29EE020 */
#define SST_29LE020		0x12	/* Same as SST29VE020 */
#define SST_29SF020		0x24
#define SST_29VF020		0x25
#define SST_29SF040		0x13
#define SST_29VF040		0x14
#define SST_39SF512		0xB4
#define SST_39SF010		0xB5
#define SST_39SF020		0xB6	/* Same as 39SF020A */
#define SST_39SF040		0xB7
#define SST_39VF512		0xD4
#define SST_39VF010		0xD5
#define SST_39VF020		0xD6	/* Same as 39LF020 */
#define SST_39VF040		0xD7	/* Same as 39LF040 */
#define SST_39VF080		0xD8	/* Same as 39LF080/39VF080/39VF088 */
#define SST_49LF040B		0x50
#define SST_49LF040		0x51
#define SST_49LF020		0x61
#define SST_49LF020A		0x52
#define SST_49LF030A		0x1C
#define SST_49LF080A		0x5B
#define SST_49LF002A		0x57
#define SST_49LF003A		0x1B
#define SST_49LF004A		0x60	/* Same as 49LF004B */
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
#define ST_M25P05_RES		0x10	/* Same code as M25P10. */
#define ST_M25P10A		0x2011
#define ST_M25P10_RES		0x10	/* Same code as M25P05. */
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
#define ST_M29F002B		0x34	/* Same as M29F002BB */
#define ST_M29F002T		0xB0	/* Same as M29F002BT/M29F002NT/M29F002BNT */
#define ST_M29F040B		0xE2	/* Same as M29F040 */
#define ST_M29F080		0xF1
#define ST_M29F200BT		0xD3
#define ST_M29F200BB		0xD4
#define ST_M29F400BT		0xD5	/* Same as M29F400T */
#define ST_M29F400BB		0xD6	/* Same as M29F400B */
#define ST_M29F800DB		0x58
#define ST_M29F800DT		0xEC
#define ST_M29W010B		0x23
#define ST_M29W040B		0xE3
#define ST_M29W512B		0x27

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
#define W_25Q80			0x4014
#define W_25Q16			0x4015
#define W_25Q32			0x4016
#define W_29C011		0xC1
#define W_29C020C		0x45	/* Same as W29C020 and ASD AE29F2008 */
#define W_29C040P		0x46	/* Same as W29C040 */
#define W_29EE011		0xC1
#define W_39L020		0xB5
#define W_39L040		0xB6
#define W_39V040FA		0x34
#define W_39V040A		0x3D
#define W_39V040B		0x54
#define W_39V040C		0x50
#define W_39V080A		0xD0
#define W_39V080FA		0xD3
#define W_39V080FA_DM		0x93
#define W_49F002U		0x0B
#define W_49F020		0x8C
#define W_49V002A		0xB0
#define W_49V002FA		0x32

#endif /* !FLASHCHIPS_H */
