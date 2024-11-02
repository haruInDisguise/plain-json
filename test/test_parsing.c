#include "test_setup.h"

SUIT(parsing, test_reset_context, NULL);

TEST(parsing, string_escapes) {
    const char *text = "[\"\\\"\\\\n\\r\\t\\f\"]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_eq(token.type, JSON_TYPE_STRING);
    test_assert_string_eq(token.value_buffer, "\\\"\\\\n\\r\\t\\f");

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_FALSE);
}

TEST(parsing, string_escapes_backslash) {
    const char *text = "[\"\xEE\xBC\xB7\"]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_eq(token.type, JSON_TYPE_STRING);
    test_assert_string_eq(token.value_buffer, "\xEE\xBC\xB7");

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_FALSE);
}

TEST(parsing, string_unicode_escape) {
    const char *text = "[\"\\uaBcD\"]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_string_eq(token.value_buffer, "\\uaBcD");

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(parsing, keywords) {
    const char *text = "[null, true, false]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_eq(token.type, JSON_TYPE_NULL);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_eq(token.type, JSON_TYPE_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_eq(token.type, JSON_TYPE_FALSE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

SUIT(parsing_malformed, test_reset_context, NULL);

TEST(parsing_malformed, string_unterminated) {
    const char *text = "[\"ohhh no!\n]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_STRING_UNTERMINATED);
}

TEST(parsing_malformed, string_eof) {
    const char *text = "[\"ohhh no!]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_UNEXPECTED_EOF);
}

TEST(parsing_malformed, string_buffer_too_short) {
    const char *text = "[\"ohhh no!\"]";

    json_load_buffer(&context, text, strlen(text));
    token.value_buffer_size = 2;

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NO_MEMORY);
}

TEST(parsing_malformed, string_control) {
    const char *text = "[\"\t\r\"]";

    json_load_buffer(&context, text, strlen(text));
    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_STRING_INVALID_ASCII);
}

TEST(parsing_malformed, string_unicode_escape_invalid) {
    const char *text = "[\"\\uy\"]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_STRING_INVALID_UNICODE_ESCAPE);
}

TEST(parsing_malformed, string_unicode_escape_eof) {
    const char *text = "[\"\\u";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_UNEXPECTED_EOF);
}

TEST(parsing_malformed, string_unicode_escape_too_short) {
    const char *text = "[\"\\u";

    json_load_buffer(&context, text, strlen(text));
    token.value_buffer_size = 2;

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_NO_MEMORY);
}

TEST(parsing_malformed, keywords_invalid) {
    const char *text = "[what, false]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_UNEXPECTED_TOKEN);
}

TEST(parsing_malformed, keyword_invalid_true) {
    const char *text = "[tru]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_KEYWORD_INVALID);
}

TEST(parsing_malformed, keyword_invalid_null) {
    const char *text = "[nulo]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_KEYWORD_INVALID);
}

TEST(parsing_malformed, keyword_invalid_false) {
    const char *text = "[falsn]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_KEYWORD_INVALID);
}

TEST(parsing, number_zero) {
    const char *text = "[0, 0.0, -0.0]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_FALSE);
}


TEST(parsing, number_integer) {
    const char *text = "[12345, -12345]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_FALSE);
}

TEST(parsing, number_float) {
    const char *text = "[123.456, -123.456]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_FALSE);
}

TEST(parsing, number_expo) {
    const char *text = "[123.123e5]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_FALSE);
}

TEST(parsing, number_expo_sign) {
    const char *text = "[123.123e+5, 123.123E-5]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_FALSE);
}

TEST(parsing_malformed, number_leading_zero) {
    const char *text = "[01]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_float) {
    const char *text = "[12.32.3]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_expo_double_dot) {
    const char *text = "[123.12.3e+5]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_expo_no_decimal) {
    const char *text = "[123.e12]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_expo_double_sign) {
    const char *text = "[123.123e++5]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_expo_trailing_sign) {
    const char *text = "[123.123e+]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NUMBER_INVALID);
}
