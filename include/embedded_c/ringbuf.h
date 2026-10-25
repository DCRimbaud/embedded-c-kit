/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Fixed-capacity byte ring buffer.
 *
 * Storage is supplied by the caller. The implementation performs no dynamic
 * allocation and applies a drop-newest policy when the buffer is full.
 *
 * The structure is not internally synchronized. Typical use is one producer
 * and one consumer with external serialization around init/reset operations.
 */
typedef struct {
    uint8_t *buffer;
    uint32_t capacity;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t dropped;
} ringbuf_t;

/**
 * Bind caller-owned storage and clear all state.
 *
 * Passing NULL storage or zero capacity leaves the ring unbound. Writes to an
 * unbound ring fail and increment the dropped counter.
 */
void ringbuf_init(ringbuf_t *ring, uint8_t *storage, uint32_t capacity);

/**
 * Append one byte.
 *
 * @return 1 when accepted, 0 when full, unbound, or invalid input.
 */
int ringbuf_push(ringbuf_t *ring, uint8_t value);

/**
 * Remove one byte from the head.
 *
 * @param out_value Written only when the function returns 1.
 * @return 1 on success, 0 when empty or invalid input.
 */
int ringbuf_pop(ringbuf_t *ring, uint8_t *out_value);

/** Return the number of buffered bytes, or 0 for a NULL ring. */
uint32_t ringbuf_count(const ringbuf_t *ring);

/** Return the cumulative number of rejected writes. */
uint32_t ringbuf_drop_count(const ringbuf_t *ring);

/**
 * Read a byte relative to the head without consuming it.
 *
 * @return 1 when the offset exists, 0 otherwise.
 */
int ringbuf_peek(const ringbuf_t *ring, uint32_t offset, uint8_t *out_value);

/**
 * Drop up to count bytes from the head.
 *
 * The operation is clamped to the current count.
 *
 * @return Number of bytes removed.
 */
uint32_t ringbuf_drop(ringbuf_t *ring, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif
