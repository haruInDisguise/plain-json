#ifndef _TEST_CONFIG_H_
#define _TEST_CONFIG_H_

#include "../plain_json.h"

#include <stdlib.h>
#include <test/test.h>

static plain_json_AllocatorConfig alloc_config = {
    .alloc_func = malloc,
    .free_func = free,
    .realloc_func = realloc,
};

#endif
