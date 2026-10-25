/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/tx_arbiter.h"
#include "embedded_c/xilinx_uartlite.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int XUartLite_Initialize(XUartLite *instance, uint16_t device_id)
{
    (void)device_id;
    return instance->init_result;
}

unsigned int XUartLite_IsSending(XUartLite *instance)
{
    return (unsigned int)instance->sending;
}

int XUartLite_Send(XUartLite *instance, uint8_t *data, unsigned int length)
{
    if ((instance == 0) || (data == 0) || (length != 1u)) {
        return -1;
    }

    instance->last_byte = data[0];
    return 0;
}

unsigned int XUartLite_IsReceiveEmpty(XUartLite *instance)
{
    return (instance->receive_pending == 0) ? 1u : 0u;
}

int XUartLite_Recv(XUartLite *instance, uint8_t *data, unsigned int length)
{
    if ((instance == 0) || (data == 0) || (length != 1u) ||
        (instance->receive_pending == 0)) {
        return 0;
    }

    *data = instance->last_byte;
    instance->receive_pending = 0;
    return 1;
}

int main(void)
{
    ec_uartlite_t uart = {0};
    ec_tx_arbiter_t arbiter;
    uint8_t byte = 0u;

    uart.instance.init_result = 0;
    assert(ec_uartlite_init(&uart, 1u) == 1);
    assert(ec_uartlite_try_write(&uart, 0x5Au) == 1);
    assert(uart.instance.last_byte == 0x5Au);

    uart.instance.sending = 1;
    assert(ec_uartlite_try_write(&uart, 0x11u) == 0);
    uart.instance.sending = 0;

    ec_tx_arbiter_init(&arbiter, ec_uartlite_try_write, &uart);
    assert(ec_tx_arbiter_frame_byte(&arbiter, 0xA5u) == 1);
    ec_tx_arbiter_end_frame(&arbiter);
    assert(uart.instance.last_byte == 0xA5u);

    uart.instance.last_byte = 0x3Cu;
    uart.instance.receive_pending = 1;
    assert(ec_uartlite_try_read(&uart, &byte) == 1);
    assert(byte == 0x3Cu);
    assert(ec_uartlite_try_read(&uart, &byte) == 0);

    puts("xilinx_uartlite adapter tests: PASS");
    return 0;
}
