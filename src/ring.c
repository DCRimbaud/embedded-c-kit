/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/ringbuf.h"

#include <stddef.h>

void ringbuf_init(ringbuf_t *ring, uint8_t *storage, uint32_t capacity)
{
    /* Initialization is idempotent; a NULL control block is ignored. */
    if (ring == NULL) {
        return;
    }

    ring->buffer = storage;
    ring->capacity = (storage != NULL) ? capacity : 0u;
    ring->head = 0u;
    ring->tail = 0u;
    ring->count = 0u;
    ring->dropped = 0u;
}

int ringbuf_push(ringbuf_t *ring, uint8_t value)
{
    /* Initialization is idempotent; a NULL control block is ignored. */
    if (ring == NULL) {
        return 0;
    }

    /* Unbound storage and a full buffer share the same reject-and-count policy. */
    if ((ring->buffer == NULL) || (ring->capacity == 0u) ||
        (ring->count >= ring->capacity)) {
        ring->dropped++;
        return 0;
    }

    ring->buffer[ring->tail] = value;
    /* Modulo wrap-around keeps the producer independent of buffer size. */
    ring->tail = (ring->tail + 1u) % ring->capacity;
    ring->count++;
    return 1;
}

int ringbuf_pop(ringbuf_t *ring, uint8_t *out_value)
{
    if ((ring == NULL) || (out_value == NULL) || (ring->buffer == NULL) ||
        (ring->count == 0u)) {
        return 0;
    }

    /* Copy the value before advancing the head so readers never observe a gap. */
    *out_value = ring->buffer[ring->head];
    ring->head = (ring->head + 1u) % ring->capacity;
    ring->count--;
    return 1;
}

uint32_t ringbuf_count(const ringbuf_t *ring)
{
    return (ring != NULL) ? ring->count : 0u;
}

uint32_t ringbuf_drop_count(const ringbuf_t *ring)
{
    return (ring != NULL) ? ring->dropped : 0u;
}

int ringbuf_peek(const ringbuf_t *ring, uint32_t offset,
                   uint8_t *out_value)
{
    uint32_t index;

    if ((ring == NULL) || (out_value == NULL) || (ring->buffer == NULL) ||
        (ring->capacity == 0u) || (offset >= ring->count)) {
        return 0;
    }

    /* Peek must not consume or reorder buffered bytes. */
    index = (ring->head + offset) % ring->capacity;
    *out_value = ring->buffer[index];
    return 1;
}

uint32_t ringbuf_drop(ringbuf_t *ring, uint32_t count)
{
    uint32_t removed;

    if ((ring == NULL) || (ring->buffer == NULL) ||
        (ring->capacity == 0u) || (count == 0u)) {
        return 0u;
    }

    /* Clamp the request: callers may ask to drop a frame that has not fully arrived. */
    removed = (count < ring->count) ? count : ring->count;
    ring->head = (ring->head + removed) % ring->capacity;
    ring->count -= removed;
    return removed;
}
