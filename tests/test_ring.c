/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/ringbuf.h"

#include <assert.h>
#include <stdio.h>

static void test_fifo_and_wrap(void)
{
    uint8_t storage[3];
    ringbuf_t ring;
    uint8_t value;

    ringbuf_init(&ring, storage, sizeof(storage));
    assert(ringbuf_push(&ring, 1u) == 1);
    assert(ringbuf_push(&ring, 2u) == 1);
    assert(ringbuf_push(&ring, 3u) == 1);
    assert(ringbuf_push(&ring, 4u) == 0);
    assert(ringbuf_drop_count(&ring) == 1u);

    assert(ringbuf_pop(&ring, &value) == 1 && value == 1u);
    assert(ringbuf_pop(&ring, &value) == 1 && value == 2u);
    assert(ringbuf_push(&ring, 4u) == 1);
    assert(ringbuf_push(&ring, 5u) == 1);

    assert(ringbuf_pop(&ring, &value) == 1 && value == 3u);
    assert(ringbuf_pop(&ring, &value) == 1 && value == 4u);
    assert(ringbuf_pop(&ring, &value) == 1 && value == 5u);
    assert(ringbuf_pop(&ring, &value) == 0);
}

static void test_peek_and_drop(void)
{
    uint8_t storage[4];
    ringbuf_t ring;
    uint8_t value;

    ringbuf_init(&ring, storage, sizeof(storage));
    assert(ringbuf_push(&ring, 10u) == 1);
    assert(ringbuf_push(&ring, 11u) == 1);
    assert(ringbuf_push(&ring, 12u) == 1);

    assert(ringbuf_peek(&ring, 0u, &value) == 1 && value == 10u);
    assert(ringbuf_peek(&ring, 2u, &value) == 1 && value == 12u);
    assert(ringbuf_peek(&ring, 3u, &value) == 0);

    assert(ringbuf_drop(&ring, 2u) == 2u);
    assert(ringbuf_count(&ring) == 1u);
    assert(ringbuf_pop(&ring, &value) == 1 && value == 12u);
    assert(ringbuf_drop(&ring, 2u) == 0u);
}

static void test_unbound_and_invalid(void)
{
    ringbuf_t ring;
    uint8_t value = 0u;

    ringbuf_init(NULL, NULL, 0u);
    ringbuf_init(&ring, NULL, 8u);
    assert(ringbuf_push(&ring, 1u) == 0);
    assert(ringbuf_drop_count(&ring) == 1u);
    assert(ringbuf_pop(&ring, &value) == 0);
    assert(ringbuf_peek(&ring, 0u, &value) == 0);
    assert(ringbuf_count(NULL) == 0u);
    assert(ringbuf_drop_count(NULL) == 0u);
}

int main(void)
{
    test_fifo_and_wrap();
    test_peek_and_drop();
    test_unbound_and_invalid();
    puts("embedded_c ring tests: PASS");
    return 0;
}
