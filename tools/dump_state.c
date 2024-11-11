#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "json_common.h"

static void print_error(plain_json_Context *context, plain_json_ErrorType type) {
    fprintf(
        stderr, "error: %s at %d:%d\n", plain_json_error_to_string(type), context->line, context->line_offset
    );
}
__attribute__((unused)) static void dump_state(plain_json_Context *context) {
    int state = context->_depth_buffer[context->_depth_buffer_index];
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

#define BUFFER_SIZE 512
static void dump(plain_json_Context *context, plain_json_Token *token) {
    static int previous_depth = 0;
    int depth = context->_depth_buffer_index;

    if (context->_depth_buffer_index > previous_depth) {
        depth--;
    }

    previous_depth = context->_depth_buffer_index;

    for (int i = 0; i < depth; i++) {
        printf(" -- ");
    }

    printf("%s", plain_json_type_to_string(token->type));

    char buffer[BUFFER_SIZE] = { 0 };
    if (token->key_length > 0) {
        memset(buffer, 0, sizeof(buffer) / sizeof(*buffer));
        memcpy(
            buffer, context->_buffer + token->key_start,
            (token->key_length <= BUFFER_SIZE) ? token->key_length : BUFFER_SIZE
        );
        printf(": \"%s\"", buffer);
    }

    if (token->length > 0) {
        memset(buffer, 0, sizeof(buffer) / sizeof(*buffer));
        memcpy(
            buffer, context->_buffer + token->start,
            (token->length <= BUFFER_SIZE) ? token->length : BUFFER_SIZE
        );
        printf(" = '%s'", buffer);
    }

    printf("\n");
}

#define TOKEN_PAGE_SIZE 256
int parse_json(char *buffer, unsigned long long buffer_size) {
    plain_json_Context context = { 0 };
    plain_json_load_buffer(&context, buffer, buffer_size);
    int status = PLAIN_JSON_HAS_REMAINING;

#if 0
    plain_json_Token token;
    while ((status = plain_json_read_token(&context, &token)) == PLAIN_JSON_TRUE) {
        if (status != PLAIN_JSON_TRUE && status != PLAIN_JSON_FALSE) {
            print_error(&context, status);
            return -1;
        }

        dump(&context, &token);
    }
#else
    plain_json_Token *tokens = malloc(TOKEN_PAGE_SIZE * sizeof(*tokens));
    int tokens_read = 0;

    status = plain_json_read_token_buffered(&context, tokens, TOKEN_PAGE_SIZE, &tokens_read);
    if (status != PLAIN_JSON_HAS_REMAINING && status != PLAIN_JSON_DONE) {
        print_error(&context, status);
        free(tokens);
        return -1;
    }

    for (int i = 0; i < tokens_read; i++) {
        dump(&context, tokens + i);
    }

    free(tokens);
#endif

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
