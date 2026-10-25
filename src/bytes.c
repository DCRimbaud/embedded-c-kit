/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/bytes.h"

#include <stddef.h>

uint16_t ec_read_be16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8u) | data[1]);
}

uint16_t ec_read_le16(const uint8_t *data)
{
    return (uint16_t)(data[0] | ((uint16_t)data[1] << 8u));
}

uint32_t ec_read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24u) |
           ((uint32_t)data[1] << 16u) |
           ((uint32_t)data[2] << 8u) |
           (uint32_t)data[3];
}

uint32_t ec_read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

uint64_t ec_read_be64(const uint8_t *data)
{
    return ((uint64_t)ec_read_be32(data) << 32u) | ec_read_be32(data + 4u);
}

uint64_t ec_read_le64(const uint8_t *data)
{
    return (uint64_t)ec_read_le32(data) |
           ((uint64_t)ec_read_le32(data + 4u) << 32u);
}

void ec_write_be16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8u);
    data[1] = (uint8_t)value;
}

void ec_write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

void ec_write_be32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24u);
    data[1] = (uint8_t)(value >> 16u);
    data[2] = (uint8_t)(value >> 8u);
    data[3] = (uint8_t)value;
}

void ec_write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

void ec_write_be64(uint8_t *data, uint64_t value)
{
    ec_write_be32(data, (uint32_t)(value >> 32u));
    ec_write_be32(data + 4u, (uint32_t)value);
}

void ec_write_le64(uint8_t *data, uint64_t value)
{
    ec_write_le32(data, (uint32_t)value);
    ec_write_le32(data + 4u, (uint32_t)(value >> 32u));
}
