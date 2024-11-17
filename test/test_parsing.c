#include "libtest/include/test/test.h"
#include "test_setup.h"

SUIT(parsing, test_reset_context, NULL);

TEST(parsing, string_escape) {
    const char *text = "[\"\\\"\\\\n\\r\\t\\f\"]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_STRING);
    test_strcmp(text + token.start, "\\\"\\\\n\\r\\t\\f", token.length);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_DONE);
}

// FIXME: What is the point of this test?
TEST(parsing, string_escape_backslash) {
    const char *text = "[\"\xEE\xBC\xB7\"]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_STRING);
    test_strcmp(text + token.start, "\xEE\xBC\xB7", token.length);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_DONE);
}

TEST(parsing, string_escape_utf16) {
    const char *text = "[\"\\uaBcD\"]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    test_strcmp(text + token.start, "\\uaBcD", token.length);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_DONE);
}

TEST(parsing, keywords) {
    const char *text = "[null, true, false]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_NULL);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_TRUE);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_FALSE);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_DONE);
}

SUIT(parsing_malformed, test_reset_context, NULL);

TEST(parsing_malformed, string_unterminated) {
    const char *text = "[\"ohhh no!\n]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_STRING_UNTERMINATED);
}

TEST(parsing_malformed, string_eof) {
    const char *text = "[\"ohhh no!]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_STRING_UNTERMINATED);
}

TEST(parsing_malformed, string_control) {
    const char *text = "[\"\t\r\"]";

    plain_json_load_buffer(&context, text, strlen(text));
    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_STRING_INVALID_ASCII);
}

TEST(parsing_malformed, string_escape_utf16) {
    const char *text = "[\"\\uy\"]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_STRING_UTF16_INVALID);
}

TEST(parsing_malformed, string_escape_utf16_incomplete) {
    const char *text = "[\"\\u";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_STRING_UTF16_INVALID);
}

TEST(parsing_malformed, keywords_invalid) {
    const char *text = "[what]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_ILLEGAL_CHAR);
}

TEST(parsing_malformed, keyword_invalid_true) {
    const char *text = "[trun]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_KEYWORD_INVALID);
}

TEST(parsing_malformed, keyword_invalid_null) {
    const char *text = "[nuln]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_KEYWORD_INVALID);
}

TEST(parsing_malformed, keyword_invalid_false) {
    const char *text = "[falsn]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_KEYWORD_INVALID);
}

TEST(parsing, number_zero) {
    const char *text = "[0, 0.0, -0.0]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_DONE);
}


TEST(parsing, number_integer) {
    const char *text = "[12345, -12345]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_DONE);
}

TEST(parsing, number_float) {
    const char *text = "[123.456, -123.456]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_DONE);
}

TEST(parsing, number_expo) {
    const char *text = "[123.123e5]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_DONE);
}

TEST(parsing, number_expo_sign) {
    const char *text = "[123.123e+5, 123.123E-5]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_DONE);
}

TEST(parsing_malformed, number_leading_zero) {
    const char *text = "[01]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO);
}

TEST(parsing_malformed, number_float) {
    const char *text = "[12.32.3]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL);
}

TEST(parsing_malformed, number_expo_double_dot) {
    const char *text = "[123.12.3e+5]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL);
}

TEST(parsing_malformed, number_expo_no_decimal) {
    const char *text = "[123.e12]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL);
}

TEST(parsing_malformed, number_expo_double_sign) {
    const char *text = "[123.123e++5]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO);
}

TEST(parsing_malformed, number_expo_trailing_sign) {
    const char *text = "[123.123e+]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO);
}

TEST(parsing_malformed, number_starting_plus) {
    const char *text = "[+1]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN);
}

TEST(parsing_malformed, number_double_sign) {
    const char *text = "[--1]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN);
}
