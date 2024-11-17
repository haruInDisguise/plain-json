#include "test_setup.h"

SUIT(unicode, test_reset_context, NULL);

#define TEST_UNICODE(name, result, encoded)                                \
    TEST(unicode, name) {                                                          \
        char *text = "[\"" encoded "\"]";                                       \
        __attribute__((unused)) plain_json_Token tokens[16] = { 0 };            \
        __attribute__((unused)) int tokens_read = 0;                            \
        plain_json_load_buffer(&context, text, strlen(text));                   \
        plain_json_ErrorType status = status =                                  \
            plain_json_read_token_buffered(&context, tokens, 16, &tokens_read); \
        test_assert_eq(status, result);                                         \
    }

/* UTF-8 */
TEST_UNICODE(utf8_invalid_two, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xDF\xCF")
TEST_UNICODE(utf8_invalid_three, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xEF\xBF\xCF")
TEST_UNICODE(utf8_invalid_four, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xF7\xBF\xBF\xCF")

TEST_UNICODE(utf8_invalid_header, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xC0")
TEST_UNICODE(utf8_high_surrogate, PLAIN_JSON_ERROR_STRING_UTF8_HAS_SURROGATE, "\xED\xBF\xBF")
TEST_UNICODE(utf8_low_surrogate, PLAIN_JSON_ERROR_STRING_UTF8_HAS_SURROGATE, "\xED\xB0\xB0")

TEST_UNICODE(utf8_valid, PLAIN_JSON_DONE, "\xF0\x9F\x90\x88")

/* UTF-16 */
TEST_UNICODE(utf16_single_high_surrogate, PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE, "\\uD800")
TEST_UNICODE(utf16_single_low_surrogate, PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE, "\\uD800")
TEST_UNICODE(utf16_invalid_second_surrogate, PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE, "\\uD800\\uD800")

TEST_UNICODE(utf16_valid, PLAIN_JSON_DONE, "\\u1234\\u5678\\u9ABC\\u9EF0")

#undef TEST_UNICODE
