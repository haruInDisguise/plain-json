#ifndef _JSON_H_
#define _JSON_H_

#ifndef JSON_MAX_DEPTH
    #define JSON_MAX_DEPTH 32
#endif

#define JSON_STATE_READ_KEY         0x01
#define JSON_STATE_READ_VALUE       0x02
#define JSON_STATE_READ_ARRAY_VALUE 0x04

#define JSON_STATE_IS_ROOT 0x20

#define JSON_STATE_READ_OBJECT_END 0x40
#define JSON_STATE_READ_ARRAY_END  0x80

typedef enum {
    JSON_FALSE = 0,
    JSON_TRUE = 1,

    JSON_ERROR_CHAR_INVALID,

    JSON_ERROR_UNTERMINATED_OBJECT,
    JSON_ERROR_UNTERMINATED_ARRAY,

    JSON_ERROR_STRING_INVALID,
    JSON_ERROR_STRING_TOO_LONG,
    JSON_ERROR_STRING_UNTERMINATED,

    JSON_ERROR_NO_MEMORY,
    JSON_ERROR_TOO_DEEP,
    JSON_ERROR_IS_ROOT,
    JSON_ERROR_UNEXPECTED_TOKEN,
} json_ErrorType;

typedef enum {
    JSON_TYPE_UNKNOWN,

    JSON_TYPE_NULL,

    JSON_TYPE_OBJECT_START,
    JSON_TYPE_OBJECT_END,

    JSON_TYPE_ARRAY_START,
    JSON_TYPE_ARRAY_END,

    JSON_TYPE_STRING,
    JSON_TYPE_INTEGER,
    JSON_TYPE_REAL,
    JSON_TYPE_BOOL,
} json_Type;

/* FIXME: Using unsigned long for "address size" values is not platform agnostic... */
typedef unsigned int json_u32;
typedef unsigned long json_usize;

typedef struct {
    union {
        float real;
        int boolean;
        int integer;
    } value;

    char *string_buffer;
    char *key_buffer;

    int key_buffer_size;
    int string_buffer_size;

    int line;
    int line_offset;
    int start;
    int end;

    json_Type type;
} json_Token;

typedef struct {
    const char *buffer;
    json_Type current_token_type;

    unsigned long buffer_size;
    unsigned long buffer_offset;

    int depth_buffer_index;
    unsigned int depth_buffer[JSON_MAX_DEPTH];

    int line;
    int line_offset;
} json_Context;

extern void json_setup(json_Context *context);
extern json_ErrorType json_read_token(json_Context *context, json_Token *token);
extern void json_load_buffer(json_Context *context, const char *buffer, json_usize buffer_size);
extern void json_token_setup(
    json_Token *token, char *key_buffer, int key_buffer_size, char *string_buffer,
    int string_buffer_size
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

#define is_blank(c)          (c == ' ' || c == '\t' || c == '\f' || c == '\n')
#define is_digit(c)          (c >= '0' && c <= '9')
#define is_alpha_or_digit(c) (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || is_digit(c))
#define is_control(c)                                                                           \
    (c == '\n' || c == '\r' || c == '\t' || c == '\b' || c == '\f' || c == '\v' || c == '\0' || \
     c == '\n')

#define is_root() context->depth_buffer_offset == 0

/* Token */

static inline void json_intern_token_new(json_Context *context, json_Token *token, json_Type type) {
    context->current_token_type = type;

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

static inline char json_intern_consume(json_Context *context) {
    json_assert(context->buffer_offset < context->buffer_size);
    char next_char = context->buffer[context->buffer_offset++];
    context->line_offset++;

    if (next_char == '\n') {
        context->line++;
        context->line_offset = 0;
    }

    return next_char;
}

static inline char json_intern_peek(json_Context *context, int offset) {
    json_assert(context->buffer_offset + offset < context->buffer_size);
    return context->buffer[context->buffer_offset + offset];
}

/* Parsing */

static inline void json_intern_read_blanks(json_Context *context) {
    while (json_intern_has_next(context, 0) && is_blank(json_intern_peek(context, 0))) {
        json_intern_consume(context);
    }
}

static inline json_ErrorType
json_intern_read_string(json_Context *context, int *length, char *buffer, int buffer_size) {
    json_assert(json_intern_peek(context, 0) == '"');
    json_intern_consume(context);

    int offset = 0;

    while (json_intern_has_next(context, 0) && json_intern_peek(context, 0) != '"' &&
           offset < buffer_size) {
        char current_char = json_intern_consume(context);

        if (current_char == '\0' || current_char == '\n') {
            return JSON_ERROR_STRING_UNTERMINATED;
        }

        if (current_char == '\\') {
            if(offset + 1 >= buffer_size) {
                return JSON_ERROR_NO_MEMORY;
            }

            current_char = json_intern_consume(context);
            offset += 1;

            switch (current_char) {
            case '"':
                buffer[offset] = '\"';
                break;
            case '\\':
                buffer[offset] = '\\';
                break;
            case '/':
                buffer[offset] = '/';
                break;
            case 'b':
                buffer[offset] = '\"';
                break;
            case 'f':
                buffer[offset] = '\"';
                break;
            case 'n':
                buffer[offset] = '\"';
                break;
            case 'r':
                buffer[offset] = '\"';
                break;
            case 't':
                buffer[offset] = '\"';
                break;
                /* unicode up to 4 bytes/hex digits */
            case 'u':
                for (int i = 0; i < 4 && offset + i < buffer_size; i++) {
                    char hex_char = json_intern_peek(context, 0);

                    /* FIXME: Actually parse the escape sequence into a unicode character */
                    if (hex_char >= '0' && hex_char <= '9') {
                        buffer[offset] = hex_char;
                    } else if (hex_char >= 'a' && hex_char <= 'f') {
                        buffer[offset] = hex_char;
                    } else {
                        break;
                    }

                    offset += 1;
                }
                break;
            default:
                return JSON_ERROR_STRING_INVALID;
            }
        }

        /* A string may not contain unescaped control characters */
        if (is_control(current_char)) {
            return JSON_ERROR_STRING_INVALID;
        }

        buffer[offset++] = current_char;
    }

    if(offset >= buffer_size) {
        return JSON_ERROR_NO_MEMORY;
    }

    if (!json_intern_has_next(context, 0) || json_intern_peek(context, 0) != '"') {
        return JSON_ERROR_STRING_UNTERMINATED;
    }

    /* Skip terminating '"' */
    json_intern_consume(context);
    buffer[offset] = 0;

    (*length) = offset;
    return offset < buffer_size;
}

#define verify_state(state)                                                    \
    if ((context->depth_buffer[context->depth_buffer_index] & (state)) == 0) { \
        return JSON_ERROR_UNEXPECTED_TOKEN;                                    \
    }

#define get_state()      context->depth_buffer[context->depth_buffer_index]
#define set_state(state) (get_state() = (state))
#define has_state(state) ((get_state() & (state)) > 0)
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

extern json_ErrorType json_read_token(json_Context *context, json_Token *token) {
    int status = 0;
    int pre_key_state = 0;
    int has_field_seperator = 0;

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
            verify_state(JSON_STATE_READ_VALUE | JSON_STATE_READ_ARRAY_VALUE | JSON_STATE_IS_ROOT);

            json_intern_token_new(context, token, JSON_TYPE_OBJECT_START);
            json_intern_consume(context);

            if (has_state(JSON_STATE_READ_VALUE)) {
                set_state(pre_key_state);
            }

            push_state();
            set_state(JSON_STATE_READ_KEY | JSON_STATE_READ_OBJECT_END);
            goto has_token;
        case '}':
            verify_state(JSON_STATE_READ_KEY | JSON_STATE_READ_OBJECT_END);

            /* Correctly handle leading commas */
            if (has_field_seperator == JSON_TRUE) {
                return JSON_ERROR_UNEXPECTED_TOKEN;
            }

            json_intern_token_new(context, token, JSON_TYPE_OBJECT_END);
            json_intern_consume(context);

            pop_state();
            goto has_token;
        case '[':
            verify_state(JSON_STATE_READ_VALUE | JSON_STATE_READ_ARRAY_VALUE | JSON_STATE_IS_ROOT);

            json_intern_token_new(context, token, JSON_TYPE_ARRAY_START);
            json_intern_consume(context);

            if (has_state(JSON_STATE_READ_VALUE)) {
                set_state(pre_key_state);
            }

            push_state();
            set_state(JSON_STATE_READ_ARRAY_VALUE);
            goto has_token;
        case ']':
            verify_state(JSON_STATE_READ_ARRAY_VALUE);

            /* Correctly handle leading commas */
            if (has_field_seperator == JSON_TRUE) {
                return JSON_ERROR_UNEXPECTED_TOKEN;
            }

            json_intern_token_new(context, token, JSON_TYPE_ARRAY_END);
            json_intern_consume(context);

            pop_state();
            goto has_token;
        case ',':
            verify_state(JSON_STATE_READ_KEY | JSON_STATE_READ_ARRAY_VALUE);
            has_field_seperator = JSON_TRUE;

            json_intern_consume(context);
            continue;
        /* A '"' can mean one of the following:
         *   1. The key of a value
         *   2. A string as a value
         *   3. A string as an array value */
        case '"':
            if (has_state(JSON_STATE_READ_KEY)) {
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

                json_intern_consume(context);

                /* Keys are only vaild inside objects and must be followed by a value.
                 * The key is read as part of its value token and consequently override the
                 * current state. The state prior to reading the key is stored so we can
                 * restore it after parsing its value. */
                pre_key_state = get_state();
                set_state(JSON_STATE_READ_VALUE);
                continue;

            } else if (has_state(JSON_STATE_READ_VALUE | JSON_STATE_READ_ARRAY_VALUE)) {
                json_intern_token_new(context, token, JSON_TYPE_STRING);
                status = json_intern_read_string(
                    context, &key_length, token->string_buffer, token->string_buffer_size
                );

                if (status != JSON_TRUE) {
                    return status;
                }

                json_intern_read_blanks(context);

                if (get_state() == JSON_STATE_READ_VALUE) {
                    set_state(pre_key_state);
                }

                goto has_token;
            }

            return JSON_ERROR_UNEXPECTED_TOKEN;
        default:
            return JSON_ERROR_UNEXPECTED_TOKEN;
        }
    }

    return JSON_FALSE;

has_token:
    json_intern_token_finish(context, token);
    return JSON_TRUE;
}

#undef verify_state
#undef has_state
#undef push_state
#undef pop_state

void json_token_setup(
    json_Token *token, char *key_buffer, int key_buffer_size, char *string_buffer,
    int string_buffer_size
) {
    token->key_buffer = key_buffer;
    token->key_buffer_size = key_buffer_size;
    token->string_buffer = string_buffer;
    token->string_buffer_size = string_buffer_size;
}

void json_setup(json_Context *context) {
    context->buffer_offset = 0;
    context->depth_buffer[0] = JSON_STATE_IS_ROOT;
}

void json_load_buffer(json_Context *context, const char *buffer, json_usize buffer_size) {
    context->buffer = buffer;
    context->buffer_size = buffer_size;
}

#endif
