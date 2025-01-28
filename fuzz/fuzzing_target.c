#define PLAIN_JSON_IMPLEMENTATION
#include "../plain-json/plain_json.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#pragma clang optimize off

void parse_json(uint8_t *buffer, intptr_t buffer_length) {
    plain_json_AllocatorConfig alloc_config = {
        .alloc_func = malloc,
        .realloc_func = realloc,
        .free_func = free,
    };

    plain_json_ErrorType state = PLAIN_JSON_NONE;

    plain_json_Context *context = plain_json_parse(alloc_config, buffer, buffer_length, &state);
    if (state < 0 || state > PLAIN_JSON_ERROR_ILLEGAL_CHAR) {
        abort();
    }

    plain_json_free(context);
}

__AFL_FUZZ_INIT();

int main(void) {
    // "Deferred Initialization" should be safe to use in our use case.
    // See:
    // https://github.com/AFLplusplus/AFLplusplus/blob/stable/instrumentation/README.persistent_mode.md#3-deferred-initialization
    __AFL_INIT();

    uint8_t *fuzz_buffer = __AFL_FUZZ_TESTCASE_BUF; // must be after __AFL_INIT

    // and before __AFL_LOOP!
    while (__AFL_LOOP(10000)) {
        uint32_t case_length = __AFL_FUZZ_TESTCASE_LEN; // don't use the macro directly in a

        uint8_t *case_buffer = malloc(case_length);
        memcpy(case_buffer, fuzz_buffer, case_length);
        parse_json(case_buffer, case_length);
    }

    return 0;
}
