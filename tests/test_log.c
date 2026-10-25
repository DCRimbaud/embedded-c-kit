/*
 * SPDX-License-Identifier: MIT
 * Author: Xue Weisen <xueweisen96@gmail.com>
 * GitHub: https://github.com/DCRimbaud
 */
#include "embedded_c/log.h"

#include <assert.h>
#include <stdio.h>

typedef struct {
    unsigned int count;
    ec_log_record_t last;
} fake_sink_t;

static void capture(void *context, const ec_log_record_t *record)
{
    fake_sink_t *sink = (fake_sink_t *)context;
    sink->count++;
    sink->last = *record;
}

int main(void)
{
    ec_log_t logger;
    fake_sink_t sink = {0};

    ec_log_init(&logger);
    ec_log_set_sink(&logger, capture, &sink);
    ec_log_set_minimum_level(&logger, EC_LOG_INFO);

    assert(ec_log_write(&logger, EC_LOG_DEBUG, "m", "hidden") == 0);
    assert(ec_log_write(&logger, EC_LOG_INFO, "m", "shown") == 1);
    assert(sink.count == 1u);
    assert(sink.last.level == EC_LOG_INFO);
    assert(sink.last.message[0] == 's');

    ec_log_quiet_begin(&logger);
    ec_log_quiet_begin(&logger);
    assert(ec_log_quiet_depth(&logger) == 2u);
    assert(ec_log_write(&logger, EC_LOG_ERROR, "m", "quiet") == 0);
    ec_log_quiet_end(&logger);
    ec_log_quiet_end(&logger);
    ec_log_quiet_end(&logger);
    assert(ec_log_quiet_depth(&logger) == 0u);
    assert(ec_log_write(&logger, EC_LOG_ERROR, "m", "after") == 1);
    assert(sink.count == 2u);

    puts("log tests: PASS");
    return 0;
}
