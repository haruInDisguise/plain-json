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

    JSON_ERROR_STRING_INVALID,
    JSON_ERROR_STRING_UNTERMINATED,

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

#ifdef __clang__
    #define json_assert(condition) \
        if (!(condition)) {        \
            __builtin_debugtrap(); \
        }
#else
    #define json_assert(...)
#endif
/* TODO: Add all blank charactes */

#define is_blank(c)        (c == ' ' || c == '\t' || c == '\f' || c == '\n')
#define is_digit(c)        (c >= '0' && c <= '9')
#define is_hex_or_digit(c) (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || is_digit(c))
#define is_control(c)                                                                           \
    (c == '\n' || c == '\r' || c == '\t' || c == '\b' || c == '\f' || c == '\v' || c == '\0' || \
     c == '\n')

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

static inline json_ErrorType
json_intern_read_string(json_Context *context, int *length, char *buffer, int buffer_size) {
    json_assert(json_intern_peek(context, 0) == '"');
    json_intern_consume(context, 0);

    int offset = 0;
    char current_char = '\0';
    char previous_char = '\0';

    while (json_intern_has_next(context, 0) && offset < buffer_size) {
        current_char = json_intern_consume(context, 0);

        /* Found unescaped quotes */
        if (previous_char != '\\' && current_char == '\"') {
            break;
        }

        if (current_char == '\0' || current_char == '\n') {
            return JSON_ERROR_STRING_UNTERMINATED;
        }

        /* ASCII 0x00 to 0x1F are refered to as control characters */
        /* JSON only supports a subset of those characts, if they are proberly escaped */
        if (current_char <= 0x1F) {
            return JSON_ERROR_STRING_INVALID;
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
                /* FIXME: Potential buffer overflow */
                buffer[offset++] = current_char;

                int i = 0;
                /* NOTE: Support UTF-16. The standard leaves it to the implementation to decide how
                 * to parse UTF-16 codepoints (has two seperate UTF-8 codepoints or a single one
                 * with 8 hex digits) */
                for (; i < 4 && offset < buffer_size && json_intern_has_next(context, 1); i++) {
                    current_char = json_intern_consume(context, 0);

                    /* Check for valid hex digits */
                    if (!is_hex_or_digit(current_char)) {
                        return JSON_ERROR_STRING_INVALID;
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
                return JSON_ERROR_STRING_INVALID;
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

/* FIXME: Replace 'json_intern_peek' with a direct memory access */
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
            if (has_expo || status != READ_DIGIT) {
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

    /* FIXME: Actually clear the buffer? */
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

    return JSON_ERROR_UNEXPECTED_EOF;

has_token:
    if (status != JSON_TRUE) {
        return status;
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
