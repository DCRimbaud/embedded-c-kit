/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/bytes.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint8_t data[8] = {0};
    uint8_t unaligned[9] = {0};

    ec_write_be16(data, 0x1234u);
    assert(data[0] == 0x12u && data[1] == 0x34u);
    assert(ec_read_be16(data) == 0x1234u);

    ec_write_le16(data, 0x1234u);
    assert(data[0] == 0x34u && data[1] == 0x12u);
    assert(ec_read_le16(data) == 0x1234u);

    ec_write_be32(data, 0x89ABCDEFu);
    assert(data[0] == 0x89u && data[3] == 0xEFu);
    assert(ec_read_be32(data) == 0x89ABCDEFu);

    ec_write_le32(data, 0x89ABCDEFu);
    assert(data[0] == 0xEFu && data[3] == 0x89u);
    assert(ec_read_le32(data) == 0x89ABCDEFu);

    ec_write_be64(data, 0x0123456789ABCDEFu);
    assert(data[0] == 0x01u && data[7] == 0xEFu);
    assert(ec_read_be64(data) == 0x0123456789ABCDEFu);

    ec_write_le64(data, 0x0123456789ABCDEFu);
    assert(data[0] == 0xEFu && data[7] == 0x01u);
    assert(ec_read_le64(data) == 0x0123456789ABCDEFu);

    ec_write_be32(&unaligned[1], 0x11223344u);
    assert(ec_read_be32(&unaligned[1]) == 0x11223344u);

    puts("bytes tests: PASS");
    return 0;
}
