#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#define PLAIN_JSON_IMPLEMENTATION
#include <plain_json.h>

#define PLAIN_JSON_STATE_IS_FIRST_TOKEN 0x01
#define PLAIN_JSON_STATE_IS_ROOT        0x02

#define PLAIN_JSON_STATE_NEEDS_KEY         0x04
#define PLAIN_JSON_STATE_NEEDS_COMMA       0x08
#define PLAIN_JSON_STATE_NEEDS_VALUE       0x10
#define PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE 0x20
#define PLAIN_JSON_STATE_NEEDS_COLON       0x40

static void print_error(plain_json_Context *context, uintptr_t position, plain_json_ErrorType type) {
    uint32_t line = 0, offset = 0;
    plain_json_compute_position(context, position, &line, &offset);
    fprintf(stderr, "error: %s at %u:%u\n", plain_json_error_to_string(type), line, offset);
}
__attribute__((unused)) static void
dump_state(plain_json_Context *context, char *buffer, int buffer_size) {
    uint32_t state = context->depth_buffer[context->depth_buffer_index];
    char *state_as_string = NULL;
    uint32_t bitcount = 0;
    uint32_t offset = 0;

    for (int i = 0; i < 31; i++) {
        uint32_t state_bit = state & (1 << i);

        if (state_bit == 0) {
            continue;
        }

        if (bitcount > 0) {
            offset += snprintf(buffer + offset, buffer_size - offset, " | ");
        }
        bitcount++;

        switch (state_bit) {
        case PLAIN_JSON_STATE_NEEDS_KEY:
            state_as_string = "needs_key";
            break;
        case PLAIN_JSON_STATE_NEEDS_VALUE:
            state_as_string = "needs_value";
            break;
        case PLAIN_JSON_STATE_NEEDS_ARRAY_VALUE:
            state_as_string = "needs_array_value";
            break;
        case PLAIN_JSON_STATE_NEEDS_COMMA:
            state_as_string = "needs_comma";
            break;
        case PLAIN_JSON_STATE_NEEDS_COLON:
            state_as_string = "needs_colon";
            break;
        case PLAIN_JSON_STATE_IS_ROOT:
            state_as_string = "is_root";
            break;
        }

        offset += snprintf(buffer + offset, buffer_size - offset, "%s", state_as_string);
    }

    buffer[offset] = 0;
}

#define BUFFER_SIZE 1024
static void dump(plain_json_Context *context, const plain_json_Token *token, uint32_t depth) {
    char buffer[BUFFER_SIZE] = { 0 };

    uint32_t offset = 0;
    for (uint32_t i = 0; i < depth; i++) {
        offset += snprintf(buffer + offset, BUFFER_SIZE, " -- ");
    }

    offset +=
        snprintf(buffer + offset, BUFFER_SIZE, "%-12s", plain_json_type_to_string(token->type));

    if (token->key_index != PLAIN_JSON_NO_KEY) {
        offset += snprintf(
            buffer + offset, BUFFER_SIZE - offset, ": \"%s\"",
            plain_json_get_key(context, token->key_index)
        );
    }

    if (token->type == PLAIN_JSON_TYPE_STRING) {
        offset += snprintf(
            buffer + offset, BUFFER_SIZE - offset, " = '%s'",
            plain_json_get_string(context, token->value.string_index)
        );
    }

    if (token->type == PLAIN_JSON_TYPE_INTEGER) {
        offset += snprintf(buffer + offset, BUFFER_SIZE - offset, " = '%lu'", token->value.integer);
    }

    /*dump_state(context, value_buffer, VALUE_BUFFER_SIZE);*/
    printf("%s\n", buffer);
}

static void custom_free(void *context, void *buffer) {
    (void)context;
    free(buffer);
}

static void *custom_alloc(void *context, uintptr_t size) {
    (void) context;
    return malloc(size);
}

static void *custom_realloc(void *context, void *buffer, uintptr_t old_size, uintptr_t new_size) {
    (void)context;
    (void)old_size;
    return realloc(buffer, new_size);
}

int parse_json(uint8_t *buffer, unsigned long long buffer_size) {
    plain_json_ErrorType result = PLAIN_JSON_HAS_REMAINING;
    plain_json_AllocatorConfig alloc_config = { .free_func = custom_free,
                                                .alloc_func = custom_alloc,
                                                .realloc_func = custom_realloc };
    plain_json_Context *context =
        plain_json_parse(alloc_config, (uint8_t *)buffer, buffer_size, &result);

    const uint32_t token_count = plain_json_get_token_count(context);
    uint32_t depth = 0;

    for (uint32_t i = 0; i < token_count; i++) {
        const plain_json_Token *token = plain_json_get_token(context, i);
        if (token->type == PLAIN_JSON_TYPE_OBJECT_END || token->type == PLAIN_JSON_TYPE_ARRAY_END) {
            depth -= 1;
        }

        dump(context, token, depth);

        if (token->type == PLAIN_JSON_TYPE_OBJECT_START ||
            token->type == PLAIN_JSON_TYPE_ARRAY_START) {
            depth += 1;
        }
    }

    if (result != PLAIN_JSON_DONE) {
        const plain_json_Token *error_token = plain_json_get_token(context, token_count - 1);
        print_error(context, error_token->start, result);
    }

    plain_json_free(context);

    return 0;
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        uint8_t buffer[512] = { 0 };
        uintptr_t buffer_size = 0;
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

    int status = parse_json((uint8_t *)buffer, file_stat.st_size);
    munmap(buffer, file_stat.st_size);
    close(file_fd);

    return status;
}
