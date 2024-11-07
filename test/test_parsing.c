#include "test_setup.h"

SUIT(parsing, test_reset_context, NULL);

TEST(parsing, string_escapes) {
    const char *text = "[\"\\\"\\\\n\\r\\t\\f\"]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_STRING);
    test_assert_string_eq(token.value_buffer, "\\\"\\\\n\\r\\t\\f");

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_FALSE);
}

TEST(parsing, string_escapes_backslash) {
    const char *text = "[\"\xEE\xBC\xB7\"]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_STRING);
    test_assert_string_eq(token.value_buffer, "\xEE\xBC\xB7");

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_FALSE);
}

TEST(parsing, string_escape_utf16) {
    const char *text = "[\"\\uaBcD\"]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    test_assert_string_eq(token.value_buffer, "\\uaBcD");

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_FALSE);
}

TEST(parsing, keywords) {
    const char *text = "[null, true, false]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_NULL);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_FALSE);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_FALSE);
}

SUIT(parsing_malformed, test_reset_context, NULL);

TEST(parsing_malformed, string_unterminated) {
    const char *text = "[\"ohhh no!\n]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_STRING_UNTERMINATED);
}

TEST(parsing_malformed, string_eof) {
    const char *text = "[\"ohhh no!]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_UNEXPECTED_EOF);
}

TEST(parsing_malformed, string_buffer_too_short) {
    const char *text = "[\"ohhh no!\"]";

    plain_json_load_buffer(&context, text, strlen(text));
    token.value_buffer_size = 2;

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NO_MEMORY);
}

TEST(parsing_malformed, string_control) {
    const char *text = "[\"\t\r\"]";

    plain_json_load_buffer(&context, text, strlen(text));
    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_STRING_INVALID_ASCII);
}

TEST(parsing_malformed, string_escape_utf16) {
    const char *text = "[\"\\uy\"]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_STRING_INVALID_UTF16_ESCAPE);
}

TEST(parsing_malformed, string_escape_utf16_incomplete) {
    const char *text = "[\"\\u";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_STRING_INVALID_UTF16_ESCAPE);
}

TEST(parsing_malformed, string_escape_utf16_buffer_too_short) {
    const char *text = "[\"\\uABCD";

    plain_json_load_buffer(&context, text, strlen(text));
    token.value_buffer_size = 2;

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_NO_MEMORY);
}

TEST(parsing_malformed, keywords_invalid) {
    const char *text = "[what]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_UNEXPECTED_TOKEN);
}

TEST(parsing_malformed, keyword_invalid_true) {
    const char *text = "[trun]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_KEYWORD_INVALID);
}

TEST(parsing_malformed, keyword_invalid_null) {
    const char *text = "[nuln]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_KEYWORD_INVALID);
}

TEST(parsing_malformed, keyword_invalid_false) {
    const char *text = "[falsn]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_KEYWORD_INVALID);
}

TEST(parsing, number_zero) {
    const char *text = "[0, 0.0, -0.0]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_FALSE);
}


TEST(parsing, number_integer) {
    const char *text = "[12345, -12345]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_FALSE);
}

TEST(parsing, number_float) {
    const char *text = "[123.456, -123.456]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_FALSE);
}

TEST(parsing, number_expo) {
    const char *text = "[123.123e5]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_FALSE);
}

TEST(parsing, number_expo_sign) {
    const char *text = "[123.123e+5, 123.123E-5]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_FALSE);
}

TEST(parsing_malformed, number_leading_zero) {
    const char *text = "[01]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_float) {
    const char *text = "[12.32.3]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_expo_double_dot) {
    const char *text = "[123.12.3e+5]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_expo_no_decimal) {
    const char *text = "[123.e12]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_expo_double_sign) {
    const char *text = "[123.123e++5]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_expo_trailing_sign) {
    const char *text = "[123.123e+]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_starting_plus) {
    const char *text = "[+1]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID);
}

TEST(parsing_malformed, number_double_sign) {
    const char *text = "[--1]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID);
}
