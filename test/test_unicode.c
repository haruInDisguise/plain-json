#include <stdio.h>

#include "test_setup.h"

SUIT(unicode, NULL, test_finalize);

#define TEST_UNICODE(name, result, encoded)                                          \
    TEST(unicode, name) {                                                            \
        const char *text = "[\"" encoded "\"]";                                      \
        plain_json_ErrorType status = PLAIN_JSON_DONE;                               \
        context = plain_json_parse(alloc_config, (u8 *)text, strlen(text), &status); \
        test_assert_eq(status, result);                                              \
    }

/* UTF-8 */

TEST_UNICODE(utf8_two_invalid_header, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xDF\xCF")
/* The two byte sequence can use all of its bits, and therefore does not have a "too big" value */
TEST_UNICODE(utf8_two_too_small, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xC1\xFF")
TEST_UNICODE(utf8_two_valid, PLAIN_JSON_DONE, "\xC2\x80") /* € */

TEST_UNICODE(utf8_three_invalid_header, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xEF\xBF\xCF")
TEST_UNICODE(utf8_three_too_small, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xE0\xAF\xFF")
TEST_UNICODE(utf8_three_too_big, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xEF\xB0\x01")
TEST_UNICODE(utf8_three_valid, PLAIN_JSON_DONE, "\xEF\xBF\xBD") /* � */

TEST_UNICODE(utf8_four_invalid_header, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xF7\xBF\xBF\xCF")
TEST_UNICODE(utf8_four_too_small, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xF0\x8F\xBF\xBF")
TEST_UNICODE(utf8_four_too_big, PLAIN_JSON_ERROR_STRING_UTF8_INVALID, "\xF7\xBF\xBF\xCF")
TEST_UNICODE(utf8_four_valid, PLAIN_JSON_DONE, "\xF0\x9F\x90\x88") /* 🐈 */

TEST_UNICODE(utf8_surrogate_high, PLAIN_JSON_ERROR_STRING_UTF8_HAS_SURROGATE, "\xED\xBF\xBF")
TEST_UNICODE(utf8_surrogate_low, PLAIN_JSON_ERROR_STRING_UTF8_HAS_SURROGATE, "\xED\xB0\xB0")

/* UTF-16 */
TEST_UNICODE(
    utf16_single_high_surrogate, PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE, "\\uD800"
)
TEST_UNICODE(utf16_single_low_surrogate, PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE, "\\uD800")
TEST_UNICODE(
    utf16_invalid_second_surrogate, PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE,
    "\\uD800\\uD800"
)

TEST_UNICODE(utf16_valid_surrogate, PLAIN_JSON_DONE, "\\uD83D\\uDC08")
TEST_UNICODE(utf16_valid, PLAIN_JSON_DONE, "\\u2190\\u21AA\\u21B0")

#define TEST_UNICODE_VALUE(name, raw, expected)                                         \
    TEST(unicode, name) {                                                               \
        const char *text = "[\"" raw "\"]";                                             \
        plain_json_ErrorType status = PLAIN_JSON_DONE;                                  \
        context = plain_json_parse(alloc_config, (u8 *)text, strlen(text), &status);    \
        test_assert_eq(status, PLAIN_JSON_DONE);                                        \
        const plain_json_Token *token = plain_json_get_token(context, 1);               \
        test_assert_ne(token, PLAIN_JSON_NULL);                                         \
        test_assert_string_eq(                                                          \
            (char *)plain_json_get_string(context, token->value.string_index), expected \
        );                                                                              \
    }

TEST_UNICODE_VALUE(parse_utf16_surrogate, "\\uD83D\\uDE11", "\xF0\x9F\x98\x91") /* 😑 */
TEST_UNICODE_VALUE(
    parse_utf16_misc, "Bear emoji: \\u0295\\u00b7\\u0361\\u1d25\\u00b7\\u0294",
    "Bear emoji: \xca\x95\xc2\xb7\xcd\xa1\xe1\xb4\xa5\xc2\xb7\xca\x94"
) /* ʕ·͡ᴥ·ʔ */

#undef TEST_UNICODE
