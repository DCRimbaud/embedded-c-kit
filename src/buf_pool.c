/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/buf_pool.h"

#include <stddef.h>

void ec_buf_pool_init(ec_buf_pool_t *pool,
                      uint8_t *storage,
                      ec_pool_slot_t *slots,
                      uint32_t slot_size,
                      uint32_t slot_count)
{
    uint32_t index;

    if (pool == NULL) {
        return;
    }

    if ((storage == NULL) || (slots == NULL) ||
        (slot_size == 0u) || (slot_count == 0u)) {
        pool->storage = NULL;
        pool->slots = NULL;
        pool->slot_size = 0u;
        pool->slot_count = 0u;
        return;
    }

    pool->storage = storage;
    pool->slots = slots;
    pool->slot_size = slot_size;
    pool->slot_count = slot_count;

    for (index = 0u; index < slot_count; index++) {
        slots[index].generation = 0u;
        slots[index].in_use = 0u;
    }
}

int ec_buf_pool_acquire(ec_buf_pool_t *pool,
                        uint8_t **out_data,
                        uint32_t *out_generation)
{
    uint32_t index;

    if ((pool == NULL) || (pool->storage == NULL) || (pool->slots == NULL) ||
        (out_data == NULL) || (out_generation == NULL)) {
        return 0;
    }

    for (index = 0u; index < pool->slot_count; index++) {
        if (pool->slots[index].in_use == 0u) {
            pool->slots[index].in_use = 1u;
            pool->slots[index].generation++;

            /* Zero is reserved so callers can initialize handles to an invalid value. */
            if (pool->slots[index].generation == 0u) {
                pool->slots[index].generation = 1u;
            }

            *out_data = pool->storage + (index * pool->slot_size);
            *out_generation = pool->slots[index].generation;
            return 1;
        }
    }

    return 0;
}

int ec_buf_pool_release(ec_buf_pool_t *pool,
                        uint32_t slot_index,
                        uint32_t generation)
{
    if ((pool == NULL) || (pool->slots == NULL) ||
        (slot_index >= pool->slot_count) || (generation == 0u)) {
        return 0;
    }

    if ((pool->slots[slot_index].in_use == 0u) ||
        (pool->slots[slot_index].generation != generation)) {
        return 0;
    }

    pool->slots[slot_index].in_use = 0u;
    return 1;
}

uint32_t ec_buf_pool_in_use(const ec_buf_pool_t *pool)
{
    uint32_t index;
    uint32_t count = 0u;

    if ((pool == NULL) || (pool->slots == NULL)) {
        return 0u;
    }

    for (index = 0u; index < pool->slot_count; index++) {
        if (pool->slots[index].in_use != 0u) {
            count++;
        }
    }

    return count;
}

uint32_t ec_buf_pool_capacity(const ec_buf_pool_t *pool)
{
    return (pool != NULL) ? pool->slot_count : 0u;
}
