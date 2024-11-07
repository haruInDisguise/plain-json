#include <stdlib.h>
#include <unistd.h>

#define JSON_IMPLEMENTATION
#include "../plain_json.h"

#pragma clang optimize off

void parse_json(const char *buffer, size_t buffer_length) {
    char string_buffer[256] = { 0 };
    char key_buffer[64] = { 0 };

    plain_jsonContext context = { 0 };
    plain_jsonsetup(&context);
    plain_jsonload_buffer(&context, buffer, buffer_length);

    plain_jsonToken token = { 0 };
    plain_jsontoken_setup(&token, key_buffer, 64, string_buffer, 256);

    int state = JSON_TRUE;
    while (state == JSON_TRUE) {
        state = plain_jsonread_token(&context, &token);
    }
}

__AFL_FUZZ_INIT();

int main(void) {
    // "Deffered Initialization" should be safe to use in our use case.
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
