/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/buf_pool.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_acquire_release_and_stale_generation(void)
{
    uint8_t storage[16];
    ec_pool_slot_t slots[2];
    ec_buf_pool_t pool;
    uint8_t *first = 0;
    uint8_t *second = 0;
    uint8_t *overflow = 0;

    ec_buf_pool_init(&pool, storage, slots, 8u, 2u);
    uint32_t first_gen = 0u;
    uint32_t second_gen = 0u;
    uint32_t overflow_gen = 0u;
    assert(ec_buf_pool_acquire(&pool, &first, &first_gen) == 1);
    assert(ec_buf_pool_acquire(&pool, &second, &second_gen) == 1);
    assert(first != second);
    assert(ec_buf_pool_in_use(&pool) == 2u);
    assert(ec_buf_pool_acquire(&pool, &overflow, &overflow_gen) == 0);

    assert(ec_buf_pool_release(&pool, 0u, first_gen) == 1);
    assert(ec_buf_pool_release(&pool, 0u, first_gen) == 0);

    uint8_t *reused = 0;
    uint32_t reused_gen = 0u;
    assert(ec_buf_pool_acquire(&pool, &reused, &reused_gen) == 1);
    assert(reused == first);
    assert(reused_gen != first_gen);
    assert(ec_buf_pool_release(&pool, 0u, first_gen) == 0);
    assert(ec_buf_pool_release(&pool, 0u, reused_gen) == 1);
}

static void test_invalid_arguments(void)
{
    ec_buf_pool_t pool;
    uint8_t *data = 0;
    uint32_t generation = 0u;

    ec_buf_pool_init(&pool, 0, 0, 0u, 0u);
    assert(ec_buf_pool_capacity(&pool) == 0u);
    assert(ec_buf_pool_acquire(&pool, &data, &generation) == 0);
    assert(ec_buf_pool_release(&pool, 0u, 1u) == 0);
}

int main(void)
{
    test_acquire_release_and_stale_generation();
    test_invalid_arguments();
    puts("buf_pool tests: PASS");
    return 0;
}
