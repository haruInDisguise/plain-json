#include <test/test.h>

#include "test_config.h"

#include "../json.h"

test_build_context()

SUIT(value_parsing, test_reset_context, NULL);

TEST(value_parsing, string_unterminated) {
    const char *text = "[\"ohhh no!]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_STRING_UNTERMINATED);
}

TEST(value_parsing, string_too_long) {
    const char *text = "[\"ohhh no!\"]";

    json_load_buffer(&context, text, strlen(text));
    token.string_buffer_size = 2;

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_NO_MEMORY);
}

TEST(value_parsing, string_with_control) {
    const char *text = "[\\\"\\n\\t\\r\"]";

    json_load_buffer(&context, text, strlen(text));
    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_STRING_INVALID);
}

/* FIXME: Actually validate the parsed string */
TEST(value_parsing, string_with_escapes) {
    const char *text = "[\"\\n\\r\\t\\ \\\f\"]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
}
