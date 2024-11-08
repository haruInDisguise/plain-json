#ifndef _TEST_CONFIG_H_
#define _TEST_CONFIG_H_

#include <stdlib.h>
#include <string.h>

#include <test/test.h>

#include "../plain_json.h"

#define TEST_VALUE_BUFFER_SIZE 128
__attribute__((unused))
static char value_buffer[TEST_VALUE_BUFFER_SIZE];

static plain_json_Context context = { 0 };
static plain_json_Token token = { 0 };

static void test_reset_context(void) {
    memset(&context, 0, sizeof(context));
    memset(&token, 0, sizeof(token));
    plain_json_setup(&context);
}

#define test_strcmp(buffer, string, length)                                        \
    do {                                                                           \
        test_assert((length) <= TEST_VALUE_BUFFER_SIZE);                           \
        memset(value_buffer, 0, TEST_VALUE_BUFFER_SIZE * sizeof(value_buffer[0])); \
        memcpy(value_buffer, buffer, length);                                      \
        /* I use memcmp to also validate the returned length */                    \
        test_assert_eq(memcmp(value_buffer, string, length), 0);                   \
    } while (0)

#endif
