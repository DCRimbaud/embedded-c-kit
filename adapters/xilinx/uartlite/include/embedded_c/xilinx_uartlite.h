/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#ifndef EMBEDDED_C_XILINX_UARTLITE_H
#define EMBEDDED_C_XILINX_UARTLITE_H

#include <stdint.h>
#include <xuartlite.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Caller-owned Xilinx UARTLite instance. */
typedef struct {
    XUartLite instance;
} ec_uartlite_t;

/** Initialize the underlying Xilinx driver. */
int ec_uartlite_init(ec_uartlite_t *uart, uint16_t device_id);

/** Non-blocking write callback compatible with ec_tx_write_fn. */
int ec_uartlite_try_write(void *context, uint8_t byte);

/** Non-blocking read of one byte. */
int ec_uartlite_try_read(ec_uartlite_t *uart, uint8_t *byte);

#ifdef __cplusplus
}
#endif

#endif
