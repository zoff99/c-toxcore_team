#ifndef TOXCORE_TEST_HELPERS_H
#define TOXCORE_TEST_HELPERS_H

#include "../toxcore/tox.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <windows.h>
#define c_sleep(x) Sleep(x)
#else
#include <unistd.h>
#define c_sleep(x) usleep(1000 * (x))
#endif

#define DEFTESTCASE(NAME)                   \
    TCase *tc_##NAME = tcase_create(#NAME); \
    tcase_add_test(tc_##NAME, test_##NAME); \
    suite_add_tcase(s, tc_##NAME)

#define DEFTESTCASE_SLOW(NAME, TIMEOUT)     \
    DEFTESTCASE(NAME);                      \
    tcase_set_timeout(tc_##NAME, TIMEOUT)

static const char *tox_log_level_name(TOX_LOG_LEVEL level)
{
    switch (level) {
        case TOX_LOG_LEVEL_TRACE:
            return "TRACE";

        case TOX_LOG_LEVEL_DEBUG:
            return "DEBUG";

        case TOX_LOG_LEVEL_INFO:
            return "INFO";

        case TOX_LOG_LEVEL_WARNING:
            return "WARNING";

        case TOX_LOG_LEVEL_ERROR:
            return "ERROR";
    }
}

static void print_debug_log(Tox *m, TOX_LOG_LEVEL level, const char *path, uint32_t line, const char *func,
                            const char *message, void *user_data)
{
    if (level == TOX_LOG_LEVEL_TRACE) {
        return;
    }

    uint32_t index = user_data ? *(uint32_t *)user_data : 0;
    const char *file = strrchr(path, '/');
    file = file ? file + 1 : path;
    printf("[#%d] %s %s:%d\t%s:\t%s\n", index, tox_log_level_name(level), file, line, func, message);
}

Tox *tox_new_log(struct Tox_Options *options, TOX_ERR_NEW *err, void *log_user_data)
{
    struct Tox_Options *log_options = options;

    if (log_options == NULL) {
        log_options = tox_options_new(NULL);
    }

    assert(log_options != NULL);

    tox_options_set_log_callback(log_options, &print_debug_log);
    tox_options_set_log_user_data(log_options, log_user_data);
    Tox *tox = tox_new(log_options, err);

    if (options == NULL) {
        tox_options_free(log_options);
    }

    return tox;
}

#endif // TOXCORE_TEST_HELPERS_H
