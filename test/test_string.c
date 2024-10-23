#include <test/test.h>

#include "test_config.h"

#include "../json.h"

test_build_context()

SUIT(value_parsing, test_reset_context, NULL);

TEST(value_parsing, string_unterminated) {
    const char *text = "[ \"a_string]";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_STRING_UNTERMINATED);
}

TEST(value_parsing, string_too_long) {
    const char *text = "[ \"a_string\"]";
    json_load_buffer(&context, text, strlen(text));

    token.value_buffer_size = 0;

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NO_MEMORY);
}
