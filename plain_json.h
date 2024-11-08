#ifndef _PLAIN_JSON_H_
#define _PLAIN_JSON_H_

#ifndef PLAIN_JSON_OPTION_MAX_DEPTH
    #define PLAIN_JSON_OPTION_MAX_DEPTH 32
#endif

#define PLAIN_JSON_STATE_IS_ROOT 0x01

#define PLAIN_JSON_STATE_NEEDS_KEY             0x02
#define PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR 0x04
#define PLAIN_JSON_STATE_NEEDS_VALUE           0x08
#define PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE     0x10

#define PLAIN_JSON_STATE_NEEDS_OBJECT_END 0x20
#define PLAIN_JSON_STATE_NEEDS_ARRAY_END  0x40

typedef enum {
    PLAIN_JSON_FALSE = 0,
    PLAIN_JSON_TRUE = 1,

    PLAIN_JSON_ERROR_STRING_INVALID_ASCII,
    PLAIN_JSON_ERROR_STRING_UNTERMINATED,
    PLAIN_JSON_ERROR_STRING_INVALID_UTF8,
    PLAIN_JSON_ERROR_STRING_INVALID_ESCAPE,
    PLAIN_JSON_ERROR_STRING_INVALID_UTF16_ESCAPE,

    PLAIN_JSON_ERROR_NUMBER_INVALID,

    PLAIN_JSON_ERROR_KEYWORD_INVALID,

    PLAIN_JSON_ERROR_NO_MEMORY,
    PLAIN_JSON_ERROR_MISSING_FIELD_SEPERATOR,
    PLAIN_JSON_ERROR_TOO_DEEP,
    PLAIN_JSON_ERROR_IS_ROOT,
    PLAIN_JSON_ERROR_UNEXPECTED_TOKEN,
    PLAIN_JSON_ERROR_UNEXPECTED_EOF,
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

typedef struct {
    unsigned int start;
    unsigned int length;

    unsigned int key_start;
    unsigned int key_length;

    plain_json_Type type;
} plain_json_Token;

/*! */
typedef struct {
    const char *buffer;

    unsigned int buffer_size;
    unsigned int buffer_offset;

    unsigned char depth_buffer_index;
    unsigned char depth_buffer[PLAIN_JSON_OPTION_MAX_DEPTH];
    /* FIXME: Find a nicer workaround for empty arrays/objects */
    int _last_token_type;

    int line;
    int line_offset;
} plain_json_Context;

extern void plain_json_setup(plain_json_Context *context);
extern plain_json_ErrorType
plain_json_read_token(plain_json_Context *context, plain_json_Token *token);
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

#define is_blank(c) (c == ' ' || c == '\t' || c == '\f' || c == '\n' || c == '\v')
#define is_digit(c) (c >= '0' && c <= '9')
#define is_hex(c)   ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (is_digit(c)))

/* Token */

static inline void plain_json_intern_token_reset(
    plain_json_Context *context, plain_json_Token *token
) {
    context->_last_token_type = token->type;

    token->start = context->buffer_offset;
    token->length = 0;
    token->key_start = 0;
    token->key_length = 0;
}

/* Util */

static inline char plain_json_intern_has_next(plain_json_Context *context, int offset) {
    return (context->buffer_offset + offset < context->buffer_size);
}

/* Consume the current character + count */
static inline char plain_json_intern_consume(plain_json_Context *context, int count) {
    json_assert(context->buffer_offset + count < context->buffer_size);
    char next_char = context->buffer[context->buffer_offset];
    do {
        if (next_char == '\n') {
            context->line++;
            context->line_offset = 0;
        }

        next_char = context->buffer[context->buffer_offset++];
        context->line_offset++;
    } while (count--);

    return next_char;
}

static inline char plain_json_intern_peek(plain_json_Context *context, int offset) {
    json_assert(context->buffer_offset + offset < context->buffer_size);
    return context->buffer[context->buffer_offset + offset];
}

/* Parsing */

static inline void plain_json_intern_read_blanks(plain_json_Context *context) {
    while (plain_json_intern_has_next(context, 0) && is_blank(plain_json_intern_peek(context, 0))) {
        plain_json_intern_consume(context, 0);
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
    long offset = 0;
    char current_char = '\0';
    char previous_char = '\0';

    while (plain_json_intern_has_next(context, 0)) {
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

        current_char = plain_json_intern_consume(context, 0);
        if ((current_char & mask_table[0]) == pattern_table[0]) {
            goto read_ascii;
        }

        /* The 4 most significant bytes encode the length of the sequence */
        unsigned char length = length_table[(unsigned char)current_char >> 4];
        unsigned long value = 0;

        if (!plain_json_intern_has_next(context, length - 1)) {
            return PLAIN_JSON_ERROR_STRING_INVALID_UTF8;
        }

        /* TODO: Accessing the file buffer through peek() and consume() would be more consistent,
         * but I find it less readable. */
        switch (length) {
        case 4:
            value |= ((unsigned char)context->buffer[context->buffer_offset + 2] << 0);
        case 3:
            value |= ((unsigned char)context->buffer[context->buffer_offset + 1] << 8);
        case 2:
            value |= ((unsigned char)context->buffer[context->buffer_offset + 0] << 16);

            value |= ((unsigned int)current_char << 24);
            break;
        case 1:
            /* Unreachable */
            json_assert(0);
        }

        /* FIXME: High/Low Surrogates are not valid UTF-8 */
        /* FIXME: Not all valid codepoints are used */
        unsigned long is_surrogate = 0; // (value >> 16) >= 0xEDA0;
        unsigned long masked = value & mask_table[length - 1];
        unsigned long code = value & code_table[length - 1];

        if (masked == pattern_table[length - 1] && (value & code) && !is_surrogate) {
            plain_json_intern_consume(context, length - 2);
            offset += length;
            previous_char = '\0';
            continue;
        }

        return PLAIN_JSON_ERROR_STRING_INVALID_UTF8;

    read_ascii:
        /* Found unescaped quotes */
        if (previous_char != '\\' && current_char == '\"') {
            break;
        }

        if (current_char == '\0' || current_char == '\n') {
            return PLAIN_JSON_ERROR_STRING_UNTERMINATED;
        }

        /* ASCII 0x00 to 0x1F are refered to as control characters */
        /* JSON supports a properly escaped subset */
        if (current_char < 0x20) {
            return PLAIN_JSON_ERROR_STRING_INVALID_ASCII;
        }

        if (previous_char == '\\') {
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
                int i = 0;
                for (; i < 4 && plain_json_intern_has_next(context, 1); i++) {
                    current_char = plain_json_intern_consume(context, 0);

                    /* Check for valid hex digits */
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
            offset ++;
            continue;
        }

        previous_char = current_char;
        offset ++;
    }

    if (current_char != '"' || !plain_json_intern_has_next(context, 0)) {
        return PLAIN_JSON_ERROR_UNEXPECTED_EOF;
    }

    (*token_length) = offset;
    return PLAIN_JSON_TRUE;
}

static inline plain_json_ErrorType
plain_json_intern_read_keyword(plain_json_Context *context, plain_json_Type type, unsigned int *token_length) {
    json_assert(
        plain_json_intern_peek(context, 0) == 't' || json_intern_peek(context, 0) == 'f' ||
        plain_json_intern_peek(context, 0) == 'n'
    );

    switch (type) {
    case PLAIN_JSON_TYPE_NULL:
        if (plain_json_intern_has_next(context, 3) && plain_json_intern_peek(context, 1) == 'u' &&
            plain_json_intern_peek(context, 2) == 'l' &&
            plain_json_intern_peek(context, 3) == 'l') {
            plain_json_intern_consume(context, 3);
            (*token_length) = 4;
            return PLAIN_JSON_TRUE;
        }
        break;
    case PLAIN_JSON_TYPE_TRUE:
        if (plain_json_intern_has_next(context, 3) && plain_json_intern_peek(context, 1) == 'r' &&
            plain_json_intern_peek(context, 2) == 'u' &&
            plain_json_intern_peek(context, 3) == 'e') {
            plain_json_intern_consume(context, 3);
            (*token_length) = 4;
            return PLAIN_JSON_TRUE;
        }
        break;
    case PLAIN_JSON_TYPE_FALSE:
        if (plain_json_intern_has_next(context, 4) && plain_json_intern_peek(context, 1) == 'a' &&
            plain_json_intern_peek(context, 2) == 'l' &&
            plain_json_intern_peek(context, 3) == 's' &&
            plain_json_intern_peek(context, 4) == 'e') {
            plain_json_intern_consume(context, 4);
            (*token_length) = 5;
            return PLAIN_JSON_TRUE;
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
        READ_DIGIT,
        READ_EXPO,
        READ_EXPO_DIGIT,
    } status = 0;

    int offset = 0;
    char current_char = 0;

    int has_dot = 0;
    int has_expo = 0;
    int has_sign = 0;

    while (plain_json_intern_has_next(context, 0)) {
        current_char = plain_json_intern_peek(context, 0);

        switch (current_char) {
        case '0':
            /* JSON does not allow leading zeros */
            if (offset == 0 || (offset == 1 && has_sign)) {
                char next_char = plain_json_intern_peek(context, 1);
                if (is_digit(next_char)) {
                    return PLAIN_JSON_ERROR_NUMBER_INVALID;
                }
            }

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            status = (status == READ_EXPO) ? READ_EXPO_DIGIT : READ_DIGIT;
            break;
        case '+':
            /* Number may NOT start with a leading '+' */
            if (offset == 0) {
                return PLAIN_JSON_ERROR_NUMBER_INVALID;
            }
        case '-':
            if (offset == 0) {
                has_sign = 1;
                break;
            }

            if (has_sign || status != READ_EXPO) {
                return PLAIN_JSON_ERROR_NUMBER_INVALID;
            }
            has_sign = 1;
            break;
        case 'e':
        case 'E':
            if (has_expo || plain_json_intern_peek(context, -1) == '.' || status != READ_DIGIT) {
                return PLAIN_JSON_ERROR_NUMBER_INVALID;
            }
            status = READ_EXPO;
            has_expo = 1;
            break;
        case '.':
            if (has_dot || status >= READ_EXPO) {
                return PLAIN_JSON_ERROR_NUMBER_INVALID;
            }
            has_dot = 1;
            break;
        default:
            goto done;
        }

        plain_json_intern_consume(context, 0);
        offset ++;
    }

done:
    /* Missing the actual exponent */
    if (status == READ_EXPO) {
        return PLAIN_JSON_ERROR_NUMBER_INVALID;
    }

    (*token_length) = offset;
    return PLAIN_JSON_TRUE;
}

#define get_state()      context->depth_buffer[context->depth_buffer_index]
#define set_state(state) (get_state() = (state))
#define has_state(state) ((get_state() & (state)) > 0)

/* A comma preceeding this token must have been read beforehand */
#define check_for_field_seperator()                                   \
    if ((get_state() & PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR) > 0) { \
        return PLAIN_JSON_ERROR_MISSING_FIELD_SEPERATOR;              \
    }

/* For this token to be valid, parts of 'state' must match the current state */
#define verify_state(state)                       \
    if ((get_state() & (state)) == 0) {           \
        return PLAIN_JSON_ERROR_UNEXPECTED_TOKEN; \
    }

#define push_state()                                                     \
    if (context->depth_buffer_index + 1 < PLAIN_JSON_OPTION_MAX_DEPTH) { \
        context->depth_buffer_index++;                                   \
    } else {                                                             \
        return PLAIN_JSON_ERROR_TOO_DEEP;                                \
    }

#define pop_state()                        \
    if (context->depth_buffer_index > 0) { \
        context->depth_buffer_index--;     \
    } else {                               \
        return PLAIN_JSON_ERROR_IS_ROOT;   \
    }

plain_json_ErrorType plain_json_read_token(plain_json_Context *context, plain_json_Token *token) {
    int status = PLAIN_JSON_TRUE;
    int pre_key_state = 0;

    plain_json_intern_token_reset(context, token);

    while (plain_json_intern_has_next(context, 0)) {
        plain_json_intern_read_blanks(context);

        if (!plain_json_intern_has_next(context, 0)) {
            goto done;
        }

        char current_char = plain_json_intern_peek(context, 0);
        switch (current_char) {
        case '{':
            /* The file root may contain object/arrays */
            verify_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_ROOT
            );
            check_for_field_seperator();

            token->type = PLAIN_JSON_TYPE_OBJECT_START;
            plain_json_intern_consume(context, 0);

            if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                set_state(pre_key_state);
            }

            push_state();
            set_state(PLAIN_JSON_STATE_NEEDS_KEY | PLAIN_JSON_STATE_NEEDS_OBJECT_END);
            goto has_token;
        case '}':
            verify_state(PLAIN_JSON_STATE_NEEDS_KEY | PLAIN_JSON_STATE_NEEDS_OBJECT_END);

            token->type = PLAIN_JSON_TYPE_OBJECT_END;
            plain_json_intern_consume(context, 0);

            /* Handle leading commas and allow empty objects */
            if (context->_last_token_type != PLAIN_JSON_TYPE_OBJECT_START &&
                !has_state(PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR)) {
                return PLAIN_JSON_ERROR_UNEXPECTED_TOKEN;
            }

            pop_state();

            /* Objects can occur at root level */
            if (!has_state(PLAIN_JSON_STATE_IS_ROOT)) {
                set_state(get_state() | PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR);
            }

            goto has_token;
        case '[':
            verify_state(
                PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE |
                PLAIN_JSON_STATE_IS_ROOT
            );
            check_for_field_seperator();

            token->type = PLAIN_JSON_TYPE_ARRAY_START;
            plain_json_intern_consume(context, 0);

            if (has_state(PLAIN_JSON_STATE_NEEDS_VALUE)) {
                set_state(pre_key_state);
            }

            push_state();
            set_state(PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE);
            goto has_token;
        case ']':
            verify_state(PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE);

            token->type = PLAIN_JSON_TYPE_ARRAY_END;
            plain_json_intern_consume(context, 0);

            /* Handle leading commas and allow empty arrays */
            if (context->_last_token_type != PLAIN_JSON_TYPE_ARRAY_START &&
                !has_state(PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR)) {
                return PLAIN_JSON_ERROR_UNEXPECTED_TOKEN;
            }

            pop_state();

            /* Arrays are special values since they can occur at root */
            if (!has_state(PLAIN_JSON_STATE_IS_ROOT)) {
                set_state(get_state() | PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR);
            }

            goto has_token;
        case ',':
            verify_state(PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR);

            plain_json_intern_consume(context, 0);
            set_state(get_state() ^ PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR);

            continue;
        /* A '"' can mean one of the following:
         *   1. The key of a value
         *   2. A string as a value
         *   3. A string as an array value */
        case '"':
            check_for_field_seperator();
            plain_json_intern_consume(context, 0);

            if (has_state(PLAIN_JSON_STATE_NEEDS_KEY)) {
                token->key_start = context->buffer_offset;
                status = plain_json_intern_read_string(context, &token->key_length);
                if (status != PLAIN_JSON_TRUE) {
                    return status;
                }

                plain_json_intern_read_blanks(context);
                /* Keys must be followed by a colon */
                if (plain_json_intern_peek(context, 0) != ':') {
                    return PLAIN_JSON_ERROR_UNEXPECTED_TOKEN;
                }
                plain_json_intern_consume(context, 0);

                /* Keys are only vaild inside objects and must be followed by a value.
                 * The key is read as part of its value token and consequently override the
                 * current state. The state prior to reading the key is stored so we can
                 * restore it after parsing its value. */
                pre_key_state = get_state();
                set_state(PLAIN_JSON_STATE_NEEDS_VALUE);
                continue;

            } else if (has_state( PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE)) {
                token->start = context->buffer_offset;
                token->type = PLAIN_JSON_TYPE_STRING;
                status = plain_json_intern_read_string(context, &token->length);

                if (get_state() == PLAIN_JSON_STATE_NEEDS_VALUE) {
                    set_state(pre_key_state);
                }

                set_state(get_state() | PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR);

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
            verify_state(PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE);
            check_for_field_seperator();

            status = plain_json_intern_read_keyword(context, token->type, &token->length);

            if (get_state() == PLAIN_JSON_STATE_NEEDS_VALUE) {
                set_state(pre_key_state);
            }
            set_state(get_state() | PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR);

            goto has_token;
        }

        if (is_digit(current_char) || current_char == '-' || current_char == '+') {
            verify_state(PLAIN_JSON_STATE_NEEDS_VALUE | PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE);
            check_for_field_seperator();

            token->start = context->buffer_offset;
            token->type = PLAIN_JSON_TYPE_NUMBER;
            status = plain_json_intern_read_number(context, &token->length);

            if (get_state() == PLAIN_JSON_STATE_NEEDS_VALUE) {
                set_state(pre_key_state);
            }
            set_state(get_state() | PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR);

            goto has_token;
        }

        return PLAIN_JSON_ERROR_UNEXPECTED_TOKEN;
    }

    /* There are no more tokens to read */
done:
    if (has_state(PLAIN_JSON_STATE_IS_ROOT)) {
        return PLAIN_JSON_FALSE;
    }

    status = PLAIN_JSON_ERROR_UNEXPECTED_EOF;

has_error:
    /* TODO: Build some sort of error token? */
    return status;
has_token:
    if (status != PLAIN_JSON_TRUE) {
        token->type = PLAIN_JSON_TYPE_ERROR;
        goto has_error;
    }

    return PLAIN_JSON_TRUE;
}

void plain_json_setup(plain_json_Context *context) {
    context->buffer_offset = 0;
    context->depth_buffer[0] = PLAIN_JSON_STATE_IS_ROOT;
}

void plain_json_load_buffer(
    plain_json_Context *context, const char *buffer, unsigned long buffer_size
) {
    context->buffer = buffer;
    context->buffer_size = buffer_size;
}

#undef check_for_field_seperator
#undef verify_state
#undef has_state
#undef push_state
#undef pop_state

#undef is_hex
#undef is_blank
#undef is_digit
#undef json_assert

#endif
