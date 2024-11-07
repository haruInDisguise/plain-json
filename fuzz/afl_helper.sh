#!/usr/bin/sh

SCRIPT_DIR="$(dirname $PWD/$0)"

BIN_TARGET="$SCRIPT_DIR/fuzzing_target"

OUTPUT="$SCRIPT_DIR/output"

INPUT_ROOT="$SCRIPT_DIR/input_raw"
INPUT_UNIQ="$SCRIPT_DIR/input_uniq"
INPUT_FINAL="$SCRIPT_DIR/input"

prepare_input() {
    rm -rf "$INPUT_FINAL"
    rm -rf "$INPUT_UNIQ"

    mkdir -p "$INPUT_UNIQ"
    afl-cmin -T all -i "$INPUT_ROOT" -o "$INPUT_UNIQ" -- "$BIN_TARGET" @@

    mkdir -p "$INPUT_FINAL"

    index=0
    for file in $INPUT_UNIQ/*.json; do
        afl-tmin -i "$file" -o "$INPUT_FINAL/case_${index}.json" -- "$BIN_TARGET" @@
        index=$((index + 1))
    done

    rm -rf "$INPUT_UNIQ"
}

build() {
    export AFL_USE_UBSAN=1
    export AFL_USE_ASAN=1
    afl-clang-fast -o "$BIN_TARGET" "$SCRIPT_DIR/fuzzing_target.c"
}

run() {
    export AFL_TESTCACHE_SIZE=1000
    export AFL_LLVM_MAP_ADDR=0x10000
    export AFL_AUTORESUME=1
    afl-fuzz -i input -o output -x json.dict -- ./$(TARGET)
}

clean() {
    rm -rf "$BIN_TARGET" "$OUTPUT"
}

if ! command -v afl-fuzz >/dev/null 2>&1 || ! command -v afl-clang-fast >/dev/null 2>&1; then
    echo "Missing AflPlusPlus binaries (https://github.com/AFLplusplus/AFLplusplus)"
    exit 1
fi

case "$1" in
    build) build;;
    run) run;;
    prepare-input) prepare_input;;
    clean) clean;;
    *) echo "Invalid command. Values: (build|run|prepare-input|clean)"; exit 1
esac
