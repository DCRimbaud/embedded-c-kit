/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#ifndef DEVICE_LINK_EXAMPLE_H
#define DEVICE_LINK_EXAMPLE_H

#include "embedded_c/buf_pool.h"
#include "embedded_c/log.h"
#include "embedded_c/ringbuf.h"
#include "embedded_c/tx_arbiter.h"
#include "embedded_c/xfer.h"
#include "embedded_c/xfer_queue.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_LINK_SOF 0x5Au              /**< Frame start byte used by the example protocol. */
#define DEVICE_LINK_MAX_PAYLOAD 32u       /**< Maximum payload bytes accepted in one frame. */
#define DEVICE_LINK_RX_CAPACITY 128u      /**< Fixed RX ring capacity in bytes. */
#define DEVICE_LINK_TX_SLOT_COUNT 4u      /**< Number of pooled response buffers. */
#define DEVICE_LINK_TX_SLOT_SIZE 64u      /**< Capacity of one pooled response buffer. */
#define DEVICE_LINK_TX_QUEUE_DEPTH 4u     /**< Maximum queued response descriptors. */

/** Counters exposed by the example for host verification and diagnostics. */
typedef struct {
    uint32_t valid_frames;          /**< Frames that passed length and checksum validation. */
    uint32_t handled_frames;        /**< Valid frames whose response was queued successfully. */
    uint32_t checksum_errors;       /**< Candidate frames rejected by checksum validation. */
    uint32_t framing_errors;        /**< Noise bytes and invalid length headers discarded. */
    uint32_t unsupported_commands;  /**< Valid frames whose command ID had no handler. */
    uint32_t rx_drops;              /**< Bytes rejected because the RX ring was full. */
    uint32_t responses_queued;      /**< Response descriptors accepted by the TX queue. */
    uint32_t responses_sent;        /**< Response descriptors fully transmitted. */
    uint32_t response_drops;        /**< Responses that could not be encoded or queued. */
    uint32_t release_errors;        /**< Pooled slots whose generation release failed. */
    uint32_t bytes_transmitted;     /**< Frame bytes accepted by the transport writer. */
} device_link_stats_t;

typedef struct device_link device_link_t;

/** Per-slot context used by the transfer descriptor release callback. */
typedef struct {
    device_link_t *owner;
    uint32_t slot_index;
    uint32_t generation;
} device_link_tx_release_t;

/**
 * Self-contained example state.
 *
 * All storage is caller-owned and statically sized. The public fields are kept
 * visible so the example can be copied into firmware without a heap allocator.
 * Production code should normally keep these fields private to its .c file.
 */
struct device_link {
    ringbuf_t rx;                                      /**< Incoming byte ring and its public state. */
    uint8_t rx_storage[DEVICE_LINK_RX_CAPACITY];       /**< Caller-owned storage for the RX ring. */
    ec_buf_pool_t tx_pool;                             /**< Fixed response-slot allocator. */
    ec_pool_slot_t tx_slots[DEVICE_LINK_TX_SLOT_COUNT];/**< Per-slot generation metadata. */
    uint8_t tx_storage[DEVICE_LINK_TX_SLOT_COUNT * DEVICE_LINK_TX_SLOT_SIZE]; /**< Pooled response bytes. */
    device_link_tx_release_t tx_release[DEVICE_LINK_TX_SLOT_COUNT]; /**< Release callback contexts. */
    ec_xfer_t tx_items[DEVICE_LINK_TX_QUEUE_DEPTH];    /**< Descriptor storage owned by the TX queue. */
    ec_xfer_queue_t tx_queue;                          /**< Fixed FIFO of response descriptors. */
    ec_xfer_t active_xfer;                             /**< Descriptor currently being transmitted. */
    uint32_t tx_offset;                                /**< Next byte index within active_xfer. */
    int tx_active;                                     /**< Non-zero while a frame owns the writer. */
    ec_log_t *logger;                                  /**< Borrowed optional logger. */
    ec_tx_arbiter_t *tx;                               /**< Borrowed transmit arbiter. */
    device_link_stats_t stats;                         /**< Counters for diagnostics and host tests. */
};

/**
 * @brief Encode one product-neutral demonstration frame.
 *
 * The frame layout is SOF, command, big-endian sequence, payload length,
 * payload bytes, then an XOR checksum over command through the last payload
 * byte. The function only writes complete frames and never partially modifies
 * the output on failure.
 *
 * @param command One-byte command identifier.
 * @param sequence Caller-supplied sequence echoed by the response.
 * @param payload Borrowed payload bytes; may be NULL only when payload_length is zero.
 * @param payload_length Payload length in bytes; zero through DEVICE_LINK_MAX_PAYLOAD.
 * @param output Caller-owned output storage; must hold at least five bytes plus payload and checksum.
 * @param output_capacity Capacity of output in bytes.
 * @return Total frame length on success, or zero for invalid arguments or insufficient capacity.
 * @pre The caller has selected output storage with static or otherwise proven lifetime.
 * @note The function does not allocate, block, or retain any pointer.
 * @author Xue Weisen
 */
uint32_t device_link_encode_frame(uint8_t command,
                                  uint16_t sequence,
                                  const uint8_t *payload,
                                  uint8_t payload_length,
                                  uint8_t *output,
                                  uint32_t output_capacity);

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
int device_link_init(device_link_t *link, ec_log_t *logger, ec_tx_arbiter_t *tx);

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
int device_link_feed(device_link_t *link, const uint8_t *data, uint32_t length);

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
uint32_t device_link_poll(device_link_t *link);

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
uint32_t device_link_flush(device_link_t *link);

#ifdef __cplusplus
}
#endif

#endif
