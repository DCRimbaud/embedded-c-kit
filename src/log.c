/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/log.h"

#include <stddef.h>

void ec_log_init(ec_log_t *logger)
{
    if (logger == NULL) {
        return;
    }

    logger->sink = NULL;
    logger->sink_context = NULL;
    logger->minimum_level = EC_LOG_INFO;
    logger->quiet_depth = 0u;
}

void ec_log_set_sink(ec_log_t *logger,
                     ec_log_sink_fn sink,
                     void *context)
{
    if (logger == NULL) {
        return;
    }

    logger->sink = sink;
    logger->sink_context = context;
}

void ec_log_set_minimum_level(ec_log_t *logger, ec_log_level_t minimum_level)
{
    if (logger != NULL) {
        logger->minimum_level = minimum_level;
    }
}

void ec_log_quiet_begin(ec_log_t *logger)
{
    if (logger != NULL) {
        logger->quiet_depth++;
    }
}

void ec_log_quiet_end(ec_log_t *logger)
{
    if ((logger != NULL) && (logger->quiet_depth > 0u)) {
        logger->quiet_depth--;
    }
}

uint32_t ec_log_quiet_depth(const ec_log_t *logger)
{
    return (logger != NULL) ? logger->quiet_depth : 0u;
}

int ec_log_write(ec_log_t *logger,
                 ec_log_level_t level,
                 const char *category,
                 const char *message)
{
    ec_log_record_t record;

    if ((logger == NULL) || (logger->sink == NULL) || (logger->quiet_depth > 0u) ||
        (category == NULL) || (message == NULL) ||
        ((uint32_t)level > (uint32_t)logger->minimum_level)) {
        return 0;
    }

    record.level = level;
    record.category = category;
    record.message = message;
    logger->sink(logger->sink_context, &record);
    return 1;
}
