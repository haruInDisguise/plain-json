# plain-json

A very small and lightweight JSON Parser.

## Features

- [STD style](https://github.com/nothings/stb), header only library. It does not require and external dependencies, including libc!
- Written in C99
- Streaming, single pass parser. This enables a very low memory footprint and support for large files[^1]
- No memory dynamic allocation
- Support UTF-8 validation [^2]
- Strictly compliance with ECMA-404/RFC8259

[^1]: This might not be the best approach for most usecases. See TODO
[^2]: Valid but unused codepoints are (yet) not considered to be invalid

## Do be done

- Port to C89?
- Switch from Make to Meson

---

- It makes a more sense to return strings and keys as slices into the file buffer itself, instead of reading them into seperate buffers
    - This would also make it possible to implement basic support for reading multiple tokens at once
- Correctly report unused UTF-8 codepoints as invalid
- Add compile time options using macros:
    - Ability to set the maximum nesting level
    - Integer/Float parsing as 64bit int/float types
    - Integer/Float parsing as 32bit int/float types

## API

The entrypoint is represented as a "context":
```c
typedef struct {
    const char *buffer;

    unsigned long buffer_size;
    unsigned long buffer_offset;

    int depth_buffer_index;
    int depth_buffer[PLAIN_JSON_OPTION_MAX_DEPTH];
    /* Reserved for internal use */
    int _last_token_type;

    int line;
    int line_offset;
} plain_json_Context;
```

This library represents JSON data as primitive Tokens:
```c
typedef struct {
    char *value_buffer;
    char *key_buffer;

    int key_buffer_size;
    int value_buffer_size;

    int start;
    int end;

    plain_json_Type type;
} plain_json_Token;
```

Each Token can represent either a value or a structural token (the start/end of an object/array).

## Resources

- ECMA404 spec: https://ecma-international.org/publications-and-standards/standards/ecma-404/
- "json.org spec": https://www.json.org/json-en.html
- RFC8259: https://www.rfc-editor.org/rfc/rfc8259

- UTF-8 Wiki: https://en.wikipedia.org/wiki/UTF-8
- "Unicode Encoding! [...]" by EmNudge: https://www.youtube.com/watch?v=uTJoJtNYcaQ&t=2s
- RFC3629: https://www.rfc-editor.org/rfc/rfc3629
