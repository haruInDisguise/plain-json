#include <stdlib.h>
#include <unistd.h>

#define PLAIN_JSON_IMPLEMENTATION
#include "../plain_json.h"

#pragma clang optimize off

void parse_json(const char *buffer, size_t buffer_length) {
    plain_json_Context context = { 0 };
    plain_json_load_buffer(&context, buffer, buffer_length);

    plain_json_Token token = { 0 };

    int state = PLAIN_JSON_HAS_REMAINING;
    while (state == PLAIN_JSON_HAS_REMAINING) {
        state = plain_json_read_token(&context, &token);
    }

    if(state < 0 || state > PLAIN_JSON_ERROR_UNEXPECTED_TOKEN) {
        abort();
    }
}

__AFL_FUZZ_INIT();

int main(void) {
    // "Deferred Initialization" should be safe to use in our use case.
    // See:
    // https://github.com/AFLplusplus/AFLplusplus/blob/stable/instrumentation/README.persistent_mode.md#3-deferred-initialization
    __AFL_INIT();

    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF; // must be after __AFL_INIT
                                                  // and before __AFL_LOOP!
    while (__AFL_LOOP(10000)) {
        int len = __AFL_FUZZ_TESTCASE_LEN; // don't use the macro directly in a
                                           // call!

        if (len < 2) {
            continue; // check for a required/useful minimum input length
                      // the smallest valid json token is an empty array/object
        }

        parse_json(buf, len);
    }

    return 0;
}
