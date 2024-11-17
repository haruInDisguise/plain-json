#ifndef _JSON_COMMON_H_
#define _JSON_COMMON_H_

/*#define PRINT_RESULT_LIST*/

#define PLAIN_JSON_IMPLEMENTATION
#include "../plain_json.h"

static const char *plain_json_type_to_string(plain_json_Type type) {
    switch (type) {
    case PLAIN_JSON_TYPE_INVALID:
        return "invalid";
    case PLAIN_JSON_TYPE_ERROR:
        return "error";
    case PLAIN_JSON_TYPE_OBJECT_START:
        return "object_start";
    case PLAIN_JSON_TYPE_OBJECT_END:
        return "object_end";
    case PLAIN_JSON_TYPE_ARRAY_START:
        return "array_start";
    case PLAIN_JSON_TYPE_ARRAY_END:
        return "array_end";
    case PLAIN_JSON_TYPE_STRING:
        return "string";
    case PLAIN_JSON_TYPE_NUMBER:
        return "integer";
    case PLAIN_JSON_TYPE_NULL:
        return "null";
    case PLAIN_JSON_TYPE_FALSE:
        return "false";
    case PLAIN_JSON_TYPE_TRUE:
        return "true";
    }

    return "invalid";
}

#ifndef PRINT_RESULT_LIST
static const char *plain_json_error_to_string(plain_json_ErrorType type) {
    switch (type) {
    case PLAIN_JSON_ERROR_STRING_UTF8_HAS_SURROGATE:
        return "string_utf8_has_surrogate";
    case PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE:
        return "string_invalid_utf16_surrogate";
    case PLAIN_JSON_ERROR_STRING_UTF16_INVALID:
        return "string_invalid_utf16";
    case PLAIN_JSON_ERROR_STRING_UTF8_INVALID:
        return "string_invalid_utf8";
    case PLAIN_JSON_ERROR_STRING_INVALID_ASCII:
        return "string_invalid_ascii";
    case PLAIN_JSON_ERROR_STRING_UNTERMINATED:
        return "string_unterminated";
    case PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL:
        return "number_invalid_decimal";
    case PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO:
        return "number_leading_zero";
    case PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO:
        return "number_invalid_exponent";
    case PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN:
        return "number_invalid_sign";
    case PLAIN_JSON_ERROR_NESTING_TOO_DEEP:
        return "nesting_too_deep";
    case PLAIN_JSON_ERROR_UNEXPECTED_ROOT:
        return "unexpected_root";
    case PLAIN_JSON_ERROR_ILLEGAL_CHAR:
        return "illegal_character";
    case PLAIN_JSON_ERROR_UNEXPECTED_EOF:
        return "unexpected_eof";
    case PLAIN_JSON_ERROR_UNEXPECTED_COMMA:
        return "unexpected_field_seperator";
    case PLAIN_JSON_ERROR_KEYWORD_INVALID:
        return "keyword_invalid";
    case PLAIN_JSON_ERROR_MISSING_COMMA:
        return "missing_field_seperator";
    case PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE:
        return "string_invalid_escape";
    case PLAIN_JSON_HAS_REMAINING:
        return "parsing_has_remaining";
    case PLAIN_JSON_DONE:
        return "done";
    }

    return "unknown error";
}

#else
static const char *plain_json_error_to_string(plain_json_ErrorType type) {
    switch (type) {
    case PLAIN_JSON_ERROR_STRING_INVALID_ASCII:
        return "PLAIN_JSON_ERROR_STRING_INVALID_ASCII";
    case PLAIN_JSON_ERROR_STRING_INVALID_UTF8:
        return "PLAIN_JSON_ERROR_STRING_INVALID_UTF8";
    case PLAIN_JSON_ERROR_STRING_UNTERMINATED:
        return "PLAIN_JSON_ERROR_STRING_UNTERMINATED";
    case PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL:
        return "PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL";
    case PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO:
        return "PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO";
    case PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO:
        return "PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO";
    case PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN:
        return "PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN";
    case PLAIN_JSON_ERROR_NESTING_TOO_DEEP:
        return "PLAIN_JSON_ERROR_NESTING_TOO_DEEP";
    case PLAIN_JSON_ERROR_UNEXPECTED_ROOT:
        return "PLAIN_JSON_ERROR_UNEXPECTED_ROOT";
    case PLAIN_JSON_ERROR_UNEXPECTED_TOKEN:
        return "PLAIN_JSON_ERROR_UNEXPECTED_TOKEN";
    case PLAIN_JSON_ERROR_UNEXPECTED_EOF:
        return "PLAIN_JSON_ERROR_UNEXPECTED_EOF";
    case PLAIN_JSON_ERROR_UNEXPECTED_COMMA:
        return "PLAIN_JSON_ERROR_UNEXPECTED_COMMA";
    case PLAIN_JSON_ERROR_KEYWORD_INVALID:
        return "PLAIN_JSON_ERROR_KEYWORD_INVALID";
    case PLAIN_JSON_ERROR_MISSING_COMMA:
        return "PLAIN_JSON_ERROR_MISSING_COMMA";
    case PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE:
        return "PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE";
    case PLAIN_JSON_ERROR_STRING_INVALID_UTF16_ESCAPE:
        return "PLAIN_JSON_ERROR_STRING_INVALID_UTF16_ESCAPE";
    case PLAIN_JSON_HAS_REMAINING:
        return "PLAIN_JSON_HAS_REMAINING";
    case PLAIN_JSON_DONE:
        return "PLAIN_JSON_DONE";
    }

    return "UNKNOWN_ERROR";
}

#endif

#endif
