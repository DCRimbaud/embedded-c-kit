/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#ifndef XUARTLITE_H
#define XUARTLITE_H

#include <stdint.h>

typedef struct {
    uint8_t last_byte;
    int sending;
    int receive_pending;
    int init_result;
} XUartLite;

int XUartLite_Initialize(XUartLite *instance, uint16_t device_id);
unsigned int XUartLite_IsSending(XUartLite *instance);
int XUartLite_Send(XUartLite *instance, uint8_t *data, unsigned int length);
unsigned int XUartLite_IsReceiveEmpty(XUartLite *instance);
int XUartLite_Recv(XUartLite *instance, uint8_t *data, unsigned int length);

#endif
