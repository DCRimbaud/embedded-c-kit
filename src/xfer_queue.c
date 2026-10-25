#include "embedded_c/xfer_queue.h"

#include <stddef.h>
#include <string.h>

void ec_xfer_queue_init(ec_xfer_queue_t *queue,
                        ec_xfer_t *items,
                        uint32_t capacity)
{
    if (queue == NULL) {
        return;
    }

    if ((items == NULL) || (capacity == 0u)) {
        queue->items = NULL;
        queue->capacity = 0u;
        queue->head = 0u;
        queue->tail = 0u;
        queue->count = 0u;
        return;
    }

    queue->items = items;
    queue->capacity = capacity;
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
}

int ec_xfer_queue_push(ec_xfer_queue_t *queue, const ec_xfer_t *xfer)
{
    if ((queue == NULL) || (queue->items == NULL) || (queue->capacity == 0u) ||
        (xfer == NULL) || (queue->count >= queue->capacity)) {
        return 0;
    }

    queue->items[queue->tail] = *xfer;
    queue->tail = (queue->tail + 1u) % queue->capacity;
    queue->count++;
    return 1;
}

int ec_xfer_queue_pop(ec_xfer_queue_t *queue, ec_xfer_t *out_xfer)
{
    if ((queue == NULL) || (queue->items == NULL) || (out_xfer == NULL) ||
        (queue->count == 0u)) {
        return 0;
    }

    *out_xfer = queue->items[queue->head];
    memset(&queue->items[queue->head], 0, sizeof(queue->items[queue->head]));
    queue->head = (queue->head + 1u) % queue->capacity;
    queue->count--;
    return 1;
}

int ec_xfer_queue_peek(const ec_xfer_queue_t *queue, ec_xfer_t *out_xfer)
{
    if ((queue == NULL) || (queue->items == NULL) || (out_xfer == NULL) ||
        (queue->count == 0u)) {
        return 0;
    }

    *out_xfer = queue->items[queue->head];
    return 1;
}

uint32_t ec_xfer_queue_count(const ec_xfer_queue_t *queue)
{
    return (queue != NULL) ? queue->count : 0u;
}
