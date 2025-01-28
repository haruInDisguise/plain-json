#include <stdio.h>

#include "test_setup.h"

SUIT(number, NULL, test_finalize);

#define TEST_NUMBER(name, result, encoded)                                     \
    TEST(number, name) {                                                       \
        const char *text = #encoded;                                           \
        plain_json_ErrorType status = PLAIN_JSON_DONE;                         \
        context =                                          \
            plain_json_parse(alloc_config, (uint8_t *)text, strlen(text), &status); \
        test_assert_eq(status, result);                                        \
    }


TEST_NUMBER(max_positive, PLAIN_JSON_DONE, 9223372036854775807)
TEST_NUMBER(overflow, PLAIN_JSON_ERROR_NUMBER_OVERFLOW, 9223372036854775808)
TEST_NUMBER(overflow_large, PLAIN_JSON_ERROR_NUMBER_OVERFLOW, 123123123123123123123)

TEST_NUMBER(max_negative, PLAIN_JSON_DONE, -9223372036854775808)
TEST_NUMBER(underflow, PLAIN_JSON_ERROR_NUMBER_UNDERFLOW, -9223372036854775809)
TEST_NUMBER(underflow_large, PLAIN_JSON_ERROR_NUMBER_UNDERFLOW, -123123123123123123123)
