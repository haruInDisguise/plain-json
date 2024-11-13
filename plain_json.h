#ifndef _PLAIN_JSON_H_
#define _PLAIN_JSON_H_

#ifndef PLAIN_JSON_OPTION_MAX_DEPTH
    #define PLAIN_JSON_OPTION_MAX_DEPTH 32
#endif

#define PLAIN_JSON_STATE_IS_START 0x01
#define PLAIN_JSON_STATE_IS_ROOT  0x02

#define PLAIN_JSON_STATE_NEEDS_KEY         0x04
#define PLAIN_JSON_STATE_NEEDS_COMMA       0x08
#define PLAIN_JSON_STATE_NEEDS_VALUE       0x10
#define PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE 0x20
#define PLAIN_JSON_STATE_NEEDS_COLON       0x40

typedef enum {
    PLAIN_JSON_DONE,
    PLAIN_JSON_HAS_REMAINING,

    PLAIN_JSON_ERROR_STRING_INVALID_ASCII,
    PLAIN_JSON_ERROR_STRING_UNTERMINATED,
    PLAIN_JSON_ERROR_STRING_INVALID_UTF8,
    PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE,
    PLAIN_JSON_ERROR_STRING_INVALID_UTF16_ESCAPE,

    PLAIN_JSON_ERROR_NUMBER_LEADING_ZERO,
    PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO,
    PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN,
    PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL,

    PLAIN_JSON_ERROR_KEYWORD_INVALID,

    PLAIN_JSON_ERROR_MISSING_COMMA,
    PLAIN_JSON_ERROR_NESTING_TOO_DEEP,

    PLAIN_JSON_ERROR_UNEXPECTED_EOF,
    PLAIN_JSON_ERROR_UNEXPECTED_COMMA,
    PLAIN_JSON_ERROR_UNEXPECTED_ROOT,
    PLAIN_JSON_ERROR_UNEXPECTED_TOKEN,
} plain_json_ErrorType;

typedef enum {
    PLAIN_JSON_TYPE_INVALID,
    PLAIN_JSON_TYPE_ERROR,

    PLAIN_JSON_TYPE_NULL,
    PLAIN_JSON_TYPE_TRUE,
    PLAIN_JSON_TYPE_FALSE,

    PLAIN_JSON_TYPE_OBJECT_START,
    PLAIN_JSON_TYPE_OBJECT_END,

    PLAIN_JSON_TYPE_ARRAY_START,
    PLAIN_JSON_TYPE_ARRAY_END,

    PLAIN_JSON_TYPE_STRING,
    PLAIN_JSON_TYPE_NUMBER,
} plain_json_Type;

/// A single JSON token
///
/// FIELDS
///     start:          The byte offset where the value (if any) starts
///     length:         The values length in bytes
///     key_start:      The byte offset where the key string (if any) starts
///     key_length:     The keys length in bytes
///     type:           The type of this token
///
/// NOTE
///     If the token has no value, both start and length will be zero. If it has no key, key_start
///     and key_length will be zero.
typedef struct {
    unsigned int start;
    unsigned int length;

    unsigned int key_start;
    unsigned int key_length;

    plain_json_Type type;
} plain_json_Token;

/// The parsing context.
///
/// FIELDS
///     line:           The current line number
///     line_offset:    The current line offset
///
/// NOTE
///     All fields should be treated as read-only. Values starting with an underscore are reserved
///     for internal use
typedef struct {
    const char *_buffer;

    unsigned int _buffer_size;
    unsigned int _buffer_offset;

    unsigned short _depth_buffer_index;
    unsigned short _depth_buffer[PLAIN_JSON_OPTION_MAX_DEPTH];

    int line;
    int line_offset;
} plain_json_Context;

/// Read a single token
///
/// ARGS
///     context:            The context that tracks the parsing process.
///     token:              The processed token
///
/// RETURN
///     PLAIN_JSON_TRUE:    There is unprocessed JSON data.
///     PLAIN_JSON_FALSE:   The JSON data has been fullly processed.
///     <error code>:       An error occured during processing.
extern plain_json_ErrorType
plain_json_read_token(plain_json_Context *context, plain_json_Token *token);

/// Read tokens into a preallocated buffer.
///
/// ARGS
///     context:            The context that tracks the parsing process
///     tokens:             A preallocated buffer of tokens.
///     num_tokens:         The size of the preallocated token buffer.
///     tokens_read:        Stores the total number of read tokens.
///
/// RETURN
///     PLAIN_JSON_TRUE:    The entire token buffer has been filled, but there is still unprocessed
///     JSON data. PLAIN_JSON_FALSE:   The JSON data has been fully processed. <error code>: An
///     error occured during processing.
extern plain_json_ErrorType plain_json_read_token_buffered(
    plain_json_Context *context, plain_json_Token *tokens, int num_tokens, int *tokens_read
);

/// Setup a new parsing context.
///
/// ARGS
///     context             The target context
///     buffer              The raw JSON text.
///     buffer_size         Size of the JSON text.
extern void
plain_json_load_buffer(plain_json_Context *context, const char *buffer, unsigned long buffer_size);

#endif

/* Implementation */

#ifdef PLAIN_JSON_IMPLEMENTATION

#ifdef PLAIN_JSON_DEBUG
    #if __has_builtin(__builtin_debugtrap)
        #define json_assert(condition) \
            if (!(condition)) {        \
                __builtin_debugtrap(); \
            }
    #elif defined(__gcc__)
        #define json_assert(condition) \
            if (!(condition)) {        \
                __builtin_debugtrap(); \
            }
    #else
        #define json_assert(...)
    #endif
#else
    #define json_assert(...)
#endif

#define is_blank(c) (c == ' ' || c == '\t' || c == '\n' || c == '\r')
#define is_digit(c) (c >= '0' && c <= '9')
#define is_hex(c)   ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (is_digit(c)))

/* Token */

static inline void
plain_json_intern_token_reset(plain_json_Context *context, plain_json_Token *token) {
    token->length = 0;
    token->start = context->_buffer_offset;
    token->key_length = 0;
    token->key_start = 0;
}

/* Util */

static inline char plain_json_intern_has_next(plain_json_Context *context, int offset) {
    return (context->_buffer_offset + offset < context->_buffer_size);
}

static inline char plain_json_intern_consume(plain_json_Context *context, int count) {
    json_assert(context->buffer_offset + count < context->buffer_size);
    char tmp = context->_buffer[context->_buffer_offset];
    context->_buffer_offset += count;
    return tmp;
}

static inline char plain_json_intern_peek(plain_json_Context *context, int offset) {
    json_assert(context->buffer_offset + offset < context->buffer_size);
    return context->_buffer[context->_buffer_offset + offset];
}

/* Parsing */

static inline void plain_json_intern_read_blanks(plain_json_Context *context) {
    while (plain_json_intern_has_next(context, 0) && is_blank(plain_json_intern_peek(context, 0))) {
        char blank = plain_json_intern_consume(context, 1);
        if (blank == '\n') {
            context->line++;
            context->line_offset = 0;
        }
    }
}

/* UTF-8 encoding
 *   - UCS characters are simply encoded as there ascii equivalent (0x00 to 0x7f)
 *   - All UCS characts greater than 0x7f are encoded using a multi byte sequence (up to 4 bytes)
 *      - Those byes range from 0x80 to 0xfd
 *      - The first byte is in the range of 0xc2 (partially within ascii range!) to 0xfd. It
 *        indicates the length of the multibyte sequence
 *      - All further bytes are in the range of 0x80 to 0xbf
 *   - The bytes 0xc0, 0xc1, 0xfe nad 0xff are not used in UTF-8 encoding
 *
 *   References used:
 *   - utf-8(7)
 *   - "Unicode Encoding! UTF-32, ...": https://www.youtube.com/watch?v=uTJoJtNYcaQ&t=1246s
 *   - "Wikipedia UTF-8": https://en.wikipedia.org/wiki/UTF-8
 *   - "RFC3629": https://www.rfc-editor.org/rfc/rfc3629
 *
 * Layout
 * Bytes are collected into a 32bit 'value' and compared to a bitmask:
 *   - The mask 'mask' validates the starting sequence byte. The result must match 'pattern'
 *   - The mask 'code' ensures that the codepoint value is within the minimum range
 *
 * surrogate    : U+D800 - U+DFFF
 * 1 byte
 * range   : U+0000 - U+007F
 * min     : .0000000 (0x00)
 * max     : .1111111 (0x7f)
 * mask    : 1....... (0x80)
 * pattern : 0....... (0x00)
 *
 * 2 bytes
 * invalid      : 0xC0, 0xC1
 * range        : U+0080 - U+07FF
 * layout       : 110xxxyy 10yyzzzz
 * min          : ...00010 ..000000
 * max          : ...11111 ..111111
 * code_mask    : ...1111. ........ (0x1E)
 * pattern      : 110..... 10...... (0xC0, 0x80)
 * mask         : 111..... 11...... (0xE0, 0xC0)
 *
 * 3 bytes
 * range        : U+0800 - U+FFFF
 * layout       : 1110wwww 10xxxxyy 10yyzzzz
 * min          : ....0000 ..100000 ..000000
 * max          : ....1111 ..111111 ..111111
 * code_mask    : ....1111 ..100000 ........ (0x0F, 0x20, 0x00)
 * surr_mask    : 11101101 10100000
 * pattern      : 1110.... 10...... 10...... (0xE0, 0x80, 0x80)
 * mask         : 1111.... 11...... 11...... (0xF0, 0xC0, 0xC0)
 *
 * 4 bytes
 * range        : U+01000000 - U+10FFFF
 * layout       : 11110uvv 10vvwwww 10xxxxyy 10yyzzzz
 * min          : ........ ...10000 ..000000 ..000000
 * max          : .....100 ..001111 ..111111 ..111111
 * code_mask    : .....111 ..11.... ........ ........ (0x07, 0x30)
 * pattern      : 11110... 10...... 10...... 10...... (0xF0, 0x80, 0x80, 0x80)
 * mask         : 11111... 11...... 11...... 11...... (0xF8, 0xC0, 0xC0, 0xC0) */

static inline plain_json_ErrorType
plain_json_intern_read_string(plain_json_Context *context, unsigned int *token_length) {
    /* Basic layout mask */
    static const unsigned long mask_table[4] = {
        0x80000000UL,
        0xE0C00000UL,
        0xF0C0C000UL,
        0xF8C0C0C0UL,
    };

    /* Check basic layout */
    static const unsigned long pattern_table[4] = {
        0x00000000UL,
        0xC0800000UL,
        0xE0808000UL,
        0xF0808080UL,
    };

    /* Check for invalid codepoints */
    static const unsigned long code_table[] = {
        0x00000000UL,
        0x1E000000UL,
        0x0F200000UL,
        0x07300000UL,
    };

    /* The high nibble of the starting byte encodes its type and
     * therefore the length of the utf8 encoded sequence */
    static const unsigned char length_table[] = {
        1, 1, 1, 1, 1, 1, 1, 1, /* 0... */
        2, 2, 2, 2,             /* 10.. continuation byte, invalid */
        2, 2,                   /* 110. */
        3,                      /* 1110 */
        4,                      /* 1111 or invalid */
    };

    long offset = 0;
    char current_char = '\0';
    char previous_char = '\0';

    while (plain_json_intern_has_next(context, 0)) {
        current_char = plain_json_intern_consume(context, 1);

        if (previous_char == '\\') {
            goto read_escape;
        }

        if ((current_char & mask_table[0]) == pattern_table[0]) {
            goto read_ascii;
        }

        unsigned char length = length_table[(unsigned char)current_char >> 4];
        unsigned long value = 0;

        if (!plain_json_intern_has_next(context, length - 1)) {
            return PLAIN_JSON_ERROR_STRING_INVALID_UTF8;
        }

        switch (length) {
        case 4:
            value |= ((unsigned char)context->_buffer[context->_buffer_offset + 2] << 0);
        case 3:
            value |= ((unsigned char)context->_buffer[context->_buffer_offset + 1] << 8);
        case 2:
            value |= ((unsigned char)context->_buffer[context->_buffer_offset + 0] << 16);

            value |= ((unsigned int)current_char << 24);
            break;
        default:
            /* unreachable */
            json_assert(0);
        }

        /* FIXME: High/Low Surrogates are not valid UTF-8 */
        /* FIXME: Not all valid codepoints are used */
        unsigned long is_surrogate = 0; // (value >> 16) >= 0xEDA0;
        unsigned long masked = value & mask_table[length - 1];
        unsigned long code = value & code_table[length - 1];

        if (masked == pattern_table[length - 1] && (value & code) && !is_surrogate) {
            plain_json_intern_consume(context, length - 1);
            offset += length;
            previous_char = '\0';
            continue;
        }

        return PLAIN_JSON_ERROR_STRING_INVALID_UTF8;

    read_ascii:
        /* Found unescaped quotes */
        if (previous_char != '\\' && current_char == '\"') {
            goto has_string;
        }

        if (current_char == '\0' || current_char == '\n') {
            return PLAIN_JSON_ERROR_STRING_UNTERMINATED;
        }

        /* ASCII 0x00 to 0x1F are refered to as control characters */
        /* JSON supports a properly escaped subset */
        if (current_char < 0x20) {
            return PLAIN_JSON_ERROR_STRING_INVALID_ASCII;
        }

        previous_char = current_char;
        offset++;

        continue;

    read_escape:
        switch (current_char) {
        /* '\n' and '\0' are handled seperately */
        case '\\':
        case '\"':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
            break;
        case 'u':;
            /* Read and parse UTF-16 surrogate pairs */
            int i = 0;
            for (; i < 4 && plain_json_intern_has_next(context, 1); i++) {
                current_char = plain_json_intern_consume(context, 1);
                if (!is_hex(current_char)) {
                    return PLAIN_JSON_ERROR_STRING_INVALID_UTF16_ESCAPE;
                }
            }

            /* Read less than four digits. The string buffer is too short or the file buffer
             * is out of characters */
            if (i < 4) {
                return PLAIN_JSON_ERROR_STRING_INVALID_UTF16_ESCAPE;
            }

            offset += i;
            previous_char = '\0';
            continue;

        default:
            return PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE;
        }

        /* Indicate that an escaped sequence was fully consumed */
        previous_char = '\0';
        offset++;
    }

    return PLAIN_JSON_ERROR_STRING_UNTERMINATED;
has_string:
    (*token_length) = offset;
    return PLAIN_JSON_HAS_REMAINING;
}

static inline plain_json_ErrorType plain_json_intern_read_keyword(
    plain_json_Context *context, plain_json_Type type, unsigned int *token_length
) {
    const char *buffer = context->_buffer + context->_buffer_offset;
    const int buffer_size = context->_buffer_size - context->_buffer_offset;
    json_assert(buffer[0] == 't' || buffer[0] == 'f' || buffer[0] == 'n');

    switch (type) {
    case PLAIN_JSON_TYPE_NULL:
        if (buffer_size > 3 && buffer[1] == 'u' && buffer[2] == 'l' && buffer[3] == 'l') {
            plain_json_intern_consume(context, 4);
            (*token_length) = 4;
            return PLAIN_JSON_HAS_REMAINING;
        }
        break;
    case PLAIN_JSON_TYPE_TRUE:
        if (buffer_size > 3 && buffer[1] == 'r' && buffer[2] == 'u' && buffer[3] == 'e') {
            plain_json_intern_consume(context, 4);
            (*token_length) = 4;
            return PLAIN_JSON_HAS_REMAINING;
        }
        break;
    case PLAIN_JSON_TYPE_FALSE:
        if (buffer_size > 4 && buffer[1] == 'a' && buffer[2] == 'l' && buffer[3] == 's' &&
            buffer[4] == 'e') {
            plain_json_intern_consume(context, 5);
            (*token_length) = 5;
            return PLAIN_JSON_HAS_REMAINING;
        }
        break;
    default:
        json_assert(0);
    }

    return PLAIN_JSON_ERROR_KEYWORD_INVALID;
}

static inline plain_json_ErrorType
plain_json_intern_read_number(plain_json_Context *context, unsigned int *token_length) {
    enum {
        READ_INT,
        READ_DECIMAL,
        READ_EXPO,
    } status = READ_INT;

    const char *buffer = context->_buffer + context->_buffer_offset;
    int buffer_size = context->_buffer_size - context->_buffer_offset;

    int offset = 0;

    /* TODO: To be used for number parsing... */
    int int_has_sign = 0;
    int int_start = 0;
    int int_length = 0;

    int decimal_start = 0;
    int decimal_length = 0;

    int expo_has_sign = 0;
    int expo_start = 0;
    int expo_length = 0;

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
            if (status == READ_INT && offset + 1 < buffer_size && buffer[offset + 1] == '.') {
                status = READ_DECIMAL;
                decimal_start = offset + 1;
                offset += 1;
                int_length = offset - int_start;

                if (offset + 1 < buffer_size && !is_digit(buffer[offset + 1])) {
                    return PLAIN_JSON_ERROR_NUMBER_INVALID_DECIMAL;
                }
            }

            break;
        case '-':
            if (status == READ_INT && offset == 0) {
                int_start = offset + 1;
                int_has_sign = 1;
                if (offset + 1 < buffer_size && !is_digit(buffer[offset + 1])) {
                    return PLAIN_JSON_ERROR_NUMBER_INVALID_SIGN;
                }

                break;
            }

            /* '+/-' are consumed as part of the exponent token 'e' */
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

            if (status == READ_INT) {
                int_length = offset - int_start;
            } else {
                decimal_length = offset - decimal_start;
            }

            char next_char = buffer[offset + 1];
            if (next_char == '+' || next_char == '-') {
                offset += 1;
                expo_has_sign = 1;
            }

            if (!is_digit(buffer[offset + 1])) {
                return PLAIN_JSON_ERROR_NUMBER_INVALID_EXPO;
            }

            expo_start = offset + 1;
            status = READ_EXPO;
            break;
        default:
            goto done;
        }

        offset++;
    }

done:
    (*token_length) = offset;
    plain_json_intern_consume(context, offset);
    return PLAIN_JSON_HAS_REMAINING;
}

#define get_state()      context->_depth_buffer[context->_depth_buffer_index]
#define set_state(state) (get_state() = (state))
#define has_state(state) ((get_state() & (state)) > 0)

#define check_for_comma()                              \
    if (has_state(PLAIN_JSON_STATE_NEEDS_COMMA) > 0) { \
        return PLAIN_JSON_ERROR_MISSING_COMMA;         \
    }
#define check_state(state)                        \
    if (!has_state(state)) {                      \
        return PLAIN_JSON_ERROR_UNEXPECTED_TOKEN; \
    }

#define push_state()                                                      \
    if (context->_depth_buffer_index + 1 < PLAIN_JSON_OPTION_MAX_DEPTH) { \
        context->_depth_buffer_index++;                                   \
    } else {                                                              \
        return PLAIN_JSON_ERROR_NESTING_TOO_DEEP;                         \
    }
#define pop_state()                              \
    if (context->_depth_buffer_index > 0) {      \
        context->_depth_buffer_index--;          \
    } else {                                     \
        return PLAIN_JSON_ERROR_UNEXPECTED_ROOT; \
    }

static plain_json_ErrorType
plain_json_intern_read_token(plain_json_Context *context, plain_json_Token *token) {
    /* Indicate if this value is prefixed by a comma. Objects/Array can be
     * empty, meaning we can't rely on the state to tell us is if a comma
     * has been processed. */
    int has_comma = 0;
    int status = PLAIN_JSON_HAS_REMAINING;
    plain_json_intern_token_reset(context, token);

    while (plain_json_intern_has_next(context, 0)) {
        plain_json_intern_read_blanks(context);
        if (!plain_json_intern_has_next(context, 0)) {
            goto is_eof;
        }

        char current_char = plain_json_intern_peek(context, 0);
        switch (current_char) {
        case '{':
            /* The file root may contain object/arrays */
            check_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_START
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
            goto has_token;
        case '}':
            check_state(PLAIN_JSON_STATE_NEEDS_KEY);
            if (has_comma) {
                return PLAIN_JSON_ERROR_UNEXPECTED_COMMA;
            }
            pop_state();

            plain_json_intern_consume(context, 1);
            token->type = PLAIN_JSON_TYPE_OBJECT_END;
            goto has_token;
        case '[':
            check_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_START
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
            goto has_token;
        case ']':
            check_state(PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE);
            if (has_comma) {
                return PLAIN_JSON_ERROR_UNEXPECTED_COMMA;
            }
            pop_state();

            plain_json_intern_consume(context, 1);
            token->type = PLAIN_JSON_TYPE_ARRAY_END;
            goto has_token;
        case ',':
            check_state(PLAIN_JSON_STATE_NEEDS_COMMA);
            /* The root level can only contain a single value */
            if (has_state(PLAIN_JSON_STATE_IS_ROOT)) {
                return PLAIN_JSON_ERROR_UNEXPECTED_COMMA;
            }
            set_state(get_state() ^ PLAIN_JSON_STATE_NEEDS_COMMA);

            plain_json_intern_consume(context, 1);
            has_comma = 1;
            continue;
        case ':':
            check_state(PLAIN_JSON_STATE_NEEDS_COLON);
            set_state(PLAIN_JSON_STATE_NEEDS_VALUE);
            plain_json_intern_consume(context, 1);
            continue;
        /* A '"' can mean one of the following:
         *   1. The key of a value
         *   2. A string as a value
         *   3. A string as an array value */
        case '"':
            check_for_comma();
            plain_json_intern_consume(context, 1);

            if (has_state(PLAIN_JSON_STATE_NEEDS_KEY)) {
                set_state(PLAIN_JSON_STATE_NEEDS_COLON);

                token->key_start = context->_buffer_offset;
                status = plain_json_intern_read_string(context, &token->key_length);
                if (status != PLAIN_JSON_HAS_REMAINING) {
                    return status;
                }
                continue;

            } else if (has_state(
                           PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                           PLAIN_JSON_STATE_IS_START
                       )) {
                if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                    set_state(PLAIN_JSON_STATE_NEEDS_KEY);
                }
                set_state(get_state() | PLAIN_JSON_STATE_NEEDS_COMMA);

                token->start = context->_buffer_offset;
                token->type = PLAIN_JSON_TYPE_STRING;
                status = plain_json_intern_read_string(context, &token->length);
                goto has_token;
            }

            return PLAIN_JSON_ERROR_UNEXPECTED_TOKEN;
        case 'n':
            token->type = PLAIN_JSON_TYPE_NULL;
            goto read_keyword;
        case 't':
            token->type = PLAIN_JSON_TYPE_TRUE;
            goto read_keyword;
        case 'f':
            token->type = PLAIN_JSON_TYPE_FALSE;
        read_keyword:
            check_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_START
            );
            check_for_comma();
            if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                set_state(PLAIN_JSON_STATE_NEEDS_KEY);
            }
            set_state(get_state() | PLAIN_JSON_STATE_NEEDS_COMMA);

            status = plain_json_intern_read_keyword(context, token->type, &token->length);
            goto has_token;
        }

        if (is_digit(current_char) || current_char == '-' || current_char == '+') {
            check_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_START
            );
            check_for_comma();
            if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                set_state(PLAIN_JSON_STATE_NEEDS_KEY);
            }
            set_state(get_state() | PLAIN_JSON_STATE_NEEDS_COMMA);

            token->start = context->_buffer_offset;
            token->type = PLAIN_JSON_TYPE_NUMBER;
            status = plain_json_intern_read_number(context, &token->length);
            goto has_token;
        }

        return PLAIN_JSON_ERROR_UNEXPECTED_TOKEN;
    }

is_eof:
    /* There are no more tokens to read. */
    if (has_state(PLAIN_JSON_STATE_IS_ROOT)) {
        return PLAIN_JSON_DONE;
    }

    return PLAIN_JSON_ERROR_UNEXPECTED_EOF;

has_token:
    context->_depth_buffer[0] = PLAIN_JSON_STATE_IS_ROOT;
    return status;
}

plain_json_ErrorType plain_json_read_token(plain_json_Context *context, plain_json_Token *token) {
    return plain_json_intern_read_token(context, token);
}

plain_json_ErrorType plain_json_read_token_buffered(
    plain_json_Context *context, plain_json_Token *tokens, int num_tokens, int *tokens_read
) {
    int status = PLAIN_JSON_HAS_REMAINING;
    for ((*tokens_read) = 0; (*tokens_read) < num_tokens && status == PLAIN_JSON_HAS_REMAINING;
         (*tokens_read)++) {
        status = plain_json_intern_read_token(context, &tokens[*tokens_read]);
    }

    return status;
}

void plain_json_load_buffer(
    plain_json_Context *context, const char *buffer, unsigned long buffer_size
) {
    context->line = 0;
    context->line_offset = 0;
    context->_depth_buffer_index = 0;
    context->_depth_buffer[0] = PLAIN_JSON_STATE_IS_START;

    context->_buffer = buffer;
    context->_buffer_size = buffer_size;
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
