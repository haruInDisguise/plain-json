#include "test_setup.h"

SUIT(utf8, test_reset_context, NULL);

#define TEST_INVALID_UTF8(name, result, codepoint)                 \
    TEST(utf8, name) {                                             \
        char *text = "[\"" codepoint "\"]";                        \
        json_load_buffer(&context, text, strlen(text));            \
        json_ErrorType status = json_read_token(&context, &token); \
        test_assert_eq(status, JSON_TRUE);                         \
        status = json_read_token(&context, &token);                \
        test_assert_eq(status, result);                            \
    }

TEST_INVALID_UTF8(two_invalid_first_byte, JSON_ERROR_STRING_INVALID_UTF8, "\xDF\xCF")
TEST_INVALID_UTF8(two_invalid_codepoint, JSON_ERROR_STRING_INVALID_UTF8, "\xC0\xBF")
TEST_INVALID_UTF8(two_missing_bytes, JSON_ERROR_STRING_INVALID_UTF8, "\xDF")

TEST_INVALID_UTF8(three_invalid_first_byte, JSON_ERROR_STRING_INVALID_UTF8, "\xEF\xCF")
TEST_INVALID_UTF8(three_invalid_second_byte, JSON_ERROR_STRING_INVALID_UTF8, "\xEF\xBF\xCF")
TEST_INVALID_UTF8(three_missing_bytes, JSON_ERROR_STRING_INVALID_UTF8, "\xEF")

TEST_INVALID_UTF8(four_invalid_first_byte, JSON_ERROR_STRING_INVALID_UTF8, "\xF7\xCF")
TEST_INVALID_UTF8(four_invalid_second_byte, JSON_ERROR_STRING_INVALID_UTF8, "\xF7\xBF\xCF")
TEST_INVALID_UTF8(four_invalid_third_byte, JSON_ERROR_STRING_INVALID_UTF8, "\xF7\xBF\xBF\xCF")
TEST_INVALID_UTF8(four_missing_bytes, JSON_ERROR_STRING_INVALID_UTF8, "\xF7")

TEST_INVALID_UTF8(invalid_header, JSON_ERROR_STRING_INVALID_UTF8, "\xC0\x00")

TEST(utf8, valid) {
    const char *text = "[\"\xDF\xBF\", \"\xEF\xBF\xBF\", \"\xF7\xBF\xBF\xBF\"]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

#undef TEST_INVALID_UTF8
