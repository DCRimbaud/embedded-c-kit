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
