#ifndef EMBEDDED_C_XFER_H
#define EMBEDDED_C_XFER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Ownership model for a transfer payload. */
typedef enum {
    EC_XFER_BORROWED = 0,
    EC_XFER_STATIC = 1,
    EC_XFER_POOLED = 2,
    EC_XFER_TRANSFERRED = 3
} ec_xfer_ownership_t;

/** Return 1 when the payload can be released, 0 to leave ownership unchanged. */
typedef int (*ec_xfer_release_fn)(void *context,
                                  uint8_t *data,
                                  uint32_t capacity);

/** Descriptor for a borrowed, static, pooled, or transferred payload. */
typedef struct {
    uint8_t *data;
    uint32_t length;
    uint32_t capacity;
    uint32_t sequence;
    uint32_t flags;
    ec_xfer_ownership_t ownership;
    ec_xfer_release_fn release;
    void *release_context;
} ec_xfer_t;

/** Initialize a borrowed payload. The caller keeps ownership. */
void ec_xfer_init_borrowed(ec_xfer_t *xfer,
                           const uint8_t *data,
                           uint32_t length);

/** Initialize a program-lifetime static payload. */
void ec_xfer_init_static(ec_xfer_t *xfer,
                         const uint8_t *data,
                         uint32_t length);

/** Initialize an owned payload that must be released through a callback. */
int ec_xfer_init_owned(ec_xfer_t *xfer,
                       uint8_t *data,
                       uint32_t length,
                       uint32_t capacity,
                       ec_xfer_ownership_t ownership,
                       ec_xfer_release_fn release,
                       void *release_context);

/**
 * Release an owned payload.
 *
 * Borrowed/static descriptors complete without invoking a callback. A failed
 * callback leaves the descriptor unchanged so the caller can retry.
 */
int ec_xfer_release(ec_xfer_t *xfer);

/** True when the descriptor does not currently own a releasable payload. */
int ec_xfer_is_empty(const ec_xfer_t *xfer);

#ifdef __cplusplus
}
#endif

#endif
