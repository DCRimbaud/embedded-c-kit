/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "device_link.h"

#include "embedded_c/bytes.h"

#include <stddef.h>
#include <string.h>

/**
 * @brief Emit one optional diagnostic record for the example state.
 *
 * The logger may be NULL because the parser and transport code remain usable
 * without diagnostics. The record strings are borrowed for the synchronous
 * sink call only.
 *
 * @param link Initialized link state.
 * @param level Severity passed to the logger.
 * @param message Static or call-lifetime diagnostic text.
 * @return Nothing.
 * @pre The logger, when present, is configured according to its own threading contract.
 * @note This helper does not block unless the caller-provided log sink blocks.
 * @author Xue Weisen
 */
static void device_link_log(device_link_t *link,
                            ec_log_level_t level,
                            const char *message)
{
    if ((link != NULL) && (link->logger != NULL)) {
        (void)ec_log_write(link->logger, level, "device-link", message);
    }
}

/**
 * @brief Return one pooled TX slot to the fixed buffer pool.
 *
 * The release callback validates the slot ownership token before releasing it.
 * A stale callback therefore cannot free a slot that has already been reused.
 *
 * @param context Pointer to device_link_tx_release_t stored in the link state.
 * @param data Slot payload pointer supplied by the transfer descriptor.
 * @param capacity Slot capacity supplied by the transfer descriptor.
 * @return One when the exact generation was released, or zero for invalid or stale data.
 * @pre The context belongs to the same link that allocated the slot.
 * @note Called synchronously by ec_xfer_release(); it performs no blocking work.
 * @author Xue Weisen
 */
static int device_link_release_tx_slot(void *context,
                                       uint8_t *data,
                                       uint32_t capacity)
{
    device_link_tx_release_t *release_context = (device_link_tx_release_t *)context;
    uint8_t *slot_data;
    uint32_t slot_index;

    if ((release_context == NULL) || (release_context->owner == NULL) ||
        (data == NULL) || (capacity != DEVICE_LINK_TX_SLOT_SIZE)) {
        return 0;
    }

    slot_index = release_context->slot_index;
    if (slot_index >= DEVICE_LINK_TX_SLOT_COUNT) {
        return 0;
    }

    /* Validate that the callback still points at the slot it originally leased. */
    slot_data = release_context->owner->tx_storage +
                (slot_index * DEVICE_LINK_TX_SLOT_SIZE);
    if (slot_data != data) {
        return 0;
    }

    return ec_buf_pool_release(&release_context->owner->tx_pool,
                               slot_index,
                               release_context->generation);
}

/**
 * @brief Encode and queue one response using a pooled payload slot.
 *
 * The slot is acquired before frame construction. On every failure after
 * acquisition, the function releases the slot before returning so the caller
 * can continue parsing later frames. On success, descriptor ownership transfers
 * to the fixed TX queue and must not be released by the caller.
 *
 * @param link Initialized link state.
 * @param command Response command identifier.
 * @param sequence Response sequence copied from the request.
 * @param payload Borrowed response payload.
 * @param payload_length Response payload length.
 * @return One when the descriptor is queued, or zero after a clean failure.
 * @pre The link is initialized and the TX queue has fixed caller-owned storage.
 * @note The function does not transmit; device_link_flush() owns writer progress.
 * @author Xue Weisen
 */
static int device_link_queue_response(device_link_t *link,
                                      uint8_t command,
                                      uint16_t sequence,
                                      const uint8_t *payload,
                                      uint8_t payload_length)
{
    device_link_tx_release_t *release_context;
    ec_xfer_t xfer;
    uint8_t *slot_data = NULL;
    uint32_t generation = 0u;
    uint32_t frame_length;
    uint32_t slot_index;

    if (link == NULL) {
        return 0;
    }

    if ((payload == NULL) && (payload_length != 0u)) {
        link->stats.response_drops++;
        return 0;
    }

    /* Acquisition establishes a lease token before frame bytes are built. */
    if (ec_buf_pool_acquire(&link->tx_pool, &slot_data, &generation) == 0) {
        link->stats.response_drops++;
        device_link_log(link, EC_LOG_WARN, "tx pool exhausted");
        return 0;
    }

    slot_index = (uint32_t)(slot_data - link->tx_storage) /
                 DEVICE_LINK_TX_SLOT_SIZE;
    release_context = &link->tx_release[slot_index];
    release_context->owner = link;
    release_context->slot_index = slot_index;
    release_context->generation = generation;

    /* Encode into the leased slot; failure returns the slot without partial publication. */
    frame_length = device_link_encode_frame(command,
                                           sequence,
                                           payload,
                                           payload_length,
                                           slot_data,
                                           DEVICE_LINK_TX_SLOT_SIZE);
    if (frame_length == 0u) {
        (void)ec_buf_pool_release(&link->tx_pool, slot_index, generation);
        link->stats.response_drops++;
        device_link_log(link, EC_LOG_ERROR, "response encoding failed");
        return 0;
    }

    if (ec_xfer_init_owned(&xfer,
                           slot_data,
                           frame_length,
                           DEVICE_LINK_TX_SLOT_SIZE,
                           EC_XFER_POOLED,
                           device_link_release_tx_slot,
                           release_context) == 0) {
        (void)ec_buf_pool_release(&link->tx_pool, slot_index, generation);
        link->stats.response_drops++;
        device_link_log(link, EC_LOG_ERROR, "response descriptor initialization failed");
        return 0;
    }

    /* Ownership transfers only after this push succeeds; a failed push releases the lease. */
    if (ec_xfer_queue_push(&link->tx_queue, &xfer) == 0) {
        if (ec_xfer_release(&xfer) == 0) {
            link->stats.release_errors++;
        }
        link->stats.response_drops++;
        device_link_log(link, EC_LOG_WARN, "response queue full");
        return 0;
    }

    link->stats.responses_queued++;
    return 1;
}

/**
 * @brief Decode one already checksum-valid frame and queue its response.
 *
 * Ping returns "PONG", echo returns the request payload, and every unsupported
 * command returns "ERR". The response command has bit 7 set; the request sequence
 * is echoed unchanged so a host can correlate request and response.
 *
 * @param link Initialized link state.
 * @param frame Complete frame beginning with DEVICE_LINK_SOF.
 * @param frame_length Total validated frame length.
 * @return One when a response was queued, or zero when queuing failed.
 * @pre frame_length and checksum were validated by device_link_poll().
 * @note No pointer into the RX ring is retained after frame consumption.
 * @author Xue Weisen
 */
static int device_link_handle_frame(device_link_t *link,
                                    const uint8_t *frame,
                                    uint32_t frame_length)
{
    static const uint8_t pong[] = {'P', 'O', 'N', 'G'};
    static const uint8_t error[] = {'E', 'R', 'R'};
    const uint8_t *payload;
    uint8_t payload_length;
    uint8_t command;
    uint16_t sequence;

    if ((link == NULL) || (frame == NULL) || (frame_length < 6u)) {
        return 0;
    }

    command = frame[1];
    sequence = ec_read_be16(&frame[2]);
    payload_length = frame[4];
    payload = &frame[5];

    /* Dispatch remains deliberately small so the example shows composition, not product policy. */
    if (command == 0x01u) {
        return device_link_queue_response(link,
                                          0x81u,
                                          sequence,
                                          pong,
                                          (uint8_t)sizeof(pong));
    }

    if (command == 0x02u) {
        return device_link_queue_response(link,
                                          0x82u,
                                          sequence,
                                          payload,
                                          payload_length);
    }

    link->stats.unsupported_commands++;
    device_link_log(link, EC_LOG_WARN, "unsupported command");
    return device_link_queue_response(link,
                                      0xFFu,
                                      sequence,
                                      error,
                                      (uint8_t)sizeof(error));
}

/**
 * @brief Build one complete frame and return its byte length.
 *
 * All range checks happen before the first output write. The checksum is XOR
 * over the command, sequence, length, and payload bytes, deliberately excluding
 * the SOF. This keeps malformed-frame recovery visible without claiming that
 * XOR is a production-grade integrity check.
 *
 * @param command One-byte command identifier.
 * @param sequence Big-endian correlation value.
 * @param payload Borrowed payload bytes; NULL is valid only with zero length.
 * @param payload_length Payload length in bytes.
 * @param output Caller-owned destination storage.
 * @param output_capacity Number of writable bytes in output.
 * @return Total frame length on success, or zero before any write on invalid input.
 * @pre output points to a valid writable region when output_capacity is non-zero.
 * @note This function is pure with respect to package state and is reentrant when
 *       separate output regions are used.
 * @author Xue Weisen
 */
uint32_t device_link_encode_frame(uint8_t command,
                                  uint16_t sequence,
                                  const uint8_t *payload,
                                  uint8_t payload_length,
                                  uint8_t *output,
                                  uint32_t output_capacity)
{
    uint32_t checksum = 0u;
    uint32_t frame_length;
    uint32_t index;

    if ((output == NULL) ||
        (payload_length > DEVICE_LINK_MAX_PAYLOAD) ||
        ((payload == NULL) && (payload_length != 0u))) {
        return 0u;
    }

    frame_length = 5u + (uint32_t)payload_length + 1u;
    if (output_capacity < frame_length) {
        return 0u;
    }

    /* Build the fixed header with an alignment-safe big-endian sequence field. */
    output[0] = DEVICE_LINK_SOF;
    output[1] = command;
    ec_write_be16(&output[2], sequence);
    output[4] = payload_length;

    if (payload_length != 0u) {
        (void)memcpy(&output[5], payload, (size_t)payload_length);
    }

    /* The checksum covers command through the final payload byte, excluding SOF. */
    for (index = 1u; index < (frame_length - 1u); index++) {
        checksum ^= output[index];
    }
    output[frame_length - 1u] = (uint8_t)checksum;
    return frame_length;
}

/**
 * @brief Bind the example to a logger and transmit arbiter and reset all state.
 *
 * The function clears every counter and queue, binds caller-owned storage to the
 * ring buffer and buffer pool, and stores the optional logger and required
 * transmitter. It does not transmit or enter any RTOS critical section.
 *
 * @param link Caller-owned example state.
 * @param logger Optional initialized logger; NULL disables diagnostic records.
 * @param tx Initialized transmit arbiter whose writer remains owned by the caller.
 * @return One when initialization succeeds, or zero for a NULL link or transmitter.
 * @pre The caller has completed any required platform and logger initialization.
 * @post The link is empty and ready to accept bytes through device_link_feed().
 * @note Call during single-threaded startup before other contexts can access link.
 * @author Xue Weisen
 */
int device_link_init(device_link_t *link, ec_log_t *logger, ec_tx_arbiter_t *tx)
{
    uint32_t index;

    if ((link == NULL) || (tx == NULL)) {
        return 0;
    }

    (void)memset(link, 0, sizeof(*link));
    link->logger = logger;
    link->tx = tx;

    /* Bind every storage region once; runtime operations never allocate or rebind it. */
    ringbuf_init(&link->rx, link->rx_storage, DEVICE_LINK_RX_CAPACITY);
    ec_buf_pool_init(&link->tx_pool,
                     link->tx_storage,
                     link->tx_slots,
                     DEVICE_LINK_TX_SLOT_SIZE,
                     DEVICE_LINK_TX_SLOT_COUNT);
    ec_xfer_queue_init(&link->tx_queue,
                       link->tx_items,
                       DEVICE_LINK_TX_QUEUE_DEPTH);

    for (index = 0u; index < DEVICE_LINK_TX_SLOT_COUNT; index++) {
        link->tx_release[index].owner = link;
        link->tx_release[index].slot_index = index;
        link->tx_release[index].generation = 0u;
    }

    device_link_log(link, EC_LOG_INFO, "device link ready");
    return 1;
}

/**
 * @brief Append received bytes to the fixed-capacity RX ring.
 *
 * Bytes are copied into caller-owned storage. If the ring becomes full, the
 * remaining suffix is rejected, counted once in rx_drops, and the successfully
 * copied prefix remains available to the parser.
 *
 * @param link Initialized link state.
 * @param data Borrowed RX bytes; may be NULL only when length is zero.
 * @param length Number of bytes to append.
 * @return One when every byte was accepted, or zero for invalid input or a full ring.
 * @pre The caller serializes this function with other ring mutation operations.
 * @note ISR use is possible only when the application provides the documented
 *       single-producer/single-consumer discipline and protects reset paths.
 * @author Xue Weisen
 */
int device_link_feed(device_link_t *link, const uint8_t *data, uint32_t length)
{
    uint32_t index;

    if ((link == NULL) || ((data == NULL) && (length != 0u))) {
        return 0;
    }

    /* A full ring keeps the accepted prefix and reports the rejected suffix once. */
    for (index = 0u; index < length; index++) {
        if (ringbuf_push(&link->rx, data[index]) == 0) {
            link->stats.rx_drops += length - index;
            device_link_log(link, EC_LOG_WARN, "rx ring full");
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Consume all complete, checksum-valid frames currently available.
 *
 * The parser resynchronizes one byte at a time after noise, invalid lengths, or
 * checksum errors. An incomplete header or payload stays queued for the next
 * call. Each valid command queues a response descriptor instead of writing
 * directly, so transmit backpressure cannot block RX parsing.
 *
 * @param link Initialized link state.
 * @return Number of complete frames handled during this call.
 * @pre Feed and poll may run in different contexts only when the ring is used as
 *      a single-producer/single-consumer queue and the application serializes resets.
 * @note The function performs no blocking waits and allocates no memory.
 * @author Xue Weisen
 */
uint32_t device_link_poll(device_link_t *link)
{
    uint8_t frame[5u + DEVICE_LINK_MAX_PAYLOAD + 1u];
    uint8_t first_byte;
    uint8_t payload_length;
    uint32_t checksum;
    uint32_t frame_length;
    uint32_t handled = 0u;
    uint32_t index;

    if (link == NULL) {
        return 0u;
    }

    while (ringbuf_count(&link->rx) != 0u) {
        /* Resynchronization: discard one non-SOF byte without disturbing later candidates. */
        if (ringbuf_peek(&link->rx, 0u, &first_byte) == 0) {
            break;
        }
        if (first_byte != DEVICE_LINK_SOF) {
            (void)ringbuf_drop(&link->rx, 1u);
            link->stats.framing_errors++;
            continue;
        }

        /* Header gate: wait for all five bytes before trusting the payload length. */
        if (ringbuf_count(&link->rx) < 5u) {
            break;
        }
        if ((ringbuf_peek(&link->rx, 4u, &payload_length) == 0) ||
            (payload_length > DEVICE_LINK_MAX_PAYLOAD)) {
            (void)ringbuf_drop(&link->rx, 1u);
            link->stats.framing_errors++;
            device_link_log(link, EC_LOG_WARN, "invalid frame length");
            continue;
        }

        frame_length = 5u + (uint32_t)payload_length + 1u;
        if (ringbuf_count(&link->rx) < frame_length) {
            break;
        }

        /* Copy the complete candidate out of the ring before validation and dispatch. */
        for (index = 0u; index < frame_length; index++) {
            if (ringbuf_peek(&link->rx, index, &frame[index]) == 0) {
                break;
            }
        }
        if (index != frame_length) {
            break;
        }

        checksum = 0u;
        for (index = 1u; index < (frame_length - 1u); index++) {
            checksum ^= frame[index];
        }
        if ((uint8_t)checksum != frame[frame_length - 1u]) {
            (void)ringbuf_drop(&link->rx, 1u);
            link->stats.checksum_errors++;
            device_link_log(link, EC_LOG_WARN, "frame checksum rejected");
            continue;
        }

        /* Consume exactly one validated frame before queueing its response. */
        (void)ringbuf_drop(&link->rx, frame_length);
        link->stats.valid_frames++;
        if (device_link_handle_frame(link, frame, frame_length) != 0) {
            handled++;
        }
    }

    link->stats.handled_frames += handled;
    return handled;
}
/**
 * @brief Transmit queued response descriptors through the frame arbiter.
 *
 * The function starts one frame at a time, resumes at the exact byte after a
 * writer backpressure failure, and releases the pooled slot only after the full
 * frame is accepted. It never mixes text output into an active frame.
 *
 * @param link Initialized link state.
 * @return Number of response descriptors fully transmitted during this call.
 * @pre device_link_init() succeeded and the arbiter writer belongs to this link's transport.
 * @note Call from one TX-owner context. The function does not wait or retry internally.
 * @author Xue Weisen
 */
uint32_t device_link_flush(device_link_t *link)
{
    uint32_t completed = 0u;

    if ((link == NULL) || (link->tx == NULL)) {
        return 0u;
    }

    while (ec_xfer_queue_count(&link->tx_queue) != 0u) {
        /* Start a frame only once; partial writes resume from tx_offset on the next call. */
        if (link->tx_active == 0) {
            if (ec_xfer_queue_peek(&link->tx_queue, &link->active_xfer) == 0) {
                break;
            }
            if (ec_tx_arbiter_begin_frame(link->tx) == 0) {
                break;
            }
            link->tx_offset = 0u;
            link->tx_active = 1;
        }

        while (link->tx_offset < link->active_xfer.length) {
            if (ec_tx_arbiter_frame_byte(link->tx,
                                         link->active_xfer.data[link->tx_offset]) == 0) {
                return completed;
            }

            link->tx_offset++;
            link->stats.bytes_transmitted++;
        }

        /* Commit only after every byte was accepted; release then returns the pooled slot. */
        ec_tx_arbiter_end_frame(link->tx);
        if (ec_xfer_queue_pop(&link->tx_queue, &link->active_xfer) == 0) {
            device_link_log(link, EC_LOG_ERROR, "tx queue commit failed");
            link->tx_active = 0;
            break;
        }

        if (ec_xfer_release(&link->active_xfer) == 0) {
            link->stats.release_errors++;
            device_link_log(link, EC_LOG_ERROR, "tx slot release failed");
        }

        link->tx_active = 0;
        link->tx_offset = 0u;
        link->stats.responses_sent++;
        completed++;
    }

    return completed;
}