#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define JSON_IMPLEMENTATION
#include "../json.h"

static const char *token_type_to_string(json_Type type) {
    switch (type) {
    case JSON_TYPE_UNKNOWN:
        return "unknown";
    case JSON_TYPE_NULL:
        return "null";
    case JSON_TYPE_OBJECT_START:
        return "object_start";
    case JSON_TYPE_OBJECT_END:
        return "object_end";
    case JSON_TYPE_ARRAY_START:
        return "array_start";
    case JSON_TYPE_ARRAY_END:
        return "array_end";
    case JSON_TYPE_STRING:
        return "string";
    case JSON_TYPE_INTEGER:
        return "integer";
    case JSON_TYPE_REAL:
        return "real";
    case JSON_TYPE_BOOL:
        return "boolean";
    }

    return "invalid";
}

static const char *token_value_to_string(json_Token *token) {
#define BUFFER_SIZE 64
    static char buffer[64] = { 0 };
    memset(buffer, 0, BUFFER_SIZE);

    switch (token->type) {
    case JSON_TYPE_UNKNOWN:
    case JSON_TYPE_NULL:
    case JSON_TYPE_OBJECT_START:
    case JSON_TYPE_OBJECT_END:
    case JSON_TYPE_ARRAY_START:
    case JSON_TYPE_ARRAY_END:
        return NULL;
    case JSON_TYPE_STRING:
        return token->string_buffer;
    case JSON_TYPE_INTEGER:
        snprintf(buffer, 64, "%d", token->value.integer);
        break;
    case JSON_TYPE_REAL:
        snprintf(buffer, 64, "%f", token->value.real);
        break;
    case JSON_TYPE_BOOL:
        snprintf(buffer, 64, "%s", (token->value.boolean) ? "true" : "false");
        break;
    }
#undef BUFFER_SIZE

    return buffer;
}

static const char *error_to_string(json_ErrorType type) {
    switch (type) {
    case JSON_ERROR_CHAR_INVALID:
        return "error_char_invalid";
    case JSON_ERROR_UNTERMINATED_OBJECT:
        return "error_unterminated_object";
    case JSON_ERROR_UNTERMINATED_ARRAY:
        return "error_unterminated_array";
    case JSON_ERROR_STRING_INVALID:
        return "error_string_invalid";
    case JSON_ERROR_STRING_TOO_LONG:
        return "error_string_too_long";
    case JSON_ERROR_STRING_UNTERMINATED:
        return "error_string_unterminated";
    case JSON_ERROR_NO_MEMORY:
        return "error_string_unterminated";
    case JSON_ERROR_TOO_DEEP:
        return "error_too_deep";
    case JSON_ERROR_IS_ROOT:
        return "error_is_root";
    case JSON_ERROR_UNEXPECTED_TOKEN:
        return "error_unexpected_token";
    default:
        break;
    }

    return "unknown error";
}

static void print_error(json_Token *token, json_ErrorType type) {
    fprintf(stderr, "error: %s at %d:%d\n", error_to_string(type), token->line, token->line_offset);
}

static void dump_state(json_Context *context) {
    int state = context->depth_buffer[context->depth_buffer_index];
    char *state_as_string = NULL;
    int bitcount = 0;

    for(int i = 0; i < 31; i++) {
        int state_bit = state & (1 << i);

        if(state_bit == 0)
            continue;

        if(bitcount > 0) printf(" | ");
        bitcount ++;

        switch(state_bit) {
            case JSON_STATE_READ_KEY:
                state_as_string = "READ_KEY";
                break;
            case JSON_STATE_READ_VALUE:
                state_as_string = "READ_VALUE";
                break;
            case JSON_STATE_READ_ARRAY_VALUE:
                state_as_string = "READ_ARRAY_VALUE";
                break;
            case JSON_STATE_IS_ROOT:
                state_as_string = "IS_ROOT";
                break;
            case JSON_STATE_READ_OBJECT_END:
                state_as_string = "READ_OBJECT_END";
                break;
            case JSON_STATE_READ_ARRAY_END:
                state_as_string = "READ_ARRAY_END";
                break;
        }

        printf("%s", state_as_string);
    }
}

static void dump(json_Context *context, json_Token *token) {
    int depth = context->depth_buffer_index;
    for (int i = 0; i < depth; i++) {
        printf(" -- ");
    }

    const char *value = token_value_to_string(token);
    const char *type = token_type_to_string(token->type);

    printf("[%s:%d:%d]", type, token->line, token->line_offset);

    if (strlen(token->key_buffer)) {
        printf(" \"%s\"", token->key_buffer);
    }

    if (value != NULL) {
        printf(": \"%s\"", value);
    }
    printf("\n");

    for (int i = 0; i < depth; i++) {
        printf(" -- ");
    }
    dump_state(context);
    printf("\n");
}

int main(void) {
#define BUFFER_SIZE 2048
    char buffer[BUFFER_SIZE] = {0};
    int buffer_size = 0;

    if((buffer_size = read(0, buffer, BUFFER_SIZE)) <= 0) {
        perror("failed to read from pipe");
        return errno;
    }

    json_Context context = {0};
    json_Token token = {0};

    char key_buffer[64];
    char string_buffer[256];

    json_setup(&context);
    json_token_setup(&token, (char*)key_buffer, 64, (char*)string_buffer, 256);

    json_load_buffer(&context, buffer, buffer_size);

    int status = JSON_TRUE;
    while(status == JSON_TRUE) {
        status = json_read_token(&context, &token);

        if(status != JSON_TRUE && status != JSON_FALSE) {
            print_error(&token, status);
            return -1;
        }

        if(status == JSON_FALSE){
            break;
        }

        dump(&context, &token);
    }

    return 0;
}
