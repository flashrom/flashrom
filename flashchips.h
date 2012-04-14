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

#define GENERIC_MANUF_ID	0xFFFF	/* Check if there is a vendor ID */
#define GENERIC_DEVICE_ID	0xFFFF	/* Only match the vendor ID */
#define SFDP_DEVICE_ID		0xFFFE
#define PROGMANUF_ID		0xFFFE	/* dummy ID for opaque chips behind a programmer */
#define PROGDEV_ID		0x01	/* dummy ID for opaque chips behind a programmer */

#define ALLIANCE_ID		0x52	/* Alliance Semiconductor */
#define ALLIANCE_AS29F002B	0x34
#define ALLIANCE_AS29F002T	0xB0
#define ALLIANCE_AS29F010	0x04
#define ALLIANCE_AS29F040	0xA4
#define ALLIANCE_AS29F200B	0x57
#define ALLIANCE_AS29F200T	0x51
#define ALLIANCE_AS29LV160B	0x49
#define ALLIANCE_AS29LV160T	0xCA
#define ALLIANCE_AS29LV400B	0xBA
#define ALLIANCE_AS29LV400T	0xB9
#define ALLIANCE_AS29LV800B	0x5B
#define ALLIANCE_AS29LV800T	0xDA

#define AMD_ID			0x01	/* AMD */
#define AMD_AM29DL400BT		0x0C
#define AMD_AM29DL400BB		0x0F
#define AMD_AM29DL800BT		0x4A
#define AMD_AM29DL800BB		0xCB
#define AMD_AM29F002BB		0x34	/* Same as Am29F002NBB */
#define AMD_AM29F002BT		0xB0	/* Same as Am29F002NBT */
#define AMD_AM29F004BB		0x7B
#define AMD_AM29F004BT		0x77
#define AMD_AM29F016D		0xAD
#define AMD_AM29F010B		0x20	/* Same as Am29F010A */
#define AMD_AM29F040B		0xA4
#define AMD_AM29F080B		0xD5
#define AMD_AM29F200BB		0x57
#define AMD_AM29F200BT		0x51
#define AMD_AM29F400BB		0xAB
#define AMD_AM29F400BT		0x23
#define AMD_AM29F800BB		0x58
#define AMD_AM29F800BT		0xD6
#define AMD_AM29LV001BB		0x6D
#define AMD_AM29LV001BT		0xED
#define AMD_AM29LV002BB		0xC2
#define AMD_AM29LV002BT		0x40
#define AMD_AM29LV004BB		0xB6
#define AMD_AM29LV004BT		0xB5
#define AMD_AM29LV008BB		0x37
#define AMD_AM29LV008BT		0x3E
#define AMD_AM29LV040B		0x4F
#define AMD_AM29LV080B		0x38	/* Same as Am29LV081B */
#define AMD_AM29LV200BB		0xBF
#define AMD_AM29LV200BT		0x3B
#define AMD_AM29LV800BB		0x5B	/* Same as Am29LV800DB */
#define AMD_AM29LV400BT		0xB9
#define AMD_AM29LV400BB		0xBA
#define AMD_AM29LV800BT		0xDA	/* Same as Am29LV800DT */

#define AMIC_ID			0x7F37	/* AMIC */
#define AMIC_ID_NOPREFIX	0x37	/* AMIC */
#define AMIC_A25L05PT		0x2020
#define AMIC_A25L05PU		0x2010
#define AMIC_A25L10PT		0x2021
#define AMIC_A25L10PU		0x2011
#define AMIC_A25L20PT		0x2022
#define AMIC_A25L20PU		0x2012
#define AMIC_A25L40PT		0x2013	/* Datasheet says T and U have
					   same device ID. Confirmed by
					   hardware testing. */
#define AMIC_A25L40PU		0x2013
#define AMIC_A25L80P		0x2014	/* Seems that no A25L80PT exists */
#define AMIC_A25L16PT		0x2025
#define AMIC_A25L16PU		0x2015
#define AMIC_A25L512		0x3010
#define AMIC_A25L010		0x3011
#define AMIC_A25L020		0x3012
#define AMIC_A25L040		0x3013
#define AMIC_A25L080		0x3014
#define AMIC_A25L016		0x3015
#define AMIC_A25L032		0x3016
#define AMIC_A25LQ032		0x4016
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

#define ATMEL_ID		0x1F	/* Atmel */
#define ATMEL_AT25DF021		0x4300
#define ATMEL_AT25DF041A	0x4401
#define ATMEL_AT25DF081		0x4502
#define ATMEL_AT25DF081A	0x4501	/* Yes, 81A has a lower number than 81 */
#define ATMEL_AT25DF161		0x4602
#define ATMEL_AT25DF321		0x4700	/* Same as 26DF321 */
#define ATMEL_AT25DF321A	0x4701
#define ATMEL_AT25DF641		0x4800
#define ATMEL_AT25DQ161		0x8600
#define ATMEL_AT25F512		/* No device ID found in datasheet. Vendor ID
				 * can be read with AT25F512A_RDID */
#define ATMEL_AT25F512A		0x65 /* Needs AT25F512A_RDID */
#define ATMEL_AT25F512B		0x6500
#define ATMEL_AT25F1024		/* No device ID found in datasheet. Vendor ID
				 * can be read with AT25F512A_RDID */
#define ATMEL_AT25F1024A		0x60 /* Needs AT25F512A_RDID */
#define ATMEL_AT25FS010		0x6601
#define ATMEL_AT25FS040		0x6604
#define ATMEL_AT26DF041		0x4400
#define ATMEL_AT26DF081		0x4500	/* guessed, no datasheet available */
#define ATMEL_AT26DF081A	0x4501
#define ATMEL_AT26DF161		0x4600
#define ATMEL_AT26DF161A	0x4601
#define ATMEL_AT26DF321		0x4700	/* Same as 25DF321 */
#define ATMEL_AT26F004		0x0400
#define ATMEL_AT29C040A		0xA4
#define ATMEL_AT29C010A		0xD5
#define ATMEL_AT29C020		0xDA
#define ATMEL_AT29C512		0x5D
#define ATMEL_AT45BR3214B	/* No ID available */
#define ATMEL_AT45CS1282	0x2920
#define ATMEL_AT45D011		/* No ID available */
#define ATMEL_AT45D021A		/* No ID available */
#define ATMEL_AT45D041A		/* No ID available */
#define ATMEL_AT45D081A		/* No ID available */
#define ATMEL_AT45D161		/* No ID available */
#define ATMEL_AT45DB011		/* No ID available */
#define ATMEL_AT45DB011B	/* No ID available */
#define ATMEL_AT45DB011D	0x2200
#define ATMEL_AT45DB021A	/* No ID available */
#define ATMEL_AT45DB021B	/* No ID available */
#define ATMEL_AT45DB021D	0x2300
#define ATMEL_AT45DB041A	/* No ID available */
#define ATMEL_AT45DB041D	0x2400
#define ATMEL_AT45DB081A	/* No ID available */
#define ATMEL_AT45DB081D	0x2500
#define ATMEL_AT45DB161		/* No ID available */
#define ATMEL_AT45DB161B	/* No ID available */
#define ATMEL_AT45DB161D	0x2600
#define ATMEL_AT45DB321		/* No ID available */
#define ATMEL_AT45DB321B	/* No ID available */
#define ATMEL_AT45DB321C	0x2700
#define ATMEL_AT45DB321D	0x2701 /* Buggy data sheet */
#define ATMEL_AT45DB642		/* No ID available */
#define ATMEL_AT45DB642D	0x2800
#define ATMEL_AT49BV512		0x03
#define ATMEL_AT49F020		0x0B
#define ATMEL_AT49F002N		0x07	/* for AT49F002(N)  */
#define ATMEL_AT49F002NT		0x08	/* for AT49F002(N)T */
#define ATMEL_AT49LH002		0xE9

/* Bright Microelectronics has the same manufacturer ID as Hyundai... */
#define BRIGHT_ID		0xAD	/* Bright Microelectronics */
#define BRIGHT_BM29F040		0x40
#define BRIGHT_BM29F400B	0xAB
#define BRIGHT_BM29F400T	0xAD

#define CATALYST_ID		0x31	/* Catalyst */
#define CATALYST_CAT28F512	0xB8

#define EMST_ID			0x8C	/* EMST / EFST Elite Flash Storage */
#define EMST_F25L008A		0x2014
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
#define EON_EN25B05		0x2010	/* Same as P05, 2^19 kbit or 2^16 kByte */
#define EON_EN25B05T		0x25
#define EON_EN25B05B		0x95
#define EON_EN25B10		0x2011	/* Same as P10 */
#define EON_EN25B10T		0x40
#define EON_EN25B10B		0x30
#define EON_EN25B20		0x2012	/* Same as P20 */
#define EON_EN25B20T		0x41
#define EON_EN25B20B		0x31
#define EON_EN25B40		0x2013	/* Same as P40 */
#define EON_EN25B40T		0x42
#define EON_EN25B40B		0x32
#define EON_EN25B80		0x2014	/* Same as P80 */
#define EON_EN25B80T		0x43
#define EON_EN25B80B		0x33
#define EON_EN25B16		0x2015	/* Same as P16 */
#define EON_EN25B16T		0x44
#define EON_EN25B16B		0x34
#define EON_EN25B32		0x2016	/* Same as P32 */
#define EON_EN25B32T		0x45
#define EON_EN25B32B		0x35
#define EON_EN25B64		0x2017	/* Same as P64 */
#define EON_EN25B64T		0x46
#define EON_EN25B64B		0x36
#define EON_EN25F05		0x3110
#define EON_EN25F10		0x3111
#define EON_EN25F20		0x3112
#define EON_EN25F40		0x3113
#define EON_EN25F80		0x3114
#define EON_EN25F16		0x3115
#define EON_EN25F32		0x3116
#define EON_EN25Q40		0x3013
#define EON_EN25Q80		0x3014
#define EON_EN25Q16		0x3015	/* Same as EN25D16 */
#define EON_EN25Q32		0x3016	/* Same as EN25Q32A and EN25Q32B */
#define EON_EN25Q64		0x3017
#define EON_EN25Q128		0x3018
#define EON_EN25QH16		0x7015
#define EON_EN29F512		0x7F21
#define EON_EN29F010		0x20
#define EON_EN29F040A		0x7F04
#define EON_EN29LV010		0x7F6E
#define EON_EN29LV040A		0x7F4F	/* EN29LV040(A) */
#define EON_EN29F002T		0x7F92	/* Same as EN29F002A */
#define EON_EN29F002B		0x7F97	/* Same as EN29F002AN */

#define FUJITSU_ID		0x04	/* Fujitsu */
#define FUJITSU_MBM29DL400BC	0x0F
#define FUJITSU_MBM29DL400TC	0x0C
#define FUJITSU_MBM29DL800BA	0xCB
#define FUJITSU_MBM29DL800TA	0x4A
#define FUJITSU_MBM29F002BC	0x34
#define FUJITSU_MBM29F002TC	0xB0
#define FUJITSU_MBM29F004BC	0x7B
#define FUJITSU_MBM29F004TC	0x77
#define FUJITSU_MBM29F040C	0xA4
#define FUJITSU_MBM29F080A	0xD5
#define FUJITSU_MBM29F200BC	0x57
#define FUJITSU_MBM29F200TC	0x51
#define FUJITSU_MBM29F400BC	0xAB
#define FUJITSU_MBM29F400TC	0x23
#define FUJITSU_MBM29F800BA	0x58
#define FUJITSU_MBM29F800TA	0xD6
#define FUJITSU_MBM29LV002BC	0xC2
#define FUJITSU_MBM29LV002TC	0x40
#define FUJITSU_MBM29LV004BC	0xB6
#define FUJITSU_MBM29LV004TC	0xB5
#define FUJITSU_MBM29LV008BA	0x37
#define FUJITSU_MBM29LV008TA	0x3E
#define FUJITSU_MBM29LV080A	0x38
#define FUJITSU_MBM29LV200BC	0xBF
#define FUJITSU_MBM29LV200TC	0x3B
#define FUJITSU_MBM29LV400BC	0xBA
#define FUJITSU_MBM29LV400TC	0xB9
#define FUJITSU_MBM29LV800BA	0x5B	/* Same as MBM29LV800BE */
#define FUJITSU_MBM29LV800TA	0xDA	/* Same as MBM29LV800TE */

#define HYUNDAI_ID		0xAD	/* Hyundai */
#define HYUNDAI_HY29F400T	0x23	/* Same as HY29F400AT */
#define HYUNDAI_HY29F800B	0x58	/* Same as HY29F800AB */
#define HYUNDAI_HY29LV800B	0x5B
#define HYUNDAI_HY29F040A	0xA4
#define HYUNDAI_HY29F400B	0xAB	/* Same as HY29F400AB */
#define HYUNDAI_HY29F002B	0x34
#define HYUNDAI_HY29F002T	0xB0
#define HYUNDAI_HY29LV400T	0xB9
#define HYUNDAI_HY29LV400B	0xBA
#define HYUNDAI_HY29F080	0xD5
#define HYUNDAI_HY29F800T	0xD6	/* Same as HY29F800AT */
#define HYUNDAI_HY29LV800T	0xDA

#define IMT_ID			0x7F1F	/* Integrated Memory Technologies */
#define IMT_IM29F004B		0xAE
#define IMT_IM29F004T		0xAF

#define INTEL_ID		0x89	/* Intel */
#define INTEL_28F320J5		0x14
#define INTEL_28F640J5		0x15
#define INTEL_28F320J3		0x16
#define INTEL_28F640J3		0x17
#define INTEL_28F128J3		0x18
#define INTEL_28F256J3		0x1D
#define INTEL_28F400T		0x70	/* 28F400BV/BX/CE/CV-T */
#define INTEL_28F400B		0x71	/* 28F400BV/BX/CE/CV-B */
#define INTEL_28F200T		0x74	/* 28F200BL/BV/BX/CV-T */
#define INTEL_28F200B		0x75	/* 28F200BL/BV/BX/CV-B */
#define INTEL_28F004T		0x78	/* 28F004B5/BE/BV/BX-T */
#define INTEL_28F004B		0x79	/* 28F004B5/BE/BV/BX-B */
#define INTEL_28F002T		0x7C	/* 28F002BC/BL/BV/BX-T */
#define INTEL_28F002B		0x7D	/* 28F002BL/BV/BX-B */
#define INTEL_28F001T		0x94	/* 28F001BN/BX-T */
#define INTEL_28F001B		0x95	/* 28F001BN/BX-B */
#define INTEL_28F008T		0x98	/* 28F008BE/BV-T */
#define INTEL_28F008B		0x99	/* 28F008BE/BV-B */
#define INTEL_28F800T		0x9C	/* 28F800B5/BV/CE/CV-T */
#define INTEL_28F800B		0x9D	/* 28F800B5/BV/CE/CV-B */
#define INTEL_28F016SV		0xA0	/* 28F016SA/SV */
#define INTEL_28F008SA		0xA2
#define INTEL_28F008S3		0xA6	/* 28F008S3/S5/SC */
#define INTEL_28F004S3		0xA7	/* 28F008S3/S5/SC */
#define INTEL_28F016XS		0xA8
#define INTEL_28F016S3		0xAA	/* 28F016S3/S5/SC */
#define INTEL_82802AC		0xAC
#define INTEL_82802AB		0xAD
#define INTEL_28F010		0xB4
#define INTEL_28F512		0xB8
#define INTEL_28F256A		0xB9
#define INTEL_28F020		0xBD
#define INTEL_28F016B3T		0xD0	/* 28F016B3-T */
#define INTEL_28F016B3B		0xD1	/* 28F016B3-B */
#define INTEL_28F008B3T		0xD2	/* 28F008B3-T */
#define INTEL_28F008B3B		0xD3	/* 28F008B3-B */
#define INTEL_28F004B3T		0xD4	/* 28F004B3-T */
#define INTEL_28F004B3B		0xD5	/* 28F004B3-B */

#define SHARP_LH28F008SA	0xA2	/* Sharp chip, Intel Vendor ID */
#define SHARP_LH28F008SC	0xA6	/* Sharp chip, Intel Vendor ID */

#define ISSI_ID			0xD5	/* ISSI Integrated Silicon Solutions */

/*
 * MX25 chips are SPI, first byte of device ID is memory type,
 * second byte of device ID is log(bitsize)-9.
 * Generalplus SPI chips seem to be compatible with Macronix
 * and use the same set of IDs.
 */
#define MACRONIX_ID		0xC2	/* Macronix (MX) */
#define MACRONIX_MX25L512	0x2010	/* Same as MX25V512 */
#define MACRONIX_MX25L1005	0x2011
#define MACRONIX_MX25L2005	0x2012
#define MACRONIX_MX25L4005	0x2013	/* MX25L4005{,A} */
#define MACRONIX_MX25L8005	0x2014	/* Same as MX25V8005 */
#define MACRONIX_MX25L1605	0x2015	/* MX25L1605{,A,D} */
#define MACRONIX_MX25L3205	0x2016	/* MX25L3205{,A} */
#define MACRONIX_MX25L6405	0x2017	/* MX25L6405{,D}, MX25L6406E, MX25L6436E */
#define MACRONIX_MX25L12805	0x2018	/* MX25L12805 */
#define MACRONIX_MX25L1635D	0x2415
#define MACRONIX_MX25L1635E	0x2515	/* MX25L1635{E} */
#define MACRONIX_MX25L3235D	0x5E16	/* MX25L3225D/MX25L3235D/MX25L3237D */
#define MACRONIX_MX29F001B	0x19
#define MACRONIX_MX29F001T	0x18
#define MACRONIX_MX29F002B	0x34	/* Same as MX29F002NB; N has reset pin n/c. */
#define MACRONIX_MX29F002T	0xB0	/* Same as MX29F002NT; N has reset pin n/c. */
#define MACRONIX_MX29F004B	0x46
#define MACRONIX_MX29F004T	0x45
#define MACRONIX_MX29F022T	0x36	/* Same as MX29F022NT */
#define MACRONIX_MX29F040	0xA4	/* Same as MX29F040C */
#define MACRONIX_MX29F080	0xD5
#define MACRONIX_MX29F200B	0x57	/* Same as MX29F200CB */
#define MACRONIX_MX29F200T	0x51	/* Same as MX29F200CT */
#define MACRONIX_MX29F400B	0xAB	/* Same as MX29F400CB */
#define MACRONIX_MX29F400T	0x23	/* Same as MX29F400CT */
#define MACRONIX_MX29F800B	0x58
#define MACRONIX_MX29F800T	0xD6
#define MACRONIX_MX29LV002CB	0x5A
#define MACRONIX_MX29LV002CT	0x59
#define MACRONIX_MX29LV004B	0xB6	/* Same as MX29LV004CB */
#define MACRONIX_MX29LV004T	0xB5	/* Same as MX29LV004CT */
#define MACRONIX_MX29LV008B	0x37	/* Same as MX29LV008CB */
#define MACRONIX_MX29LV008T	0x3E	/* Same as MX29LV008CT */
#define MACRONIX_MX29LV040	0x4F	/* Same as MX29LV040C */
#define MACRONIX_MX29LV081	0x38
#define MACRONIX_MX29LV128DB	0x7A
#define MACRONIX_MX29LV128DT	0x7E
#define MACRONIX_MX29LV160DB	0x49	/* Same as MX29LV161DB/MX29LV160CB */
#define MACRONIX_MX29LV160DT	0xC4	/* Same as MX29LV161DT/MX29LV160CT */
#define MACRONIX_MX29LV320DB	0xA8	/* Same as MX29LV321DB */
#define MACRONIX_MX29LV320DT	0xA7	/* Same as MX29LV321DT */
#define MACRONIX_MX29LV400B	0xBA	/* Same as MX29LV400CB */
#define MACRONIX_MX29LV400T	0xB9	/* Same as MX29LV400CT */
#define MACRONIX_MX29LV640DB	0xCB	/* Same as MX29LV640EB */
#define MACRONIX_MX29LV640DT	0xC9	/* Same as MX29LV640ET */
#define MACRONIX_MX29LV800B	0x5B	/* Same as MX29LV800CB */
#define MACRONIX_MX29LV800T	0xDA	/* Same as MX29LV800CT */
#define MACRONIX_MX29SL402CB	0xF1
#define MACRONIX_MX29SL402CT	0x70
#define MACRONIX_MX29SL800CB	0x6B	/* Same as MX29SL802CB */
#define MACRONIX_MX29SL800CT	0xEA	/* Same as MX29SL802CT */

/*
 * Programmable Micro Corp is listed in JEP106W in bank 2, so it should
 * have a 0x7F continuation code prefix.
 */
#define PMC_ID			0x7F9D	/* PMC */
#define PMC_ID_NOPREFIX		0x9D	/* PMC, missing 0x7F prefix */
#define PMC_PM25LV512		0x7B
#define PMC_PM25LV010		0x7C
#define PMC_PM25LV020		0x7D
#define PMC_PM25LV040		0x7E
#define PMC_PM25LV080B		0x13
#define PMC_PM25LV016B		0x14
#define PMC_PM29F002T		0x1D
#define PMC_PM29F002B		0x2D
#define PMC_PM39LV512		0x1B
#define PMC_PM39F010		0x1C	/* Same as Pm39LV010 */
#define PMC_PM39LV020		0x3D
#define PMC_PM39LV040		0x3E
#define PMC_PM39F020		0x4D
#define PMC_PM39F040		0x4E
#define PMC_PM49FL002		0x6D
#define PMC_PM49FL004		0x6E

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
#define SPANSION_S25FL004A	0x0212
#define SPANSION_S25FL008A	0x0213
#define SPANSION_S25FL016A	0x0214
#define SPANSION_S25FL032A	0x0215
#define SPANSION_S25FL064A	0x0216

/*
 * SST25 chips are SPI, first byte of device ID is memory type, second
 * byte of device ID is related to log(bitsize) at least for some chips.
 */
#define SST_ID			0xBF	/* SST */
#define SST_SST25WF512		0x2501
#define SST_SST25WF010		0x2502
#define SST_SST25WF020		0x2503
#define SST_SST25WF040		0x2504
#define SST_SST25VF512A_REMS	0x48	/* REMS or RES opcode */
#define SST_SST25VF010_REMS	0x49	/* REMS or RES opcode */
#define SST_SST25VF020_REMS	0x43	/* REMS or RES opcode */
#define SST_SST25VF040_REMS	0x44	/* REMS or RES opcode, same as SST25LF040A */
#define SST_SST25VF040B		0x258D
#define SST_SST25VF040B_REMS	0x8D	/* REMS or RES opcode */
#define SST_SST25VF080_REMS	0x80	/* REMS or RES opcode, same as SST25LF080A */
#define SST_SST25VF080B		0x258E
#define SST_SST25VF080B_REMS	0x8E	/* REMS or RES opcode */
#define SST_SST25VF016B		0x2541
#define SST_SST25VF032B		0x254A
#define SST_SST25VF032B_REMS	0x4A	/* REMS or RES opcode */
#define SST_SST25VF064C		0x254B
#define SST_SST26VF016		0x2601
#define SST_SST26VF032		0x2602
#define SST_SST27SF512		0xA4
#define SST_SST27SF010		0xA5
#define SST_SST27SF020		0xA6
#define SST_SST27VF010		0xA9
#define SST_SST27VF020		0xAA
#define SST_SST28SF040		0x04
#define SST_SST29EE512		0x5D
#define SST_SST29EE010		0x07
#define SST_SST29LE010		0x08	/* Same as SST29VE010 */
#define SST_SST29EE020A		0x10	/* Same as SST29EE020 */
#define SST_SST29LE020		0x12	/* Same as SST29VE020 */
#define SST_SST29SF020		0x24
#define SST_SST29VF020		0x25
#define SST_SST29SF040		0x13
#define SST_SST29VF040		0x14
#define SST_SST39SF512		0xB4
#define SST_SST39SF010		0xB5
#define SST_SST39SF020		0xB6	/* Same as 39SF020A */
#define SST_SST39SF040		0xB7
#define SST_SST39VF512		0xD4
#define SST_SST39VF010		0xD5
#define SST_SST39VF020		0xD6	/* Same as 39LF020 */
#define SST_SST39VF040		0xD7	/* Same as 39LF040 */
#define SST_SST39VF080		0xD8	/* Same as 39LF080/39VF080/39VF088 */
#define SST_SST49LF040B		0x50
#define SST_SST49LF040		0x51
#define SST_SST49LF020		0x61
#define SST_SST49LF020A		0x52
#define SST_SST49LF030A		0x1C
#define SST_SST49LF080A		0x5B
#define SST_SST49LF002A		0x57
#define SST_SST49LF003A		0x1B
#define SST_SST49LF004A		0x60	/* Same as 49LF004B */
#define SST_SST49LF008A		0x5A
#define SST_SST49LF004C		0x54
#define SST_SST49LF008C		0x59
#define SST_SST49LF016C		0x5C
#define SST_SST49LF160C		0x4C

/*
 * ST25P chips are SPI, first byte of device ID is memory type, second
 * byte of device ID is related to log(bitsize) at least for some chips.
 */
#define ST_ID			0x20	/* ST / SGS/Thomson / Numonyx (later acquired by Micron) */
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
#define ST_M25PX16		0x7115
#define ST_M25PX32		0x7116
#define ST_M25PX64		0x7117
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
#define ST_N25Q064		0xBA17

#define SYNCMOS_MVC_ID		0x40	/* SyncMOS (SM) and Mosel Vitelic Corporation (MVC) */
#define MVC_V29C51000T		0x00
#define MVC_V29C51400T		0x13
#define MVC_V29LC51000		0x20
#define MVC_V29LC51001		0x60
#define MVC_V29LC51002		0x82
#define MVC_V29C51000B		0xA0
#define MVC_V29C51400B		0xB3
#define SM_MVC_29C51001T	0x01	/* Identical chips: {F,S,V}29C51001T */
#define SM_MVC_29C51002T	0x02	/* Identical chips: {F,S,V}29C51002T */
#define SM_MVC_29C51004T	0x03	/* Identical chips: {F,S,V}29C51004T */
#define SM_MVC_29C31004T	0x63	/* Identical chips: {S,V}29C31004T */
#define SM_MVC_29C31004B	0x73	/* Identical chips: {S,V}29C31004B */
#define SM_MVC_29C51001B	0xA1	/* Identical chips: {F,S,V}29C51001B */
#define SM_MVC_29C51002B	0xA2	/* Identical chips: {F,S,V}29C51002B */
#define SM_MVC_29C51004B	0xA3	/* Identical chips: {F,S,V}29C51004B */

#define TI_ID			0x97	/* Texas Instruments */
#define TI_OLD_ID		0x01	/* TI chips from last century */
#define TI_TMS29F002RT		0xB0
#define TI_TMS29F002RB		0x34

/*
 * W25X chips are SPI, first byte of device ID is memory type, second
 * byte of device ID is related to log(bitsize).
 */
#define WINBOND_NEX_ID		0xEF	/* Winbond (ex Nexcom) serial flashes */
#define WINBOND_NEX_W25X10	0x3011
#define WINBOND_NEX_W25X20	0x3012
#define WINBOND_NEX_W25X40	0x3013
#define WINBOND_NEX_W25X80	0x3014
#define WINBOND_NEX_W25X16	0x3015
#define WINBOND_NEX_W25X32	0x3016
#define WINBOND_NEX_W25X64	0x3017
#define WINBOND_NEX_W25Q40	0x4013
#define WINBOND_NEX_W25Q80	0x4014
#define WINBOND_NEX_W25Q16	0x4015
#define WINBOND_NEX_W25Q32	0x4016
#define WINBOND_NEX_W25Q64	0x4017
#define WINBOND_NEX_W25Q128	0x4018

#define WINBOND_ID		0xDA	/* Winbond */
#define WINBOND_W19B160BB	0x49
#define WINBOND_W19B160BT	0xC4
#define WINBOND_W19B320SB	0x2A    /* Same as W19L320SB */
#define WINBOND_W19B320ST	0xBA    /* Same as W19L320ST */
#define WINBOND_W19B322MB	0x92
#define WINBOND_W19B322MT	0x10
#define WINBOND_W19B323MB	0x94
#define WINBOND_W19B323MT	0x13
#define WINBOND_W19B324MB	0x97
#define WINBOND_W19B324MT	0x16
#define WINBOND_W29C010		0xC1    /* Same as W29C010M, W29C011A, W29EE011, W29EE012, and ASD AE29F1008 */
#define WINBOND_W29C020		0x45    /* Same as W29C020C, W29C022 and ASD AE29F2008 */
#define WINBOND_W29C040		0x46    /* Same as W29C040P */
#define WINBOND_W29C512A	0xC8    /* Same as W29EE512 */
#define WINBOND_W39L010		0x31
#define WINBOND_W39L020		0xB5
#define WINBOND_W39L040		0xB6
#define WINBOND_W39L040A	0xD6
#define WINBOND_W39L512		0x38
#define WINBOND_W39V040A	0x3D
#define WINBOND_W39V040FA	0x34
#define WINBOND_W39V040B	0x54    /* Same as W39V040FB */
#define WINBOND_W39V040C	0x50    /* Same as W39V040FC */
#define WINBOND_W39V080A	0xD0
#define WINBOND_W39V080FA	0xD3
#define WINBOND_W39V080FA_DM	0x93    /* W39V080FA dual mode */
#define WINBOND_W49F002		0x25    /* Same as W49F002B */
#define WINBOND_W49F002U	0x0B    /* Same as W49F002N and ASD AE49F2008 */
#define WINBOND_W49F020		0x8C
#define WINBOND_W49V002A	0xB0
#define WINBOND_W49V002FA	0x32

#endif /* !FLASHCHIPS_H */
