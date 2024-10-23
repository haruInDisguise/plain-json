#include <test/test.h>

#include "test_config.h"

#include "../json.h"

test_build_context()

SUIT(value_parsing, test_reset_context, NULL);

/* FIXME: Actually validate the parsed string */
TEST(value_parsing, string_escapes) {
    const char *text = "[\"\\n\\r\\t\\f\"]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_eq(token.type, JSON_TYPE_STRING);
    test_assert_string_eq(token.string_buffer, "\\n\\r\\t\\f");

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_FALSE);
}

TEST(value_parsing, string_unicode) {
    const char *text = "[\"\\uabcd\"]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_string_eq(token.string_buffer, "\\uabcd");

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

SUIT(value_parsing_malformed, test_reset_context, NULL);

TEST(value_parsing_malformed, string_unterminated) {
    const char *text = "[\"ohhh no!\n]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_STRING_UNTERMINATED);
}

TEST(value_parsing_malformed, string_eof) {
    const char *text = "[\"ohhh no!]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_UNEXPECTED_EOF);
}

TEST(value_parsing_malformed, string_buffer_too_short) {
    const char *text = "[\"ohhh no!\"]";

    json_load_buffer(&context, text, strlen(text));
    token.string_buffer_size = 2;

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NO_MEMORY);
}

TEST(value_parsing_malformed, string_control) {
    const char *text = "[\"\t\r\"]";

    json_load_buffer(&context, text, strlen(text));
    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_STRING_INVALID);
}

TEST(value_parsing_malformed, string_unicode_invalid) {
    const char *text = "[\"\\uy\"]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_STRING_INVALID_HEX);
}

TEST(value_parsing_malformed, string_unicode_eof) {
    const char *text = "[\"\\u";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_UNEXPECTED_EOF);
}

TEST(value_parsing_malformed, string_unicode_too_short) {
    const char *text = "[\"\\u";

    json_load_buffer(&context, text, strlen(text));
    token.string_buffer_size = 2;

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_NO_MEMORY);
}
