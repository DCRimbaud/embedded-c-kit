/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/tx_arbiter.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t bytes[64];
    uint32_t count;
    int blocked;
} fake_writer_t;

static int fake_write(void *context, uint8_t byte)
{
    fake_writer_t *writer = (fake_writer_t *)context;
    if (writer->blocked != 0) {
        return 0;
    }

    writer->bytes[writer->count++] = byte;
    return 1;
}

static void test_frame_blocks_text(void)
{
    fake_writer_t writer = {0};
    ec_tx_arbiter_t arbiter;

    ec_tx_arbiter_init(&arbiter, fake_write, &writer);
    assert(ec_tx_arbiter_begin_frame(&arbiter) == 1);
    assert(ec_tx_arbiter_text_byte(&arbiter, (uint8_t)'x') == 0);
    assert(ec_tx_arbiter_frame_byte(&arbiter, 0xAAu) == 1);
    ec_tx_arbiter_end_frame(&arbiter);
    assert(ec_tx_arbiter_text_byte(&arbiter, (uint8_t)'x') == 1);
    assert(writer.count == 2u);
    assert(writer.bytes[0] == 0xAAu);
    assert(writer.bytes[1] == (uint8_t)'x');
}

static void test_retry_keeps_frame_owner(void)
{
    fake_writer_t writer = {0};
    ec_tx_arbiter_t arbiter;

    writer.blocked = 1;
    ec_tx_arbiter_init(&arbiter, fake_write, &writer);
    assert(ec_tx_arbiter_frame_byte(&arbiter, 0x55u) == 0);
    assert(ec_tx_arbiter_owner(&arbiter) == EC_TX_OWNER_FRAME);
    assert(ec_tx_arbiter_text_byte(&arbiter, (uint8_t)'x') == 0);
    writer.blocked = 0;
    assert(ec_tx_arbiter_frame_byte(&arbiter, 0x55u) == 1);
    assert(writer.count == 1u && writer.bytes[0] == 0x55u);
}

static void test_text_string_stops_on_backpressure(void)
{
    fake_writer_t writer = {0};
    ec_tx_arbiter_t arbiter;

    ec_tx_arbiter_init(&arbiter, fake_write, &writer);
    assert(ec_tx_arbiter_text_string(&arbiter, "OK") == 2u);
    writer.blocked = 1;
    assert(ec_tx_arbiter_text_string(&arbiter, "FAIL") == 0u);
    assert(writer.count == 2u);
}

int main(void)
{
    test_frame_blocks_text();
    test_retry_keeps_frame_owner();
    test_text_string_stops_on_backpressure();
    puts("tx_arbiter tests: PASS");
    return 0;
}
