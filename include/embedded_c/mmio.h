/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#ifndef EMBEDDED_C_MMIO_H
#define EMBEDDED_C_MMIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write one 32-bit memory-mapped register.
 *
 * The access uses volatile semantics. The caller owns address alignment,
 * serialization, barriers, and cache policy.
 */
void ec_mmio_write32(uintptr_t address, uint32_t value);

/**
 * Read one 32-bit memory-mapped register.
 *
 * The access uses volatile semantics. The caller owns address alignment,
 * serialization, barriers, and cache policy.
 */
uint32_t ec_mmio_read32(uintptr_t address);

#ifdef __cplusplus
}
#endif

#endif
