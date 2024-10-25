#include <test/test.h>

#include "test_config.h"

#include "../json.h"

test_build_context()

    SUIT(structure, test_reset_context, NULL);

TEST(structure, root_empty) {
    const char *text = "";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(structure, array_empty) {
    const char *text = "[]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(structure, array_of_objects) {
    const char *text = "[{\"key\": null}, {\"key\": null}]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);

    for (int i = 0; i < 8; i++) {
        test_assert(status == JSON_TRUE);
        status = json_read_token(&context, &token);
    }

    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(structure, array_deeply_nested) {
    const char *text = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]";
    json_load_buffer(&context, text, strlen(text));

    for (uint32_t i = 0; i < 62; i++) {
        json_ErrorType status = json_read_token(&context, &token);
        test_assert(status == JSON_TRUE);
    }

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(structure, newline) {
    const char *text = "\n[\n\n]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

TEST(structure_malformed, object_empty) {
    const char *text = "{}";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_FALSE);
}

SUIT(structure_malformed, test_reset_context, NULL);

TEST(structure_malformed, array_trailing_comma) {
    const char *text = "[ \"one\",]";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_UNEXPECTED_TOKEN);
}

TEST(structure_malformed, root_unexpected) {
    const char *text = "{\"array\": []";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_UNEXPECTED_EOF);
}

TEST(structure_malformed, array_unmatched) {
    const char *text = "[]]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_UNEXPECTED_TOKEN);
}

TEST(structure_malformed, object_trailing_comma) {
    const char *text = "{ \"key\": \"value\",}";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert_eq(status, JSON_ERROR_UNEXPECTED_TOKEN);
}

TEST(structure_malformed, root_value) {
    const char *text = "\"i do not belong here\"";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_UNEXPECTED_TOKEN);
}

TEST(structure_malformed, root_too_deep) {
    const char *text = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[";
    json_load_buffer(&context, text, strlen(text));

    for (uint32_t i = 0; i < 31; i++) {
        json_ErrorType status = json_read_token(&context, &token);
        test_assert(status == JSON_TRUE);
    }

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_TOO_DEEP);
}

TEST(structure_malformed, array_comma_missing) {
    const char *text = "[4 3]";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_MISSING_FIELD_SEPERATOR);
}

TEST(structure_malformed, object_nested_comma_missing) {
    const char *text = "{\"key\": {\"key\": null} \"key\": null}";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_MISSING_FIELD_SEPERATOR);
}

TEST(structure_malformed, object_comma_missing) {
    const char *text = "{\"value\": null \"misplaced_key\": null}";
    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);
    status = json_read_token(&context, &token);
    test_assert(status == JSON_TRUE);

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_MISSING_FIELD_SEPERATOR);
}

TEST(structure, array_of_objects_comma_missing) {
    const char *text = "[{\"key\": null} {\"key\": null}]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);

    for (int i = 0; i < 4; i++) {
        test_assert(status == JSON_TRUE);
        status = json_read_token(&context, &token);
    }

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_MISSING_FIELD_SEPERATOR);
}

TEST(structure, array_of_arrays_comma_missing) {
    const char *text = "[[null] [null]]";

    json_load_buffer(&context, text, strlen(text));

    json_ErrorType status = json_read_token(&context, &token);

    for (int i = 0; i < 4; i++) {
        test_assert(status == JSON_TRUE);
        status = json_read_token(&context, &token);
    }

    status = json_read_token(&context, &token);
    test_assert(status == JSON_ERROR_MISSING_FIELD_SEPERATOR);
}
