#ifndef _JSON_H_
#define _JSON_H_

#ifndef JSON_MAX_DEPTH
    #define JSON_MAX_DEPTH 32
#endif

#define JSON_STATE_IS_ROOT 0x01

#define JSON_STATE_NEEDS_KEY             0x02
#define JSON_STATE_NEEDS_FIELD_SEPERATOR 0x04
#define JSON_STATE_NEEDS_VALUE           0x08
#define JSON_STATE_NEEDS_ARRAY_VALUE     0x10

#define JSON_STATE_NEEDS_OBJECT_END 0x20
#define JSON_STATE_NEEDS_ARRAY_END  0x40

typedef enum {
    JSON_FALSE = 0,
    JSON_TRUE = 1,

    JSON_ERROR_STRING_INVALID_ASCII,
    JSON_ERROR_STRING_UNTERMINATED,
    JSON_ERROR_STRING_INVALID_UTF8,
    JSON_ERROR_STRING_INVALID_ESCAPE,
    JSON_ERROR_STRING_INVALID_UNICODE_ESCAPE,

    JSON_ERROR_NUMBER_INVALID,

    JSON_ERROR_KEYWORD_INVALID,

    JSON_ERROR_NO_MEMORY,
    JSON_ERROR_MISSING_FIELD_SEPERATOR,
    JSON_ERROR_TOO_DEEP,
    JSON_ERROR_IS_ROOT,
    JSON_ERROR_UNEXPECTED_TOKEN,
    JSON_ERROR_UNEXPECTED_EOF,
} json_ErrorType;

typedef enum {
    JSON_TYPE_UNKNOWN,

    JSON_TYPE_NULL,
    JSON_TYPE_TRUE,
    JSON_TYPE_FALSE,

    JSON_TYPE_OBJECT_START,
    JSON_TYPE_OBJECT_END,

    JSON_TYPE_ARRAY_START,
    JSON_TYPE_ARRAY_END,

    JSON_TYPE_STRING,
    JSON_TYPE_NUMBER,
} json_Type;

typedef struct {
    char *value_buffer;
    char *key_buffer;

    int key_buffer_size;
    int value_buffer_size;

    int line;
    int line_offset;
    int start;
    int end;

    json_Type type;
} json_Token;

typedef struct {
    const char *buffer;

    unsigned long buffer_size;
    unsigned long buffer_offset;

    int depth_buffer_index;
    int depth_buffer[JSON_MAX_DEPTH];
    /* FIXME: Find a nicer workaround for empty arrays/objects */
    int _last_token_type;

    int line;
    int line_offset;
} json_Context;

extern void json_setup(json_Context *context);
extern json_ErrorType json_read_token(json_Context *context, json_Token *token);
extern void json_load_buffer(json_Context *context, const char *buffer, unsigned long buffer_size);
extern void json_token_setup(
    json_Token *token, char *key_buffer, int key_buffer_size, char *value_buffer,
    int value_buffer_size
);

#endif

/* Implementation */

#ifdef JSON_IMPLEMENTATION

#ifdef JSON_DEBUG
    #ifdef __clang__
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

#define is_blank(c)        (c == ' ' || c == '\t' || c == '\f' || c == '\n' || c == '\v')
#define is_digit(c)        (c >= '0' && c <= '9')
#define is_hex_or_digit(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (is_digit(c)))

#define is_root() context->depth_buffer_offset == 0

/* Token */

static inline void json_intern_token_new(json_Context *context, json_Token *token, json_Type type) {
    context->_last_token_type = token->type;
    token->type = type;
    token->start = context->buffer_offset;
    token->line = context->line;
    token->line_offset = context->line_offset;
}

static inline void json_intern_token_finish(json_Context *context, json_Token *token) {
    token->end = context->buffer_offset;
}

/* Util */

static inline int json_intern_has_next(json_Context *context, int offset) {
    return (context->buffer_offset + offset < context->buffer_size);
}

/* Consume the current character + count */
static inline char json_intern_consume(json_Context *context, int count) {
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

static inline char json_intern_peek(json_Context *context, int offset) {
    json_assert(context->buffer_offset + offset < context->buffer_size);
    return context->buffer[context->buffer_offset + offset];
}

/* Parsing */

static inline void json_intern_read_blanks(json_Context *context) {
    while (json_intern_has_next(context, 0) && is_blank(json_intern_peek(context, 0))) {
        json_intern_consume(context, 0);
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
 * mask         : 11111... 11...... 11...... 11...... (0xF8, 0xC0, 0xC0, 0xC0)
 *
 * TODO: Not all valid sequences represent valid/used UTF-8 */
static inline json_ErrorType
json_intern_read_string(json_Context *context, int *length, char *buffer, int buffer_size) {
    json_assert(json_intern_peek(context, 0) == '"');
    json_intern_consume(context, 0);

    int offset = 0;
    char current_char = '\0';
    char previous_char = '\0';

    while (json_intern_has_next(context, 0) && offset < buffer_size) {
        /* Basic layout mask */
        static const unsigned int mask_table[4] = {
            0x80000000UL,
            0xE0C00000UL,
            0xF0C0C000UL,
            0xF8C0C0C0UL,
        };

        /* Check basic layout */
        static const unsigned int pattern_table[4] = {
            0x00000000UL,
            0xC0800000UL,
            0xE0808000UL,
            0xF0808080UL,
        };

        /* Check for invalid codepoints */
        static const unsigned int code_table[] = {
            0x00000000UL,
            0x1E000000UL,
            0x0F200000UL,
            0x07300000UL,
        };

        /* The top nibble of the starting byte encodes its type and
         * therefore the length of the utf8 encoded sequence */
        static const unsigned char length_table[] = {
            1, 1, 1, 1, 1, 1, 1, 1, /* 0... */
            2, 2, 2, 2,             /* 10.. invalid */
            2, 2,                   /* 110. */
            3,                      /* 1110 */
            4,                      /* 1111 or invalid */
        };

        current_char = json_intern_consume(context, 0);
        if ((current_char & mask_table[0]) == pattern_table[0]) {
            goto read_ascii;
        }

        /* The 4 most significant bytes encode the length of the sequence */
        unsigned char length = length_table[(unsigned char)current_char >> 4];
        unsigned int value = 0;

        if (!json_intern_has_next(context, length - 1)) {
            return JSON_ERROR_STRING_INVALID_UTF8;
        }

        if (buffer_size - offset <= 0) {
            return JSON_ERROR_NO_MEMORY;
        }

        /* TODO: Excessing the file buffer through peek() and consume() would be more consistent,
         * but I find it less readable. */
        switch (length) {
        case 4:
            value |= ((unsigned char)context->buffer[context->buffer_offset + 2] << 0);
            buffer[offset + 3] = value;
        case 3:
            value |= ((unsigned char)context->buffer[context->buffer_offset + 1] << 8);
            buffer[offset + 2] = value >> 8;
        case 2:
            value |= ((unsigned char)context->buffer[context->buffer_offset + 0] << 16);
            buffer[offset + 1] = value >> 16;

            value |= ((unsigned int)current_char << 24);
            buffer[offset] = current_char;
            break;
        case 1:
            /* Unreachable */
            json_assert(0);
        }

        /* Surrogates are UTF-16 codepoints that do not belong in UTF-8, but are
         * sometimes used no the less. */
        /* High/Low Surrogates start with 0xED, followed by at least 0xA0 */
        unsigned int is_surrogate = 0; // (value >> 16) >= 0xEDA0;
        unsigned int masked = value & mask_table[length - 1];
        unsigned int code = value & code_table[length - 1];

        if (masked == pattern_table[length - 1] && (value & code) && !is_surrogate) {
            json_intern_consume(context, length - 2);
            offset += length;
            continue;
        }

        return JSON_ERROR_STRING_INVALID_UTF8;

    read_ascii:
        /* Found unescaped quotes */
        if (previous_char != '\\' && current_char == '\"') {
            break;
        }

        if (current_char == '\0' || current_char == '\n') {
            return JSON_ERROR_STRING_UNTERMINATED;
        }

        /* ASCII 0x00 to 0x1F are refered to as control characters */
        /* JSON supports a properly escaped subset */
        if (current_char < 0x20) {
            return JSON_ERROR_STRING_INVALID_ASCII;
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
            case 'u':
                buffer[offset++] = current_char;

                int i = 0;
                for (; i < 4 && offset < buffer_size && json_intern_has_next(context, 1); i++) {
                    current_char = json_intern_consume(context, 0);

                    /* Check for valid hex digits */
                    if (!is_hex_or_digit(current_char)) {
                        return JSON_ERROR_STRING_INVALID_UNICODE_ESCAPE;
                    }

                    buffer[offset++] = current_char;
                }

                /* Read less than four digits. The string buffer is too short or the file buffer
                 * is out of characters */
                if (i < 4) {
                    if (offset >= buffer_size) {
                        return JSON_ERROR_NO_MEMORY;
                    } else {
                        return JSON_ERROR_UNEXPECTED_EOF;
                    }
                }
                previous_char = '\0';
                continue;

            default:
                return JSON_ERROR_STRING_INVALID_ESCAPE;
            }

            /* Indicate that an escaped sequence was fully consumed */
            previous_char = '\0';
            buffer[offset++] = current_char;
            continue;
        }

        buffer[offset++] = current_char;
        previous_char = current_char;
    }

    if (offset >= buffer_size) {
        return JSON_ERROR_NO_MEMORY;
    }

    if (current_char != '"' || !json_intern_has_next(context, 0)) {
        return JSON_ERROR_UNEXPECTED_EOF;
    }

    /* Make sure the output buffer contains a valid c string */
    buffer[offset] = 0;

    (*length) = offset;
    return JSON_TRUE;
}

int json_intern_read_keyword(json_Context *context, json_Token *token) {
    char current_char = json_intern_peek(context, 0);
    json_assert(current_char == 't' || current_char == 'f' || current_char == 'n');

    switch (token->type) {
    case JSON_TYPE_NULL:
        if (json_intern_has_next(context, 3) && json_intern_peek(context, 1) == 'u' &&
            json_intern_peek(context, 2) == 'l' && json_intern_peek(context, 3) == 'l') {
            json_intern_consume(context, 3);
            return JSON_TRUE;
        }
        break;
    case JSON_TYPE_TRUE:
        if (json_intern_has_next(context, 3) && json_intern_peek(context, 1) == 'r' &&
            json_intern_peek(context, 2) == 'u' && json_intern_peek(context, 3) == 'e') {
            json_intern_consume(context, 3);
            return JSON_TRUE;
        }
        break;
    case JSON_TYPE_FALSE:
        if (json_intern_has_next(context, 4) && json_intern_peek(context, 1) == 'a' &&
            json_intern_peek(context, 2) == 'l' && json_intern_peek(context, 3) == 's' &&
            json_intern_peek(context, 4) == 'e') {
            json_intern_consume(context, 4);
            return JSON_TRUE;
        }
        break;
    default:
        break;
    }

    return JSON_ERROR_KEYWORD_INVALID;
}

static inline int json_intern_read_number(json_Context *context, char *buffer, int buffer_size) {
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

    while (json_intern_has_next(context, 0)) {
        current_char = json_intern_peek(context, 0);

        switch (current_char) {
        case '0':
            /* JSON does not support leading zeros */
            if (offset == 0 || (offset == 1 && has_sign)) {
                char next_char = json_intern_peek(context, 1);
                if (is_digit(next_char)) {
                    return JSON_ERROR_NUMBER_INVALID;
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
        case '-':
            /* Allow sign */
            if (offset == 0) {
                break;
            }

            if (has_sign || status != READ_EXPO) {
                return JSON_ERROR_NUMBER_INVALID;
            }
            has_sign = 1;
            break;
        case 'e':
        case 'E':
            if (has_expo || json_intern_peek(context, -1) == '.' || status != READ_DIGIT) {
                return JSON_ERROR_NUMBER_INVALID;
            }
            status = READ_EXPO;
            has_expo = 1;
            break;
        case '.':
            if (has_dot || status >= READ_EXPO) {
                return JSON_ERROR_NUMBER_INVALID;
            }
            has_dot = 1;
            break;
        default:
            goto done;
        }

        if (offset >= buffer_size) {
            return JSON_ERROR_NO_MEMORY;
        }

        json_intern_consume(context, 0);
        buffer[offset++] = current_char;
    }

done:
    /* Missing the actual exponent */
    if (status == READ_EXPO) {
        return JSON_ERROR_NUMBER_INVALID;
    }

    buffer[offset] = 0;
    return JSON_TRUE;
}

#define get_state()      context->depth_buffer[context->depth_buffer_index]
#define set_state(state) (get_state() = (state))
#define has_state(state) ((get_state() & (state)) > 0)

/* A comma preceeding this token must have been read beforehand */
#define check_for_field_seperator()                             \
    if ((get_state() & JSON_STATE_NEEDS_FIELD_SEPERATOR) > 0) { \
        return JSON_ERROR_MISSING_FIELD_SEPERATOR;              \
    }

/* For this token to be valid, parts of 'state' must match the current state */
#define verify_state(state)                 \
    if ((get_state() & (state)) == 0) {     \
        return JSON_ERROR_UNEXPECTED_TOKEN; \
    }

#define push_state()                                        \
    if (context->depth_buffer_index + 1 < JSON_MAX_DEPTH) { \
        context->depth_buffer_index++;                      \
    } else {                                                \
        return JSON_ERROR_TOO_DEEP;                         \
    }

#define pop_state()                        \
    if (context->depth_buffer_index > 0) { \
        context->depth_buffer_index--;     \
    } else {                               \
        return JSON_ERROR_IS_ROOT;         \
    }

json_ErrorType json_read_token(json_Context *context, json_Token *token) {
    int status = JSON_TRUE;
    int pre_key_state = 0;

    token->key_buffer[0] = 0;

    while (json_intern_has_next(context, 0)) {
        json_intern_read_blanks(context);

        if (!json_intern_has_next(context, 0)) {
            return JSON_FALSE;
        }

        char current_char = json_intern_peek(context, 0);
        int key_length = 0;

        switch (current_char) {
        case '{':
            /* The file root may contain object/arrays */
            verify_state(
                JSON_STATE_NEEDS_VALUE | JSON_STATE_NEEDS_ARRAY_VALUE | JSON_STATE_IS_ROOT
            );
            check_for_field_seperator();

            json_intern_token_new(context, token, JSON_TYPE_OBJECT_START);
            json_intern_consume(context, 0);

            if (has_state(JSON_STATE_NEEDS_VALUE)) {
                set_state(pre_key_state);
            }

            push_state();
            set_state(JSON_STATE_NEEDS_KEY | JSON_STATE_NEEDS_OBJECT_END);
            goto has_token;
        case '}':
            verify_state(JSON_STATE_NEEDS_KEY | JSON_STATE_NEEDS_OBJECT_END);

            json_intern_token_new(context, token, JSON_TYPE_OBJECT_END);
            json_intern_consume(context, 0);

            /* Handle leading commas and allow empty arrays */
            if (context->_last_token_type != JSON_TYPE_OBJECT_START &&
                !has_state(JSON_STATE_NEEDS_FIELD_SEPERATOR)) {
                return JSON_ERROR_UNEXPECTED_TOKEN;
            }

            pop_state();

            /* Objects can occur at root level */
            if (!has_state(JSON_STATE_IS_ROOT)) {
                set_state(get_state() | JSON_STATE_NEEDS_FIELD_SEPERATOR);
            }

            goto has_token;
        case '[':
            verify_state(
                JSON_STATE_NEEDS_VALUE | JSON_STATE_NEEDS_ARRAY_VALUE | JSON_STATE_IS_ROOT
            );
            check_for_field_seperator();

            json_intern_token_new(context, token, JSON_TYPE_ARRAY_START);
            json_intern_consume(context, 0);

            if (has_state(JSON_STATE_NEEDS_VALUE)) {
                set_state(pre_key_state);
            }

            push_state();
            set_state(JSON_STATE_NEEDS_ARRAY_VALUE);
            goto has_token;
        case ']':
            verify_state(JSON_STATE_NEEDS_ARRAY_VALUE);

            json_intern_token_new(context, token, JSON_TYPE_ARRAY_END);
            json_intern_consume(context, 0);

            /* Handle leading commas and allow empty arrays */
            if (context->_last_token_type != JSON_TYPE_ARRAY_START &&
                !has_state(JSON_STATE_NEEDS_FIELD_SEPERATOR)) {
                return JSON_ERROR_UNEXPECTED_TOKEN;
            }

            pop_state();

            /* Arrays are special values since they can occur at root */
            if (!has_state(JSON_STATE_IS_ROOT)) {
                set_state(get_state() | JSON_STATE_NEEDS_FIELD_SEPERATOR);
            }

            goto has_token;
        case ',':
            verify_state(JSON_STATE_NEEDS_FIELD_SEPERATOR);

            json_intern_consume(context, 0);
            /* We've read the field seperator, we can remove it from the current state */
            set_state(get_state() ^ JSON_STATE_NEEDS_FIELD_SEPERATOR);

            continue;
        /* A '"' can mean one of the following:
         *   1. The key of a value
         *   2. A string as a value
         *   3. A string as an array value */
        case '"':
            check_for_field_seperator();

            if (has_state(JSON_STATE_NEEDS_KEY)) {
                status = json_intern_read_string(
                    context, &key_length, token->key_buffer, token->key_buffer_size
                );
                if (status != JSON_TRUE) {
                    return status;
                }

                json_intern_read_blanks(context);
                /* Keys must be followed by a colon */
                if (json_intern_peek(context, 0) != ':') {
                    return JSON_ERROR_UNEXPECTED_TOKEN;
                }
                json_intern_consume(context, 0);

                /* Keys are only vaild inside objects and must be followed by a value.
                 * The key is read as part of its value token and consequently override the
                 * current state. The state prior to reading the key is stored so we can
                 * restore it after parsing its value. */
                pre_key_state = get_state();
                set_state(JSON_STATE_NEEDS_VALUE);
                continue;

            } else if (has_state(JSON_STATE_NEEDS_VALUE | JSON_STATE_NEEDS_ARRAY_VALUE)) {
                json_intern_token_new(context, token, JSON_TYPE_STRING);
                status = json_intern_read_string(
                    context, &key_length, token->value_buffer, token->value_buffer_size
                );

                if (get_state() == JSON_STATE_NEEDS_VALUE) {
                    set_state(pre_key_state);
                }

                set_state(get_state() | JSON_STATE_NEEDS_FIELD_SEPERATOR);

                goto has_token;
            }

            return JSON_ERROR_UNEXPECTED_TOKEN;
        case 'n':
            token->type = JSON_TYPE_NULL;
            goto read_keyword;
        case 't':
            token->type = JSON_TYPE_TRUE;
            goto read_keyword;
        case 'f':
            token->type = JSON_TYPE_FALSE;
        read_keyword:
            verify_state(JSON_STATE_NEEDS_VALUE | JSON_STATE_NEEDS_ARRAY_VALUE);
            check_for_field_seperator();

            json_intern_token_new(context, token, token->type);
            status = json_intern_read_keyword(context, token);

            if (get_state() == JSON_STATE_NEEDS_VALUE) {
                set_state(pre_key_state);
            }
            set_state(get_state() | JSON_STATE_NEEDS_FIELD_SEPERATOR);

            goto has_token;
        }

        if (is_digit(current_char) || current_char == '-') {
            verify_state(JSON_STATE_NEEDS_VALUE | JSON_STATE_NEEDS_ARRAY_VALUE);
            check_for_field_seperator();

            json_intern_token_new(context, token, JSON_TYPE_NUMBER);
            status =
                json_intern_read_number(context, token->value_buffer, token->value_buffer_size);

            if (get_state() == JSON_STATE_NEEDS_VALUE) {
                set_state(pre_key_state);
            }
            set_state(get_state() | JSON_STATE_NEEDS_FIELD_SEPERATOR);

            goto has_token;
        }

        return JSON_ERROR_UNEXPECTED_TOKEN;
    }

    if (has_state(JSON_STATE_IS_ROOT)) {
        return JSON_FALSE;
    }

    status = JSON_ERROR_UNEXPECTED_EOF;

has_error:
    return status;
has_token:
    if (status != JSON_TRUE) {
        goto has_error;
    }

    json_intern_token_finish(context, token);
    return JSON_TRUE;
}

#undef verify_state
#undef has_state
#undef push_state
#undef pop_state

void json_token_setup(
    json_Token *token, char *key_buffer, int key_buffer_size, char *value_buffer,
    int value_buffer_size
) {
    token->key_buffer = key_buffer;
    token->key_buffer_size = key_buffer_size;
    token->value_buffer = value_buffer;
    token->value_buffer_size = value_buffer_size;
}

void json_setup(json_Context *context) {
    context->buffer_offset = 0;
    context->depth_buffer[0] = JSON_STATE_IS_ROOT;
}

void json_load_buffer(json_Context *context, const char *buffer, unsigned long buffer_size) {
    context->buffer = buffer;
    context->buffer_size = buffer_size;
}

#endif
