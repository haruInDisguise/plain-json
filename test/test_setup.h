#ifndef _TEST_CONFIG_H_
#define _TEST_CONFIG_H_

#include <plain_json.h>

#include <stdlib.h>
#include <test/test.h>

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

static plain_json_AllocatorConfig alloc_config = {
    .alloc_func = custom_alloc,
    .free_func = custom_free,
    .realloc_func = custom_realloc,
};
static plain_json_Context *context = NULL;

static void test_finalize(void) {
    plain_json_free(context);
}

#endif
