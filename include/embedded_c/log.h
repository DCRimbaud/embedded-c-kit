/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#ifndef EMBEDDED_C_LOG_H
#define EMBEDDED_C_LOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Log severity ordered from most to least severe. */
typedef enum {
    EC_LOG_ERROR = 0,
    EC_LOG_WARN = 1,
    EC_LOG_INFO = 2,
    EC_LOG_DEBUG = 3
} ec_log_level_t;

/** Borrowed log record passed to a sink for the duration of the call. */
typedef struct {
    ec_log_level_t level;
    const char *category;
    const char *message;
} ec_log_record_t;

/** Log sink callback. The record and strings are borrowed only during the call. */
typedef void (*ec_log_sink_fn)(void *context, const ec_log_record_t *record);

/** Logger state. Callers serialize concurrent writes. */
typedef struct {
    ec_log_sink_fn sink;
    void *sink_context;
    ec_log_level_t minimum_level;
    uint32_t quiet_depth;
} ec_log_t;

/** Initialize a logger with no sink and INFO filtering. */
void ec_log_init(ec_log_t *logger);

/** Bind or clear the sink callback. */
void ec_log_set_sink(ec_log_t *logger,
                     ec_log_sink_fn sink,
                     void *context);

/** Set the minimum accepted severity. */
void ec_log_set_minimum_level(ec_log_t *logger, ec_log_level_t minimum_level);

/** Enter a nested quiet window; records are suppressed until the matching exit. */
void ec_log_quiet_begin(ec_log_t *logger);

/** Exit one quiet window. Extra exits never underflow. */
void ec_log_quiet_end(ec_log_t *logger);

/** Return the current quiet depth. */
uint32_t ec_log_quiet_depth(const ec_log_t *logger);

/**
 * Emit one record when a sink is bound, the level is accepted, and the logger is not quiet.
 *
 * @return 1 when delivered to the sink, 0 otherwise.
 */
int ec_log_write(ec_log_t *logger,
                 ec_log_level_t level,
                 const char *category,
                 const char *message);

#ifdef __cplusplus
}
#endif

#endif
