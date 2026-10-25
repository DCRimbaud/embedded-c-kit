/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#ifndef EMBEDDED_C_BYTES_H
#define EMBEDDED_C_BYTES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Read a big-endian 16-bit value from an unaligned byte buffer. */
uint16_t ec_read_be16(const uint8_t *data);

/** Read a little-endian 16-bit value from an unaligned byte buffer. */
uint16_t ec_read_le16(const uint8_t *data);

/** Read a big-endian 32-bit value from an unaligned byte buffer. */
uint32_t ec_read_be32(const uint8_t *data);

/** Read a little-endian 32-bit value from an unaligned byte buffer. */
uint32_t ec_read_le32(const uint8_t *data);

/** Read a big-endian 64-bit value from an unaligned byte buffer. */
uint64_t ec_read_be64(const uint8_t *data);

/** Read a little-endian 64-bit value from an unaligned byte buffer. */
uint64_t ec_read_le64(const uint8_t *data);

/** Write a big-endian 16-bit value into a byte buffer. */
void ec_write_be16(uint8_t *data, uint16_t value);

/** Write a little-endian 16-bit value into a byte buffer. */
void ec_write_le16(uint8_t *data, uint16_t value);

/** Write a big-endian 32-bit value into a byte buffer. */
void ec_write_be32(uint8_t *data, uint32_t value);

/** Write a little-endian 32-bit value into a byte buffer. */
void ec_write_le32(uint8_t *data, uint32_t value);

/** Write a big-endian 64-bit value into a byte buffer. */
void ec_write_be64(uint8_t *data, uint64_t value);

/** Write a little-endian 64-bit value into a byte buffer. */
void ec_write_le64(uint8_t *data, uint64_t value);

#ifdef __cplusplus
}
#endif

#endif
