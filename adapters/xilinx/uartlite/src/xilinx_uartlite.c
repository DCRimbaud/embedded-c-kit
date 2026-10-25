/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/xilinx_uartlite.h"

#include <stddef.h>

int ec_uartlite_init(ec_uartlite_t *uart, uint16_t device_id)
{
    if (uart == NULL) {
        return 0;
    }

    return (XUartLite_Initialize(&uart->instance, device_id) == 0) ? 1 : 0;
}

int ec_uartlite_try_write(void *context, uint8_t byte)
{
    ec_uartlite_t *uart = (ec_uartlite_t *)context;

    if ((uart == NULL) || (XUartLite_IsSending(&uart->instance) != 0u)) {
        return 0;
    }

    /* XUartLite_Send copies the byte into the driver FIFO before returning. */
    return (XUartLite_Send(&uart->instance, &byte, 1u) == 0) ? 1 : 0;
}

int ec_uartlite_try_read(ec_uartlite_t *uart, uint8_t *byte)
{
    if ((uart == NULL) || (byte == NULL) ||
        (XUartLite_IsReceiveEmpty(&uart->instance) != 0u)) {
        return 0;
    }

    return (XUartLite_Recv(&uart->instance, byte, 1u) == 1) ? 1 : 0;
}
