/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/tx_arbiter.h"

#include <stddef.h>

void ec_tx_arbiter_init(ec_tx_arbiter_t *arbiter,
                        ec_tx_write_fn write,
                        void *context)
{
    if (arbiter == NULL) {
        return;
    }

    arbiter->write = write;
    arbiter->context = context;
    arbiter->owner = EC_TX_OWNER_IDLE;
}

int ec_tx_arbiter_begin_frame(ec_tx_arbiter_t *arbiter)
{
    if ((arbiter == NULL) || (arbiter->write == NULL) ||
        (arbiter->owner == EC_TX_OWNER_FRAME)) {
        return 0;
    }

    arbiter->owner = EC_TX_OWNER_FRAME;
    return 1;
}

void ec_tx_arbiter_end_frame(ec_tx_arbiter_t *arbiter)
{
    if (arbiter != NULL) {
        arbiter->owner = EC_TX_OWNER_IDLE;
    }
}

int ec_tx_arbiter_frame_byte(ec_tx_arbiter_t *arbiter, uint8_t byte)
{
    if ((arbiter == NULL) || (arbiter->write == NULL)) {
        return 0;
    }

    if (arbiter->owner == EC_TX_OWNER_IDLE) {
        /* Acquire before the first write; failed writes keep the frame ownership. */
        arbiter->owner = EC_TX_OWNER_FRAME;
    }

    return arbiter->write(arbiter->context, byte);
}

int ec_tx_arbiter_text_byte(ec_tx_arbiter_t *arbiter, uint8_t byte)
{
    if ((arbiter == NULL) || (arbiter->write == NULL) ||
        (arbiter->owner == EC_TX_OWNER_FRAME)) {
        return 0;
    }

    return arbiter->write(arbiter->context, byte);
}

uint32_t ec_tx_arbiter_text_string(ec_tx_arbiter_t *arbiter, const char *text)
{
    uint32_t written = 0u;

    if ((arbiter == NULL) || (text == NULL)) {
        return 0u;
    }

    while (*text != '\0') {
        if (ec_tx_arbiter_text_byte(arbiter, (uint8_t)*text) == 0) {
            break;
        }

        written++;
        text++;
    }

    return written;
}

uint32_t ec_tx_arbiter_owner(const ec_tx_arbiter_t *arbiter)
{
    return (arbiter != NULL) ? arbiter->owner : EC_TX_OWNER_IDLE;
}
