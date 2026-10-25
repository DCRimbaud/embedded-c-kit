#include "embedded_c/xfer.h"

#include <stddef.h>

static void clear_descriptor(ec_xfer_t *xfer)
{
    xfer->data = NULL;
    xfer->length = 0u;
    xfer->capacity = 0u;
    xfer->sequence = 0u;
    xfer->flags = 0u;
    xfer->ownership = EC_XFER_BORROWED;
    xfer->release = NULL;
    xfer->release_context = NULL;
}

void ec_xfer_init_borrowed(ec_xfer_t *xfer,
                           const uint8_t *data,
                           uint32_t length)
{
    if (xfer == NULL) {
        return;
    }

    clear_descriptor(xfer);
    xfer->data = (uint8_t *)data;
    xfer->length = length;
    xfer->ownership = EC_XFER_BORROWED;
}

void ec_xfer_init_static(ec_xfer_t *xfer,
                         const uint8_t *data,
                         uint32_t length)
{
    if (xfer == NULL) {
        return;
    }

    clear_descriptor(xfer);
    xfer->data = (uint8_t *)data;
    xfer->length = length;
    xfer->ownership = EC_XFER_STATIC;
}

int ec_xfer_init_owned(ec_xfer_t *xfer,
                       uint8_t *data,
                       uint32_t length,
                       uint32_t capacity,
                       ec_xfer_ownership_t ownership,
                       ec_xfer_release_fn release,
                       void *release_context)
{
    if ((xfer == NULL) || (data == NULL) || (length > capacity) ||
        ((ownership != EC_XFER_POOLED) && (ownership != EC_XFER_TRANSFERRED)) ||
        (release == NULL)) {
        return 0;
    }

    clear_descriptor(xfer);
    xfer->data = data;
    xfer->length = length;
    xfer->capacity = capacity;
    xfer->ownership = ownership;
    xfer->release = release;
    xfer->release_context = release_context;
    return 1;
}

int ec_xfer_release(ec_xfer_t *xfer)
{
    int released;

    if (xfer == NULL) {
        return 0;
    }

    if (xfer->release == NULL) {
        /* Borrowed and static payloads require no callback. */
        clear_descriptor(xfer);
        return 1;
    }

    released = xfer->release(xfer->release_context, xfer->data, xfer->capacity);
    if (released != 0) {
        clear_descriptor(xfer);
        return 1;
    }

    return 0;
}

int ec_xfer_is_empty(const ec_xfer_t *xfer)
{
    return (xfer == NULL) || (xfer->data == NULL) || (xfer->length == 0u);
}
