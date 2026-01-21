/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2022 secunet Security Networks AG (written by Thomas Heijligen <thomas.heijligen@secunet.com)
 *
 * This header file provides functions for endianness conversions between CPU native endianness and little/big endian
 * formats, as well as functions to read values from memory with specified endianness.
 * These utilities abstract different byte orders across the supported hardware platforms.
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__ 1

#include <stddef.h>
#include <stdint.h>

/**
 * \brief Converts an 8-bit unsigned integer from CPU native endianness to little endian.
 * \param value The value to convert.
 * \return The converted value.
 */
uint8_t  cpu_to_le8 (uint8_t  value);

/**
 * \brief Converts a 16-bit unsigned integer from CPU native endianness to little endian.
 * \param value The value to convert.
 * \return The converted value.
 */
uint16_t cpu_to_le16(uint16_t value);

/**
 * \brief Converts a 32-bit unsigned integer from CPU native endianness to little endian.
 * \param value The value to convert.
 * \return The converted value.
 */
uint32_t cpu_to_le32(uint32_t value);

/**
 * \brief Converts a 64-bit unsigned integer from CPU native endianness to little endian.
 * \param value The value to convert.
 * \return The converted value.
 */
uint64_t cpu_to_le64(uint64_t value);

/**
 * \brief Converts an 8-bit unsigned integer from CPU native endianness to big endian.
 * \param value The value to convert.
 * \return The converted value.
 */
uint8_t  cpu_to_be8 (uint8_t  value);

/**
 * \brief Converts a 16-bit unsigned integer from CPU native endianness to big endian.
 * \param value The value to convert.
 * \return The converted value.
 */
uint16_t cpu_to_be16(uint16_t value);

/**
 * \brief Converts a 32-bit unsigned integer from CPU native endianness to big endian.
 * \param value The value to convert.
 * \return The converted value.
 */
uint32_t cpu_to_be32(uint32_t value);

/**
 * \brief Converts a 64-bit unsigned integer from CPU native endianness to big endian.
 * \param value The value to convert.
 * \return The converted value.
 */
uint64_t cpu_to_be64(uint64_t value);

/**
 * \brief Converts an 8-bit unsigned integer from little endian to CPU native endianness.
 * \param value The value to convert.
 * \return The converted value.
 */
uint8_t  le_to_cpu8 (uint8_t  value);

/**
 * \brief Converts a 16-bit unsigned integer from little endian to CPU native endianness.
 * \param value The value to convert.
 * \return The converted value.
 */
uint16_t le_to_cpu16(uint16_t value);

/**
 * \brief Converts a 32-bit unsigned integer from little endian to CPU native endianness.
 * \param value The value to convert.
 * \return The converted value.
 */
uint32_t le_to_cpu32(uint32_t value);

/**
 * \brief Converts a 64-bit unsigned integer from little endian to CPU native endianness.
 * \param value The value to convert.
 * \return The converted value.
 */
uint64_t le_to_cpu64(uint64_t value);


/**
 * \brief Converts an 8-bit unsigned integer from big endian to CPU native endianness.
 * \param value The value to convert.
 * \return The converted value.
 */
uint8_t  be_to_cpu8 (uint8_t  value);

/**
 * \brief Converts a 16-bit unsigned integer from big endian to CPU native endianness.
 * \param value The value to convert.
 * \return The converted value.
 */
uint16_t be_to_cpu16(uint16_t value);

/**
 * \brief Converts a 32-bit unsigned integer from big endian to CPU native endianness.
 * \param value The value to convert.
 * \return The converted value.
 */
uint32_t be_to_cpu32(uint32_t value);

/**
 * \brief Converts a 64-bit unsigned integer from big endian to CPU native endianness.
 * \param value The value to convert.
 * \return The converted value.
 */
uint64_t be_to_cpu64(uint64_t value);

/**
 * \brief Reads an 8-bit unsigned integer from memory in little endian format.
 * \param base The base address to read from.
 * \param offset The offset from the base address.
 * \return The read value in CPU native endianness.
 */
uint8_t  read_le8 (const void *base, size_t offset);

/**
 * \brief Reads a 16-bit unsigned integer from memory in little endian format.
 * \param base The base address to read from.
 * \param offset The offset from the base address.
 * \return The read value in CPU native endianness.
 */
uint16_t read_le16(const void *base, size_t offset);

/**
 * \brief Reads a 32-bit unsigned integer from memory in little endian format.
 * \param base The base address to read from.
 * \param offset The offset from the base address.
 * \return The read value in CPU native endianness.
 */
uint32_t read_le32(const void *base, size_t offset);

/**
 * \brief Reads a 64-bit unsigned integer from memory in little endian format.
 * \param base The base address to read from.
 * \param offset The offset from the base address.
 * \return The read value in CPU native endianness.
 */
uint64_t read_le64(const void *base, size_t offset);


/**
 * \brief Reads an 8-bit unsigned integer from memory in big endian format.
 * \param base The base address to read from.
 * \param offset The offset from the base address.
 * \return The read value in CPU native endianness.
 */
uint8_t  read_be8 (const void *base, size_t offset);

/**
 * \brief Reads a 16-bit unsigned integer from memory in big endian format.
 * \param base The base address to read from.
 * \param offset The offset from the base address.
 * \return The read value in CPU native endianness.
 */
uint16_t read_be16(const void *base, size_t offset);

/**
 * \brief Reads a 32-bit unsigned integer from memory in big endian format.
 * \param base The base address to read from.
 * \param offset The offset from the base address.
 * \return The read value in CPU native endianness.
 */
uint32_t read_be32(const void *base, size_t offset);

/**
 * \brief Reads a 64-bit unsigned integer from memory in big endian format.
 * \param base The base address to read from.
 * \param offset The offset from the base address.
 * \return The read value in CPU native endianness.
 */
uint64_t read_be64(const void *base, size_t offset);

#endif /* !__PLATFORM_H__ */
