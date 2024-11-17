#include "test_setup.h"

SUIT(structure, test_reset_context, NULL);

TEST(structure, array_named) {
    const char *text = "{ \"test_key\": []}";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    test_assert_eq(token.type, PLAIN_JSON_TYPE_ARRAY_START);
    test_strcmp(text + token.key_start, "test_key", token.key_length);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_DONE);
}

TEST(structure, array_of_strings) {
    const char *text = "[ \"one\", \"two\", \"three\"]";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_STRING);
    test_strcmp(text + token.start, "one", token.length);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_STRING);
    test_strcmp(text + token.start, "two", token.length);

    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    test_assert_eq(token.type, PLAIN_JSON_TYPE_STRING);
    test_strcmp(text + token.start, "three", token.length);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_DONE);
}

TEST(structure, array_empty) {
    const char *text = "[]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_DONE);
}

TEST(structure, array_of_objects) {
    const char *text = "[{\"key\": null}, {\"key\": null}]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);

    for (int i = 0; i < 8; i++) {
        test_assert(status == PLAIN_JSON_HAS_REMAINING);
        status = plain_json_read_token(&context, &token);
    }

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_DONE);
}

TEST(structure, array_deeply_nested) {
    const char *text = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]";
    plain_json_load_buffer(&context, text, strlen(text));

    for (uint32_t i = 0; i < 62; i++) {
        plain_json_ErrorType status = plain_json_read_token(&context, &token);
        test_assert(status == PLAIN_JSON_HAS_REMAINING);
    }

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_DONE);
}

TEST(structure, blanks) {
    const char *text = "\n[\n\n]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_DONE);
}

TEST(structure_malformed, object_empty) {
    const char *text = "{}";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_DONE);
}

SUIT(structure_malformed, test_reset_context, NULL);

TEST(structure_malformed, object_trailing_key) {
    const char *text = "{ \"test_key\": }}";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_ILLEGAL_CHAR);
}

TEST(structure_malformed, array_key) {
    const char *text = "[ \"test_key\": \"oh no!\"]";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_ILLEGAL_CHAR);
}

TEST(structure, root_empty) {
    const char *text = "";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_UNEXPECTED_EOF);
}


TEST(structure_malformed, array_trailing_comma) {
    const char *text = "[ \"one\",]";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_UNEXPECTED_COMMA);
}

TEST(structure_malformed, root_unexpected) {
    const char *text = "{\"array\": []";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_UNEXPECTED_EOF);
}

TEST(structure_malformed, array_unmatched) {
    const char *text = "[]]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_ILLEGAL_CHAR);
}

TEST(structure_malformed, object_trailing_comma) {
    const char *text = "{ \"key\": \"value\",}";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert_eq(status, PLAIN_JSON_ERROR_UNEXPECTED_COMMA);
}

TEST(structure_malformed, root_too_deep) {
    const char *text = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[";
    plain_json_load_buffer(&context, text, strlen(text));

    for (uint32_t i = 0; i < 31; i++) {
        plain_json_ErrorType status = plain_json_read_token(&context, &token);
        test_assert(status == PLAIN_JSON_HAS_REMAINING);
    }

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_NESTING_TOO_DEEP);
}

TEST(structure_malformed, array_comma_missing) {
    const char *text = "[4 3]";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_MISSING_COMMA);
}

TEST(structure_malformed, object_nested_comma_missing) {
    const char *text = "{\"key\": {\"key\": null} \"key\": null}";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_MISSING_COMMA);
}

TEST(structure_malformed, object_comma_missing) {
    const char *text = "{\"value\": null \"misplaced_key\": null}";
    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);
    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_HAS_REMAINING);

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_MISSING_COMMA);
}

TEST(structure, array_of_objects_comma_missing) {
    const char *text = "[{\"key\": null} {\"key\": null}]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);

    for (int i = 0; i < 4; i++) {
        test_assert(status == PLAIN_JSON_HAS_REMAINING);
        status = plain_json_read_token(&context, &token);
    }

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_MISSING_COMMA);
}

TEST(structure, array_of_arrays_comma_missing) {
    const char *text = "[[null] [null]]";

    plain_json_load_buffer(&context, text, strlen(text));

    plain_json_ErrorType status = plain_json_read_token(&context, &token);

    for (int i = 0; i < 4; i++) {
        test_assert(status == PLAIN_JSON_HAS_REMAINING);
        status = plain_json_read_token(&context, &token);
    }

    status = plain_json_read_token(&context, &token);
    test_assert(status == PLAIN_JSON_ERROR_MISSING_COMMA);
}
