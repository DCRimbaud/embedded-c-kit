/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#ifndef EMBEDDED_C_TX_ARBITER_H
#define EMBEDDED_C_TX_ARBITER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Current owner of the byte transport. */
enum ec_tx_owner {
    EC_TX_OWNER_IDLE = 0,
    EC_TX_OWNER_FRAME = 1
};

/** Write one byte. Return 1 when accepted or 0 when temporarily unavailable. */
typedef int (*ec_tx_write_fn)(void *context, uint8_t byte);

/** Frame/text transmit arbiter state. */
typedef struct {
    ec_tx_write_fn write;
    void *context;
    uint32_t owner;
} ec_tx_arbiter_t;

/** Bind a writer callback and reset ownership. */
void ec_tx_arbiter_init(ec_tx_arbiter_t *arbiter,
                        ec_tx_write_fn write,
                        void *context);

/** Acquire exclusive frame ownership. */
int ec_tx_arbiter_begin_frame(ec_tx_arbiter_t *arbiter);

/** Release frame ownership. */
void ec_tx_arbiter_end_frame(ec_tx_arbiter_t *arbiter);

/**
 * Write a frame byte.
 *
 * The first successful call acquires frame ownership. A failed write leaves
 * ownership with the frame so the caller can retry without interleaving text.
 */
int ec_tx_arbiter_frame_byte(ec_tx_arbiter_t *arbiter, uint8_t byte);

/** Write one text byte unless a frame currently owns the transport. */
int ec_tx_arbiter_text_byte(ec_tx_arbiter_t *arbiter, uint8_t byte);

/** Write a NUL-terminated string until completion or backpressure. */
uint32_t ec_tx_arbiter_text_string(ec_tx_arbiter_t *arbiter, const char *text);

/** Return EC_TX_OWNER_IDLE or EC_TX_OWNER_FRAME. */
uint32_t ec_tx_arbiter_owner(const ec_tx_arbiter_t *arbiter);

#ifdef __cplusplus
}
#endif

#endif
