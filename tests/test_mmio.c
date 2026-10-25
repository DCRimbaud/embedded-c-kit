/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/mmio.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_read_write(void)
{
    uint32_t word = 0u;
    ec_mmio_write32((uintptr_t)&word, 0x12345678u);
    assert(word == 0x12345678u);
    assert(ec_mmio_read32((uintptr_t)&word) == 0x12345678u);
}

int main(void)
{
    test_read_write();
    puts("mmio tests: PASS");
    return 0;
}
