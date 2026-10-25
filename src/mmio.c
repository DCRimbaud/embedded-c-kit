/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/mmio.h"

void ec_mmio_write32(uintptr_t address, uint32_t value)
{
    /* The volatile access is intentionally performed exactly once per call. */
    *(volatile uint32_t *)address = value;
}

uint32_t ec_mmio_read32(uintptr_t address)
{
    /* A volatile read prevents the compiler from reusing a cached value. */
    return *(volatile uint32_t *)address;
}
