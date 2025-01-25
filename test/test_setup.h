#ifndef _TEST_CONFIG_H_
#define _TEST_CONFIG_H_

#include "../plain_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <test/test.h>

#define TEST_VALUE_BUFFER_SIZE 128
__attribute__((unused)) static char value_buffer[TEST_VALUE_BUFFER_SIZE];

static plain_json_Context context = { 0 };
static plain_json_AllocatorConfig alloc_config = {
    .alloc_func = malloc,
    .free_func = free,
    .realloc_func = realloc,
};

static void test_finalize(void) {
    plain_json_free(&context);
    memset(&context, 0, sizeof(context));
}

#define test_json(buffer, status)                                                                 \
    do {                                                                                          \
        test_assert_eq(plain_json_parse(&context, alloc_conf, buffer, strlen(buffer)), (status)); \
    } while (0)

#endif
