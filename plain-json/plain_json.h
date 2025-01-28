#include <stdbool.h>
#include <stdint.h>

#ifndef _PLAIN_JSON_H_
    #define _PLAIN_JSON_H_

    #ifndef PLAIN_JSON_OPTION_MAX_DEPTH
        #define PLAIN_JSON_OPTION_MAX_DEPTH 32
    #endif

    #define PLAIN_JSON_STATE_IS_FIRST_TOKEN 0x01
    #define PLAIN_JSON_STATE_IS_ROOT        0x02

    #define PLAIN_JSON_STATE_NEEDS_KEY         0x04
    #define PLAIN_JSON_STATE_NEEDS_COMMA       0x08
    #define PLAIN_JSON_STATE_NEEDS_VALUE       0x10
    #define PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE 0x20
    #define PLAIN_JSON_STATE_NEEDS_COLON       0x40

/// If the error_code is not equal to PLAIN_JSON_DONE, the parser encountered an issue.
/// "plain-json" tries to be explicit when it comes to error handeling, meaning most
/// uniq parsing issues have there own error code.
typedef enum {
    PLAIN_JSON_NONE,

    PLAIN_JSON_DONE,
    PLAIN_JSON_HAS_REMAINING,

    PLAIN_JSON_ERROR_NO_MEMORY,

    PLAIN_JSON_ERROR_STRING_INVALID_ASCII,
    PLAIN_JSON_ERROR_STRING_UNTERMINATED,
    PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE,

    PLAIN_JSON_ERROR_STRING_UTF8_INVALID,
    PLAIN_JSON_ERROR_STRING_UTF8_HAS_SURROGATE,

    PLAIN_JSON_ERROR_STRING_UTF16_INVALID,
    PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE,

    PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO,
    PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO,
    PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN,
    PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL,
    PLAIN_JSON_ERROR_NUMBER_OVERFLOW,
    PLAIN_JSON_ERROR_NUMBER_UNDERFLOW,

    PLAIN_JSON_ERROR_KEYWORD_INVALID,

    PLAIN_JSON_ERROR_MISSING_COMMA,
    PLAIN_JSON_ERROR_NESTING_TOO_DEEP,

    PLAIN_JSON_ERROR_UNEXPECTED_EOF,
    PLAIN_JSON_ERROR_UNEXPECTED_COMMA,
    PLAIN_JSON_ERROR_UNEXPECTED_ROOT,
    PLAIN_JSON_ERROR_ILLEGAL_CHAR,
} plain_json_ErrorType;

/// The token type.
typedef enum {
    PLAIN_JSON_TYPE_INVALID,
    PLAIN_JSON_TYPE_ERROR,

    PLAIN_JSON_TYPE_NULL,

    PLAIN_JSON_TYPE_OBJECT_START,
    PLAIN_JSON_TYPE_OBJECT_END,

    PLAIN_JSON_TYPE_ARRAY_START,
    PLAIN_JSON_TYPE_ARRAY_END,

    PLAIN_JSON_TYPE_TRUE,
    PLAIN_JSON_TYPE_FALSE,
    PLAIN_JSON_TYPE_STRING,
    PLAIN_JSON_TYPE_INTEGER,
    PLAIN_JSON_TYPE_FLOAT32,
    PLAIN_JSON_TYPE_FLOAT64,
} plain_json_Type;

    #define PLAIN_JSON_NO_KEY (-1U)
    #define PLAIN_JSON_NULL   (void *)0

/// A tokens fields describe its layout. All fields can (and should) be
/// accessed directly. The library only hands out const references that
/// should be considered as read-only
typedef struct {
    uintptr_t start;
    uint32_t length;

    /// The internal index of this tokens key value.
    /// Should be passed to 'plain_json_get_key()'
    uint32_t key_index;
    /// Check the type to resolve out the value union.
    plain_json_Type type;
    union {
        /// The internal index of this tokens string value (if any).
        /// See 'plain_json_get_string()'
        uint32_t string_index;
        uint64_t integer;
        float real32;
        double real64;
        bool boolean;
    } value;
} plain_json_Token;

/// The parser requires "libc compatible (malloc/free/realloc)" allocator functions
typedef struct {
    void *(*alloc_func)(uintptr_t size);
    void *(*realloc_func)(void *buffer, uintptr_t size);
    void (*free_func)(void *buffer);
} plain_json_AllocatorConfig;

/// The context represents the parsers internal state and is required by most
/// library functions.
typedef struct plain_json_Context plain_json_Context;

/// Fully parse the given text buffer. The function requires a "plain_json_ErrorType"
/// argument that should be initialized to "PLAIN_JSON_NONE".
/// Check the last token to track down the errors exact position.
extern plain_json_Context *plain_json_parse(
    plain_json_AllocatorConfig alloc_config, const uint8_t *buffer, uintptr_t buffer_size,
    plain_json_ErrorType *error
);
/// Release the internal state, after processing the parsing results.
extern void plain_json_free(plain_json_Context *context);

/// Get the total token count.
extern uint32_t plain_json_get_token_count(plain_json_Context *context);
/// Get a specific token, given its index. Tokens are stored in order.
extern const plain_json_Token *plain_json_get_token(plain_json_Context *context, uint32_t index);

/// Get a tokens key (if any), given a tokens "key_index" field.
/// Returns NULL if the token does not have a key.
extern const uint8_t *plain_json_get_key(plain_json_Context *context, uint32_t key_index);
/// Get the value of a token with type string. Any other type will
/// produce misleadingerrornous results.
/// Returns NULL if the string index is invalid.
extern const uint8_t *plain_json_get_string(plain_json_Context *context, uint32_t string_index);

/// Turn a tokens offset field into an absolute position.
/// Requires a "line" and "line_offset" argument to store the result.
/// Returns false if the offset is beyond the raw jsons size.
/// This function is useful to track down the exact position of an error token.
extern bool plain_json_compute_position(
    plain_json_Context *context, uintptr_t offset, uint32_t *line, uint32_t *line_offset
);
/// Turn an error code into its string representation.
extern const char *plain_json_error_to_string(plain_json_ErrorType type);
/// Turn a type into its string representation.
extern const char *plain_json_type_to_string(plain_json_Type type);

#endif

/* Implementation */

#ifdef PLAIN_JSON_IMPLEMENTATION

#ifdef PLAIN_JSON_DEBUG
    #if __has_builtin(__builtin_debugtrap)
        #define json_assert(condition) \
            if (!(condition)) {        \
                __builtin_debugtrap(); \
            }
    #else
        #define json_assert(condition) \
            if (!(condition)) {        \
                __asm__("int3");       \
            }
    #endif
#else
    #define json_assert(...)
#endif

#define is_blank(c) (c == ' ' || c == '\t' || c == '\n' || c == '\r')
#define is_digit(c) (c >= '0' && c <= '9')
#define is_hex(c)   ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (is_digit(c)))

#define PLAIN_JSON_INTERN_STRING_CACHE 64

typedef struct {
    uint8_t *buffer;
    uint8_t item_size;
    uintptr_t buffer_size;
} plain_json_List;

struct plain_json_Context {
    const uint8_t *buffer;

    uintptr_t buffer_size;
    uintptr_t buffer_offset;

    uint8_t depth_buffer_index;
    uint8_t depth_buffer[PLAIN_JSON_OPTION_MAX_DEPTH];

    plain_json_AllocatorConfig alloc_config;
    plain_json_List string_buffer;
    plain_json_List token_buffer;
};

static void *plain_json_intern_memset(void *start, int value, uintptr_t length) {
    while (length--) {
        ((uint8_t *)start)[length] = (uint8_t)value;
    }

    return start;
}

static void *plain_json_intern_memcpy(void *dest, const void *src, uintptr_t length) {
    while (length--) {
        ((uint8_t *)dest)[length] = ((uint8_t *)src)[length];
    }

    return dest;
}

static inline bool plain_json_intern_list_append(
    plain_json_List *list, plain_json_AllocatorConfig *config, void *raw_data, uint32_t count
) {
    list->buffer = config->realloc_func(list->buffer, list->buffer_size + list->item_size * count);
    if (list->buffer == PLAIN_JSON_NULL) {
        return false;
    }

    plain_json_intern_memcpy(list->buffer + list->buffer_size, raw_data, list->item_size * count);
    list->buffer_size += list->item_size * count;
    return true;
}

/* TODO: come up with a way of accessing the text buffer that feels less akward */
static inline bool plain_json_intern_has_next(plain_json_Context *context, uintptr_t offset) {
    return (context->buffer_offset + offset < context->buffer_size);
}

/* TODO: come up with a way of accessing the text buffer that feels less akward */
static inline uint8_t plain_json_intern_consume(plain_json_Context *context, uintptr_t count) {
    json_assert(context->buffer_offset + count <= context->buffer_size);
    uint8_t tmp = context->buffer[context->buffer_offset];
    context->buffer_offset += count;
    return tmp;
}

/* TODO: come up with a way of accessing the text buffer that feels less akward */
static inline uint8_t plain_json_intern_peek(plain_json_Context *context, uintptr_t offset) {
    json_assert(context->buffer_offset + offset < context->buffer_size);
    return context->buffer[context->buffer_offset + offset];
}

/* Parsing */

static inline bool plain_json_intern_parse_utf16(
    const uint8_t *buffer, const uintptr_t buffer_size, uint32_t *codepoint
) {
    if (4 >= buffer_size) {
        return false;
    }

    for (uint32_t i = 0; i < 4; i++) {
        uint32_t shift = 4 * (3 - i);
        if (buffer[i] >= 'a' && buffer[i] <= 'f') {
            (*codepoint) |= (((uint32_t)buffer[i] - 'a' + 10) & 0xff) << shift;
        } else if (buffer[i] >= 'A' && buffer[i] <= 'F') {
            (*codepoint) |= (((uint32_t)buffer[i] - 'A' + 10) & 0xff) << shift;
        } else if (buffer[i] >= '0' && buffer[i] <= '9') {
            (*codepoint) |= (((uint32_t)buffer[i] - '0') & 0xff) << shift;
        } else {
            return false;
        }
    }

    return true;
}

static inline bool
plain_json_intern_encode_utf8(uint32_t codepoint, uint8_t *cache, uint32_t *cache_offset_ptr) {
    uint32_t cache_offset = *cache_offset_ptr;
    uint32_t part = 0;

    if (codepoint <= 0x07FF) {
        part = codepoint;
        cache[cache_offset] = 0xC0 | ((part >> 6) & 0x1F);
        cache[cache_offset + 1] = 0x80 | (part & 0x3F);
        cache_offset += 2;
    } else if (codepoint <= 0xFFFF) {
        part = codepoint;
        cache[cache_offset] = 0xE0 | ((part >> 12) & 0x0F);
        cache[cache_offset + 1] = 0x80 | ((part >> 6) & 0x3F);
        cache[cache_offset + 2] = 0x80 | (part & 0x3F);
        cache_offset += 3;
    } else if (codepoint <= 0x10FFFF) {
        part = codepoint;
        cache[cache_offset] = 0xF0 | ((part >> 18) & 0x07);
        cache[cache_offset + 1] = 0x80 | ((part >> 12) & 0x3F);
        cache[cache_offset + 2] = 0x80 | ((part >> 6) & 0x3F);
        cache[cache_offset + 3] = 0x80 | (part & 0x3F);
        cache_offset += 4;
    } else {
        return false;
    }

    (*cache_offset_ptr) = cache_offset;
    return true;
}

static inline plain_json_ErrorType plain_json_intern_read_utf8(
    const uint8_t *buffer, uintptr_t buffer_size, uintptr_t *offset_ptr, uint8_t *cache,
    uint32_t *cache_offset_ptr, plain_json_ErrorType *status
) {
    static const uint32_t utf8_surrogate_mask = 0x0F200000;
    static const uint32_t utf8_surrogate_layout = 0x0D200000;
    static const uint32_t seq4_range_mask = 0x07300000;
    static const uint32_t seq4_pattern_mask = 0x04000000;

    static const uint32_t header_mask[4] = {
        0x80000000,
        0xE0C00000,
        0xF0C0C000,
        0xF8C0C0C0,
    };

    static const uint32_t header_layout[4] = {
        0x00000000,
        0xC0800000,
        0xE0808000,
        0xF0808080,
    };

    static const uint32_t range_mask[4] = {
        0x7F000000,
        0x1E000000,
        0x0F200000,
        0x07300000,
    };

    static const uint8_t length_table[16] = {
        1, 1, 1, 1, 1, 1, 1, 1, /* 0... */
        1, 1, 1, 1,             /* 10.. continuation byte, invalid */
        2, 2,                   /* 110. */
        3,                      /* 1110 */
        4,                      /* 1111 or invalid */
    };

    uintptr_t offset = *offset_ptr;
    uint32_t cache_offset = *cache_offset_ptr;

    uint8_t current_char = buffer[offset];

    /* Read UTF-8 */
    uint8_t seq_length = length_table[current_char >> 4];
    uint32_t value = 0;

    if (offset + seq_length >= buffer_size) {
        (*status) = PLAIN_JSON_ERROR_STRING_UTF8_INVALID;
        return false;
    }

    switch (seq_length) {
    case 4:
        value |= (uint32_t)buffer[offset + 3] & 0xff;
        cache[cache_offset + 3] = buffer[offset + 3];
        /* fallthrough */
    case 3:
        value |= ((uint32_t)buffer[offset + 2] & 0xff) << 8;
        cache[cache_offset + 2] = buffer[offset + 2];
        /* fallthrough */
    case 2:
        value |= ((uint32_t)buffer[offset + 1] & 0xff) << 16;
        cache[cache_offset + 1] = buffer[offset + 1];
        /* fallthrough */
    case 1:
        value |= ((uint32_t)current_char & 0xff) << 24;
        cache[cache_offset] = buffer[offset];
        break;
    default:
        /* unreachable */
        json_assert(false);
    }

    if (((value & header_mask[seq_length - 1]) != header_layout[seq_length - 1]) ||
        (value & range_mask[seq_length - 1]) == 0) {
        (*status) = PLAIN_JSON_ERROR_STRING_UTF8_INVALID;
        return false;
    }

    /* Encoded surrogate pairs are not legal utf-8 */
    if ((value & utf8_surrogate_mask) == utf8_surrogate_layout) {
        (*status) = PLAIN_JSON_ERROR_STRING_UTF8_HAS_SURROGATE;
        return false;
    }

    if (seq_length == 4 && (value & seq4_range_mask) > seq4_pattern_mask) {
        (*status) = PLAIN_JSON_ERROR_STRING_UTF8_INVALID;
        return false;
    }

    (*offset_ptr) = offset + seq_length;
    (*cache_offset_ptr) = cache_offset + seq_length;

    return PLAIN_JSON_DONE;
}

static inline plain_json_ErrorType plain_json_intern_read_escape(
    const uint8_t *buffer, uintptr_t buffer_size, uintptr_t *offset_ptr, uint8_t *cache,
    uint32_t *cache_offset_ptr, plain_json_ErrorType *status
) {
    static const uint32_t surrogate_mask = 0x0000FC00;
    static const uint32_t high_surrogate_layout = 0x0000D800;
    static const uint32_t low_surrogate_layout = 0x0000DC00;

    uintptr_t offset = *offset_ptr;
    uint32_t cache_offset = *cache_offset_ptr;

    uint8_t current_char = buffer[++offset];

    /* At this point, we will always read at least two characters. */
    offset++;
    switch (current_char) {
    case '\\':
        cache[cache_offset] = '\\';
        break;
    case '\"':
        cache[cache_offset] = '"';
        break;
    case '/':
        cache[cache_offset] = '/';
        break;
    case 'b':
        cache[cache_offset] = '\b';
        break;
    case 'f':
        cache[cache_offset] = '\f';
        break;
    case 'n':
        cache[cache_offset] = '\n';
        break;
    case 'r':
        cache[cache_offset] = '\r';
        break;
    case 't':
        cache[cache_offset] = '\t';
        break;
    case 'u':;
        /* Parse UTF-16 */
        uint32_t codepoint = 0;
        if (offset + 4 >= buffer_size ||
            !plain_json_intern_parse_utf16(buffer + offset, buffer_size - offset, &codepoint)) {
            /* Error: Codepoint is invalid */
            (*status) = PLAIN_JSON_ERROR_STRING_UTF16_INVALID;
            return false;
        }
        offset += 4;

        if ((codepoint & surrogate_mask) == low_surrogate_layout) {
            /* Error: Unexpected low surrogate */
            (*status) = PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE;
            return false;
        }

        if ((codepoint & surrogate_mask) == high_surrogate_layout) {
            /* Read low surrogate */
            if (offset + 2 >= buffer_size || buffer[offset] != '\\' || buffer[offset + 1] != 'u') {
                /* Error: Second codepoint missing */
                (*status) = PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE;
                return false;
            }
            offset += 2;

            uint32_t high_surrogate = codepoint;
            codepoint = 0;
            if (offset + 4 >= buffer_size ||
                !plain_json_intern_parse_utf16(buffer + offset, buffer_size - offset, &codepoint)) {
                /* Error: Second codepoint is invalid */
                (*status) = PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE;
                return false;
            }

            offset += 4;
            if ((codepoint & surrogate_mask) != low_surrogate_layout) {
                /* Error: Second codepoint is not a valid lower surrogate half */
                (*status) = PLAIN_JSON_ERROR_STRING_UTF16_INVALID_SURROGATE;
                return false;
            }

            uint32_t low_surrogate = codepoint;
            codepoint = ((high_surrogate & 0x3FF) << 10) | (low_surrogate & 0x3FF);
            codepoint += 0x10000;
        }

        if (!plain_json_intern_encode_utf8(codepoint, cache, &cache_offset)) {
            (*status) = PLAIN_JSON_ERROR_STRING_UTF16_INVALID;
            return false;
        }

        break;

    default:
        (*status) = PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE;
        return false;
    }

    (*offset_ptr) = offset;
    (*cache_offset_ptr) = cache_offset;

    return PLAIN_JSON_DONE;
}

static plain_json_ErrorType plain_json_intern_read_string(plain_json_Context *context) {
    const uint8_t *buffer = context->buffer + context->buffer_offset;
    const uintptr_t buffer_size = context->buffer_size - context->buffer_offset;

    uint8_t cache[PLAIN_JSON_INTERN_STRING_CACHE] = { 0 };
    uint32_t cache_offset = 0;

    uintptr_t offset = 0;
    uint8_t current_char = '\0';

    while (offset < buffer_size) {
        current_char = buffer[offset];

        /* Commit the cache and reserve 5 bytes for potential unicode characters + '\0' */
        if (cache_offset + 5 >= PLAIN_JSON_INTERN_STRING_CACHE || current_char == '\"') {
            if (!plain_json_intern_list_append(
                    &context->string_buffer, &context->alloc_config, cache,
                    cache_offset + (4 - cache_offset % 4)
                )) {
                return PLAIN_JSON_ERROR_NO_MEMORY;
            }

            cache_offset = 0;
        }

        if (current_char == '\"') {
            break;
        }

        /* Read escapes */
        if (current_char == '\\') {
            if (offset + 1 >= buffer_size) {
                return PLAIN_JSON_ERROR_STRING_UNTERMINATED;
            }

            plain_json_ErrorType status = PLAIN_JSON_DONE;
            if (!plain_json_intern_read_escape(
                    buffer, buffer_size, &offset, cache, &cache_offset, &status
                )) {
                return status;
            }

            continue;
        }

        if (current_char == '\0' || current_char == '\n') {
            return PLAIN_JSON_ERROR_STRING_UNTERMINATED;
        }

        if (current_char < 0x20) {
            return PLAIN_JSON_ERROR_STRING_INVALID_ASCII;
        }

        /* Read ASCII */
        if (current_char < 0x80) {
            cache[cache_offset] = current_char;
            cache_offset += 1;
            offset += 1;
            continue;
        }

        plain_json_ErrorType status = PLAIN_JSON_DONE;
        if (!plain_json_intern_read_utf8(
                buffer, buffer_size, &offset, cache, &cache_offset, &status
            )) {
            return status;
        }
    }

    if (offset >= buffer_size) {
        return PLAIN_JSON_ERROR_STRING_UNTERMINATED;
    }

    plain_json_intern_consume(context, offset + 1);
    return PLAIN_JSON_HAS_REMAINING;
}

static inline plain_json_ErrorType
plain_json_intern_read_keyword(plain_json_Context *context, plain_json_Token *token) {
    const uint8_t *buffer = context->buffer + context->buffer_offset;
    const uintptr_t buffer_size = context->buffer_size - context->buffer_offset;
    json_assert(buffer[0] == 't' || buffer[0] == 'f' || buffer[0] == 'n');

    switch (buffer[0]) {
    case 'n':
        if (buffer_size > 3 && buffer[1] == 'u' && buffer[2] == 'l' && buffer[3] == 'l') {
            plain_json_intern_consume(context, 4);
            token->type = PLAIN_JSON_TYPE_NULL;
            token->length = 4;
            return PLAIN_JSON_HAS_REMAINING;
        }
        break;
    case 't':
        if (buffer_size > 3 && buffer[1] == 'r' && buffer[2] == 'u' && buffer[3] == 'e') {
            plain_json_intern_consume(context, 4);
            token->type = PLAIN_JSON_TYPE_TRUE;
            token->length = 4;
            return PLAIN_JSON_HAS_REMAINING;
        }
        break;
    case 'f':
        if (buffer_size > 4 && buffer[1] == 'a' && buffer[2] == 'l' && buffer[3] == 's' &&
            buffer[4] == 'e') {
            plain_json_intern_consume(context, 5);
            token->type = PLAIN_JSON_TYPE_FALSE;
            token->length = 5;
            return PLAIN_JSON_HAS_REMAINING;
        }
        break;
    }

    return PLAIN_JSON_ERROR_KEYWORD_INVALID;
}

static inline plain_json_ErrorType
plain_json_intern_read_number(plain_json_Context *context, plain_json_Token *token) {
    enum {
        READ_INT,
        READ_DECIMAL,
        READ_EXPO,
    } status = READ_INT;

    static const int64_t int_max = INT64_MAX;

    const uint8_t *buffer = context->buffer + context->buffer_offset;
    uintptr_t buffer_size = context->buffer_size - context->buffer_offset;
    uintptr_t offset = 0;

    uint8_t int_has_sign = 0;
    int64_t int_value = 0;

    __attribute__((unused)) uint8_t expo_has_sign = 0;
    intptr_t expo_value = 0;

    __attribute__((unused)) uint32_t decimal_start = 0;
    __attribute__((unused)) uint32_t decimal_length = 0;

    while (offset < buffer_size) {
        switch (buffer[offset]) {
        case '0':
            if (status == READ_INT && (offset == 0 || (offset == 1 && int_has_sign))) {
                if (offset + 1 < buffer_size && is_digit(buffer[offset + 1])) {
                    return PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO;
                }
            }
            /* fallthrough */
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (status == READ_INT) {
                /* Check for over/underflow */

                // FIXME: Depending on the wordsize, the int_max value might end in different digits
                // (ie. i32 => '8', i64 => '7').
                if (!int_has_sign) {
                    if (int_value > (int_max / 10) ||
                        (int_value == (int_max / 10) && buffer[offset] > '7')) {
                        return PLAIN_JSON_ERROR_NUMBER_OVERFLOW;
                    }

                    int_value *= 10;
                    int_value += buffer[offset] - '0';
                } else {
                    if (int_value < (int_max / 10) * -1 ||
                        (int_value == (int_max / 10) * -1 && buffer[offset] > '8')) {
                        return PLAIN_JSON_ERROR_NUMBER_UNDERFLOW;
                    }

                    int_value *= 10;
                    int_value -= buffer[offset] - '0';
                }

                if (offset + 1 < buffer_size && buffer[offset + 1] == '.') {
                    status = READ_DECIMAL;
                    decimal_start = offset + 1;
                    offset += 1;

                    if (offset + 1 < buffer_size && !is_digit(buffer[offset + 1])) {
                        return PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL;
                    }
                }
            }

            if (status == READ_EXPO) {
                /* TODO: Make sure the resulting number (int/float * expo) can fit into the wordsize
                 */
                if (expo_value > (int_max / 10) ||
                    (int_value == (int_max / 10) && buffer[offset] > '7')) {
                    return PLAIN_JSON_ERROR_NUMBER_OVERFLOW;
                }

                expo_value *= 10;
                expo_value += buffer[offset] - '0';
            }

            break;
        case '-':
            if (status == READ_INT && offset == 0) {
                int_has_sign = 1;
                if (offset + 1 < buffer_size && !is_digit(buffer[offset + 1])) {
                    return PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN;
                }

                break;
            }

            /* '+/-' are consumed as part of the exponent token 'e' */
            /* fallthrough */
        case '+':
            return PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN;
            /* '.' is consumed as part of the int part */
        case '.':
            return PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL;
        case 'e':
        case 'E':
            if (status == READ_EXPO) {
                return PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO;
            }

            if (offset + 1 >= buffer_size) {
                return PLAIN_JSON_ERROR_UNEXPECTED_EOF;
            }

            if (status != READ_INT) {
                decimal_length = offset - decimal_start;
            }

            uint8_t next_char = buffer[offset + 1];
            if (next_char == '+' || next_char == '-') {
                offset += 1;
                expo_has_sign = 1;
            }

            if (offset + 1 < buffer_size && !is_digit(buffer[offset + 1])) {
                return PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO;
            }

            status = READ_EXPO;
            break;

        default:
            goto done;
        }

        offset++;
    }

done:
    switch (status) {
    case READ_INT:
        token->value.integer = int_value;
        break;
    case READ_DECIMAL:
        token->value.real32 = 0;
        break;
    case READ_EXPO:
        break;
    }

    token->length = offset;
    plain_json_intern_consume(context, offset);
    return PLAIN_JSON_HAS_REMAINING;
}

#define get_state()      context->depth_buffer[context->depth_buffer_index]
#define set_state(state) (get_state() = (state))
#define has_state(state) ((get_state() & (state)) > 0)

#define check_for_comma()                              \
    if (has_state(PLAIN_JSON_STATE_NEEDS_COMMA) > 0) { \
        status = PLAIN_JSON_ERROR_MISSING_COMMA;       \
        break;                                         \
    }
#define check_state(state)                      \
    if (!has_state(state)) {                    \
        status = PLAIN_JSON_ERROR_ILLEGAL_CHAR; \
        break;                                  \
    }

#define push_state()                                                     \
    if (context->depth_buffer_index + 1 < PLAIN_JSON_OPTION_MAX_DEPTH) { \
        context->depth_buffer_index++;                                   \
    } else {                                                             \
        status = PLAIN_JSON_ERROR_NESTING_TOO_DEEP;                      \
        break;                                                           \
    }
#define pop_state()                                \
    if (context->depth_buffer_index > 0) {         \
        context->depth_buffer_index--;             \
    } else {                                       \
        status = PLAIN_JSON_ERROR_UNEXPECTED_ROOT; \
        break;                                     \
    }

static plain_json_ErrorType
plain_json_intern_read_token(plain_json_Context *context, plain_json_Token *token) {
    /* Indicate if this value is prefixed by a comma. Objects/Array can be
     * empty, meaning we can't rely on the state to tell us is if a comma
     * has been processed. */
    bool has_comma = false;
    plain_json_ErrorType status = PLAIN_JSON_HAS_REMAINING;

    while (plain_json_intern_has_next(context, 0)) {
        token->start = context->buffer_offset;
        token->length = 1;

        uint8_t current_char = plain_json_intern_peek(context, 0);
        switch (current_char) {
            /* Read blanks */
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            plain_json_intern_consume(context, 1);
            continue;
        case '{':
            /* The file root may contain object/arrays */
            check_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_FIRST_TOKEN
            );
            check_for_comma();
            plain_json_intern_consume(context, 1);
            if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                set_state(PLAIN_JSON_STATE_NEEDS_KEY);
            }

            set_state(get_state() | PLAIN_JSON_STATE_NEEDS_COMMA);
            push_state();
            set_state(PLAIN_JSON_STATE_NEEDS_KEY);

            token->type = PLAIN_JSON_TYPE_OBJECT_START;
            break;
        case '}':
            check_state(PLAIN_JSON_STATE_NEEDS_KEY);
            if (has_comma) {
                status = PLAIN_JSON_ERROR_UNEXPECTED_COMMA;
                break;
            }
            pop_state();

            plain_json_intern_consume(context, 1);
            token->type = PLAIN_JSON_TYPE_OBJECT_END;
            break;
        case '[':
            check_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_FIRST_TOKEN
            );
            check_for_comma();

            if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                set_state(PLAIN_JSON_STATE_NEEDS_KEY);
            }
            set_state(get_state() | PLAIN_JSON_STATE_NEEDS_COMMA);
            push_state();
            set_state(PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE);

            plain_json_intern_consume(context, 1);
            token->type = PLAIN_JSON_TYPE_ARRAY_START;
            break;
        case ']':
            check_state(PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE);
            if (has_comma) {
                return PLAIN_JSON_ERROR_UNEXPECTED_COMMA;
            }
            pop_state();

            plain_json_intern_consume(context, 1);
            token->type = PLAIN_JSON_TYPE_ARRAY_END;
            break;
        case ',':
            check_state(PLAIN_JSON_STATE_NEEDS_COMMA);
            /* The root level can only contain a single value */
            if (has_state(PLAIN_JSON_STATE_IS_ROOT)) {
                status = PLAIN_JSON_ERROR_UNEXPECTED_COMMA;
                break;
            }
            set_state(get_state() ^ PLAIN_JSON_STATE_NEEDS_COMMA);

            plain_json_intern_consume(context, 1);
            has_comma = true;
            continue;
        case ':':
            check_state(PLAIN_JSON_STATE_NEEDS_COLON);
            set_state(PLAIN_JSON_STATE_NEEDS_VALUE);
            plain_json_intern_consume(context, 1);
            continue;
        case '"':
            check_for_comma();
            plain_json_intern_consume(context, 1);

            if (has_state(PLAIN_JSON_STATE_NEEDS_KEY)) {
                set_state(PLAIN_JSON_STATE_NEEDS_COLON);

                token->key_index = context->string_buffer.buffer_size;
                status = plain_json_intern_read_string(context);
                if (status != PLAIN_JSON_HAS_REMAINING) {
                    break;
                }
                continue;

            } else if (has_state(
                           PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                           PLAIN_JSON_STATE_IS_FIRST_TOKEN
                       )) {
                if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                    set_state(PLAIN_JSON_STATE_NEEDS_KEY);
                }
                set_state(get_state() | PLAIN_JSON_STATE_NEEDS_COMMA);

                token->start = context->buffer_offset;
                token->type = PLAIN_JSON_TYPE_STRING;
                token->value.string_index = context->string_buffer.buffer_size;
                status = plain_json_intern_read_string(context);
                break;
            }

            status = PLAIN_JSON_ERROR_ILLEGAL_CHAR;
            break;
        case 'n':
        case 't':
        case 'f':
            check_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_FIRST_TOKEN
            );
            check_for_comma();
            if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                set_state(PLAIN_JSON_STATE_NEEDS_KEY);
            }
            set_state(get_state() | PLAIN_JSON_STATE_NEEDS_COMMA);
            status = plain_json_intern_read_keyword(context, token);
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '0':
        case '-':
        case '+':
            check_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_FIRST_TOKEN
            );
            check_for_comma();
            if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                set_state(PLAIN_JSON_STATE_NEEDS_KEY);
            }
            set_state(get_state() | PLAIN_JSON_STATE_NEEDS_COMMA);

            token->type = PLAIN_JSON_TYPE_INTEGER;
            status = plain_json_intern_read_number(context, token);
            break;
        default:
            status = PLAIN_JSON_ERROR_ILLEGAL_CHAR;
            break;
        }

        /* Workaround to correctly handle lone values at root level */
        context->depth_buffer[0] = PLAIN_JSON_STATE_IS_ROOT;

        if (status != PLAIN_JSON_HAS_REMAINING) {
            token->type = PLAIN_JSON_TYPE_ERROR;
            token->start = context->buffer_offset;
            token->value.string_index = 0;
        }

        return status;
    }

    if (has_state(PLAIN_JSON_STATE_IS_ROOT)) {
        return PLAIN_JSON_DONE;
    }

    return PLAIN_JSON_ERROR_UNEXPECTED_EOF;
}

plain_json_Context *plain_json_parse(
    plain_json_AllocatorConfig alloc_config, const uint8_t *buffer, uintptr_t buffer_size,
    plain_json_ErrorType *error
) {
    plain_json_Context *context = alloc_config.alloc_func(sizeof(*context));
    plain_json_intern_memset(context, 0, sizeof(*context));

    context->alloc_config = alloc_config;
    context->string_buffer.item_size = 1;
    context->token_buffer.item_size = sizeof(plain_json_Token);

    context->depth_buffer_index = 0;
    /* Workaround to correctly handle lone values at root level. This state is only valid for the
     * topmost/first parsed token */
    context->depth_buffer[0] = PLAIN_JSON_STATE_IS_FIRST_TOKEN;

    context->buffer = (uint8_t *)buffer;
    context->buffer_size = buffer_size;

    plain_json_ErrorType status = PLAIN_JSON_HAS_REMAINING;
    while (status == PLAIN_JSON_HAS_REMAINING) {
        plain_json_Token token = { 0 };

        token.type = PLAIN_JSON_TYPE_INVALID;
        token.key_index = PLAIN_JSON_NO_KEY;
        token.value.real64 = 0;

        status = plain_json_intern_read_token(context, &token);

        if (status != PLAIN_JSON_DONE &&
            !plain_json_intern_list_append(
                &context->token_buffer, &context->alloc_config, &token, 1
            )) {
            (*error) = status;
            return context;
        }
    }

    (*error) = status;
    return context;
}

void plain_json_free(plain_json_Context *context) {
    if (context->token_buffer.buffer != PLAIN_JSON_NULL) {
        context->alloc_config.free_func(context->token_buffer.buffer);
        context->token_buffer.buffer = PLAIN_JSON_NULL;
    }
    if (context->string_buffer.buffer != PLAIN_JSON_NULL) {
        context->alloc_config.free_func(context->string_buffer.buffer);
        context->string_buffer.buffer = PLAIN_JSON_NULL;
    }

    context->alloc_config.free_func(context);
}

static const uint8_t *plain_json_list_get(plain_json_List *list, uint32_t index) {
    if (index >= list->buffer_size / list->item_size) {
        return PLAIN_JSON_NULL;
    }

    return list->buffer + list->item_size * index;
}

const uint8_t *plain_json_get_key(plain_json_Context *context, uint32_t key_index) {
    return plain_json_list_get(&context->string_buffer, key_index);
}

const uint8_t *plain_json_get_string(plain_json_Context *context, uint32_t key_index) {
    return plain_json_list_get(&context->string_buffer, key_index);
}

const plain_json_Token *plain_json_get_token(plain_json_Context *context, uint32_t index) {
    return (plain_json_Token *)plain_json_list_get(&context->token_buffer, index);
}

uint32_t plain_json_get_token_count(plain_json_Context *context) {
    return context->token_buffer.buffer_size / context->token_buffer.item_size;
}

bool plain_json_compute_position(
    plain_json_Context *context, uintptr_t offset, uint32_t *line, uint32_t *line_offset
) {
    if (offset >= context->buffer_size) {
        return false;
    }

    (*line) = 0;
    (*line_offset) = 0;
    for (uintptr_t i = 0; i < offset; i++) {
        (*line_offset)++;
        if (context->buffer[i] == '\n') {
            (*line_offset) = 0;
            (*line)++;
        }
    }

    return true;
}

const char *plain_json_type_to_string(plain_json_Type type) {
    switch (type) {
    case PLAIN_JSON_TYPE_INVALID:
        return "invalid";
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
    case PLAIN_JSON_TYPE_NULL:
        return "null";
    case PLAIN_JSON_TYPE_TRUE:
        return "true";
    case PLAIN_JSON_TYPE_FALSE:
        return "false";
    case PLAIN_JSON_TYPE_INTEGER:
        return "integer";
    case PLAIN_JSON_TYPE_FLOAT32:
        return "real32";
    case PLAIN_JSON_TYPE_FLOAT64:
        return "real64";
    case PLAIN_JSON_TYPE_ERROR:
        return "error_token";
    }

    return "unknown_type";
}

const char *plain_json_error_to_string(plain_json_ErrorType type) {
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
    case PLAIN_JSON_ERROR_NUMBER_OVERFLOW:
        return "number_overflow";
    case PLAIN_JSON_ERROR_NUMBER_UNDERFLOW:
        return "number_underflow";
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
    case PLAIN_JSON_ERROR_NO_MEMORY:
        return "no_memory";
    case PLAIN_JSON_DONE:
        return "done";
    case PLAIN_JSON_NONE:
        return "none";
    }

    return "unknown_error";
}

    #undef check_for_comma
    #undef verify_state
    #undef has_state
    #undef push_state
    #undef pop_state

    #undef is_hex
    #undef is_blank
    #undef is_digit
    #undef json_assert

#endif
