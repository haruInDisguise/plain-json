# plain-json

A plain/"no bullshit" approach to JSON serialization/deserialization in C.

## What it is

- [STD style](https://github.com/nothings/stb), header only library
- Hackable, integratable and customizable, thanks to its lack of dynamic memory allocation, external
  dependencies (even libc) and small code footprint.
- Completely free of external dependencies (even libc) or dynamic memory allocation.
- Written in C99
- Streaming, single pass parser. This enables a very low memory footprint and support for large files
- Correct. The library is strictly compliance with ECMA-404/RFC8259 and passes all the parsing related
  test cases provided by the [JSONTestSuit](https://github.com/nst/JSONTestSuite), with sane choices
  regarding the implementation defined test cases.

## What it isn't

- Feature rich. This parser does one job: Serialize/Deserialize JSON.
- Extremely performant. While this library should be reasonably quick (on account of it only doing what
  is necessary, and including some "low hanging" speedups), not a lot of effort went into optimization.

## JSONTestSuit

https://github.com/nst/JSONTestSuite

## API

NOTE: This library is in early development and might introduce breaking changes to the API.

The context contains the state of ongoing/completed parsing operations. Unless you know what you are doing,
fields starting with an underscore should be ignored, as they are reserved for internal use.
In general, all fields in this struct shoud be treated as read only.

```c
typedef struct {
    const char *_buffer;

    unsigned int _buffer_size;
    unsigned int _buffer_offset;

    unsigned short _depth_buffer_index;
    unsigned short _depth_buffer[PLAIN_JSON_OPTION_MAX_DEPTH];

    int line;           /* The parsers current offset as lines/line_offset. */
    int line_offset;    /* Usefull for locateing errors. */
} plain_json_Context;
```


```c
typedef struct {
    unsigned int start;
    unsigned int length;

    unsigned int key_start;
    unsigned int key_length;

    plain_json_Type type;
} plain_json_Token;
```

Each Token can represent either a value or a structural token (the start/end of an object/array).

## Integration with C++

## Resources

- ECMA404 spec: https://ecma-international.org/publications-and-standards/standards/ecma-404/
- RFC8259: https://www.rfc-editor.org/rfc/rfc8259

- RFC3629: https://www.rfc-editor.org/rfc/rfc3629
- UTF-8 Wiki: https://en.wikipedia.org/wiki/UTF-8
- "Unicode Encoding! [...]" by EmNudge: https://www.youtube.com/watch?v=uTJoJtNYcaQ&t=2s
