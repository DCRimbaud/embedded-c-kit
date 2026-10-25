/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "device_link.h"

#include <stdio.h>
#include <string.h>

#define CAPTURE_CAPACITY 256u
#define EXPECTED_CAPACITY 256u

/** Deterministic byte sink used to prove the example without a UART. */
typedef struct {
    uint8_t bytes[CAPTURE_CAPACITY];
    uint32_t count;
} capture_t;

/**
 * @brief Accept one byte into the deterministic transmit capture.
 *
 * A full capture returns backpressure so device_link_flush() can be tested for
 * resume behavior. The context is borrowed and must remain valid for the call.
 *
 * @param context Pointer to capture_t.
 * @param byte Byte offered by the transmit arbiter.
 * @return One when stored, or zero when the capture is full or the context is NULL.
 * @note Not thread-safe; the example owns the capture from one thread.
 * @author Xue Weisen
 */
static int capture_write(void *context, uint8_t byte)
{
    capture_t *capture = (capture_t *)context;

    if ((capture == NULL) || (capture->count >= CAPTURE_CAPACITY)) {
        return 0;
    }

    capture->bytes[capture->count] = byte;
    capture->count++;
    return 1;
}

/**
 * @brief Convert a log level to a stable console label.
 *
 * @param level Logger severity.
 * @return Static English label; unknown values return "UNKNOWN".
 * @note Borrowed return; the caller must not free it.
 * @author Xue Weisen
 */
static const char *log_level_name(ec_log_level_t level)
{
    switch (level) {
    case EC_LOG_ERROR:
        return "ERROR";
    case EC_LOG_WARN:
        return "WARN";
    case EC_LOG_INFO:
        return "INFO";
    case EC_LOG_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Forward one logger record to stdout for the host demonstration.
 *
 * @param context Unused sink context.
 * @param record Borrowed record valid only for this synchronous call.
 * @return Nothing.
 * @note The sink blocks on stdio and is not intended for an ISR.
 * @author Xue Weisen
 */
static void console_sink(void *context, const ec_log_record_t *record)
{
    (void)context;
    if (record == NULL) {
        return;
    }

    (void)printf("[%s] %s: %s\n",
                 log_level_name(record->level),
                 (record->category != NULL) ? record->category : "unknown",
                 (record->message != NULL) ? record->message : "");
}

/**
 * @brief Feed a byte range through the RX ring and verify acceptance.
 *
 * @param link Initialized device link.
 * @param data Borrowed frame bytes.
 * @param length Number of bytes to feed.
 * @return One when all bytes were accepted, or zero on a feed failure.
 * @note Called from the example's single producer context.
 * @author Xue Weisen
 */
static int feed_exact(device_link_t *link, const uint8_t *data, uint32_t length)
{
    return device_link_feed(link, data, length);
}

/**
 * @brief Append one encoded frame to a caller-owned expected-byte buffer.
 *
 * @param destination Output descriptor containing bytes and current length.
 * @param command Response command identifier.
 * @param sequence Response sequence.
 * @param payload Borrowed payload bytes.
 * @param payload_length Payload length.
 * @return One on success, or zero when encoding or capacity validation fails.
 * @pre destination->count is the current used prefix and remains unchanged on failure.
 * @note The helper is intentionally local to the example verification path.
 * @author Xue Weisen
 */
static int append_expected_frame(capture_t *destination,
                                 uint8_t command,
                                 uint16_t sequence,
                                 const uint8_t *payload,
                                 uint8_t payload_length)
{
    uint32_t encoded;

    if ((destination == NULL) || (destination->count > EXPECTED_CAPACITY)) {
        return 0;
    }

    encoded = device_link_encode_frame(command,
                                       sequence,
                                       payload,
                                       payload_length,
                                       &destination->bytes[destination->count],
                                       EXPECTED_CAPACITY - destination->count);
    if (encoded == 0u) {
        return 0;
    }

    destination->count += encoded;
    return 1;
}

/**
 * @brief Run the complete host-side integration demonstration.
 *
 * The scenario feeds a split ping frame, rejects a corrupted echo frame, accepts
 * a corrected echo, sends an unsupported command, flushes all queued responses,
 * and compares the transmitted bytes against independently assembled expected
 * frames. Each phase is separated so a future project can replace one policy
 * without copying the whole example blindly.
 *
 * @return Zero when every integration invariant passes, or one on failure.
 * @pre The host C runtime and stdio are available; no target hardware is required.
 * @note This is a host example, not a production application entry point. It uses
 *       one thread and deliberately exercises parser resynchronization.
 * @author Xue Weisen
 */
int main(void)
{
    static const uint8_t pong[] = {'P', 'O', 'N', 'G'};
    static const uint8_t echo_payload[] = {'a', 'b', 'c'};
    static const uint8_t unknown_payload[] = {'x'};
    static const uint8_t error_payload[] = {'E', 'R', 'R'};
    uint8_t ping_frame[64];
    uint8_t echo_frame[64];
    uint8_t corrupt_frame[64];
    uint8_t unknown_frame[64];
    uint32_t ping_length;
    uint32_t echo_length;
    uint32_t unknown_length;
    capture_t capture = {0};
    capture_t expected = {0};
    ec_log_t logger;
    ec_tx_arbiter_t tx;
    device_link_t link;

    /* Initialization phase: bind a deterministic writer, logger, and all fixed storage. */
    ec_log_init(&logger);
    ec_log_set_sink(&logger, console_sink, NULL);
    ec_log_set_minimum_level(&logger, EC_LOG_DEBUG);
    ec_tx_arbiter_init(&tx, capture_write, &capture);
    if (device_link_init(&link, &logger, &tx) == 0) {
        (void)fprintf(stderr, "device link example: initialization failed\n");
        return 1;
    }

    /* Input phase: encode one request for each supported demonstration path. */
    ping_length = device_link_encode_frame(0x01u, 0x1234u, NULL, 0u,
                                           ping_frame, (uint32_t)sizeof(ping_frame));
    echo_length = device_link_encode_frame(0x02u, 0x1235u,
                                           echo_payload,
                                           (uint8_t)sizeof(echo_payload),
                                           echo_frame,
                                           (uint32_t)sizeof(echo_frame));
    unknown_length = device_link_encode_frame(0x55u, 0x1236u,
                                              unknown_payload,
                                              (uint8_t)sizeof(unknown_payload),
                                              unknown_frame,
                                              (uint32_t)sizeof(unknown_frame));
    if ((ping_length == 0u) || (echo_length == 0u) || (unknown_length == 0u)) {
        (void)fprintf(stderr, "device link example: request encoding failed\n");
        return 1;
    }

    /* Partial-frame phase: the parser must wait until the complete ping arrives. */
    if (feed_exact(&link, ping_frame, 4u) == 0) {
        return 1;
    }
    if (device_link_poll(&link) != 0u) {
        (void)fprintf(stderr, "device link example: incomplete frame was consumed\n");
        return 1;
    }
    if (feed_exact(&link, &ping_frame[4], ping_length - 4u) == 0) {
        return 1;
    }
    if (device_link_poll(&link) != 1u) {
        (void)fprintf(stderr, "device link example: ping frame was not handled\n");
        return 1;
    }

    /* Recovery phase: corrupt one checksum, then prove the parser resynchronizes on the next frame. */
    (void)memcpy(corrupt_frame, echo_frame, echo_length);
    corrupt_frame[echo_length - 1u] ^= 0xFFu;
    if (feed_exact(&link, corrupt_frame, echo_length) == 0) {
        return 1;
    }
    if (device_link_poll(&link) != 0u) {
        (void)fprintf(stderr, "device link example: corrupted frame was accepted\n");
        return 1;
    }
    if (feed_exact(&link, echo_frame, echo_length) == 0) {
        return 1;
    }
    if (device_link_poll(&link) != 1u) {
        (void)fprintf(stderr, "device link example: echo frame was not handled\n");
        return 1;
    }

    /* Unsupported-command phase: the dispatcher must produce one explicit error response. */
    if (feed_exact(&link, unknown_frame, unknown_length) == 0) {
        return 1;
    }
    if (device_link_poll(&link) != 1u) {
        (void)fprintf(stderr, "device link example: unknown command was not handled\n");
        return 1;
    }

    /* Transmit phase: flush all descriptors and release every pooled slot. */
    if (device_link_flush(&link) != 3u) {
        (void)fprintf(stderr, "device link example: response flush failed\n");
        return 1;
    }
    if (ec_buf_pool_in_use(&link.tx_pool) != 0u) {
        (void)fprintf(stderr, "device link example: pooled slot leak\n");
        return 1;
    }

    /* Verification phase: compare exact output bytes and the observable counters. */
    if ((append_expected_frame(&expected, 0x81u, 0x1234u,
                               pong, (uint8_t)sizeof(pong)) == 0) ||
        (append_expected_frame(&expected, 0x82u, 0x1235u,
                               echo_payload, (uint8_t)sizeof(echo_payload)) == 0) ||
        (append_expected_frame(&expected, 0xFFu, 0x1236u,
                               error_payload, (uint8_t)sizeof(error_payload)) == 0)) {
        return 1;
    }

    if ((capture.count != expected.count) ||
        (memcmp(capture.bytes, expected.bytes, capture.count) != 0)) {
        (void)fprintf(stderr, "device link example: response bytes differ\n");
        return 1;
    }

    if ((link.stats.valid_frames != 3u) ||
        (link.stats.handled_frames != 3u) ||
        (link.stats.checksum_errors != 1u) ||
        (link.stats.unsupported_commands != 1u) ||
        (link.stats.responses_queued != 3u) ||
        (link.stats.responses_sent != 3u) ||
        (link.stats.release_errors != 0u)) {
        (void)fprintf(stderr, "device link example: counter invariant failed\n");
        return 1;
    }

    (void)printf("device link example: PASS\n");
    return 0;
}