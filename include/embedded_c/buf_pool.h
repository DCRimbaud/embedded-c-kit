/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#ifndef EMBEDDED_C_BUF_POOL_H
#define EMBEDDED_C_BUF_POOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Per-slot metadata owned by the caller. */
typedef struct {
    uint32_t generation;
    uint8_t in_use;
} ec_pool_slot_t;

/** Fixed-capacity buffer pool with caller-owned storage and metadata. */
typedef struct {
    uint8_t *storage;
    ec_pool_slot_t *slots;
    uint32_t slot_size;
    uint32_t slot_count;
} ec_buf_pool_t;

/** Bind caller-owned storage and reset slot state. */
void ec_buf_pool_init(ec_buf_pool_t *pool,
                      uint8_t *storage,
                      ec_pool_slot_t *slots,
                      uint32_t slot_size,
                      uint32_t slot_count);

/**
 * Acquire one slot.
 *
 * @param out_data Receives the slot payload pointer.
 * @param out_generation Receives the generation required for release.
 * @return 1 on success, 0 when full or invalid.
 */
int ec_buf_pool_acquire(ec_buf_pool_t *pool,
                        uint8_t **out_data,
                        uint32_t *out_generation);

/**
 * Release one slot by generation.
 *
 * A stale generation is rejected, preventing an old handle from releasing a
 * slot that has already been reused.
 *
 * @return 1 on success, 0 for invalid or stale handles.
 */
int ec_buf_pool_release(ec_buf_pool_t *pool,
                        uint32_t slot_index,
                        uint32_t generation);

/** Return the number of slots currently in use. */
uint32_t ec_buf_pool_in_use(const ec_buf_pool_t *pool);

/** Return the total slot capacity. */
uint32_t ec_buf_pool_capacity(const ec_buf_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif
