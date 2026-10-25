#include "embedded_c/xfer.h"
#include "embedded_c/xfer_queue.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    unsigned int releases;
    uint32_t capacity;
} release_probe_t;

static int release_payload(void *context, uint8_t *data, uint32_t capacity)
{
    release_probe_t *probe = (release_probe_t *)context;
    if ((data == NULL) || (capacity == 0u) || (probe == NULL)) {
        return 0;
    }

    probe->releases++;
    probe->capacity = capacity;
    return 1;
}

int main(void)
{
    uint8_t first_bytes[4] = {1u, 2u, 3u, 4u};
    uint8_t second_bytes[2] = {5u, 6u};
    release_probe_t probe = {0};
    ec_xfer_t first;
    ec_xfer_t second;
    ec_xfer_t out;
    ec_xfer_t items[1];
    ec_xfer_queue_t queue;

    ec_xfer_init_borrowed(&first, first_bytes, 4u);
    assert(ec_xfer_release(&first) == 1);
    assert(ec_xfer_is_empty(&first) == 1);

    assert(ec_xfer_init_owned(&second, second_bytes, 2u, 2u,
                              EC_XFER_POOLED, release_payload, &probe) == 1);
    assert(ec_xfer_is_empty(&second) == 0);

    ec_xfer_queue_init(&queue, items, 1u);
    assert(ec_xfer_queue_push(&queue, &second) == 1);
    assert(ec_xfer_queue_push(&queue, &second) == 0);
    assert(ec_xfer_queue_peek(&queue, &out) == 1);
    assert(out.data == second_bytes);
    assert(ec_xfer_queue_pop(&queue, &out) == 1);
    assert(out.data == second_bytes);
    assert(ec_xfer_queue_count(&queue) == 0u);
    assert(ec_xfer_release(&out) == 1);
    assert(probe.releases == 1u);
    assert(probe.capacity == 2u);

    puts("xfer tests: PASS");
    return 0;
}
