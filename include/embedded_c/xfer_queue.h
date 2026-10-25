#ifndef EMBEDDED_C_XFER_QUEUE_H
#define EMBEDDED_C_XFER_QUEUE_H

#include "embedded_c/xfer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Fixed descriptor queue with caller-owned storage. */
typedef struct {
    ec_xfer_t *items;
    uint32_t capacity;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} ec_xfer_queue_t;

/** Bind descriptor storage and reset queue state. */
void ec_xfer_queue_init(ec_xfer_queue_t *queue,
                        ec_xfer_t *items,
                        uint32_t capacity);

/**
 * Copy a descriptor into the queue.
 *
 * On success, the queue owns the descriptor and the caller must not release it.
 * On failure (full or invalid), ownership remains with the caller.
 */
int ec_xfer_queue_push(ec_xfer_queue_t *queue, const ec_xfer_t *xfer);

/** Remove one descriptor. Ownership transfers to the caller on success. */
int ec_xfer_queue_pop(ec_xfer_queue_t *queue, ec_xfer_t *out_xfer);

/** Inspect the oldest descriptor without removing it. */
int ec_xfer_queue_peek(const ec_xfer_queue_t *queue, ec_xfer_t *out_xfer);

/** Return the number of queued descriptors. */
uint32_t ec_xfer_queue_count(const ec_xfer_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif
