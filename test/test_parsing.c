#include <test/test.h>

#include "test_config.h"

test_build_context()

SUIT(parsing, test_reset_context, NULL);

/* Strings */

TEST(parsing, single_named_string) {
    const char *text = "{ \"test_key\": \"test_value\"}";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);

    test_assert_eq(token.type, JSON_TYPE_STRING);
    test_assert_string_eq("test_key", token.key_buffer);
    test_assert_string_eq("test_value", token.value_buffer);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(parsing, single_named_object) {
    const char *text = "{ \"test_key\": {}}";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    test_assert_eq(token.type, JSON_TYPE_OBJECT_START);
    test_assert_string_eq("test_key", token.key_buffer);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(parsing, single_named_array) {
    const char *text = "{ \"test_key\": []}";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    test_assert_eq(token.type, JSON_TYPE_ARRAY_START);
    test_assert_string_eq("test_key", token.key_buffer);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(parsing, array_of_strings) {
    const char *text = "[ \"one\", \"two\", \"three\"]";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_eq(token.type, JSON_TYPE_STRING);
    test_assert_string_eq("one", token.value_buffer);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_eq(token.type, JSON_TYPE_STRING);
    test_assert_string_eq("two", token.value_buffer);

    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    test_assert_eq(token.type, JSON_TYPE_STRING);
    test_assert_string_eq("three", token.value_buffer);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(parsing, array_of_objects) {
    const char *text = "[ {}, {}, {}]";
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
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

SUIT(parsing_malformed, test_reset_context, NULL);

TEST(parsing_malformed, named_object) {
    const char *text = "{ \"test_key\": }}";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_UNEXPECTED_TOKEN);
}

TEST(parsing_malformed, key_buffer_too_short) {
    const char *text = "{ \"test_key\": }}";
    json_load_buffer(&context, text, strlen(text));

    token.key_buffer_size = 2;

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_NO_MEMORY);
}

TEST(parsing_malformed, named_array) {
    const char *text = "{ \"test_key\": ]}";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_UNEXPECTED_TOKEN);
}

/* FIXME: Currently, the key is parsed as a string value and the error
 * arises from the unexpected colon inside the array.
 * Should we consider the colon to identifiy a key?
 * This is not compatible with the current implementation, since
 * keys and string values are read into sperate buffers. */
TEST(parsing_malformed, key_in_array) {
    const char *text = "[ \"test_key\": \"oh no!\"]";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_UNEXPECTED_TOKEN);
}
