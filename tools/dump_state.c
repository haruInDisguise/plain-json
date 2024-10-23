#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#define JSON_IMPLEMENTATION
#include "../json.h"

static const char *token_type_to_string(json_Type type) {
    switch (type) {
    case JSON_TYPE_UNKNOWN:
        return "unknown";
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
    case JSON_TYPE_NUMBER:
        return "integer";
    case JSON_TYPE_NULL:
        return "null";
    case JSON_TYPE_FALSE:
        return "false";
    case JSON_TYPE_TRUE:
        return "true";
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
    case JSON_TYPE_TRUE:
    case JSON_TYPE_FALSE:
    case JSON_TYPE_OBJECT_START:
    case JSON_TYPE_OBJECT_END:
    case JSON_TYPE_ARRAY_START:
    case JSON_TYPE_ARRAY_END:
        return NULL;
    case JSON_TYPE_NUMBER:
        return token->value_buffer;
    case JSON_TYPE_STRING:
        return token->value_buffer;
        break;
    }
#undef BUFFER_SIZE

    return buffer;
}

static const char *error_to_string(json_ErrorType type) {
    switch (type) {
    case JSON_ERROR_NUMBER_INVALID:
        return "error_number_invalid";
    case JSON_ERROR_STRING_INVALID:
        return "error_string_invalid";
    case JSON_ERROR_STRING_INVALID_HEX:
        return "error_string_invalid_hex";
    case JSON_ERROR_STRING_UNTERMINATED:
        return "error_string_unterminated";
    case JSON_ERROR_NO_MEMORY:
        return "error_no_memory";
    case JSON_ERROR_TOO_DEEP:
        return "error_too_deep";
    case JSON_ERROR_IS_ROOT:
        return "error_is_root";
    case JSON_ERROR_UNEXPECTED_TOKEN:
        return "error_unexpected_token";
    case JSON_ERROR_KEYWORD_INVALID:
        return "error_keyword_invalid";
    case JSON_ERROR_MISSING_FIELD_SEPERATOR:
        return "error_missing_field_seperator";
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

    for (int i = 0; i < 31; i++) {
        int state_bit = state & (1 << i);

        if (state_bit == 0) {
            continue;
        }

        if (bitcount > 0) {
            printf(" | ");
        }
        bitcount++;

        switch (state_bit) {
        case JSON_STATE_NEEDS_KEY:
            state_as_string = "NEEDS_KEY";
            break;
        case JSON_STATE_NEEDS_VALUE:
            state_as_string = "NEEDS_VALUE";
            break;
        case JSON_STATE_NEEDS_ARRAY_VALUE:
            state_as_string = "NEEDS_ARRAY_VALUE";
            break;
        case JSON_STATE_NEEDS_FIELD_SEPERATOR:
            state_as_string = "NEEDS_FIELD_SEPERATOR";
            break;
        case JSON_STATE_IS_ROOT:
            state_as_string = "IS_ROOT";
            break;
        case JSON_STATE_NEEDS_OBJECT_END:
            state_as_string = "NEEDS_OBJECT_END";
            break;
        case JSON_STATE_NEEDS_ARRAY_END:
            state_as_string = "NEEDS_ARRAY_END";
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

int parse_json(char *buffer, unsigned long long buffer_size) {

    json_Context context = { 0 };
    json_Token token = { 0 };

    char key_buffer[64];
    char string_buffer[256];

    json_setup(&context);
    json_token_setup(&token, (char *)key_buffer, 64, (char *)string_buffer, 256);

    json_load_buffer(&context, buffer, buffer_size);

    int status = JSON_TRUE;
    while (status == JSON_TRUE) {
        status = json_read_token(&context, &token);

        if (status != JSON_TRUE && status != JSON_FALSE) {
            print_error(&token, status);
            return -1;
        }

        if (status == JSON_FALSE) {
            break;
        }

        dump(&context, &token);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        char buffer[512] = { 0 };
        int buffer_size = 0;
        if ((buffer_size = read(0, buffer, 512)) <= 0) {
            perror("failed to read from stdin");
            return errno;
        }

        return parse_json(buffer, buffer_size);
    }

    struct stat file_stat;
    if (stat(argv[1], &file_stat) != 0) {
        perror("stat failed");
        return errno;
    }

    int file_fd = open(argv[1], O_RDONLY);
    if (file_fd < 0) {
        perror("open failed");
        return errno;
    }

    char *buffer = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
    if (buffer == MAP_FAILED) {
        perror("mmap failed");
        return errno;
    }

    int status = parse_json(buffer, file_stat.st_size);
    munmap(buffer, file_stat.st_size);
    close(file_fd);

    return status;
}
