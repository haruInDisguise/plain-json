#ifndef _TEST_CONFIG_H_
#define _TEST_CONFIG_H_

#include <string.h>

#include <test/test.h>

#include "../json.h"

#define TEST_VALUE_BUFFER_SIZE 64
#define TEST_KEY_BUFFER_SIZE   64

static char *key_buffer[TEST_KEY_BUFFER_SIZE] = { 0 };
static char *value_buffer[TEST_VALUE_BUFFER_SIZE] = { 0 };
static json_Context context = { 0 };
static json_Token token = { 0 };

static void test_reset_context(void) {
    memset(&context, 0, sizeof(context));
    memset(&token, 0, sizeof(token));
    memset(value_buffer, 0, sizeof(value_buffer));
    memset(key_buffer, 0, sizeof(key_buffer));
    json_setup(&context);
    json_token_setup(&token, (char *)key_buffer, TEST_KEY_BUFFER_SIZE, (char *)value_buffer, TEST_VALUE_BUFFER_SIZE);
}

#endif
