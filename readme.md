# plain-json

A plain/"no bullshit" approach to JSON serialization/deserialization in C.

## What it is

- A hobby project
- [STD style](https://github.com/nothings/stb), header only library
- Hackable, zero dependencies (even libc) and a small code footprint.
- Written in C99
- Single pass parser
- Correct. The library is strictly compliant with ECMA-404/RFC8259 and passes all the parsing related
  test cases provided by the [JSONTestSuit](https://github.com/nst/JSONTestSuite), with sane choices
  regarding the implementation defined test cases. Some of the results for the later are still up for consideration.

## What it isn't

- Feature rich. This parser does one job: Deserialize JSON.
- Highly performant. While this library should be reasonably quick (on account of it only doing what
  is necessary, and including some "low hanging" speedups), not a lot of effort went into optimization.

## What it needs

- Floating point parsing
- JSON serialization
- Key/Query lookup
- Instructions for including a meson project with cmake (ExternalProject)

## Getting started

> [!WARNING]
> This library is under (somewhat) active development and future
> versions might introduce breaking changes.

The following instructions assume that you use the [meson build system](https://mesonbuild.com).
You could add this project as a meson subproject/[wrap dependency](https://mesonbuild.com/Wrap-dependency-system-manual.html) and then integrate it into your setup:

1. Create a folder called ```subprojects``` with a file named ```plain-json.wrap```:
```meson
# subprojects/plain-json.wrap

[wrap-git]
url = https://github.com/haruInDisguise/plain-json.git
revision = HEAD # FIXME: Set the wrap branch to a stable release
depth = 1

[provide]
dependency_names = 'plain-json'
```

2. and then integrate it into your build setup:
```meson
# meson.build

plain_json_dep = dependency('plain-json', required: true)

my_exe = executable(..., dependencies: [ plain_json_dep, ... ])
```

### Parsing/Deserialization

> The API is documented in-source at [plain_json.h](/blob/main/plain-json/plain_json.h)

```c
// While the library itself does not have any dependencies,
// this example does uses libc for clarity.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include this directive in the source file of your application.
#define PLAIN_JSON_IMPLEMENTATION
#include "../plain_json.h"

int main(void) {
    const char *text = "{\"database\": [ { \"item\": \"\\uD83D\\uDC08\", \"count\": 12345} ] }";

    // The parser requires "libc compatible (malloc/free/realloc)" allocator functions
    plain_json_AllocatorConfig alloc_config = {
        .free_func = free,
        .alloc_func = malloc,
        .realloc_func = realloc,
    };

    plain_json_ErrorType error_code = 0;

    plain_json_Context *context = plain_json_parse(alloc_config, (const u8 *)text, strlen(text), &error_code);
    const u32 token_count = plain_json_get_token_count(context);

    if (error_code != PLAIN_JSON_DONE) {
        // If an error occured, the last token will be of type "PLAIN_JSON_ERROR"
        const plain_json_Token *error_token = plain_json_get_token(context, token_count - 1);

        // Figure out the readable type and offset file offset that contains the error.
        // The tokens offset field is guaranteed to represent a valid position.
        u32 line = 0, line_offset = 0;
        if(!plain_json_compute_position(context, error_token->start, &line, &line_offset)) {
            fprintf(stderr, "error: token start is out of bounds\n");
            goto on_error;
        }

        const char *error_string = plain_json_error_to_string(error_code);
        fprintf(stderr, "error: %zu:%zu: %s\n", line, line_offset, error_string);
        goto on_error;
    }

    for (u32 i = 0; i < token_count; i++) {
        const plain_json_Token *token = plain_json_get_token(context, i);

        if(token->key_index == PLAIN_JSON_NO_KEY)
            continue;

        const u8 *key = plain_json_get_key(context, token->key_index);
        if(strcmp((char*)key, "count") == 0) {
            printf("%s = %lld\n", key, token->value.integer);
        }

        if(strcmp((char*)key, "item") == 0) {#
            // String values reference library internal datastructures and should be copied for reuse (strdup(3) etc.)
            printf("%s = %s\n", key, plain_json_get_string(context, token->value.string_index));
        }
    }

    plain_json_free(context);
    return 0;

on_error:
    plain_json_free(context);
    return 1;
}
```

## Testing

To build the test suit aswell as the libraries JSONTestSuite integration, build the project like so:
```sh
git clone https://github.com/haruInDisguise/plain-json
cd plain-json

meson setup build
meson compile -C build
```

This process builds the folowing binaries:

1. ```build/test/run_tests```: Run unit tests.
2. ```build/tools/json_test_suit```: Compare the library against the JSONTestSuite.
3. ```build/tools/dump_state```: A small utility that reads json from stdout and logs its parsed layout/any errors

## Development
At this point, this is mostly a hobby project, born out of curiosity and too much free time.
There are still a few features I want to implement.

## Resources

- ECMA404 spec: https://ecma-international.org/publications-and-standards/standards/ecma-404/
- RFC8259: https://www.rfc-editor.org/rfc/rfc8259

- RFC3629: https://www.rfc-editor.org/rfc/rfc3629
- UTF-8 Wiki: https://en.wikipedia.org/wiki/UTF-8
- UTF-16 Wiki: https://en.wikipedia.org/wiki/UTF-16
- "Unicode Encoding! [...]" by EmNudge: https://www.youtube.com/watch?v=uTJoJtNYcaQ&t=2s
