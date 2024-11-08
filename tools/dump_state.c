#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#define PLAIN_JSON_IMPLEMENTATION
#include "../plain_json.h"

static const char *token_type_to_string(plain_json_Type type) {
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

static const char *error_to_string(plain_json_ErrorType type) {
    switch (type) {
    case PLAIN_JSON_ERROR_NUMBER_INVALID:
        return "error_number_invalid";
    case PLAIN_JSON_ERROR_STRING_INVALID_ASCII:
        return "error_string_invalid_ascii";
    case PLAIN_JSON_ERROR_STRING_INVALID_UTF8:
        return "error_string_invalid_utf8";
    case PLAIN_JSON_ERROR_STRING_UNTERMINATED:
        return "error_string_unterminated";
    case PLAIN_JSON_ERROR_NO_MEMORY:
        return "error_no_memory";
    case PLAIN_JSON_ERROR_TOO_DEEP:
        return "error_too_deep";
    case PLAIN_JSON_ERROR_IS_ROOT:
        return "error_is_root";
    case PLAIN_JSON_ERROR_UNEXPECTED_TOKEN:
        return "error_unexpected_token";
    case PLAIN_JSON_ERROR_KEYWORD_INVALID:
        return "error_keyword_invalid";
    case PLAIN_JSON_ERROR_MISSING_FIELD_SEPERATOR:
        return "error_missing_field_seperator";
    default:
        break;
    }

    return "unknown error";
}

static void print_error(plain_json_Context *context, plain_json_ErrorType type) {
    fprintf(stderr, "error: %s at %d:%d\n", error_to_string(type), context->line, context->line_offset);
}
__attribute__((unused))
static void dump_state(plain_json_Context *context) {
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
        case PLAIN_JSON_STATE_NEEDS_KEY:
            state_as_string = "NEEDS_KEY";
            break;
        case PLAIN_JSON_STATE_NEEDS_VALUE:
            state_as_string = "NEEDS_VALUE";
            break;
        case PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE:
            state_as_string = "NEEDS_ARRAY_VALUE";
            break;
        case PLAIN_JSON_STATE_NEEDS_FIELD_SEPERATOR:
            state_as_string = "NEEDS_FIELD_SEPERATOR";
            break;
        case PLAIN_JSON_STATE_IS_ROOT:
            state_as_string = "IS_ROOT";
            break;
        case PLAIN_JSON_STATE_NEEDS_OBJECT_END:
            state_as_string = "NEEDS_OBJECT_END";
            break;
        case PLAIN_JSON_STATE_NEEDS_ARRAY_END:
            state_as_string = "NEEDS_ARRAY_END";
            break;
        }

        printf("%s", state_as_string);
    }
}

#define BUFFER_SIZE 128
static void dump(plain_json_Context *context, plain_json_Token *token) {
    int depth = context->depth_buffer_index;
    for (int i = 0; i < depth; i++) {
        printf(" -- ");
    }

    printf("%s", token_type_to_string(token->type));

    char key_buffer[BUFFER_SIZE] = {0};
    if(token->key_length > 0) {
        memcpy(key_buffer, context->buffer + token->key_start, (token->key_length <= BUFFER_SIZE) ? token->key_length : BUFFER_SIZE);
        printf(": \"%s\" = ", key_buffer);
    }

    char value_buffer[BUFFER_SIZE] = {0};
    if(token->start > 0) {
        memcpy(value_buffer, context->buffer + token->start, (token->length <= BUFFER_SIZE) ? token->length : BUFFER_SIZE);
        printf(" '%s'", value_buffer);
    }

    printf(" [");
    dump_state(context);
    printf("]\n");
}

int parse_json(char *buffer, unsigned long long buffer_size) {
    plain_json_Context context = { 0 };
    plain_json_Token token = { 0 };

    plain_json_setup(&context);
    plain_json_load_buffer(&context, buffer, buffer_size);

    int status = PLAIN_JSON_TRUE;
    while (status == PLAIN_JSON_TRUE) {
        status = plain_json_read_token(&context, &token);

        if (status != PLAIN_JSON_TRUE && status != PLAIN_JSON_FALSE) {
            print_error(&context, status);
            return -1;
        }

        if (status == PLAIN_JSON_FALSE) {
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
