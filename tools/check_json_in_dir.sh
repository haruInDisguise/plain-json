#!/usr/bin/sh

SCRIPT_DIR="$(dirname $PWD/$0)"
ROOT="$(readlink -f "$SCRIPT_DIR/..")"

SAMPLES_DIR="$ROOT/json_test_data"
EXEC="$ROOT/build/dump_state"

if ! [ -x "$EXEC" ]; then
    echo "Missing dump exectuable"
    exit 1
fi

for file in "$SAMPLES_DIR/"pass*.json; do
    "$EXEC" "$file" &>/dev/null
    if [ $? -ne 0 ]; then
        echo Test failed: $file
        exit 1
    fi
done

echo "roundabout tests were successful"

for file in "$SAMPLES_DIR"/fail*.json; do
    "$EXEC" "$file" &>/dev/null
    if [ $? -eq 0 ]; then
        echo Test failed: $file
        exit 1
    fi
done
echo "fail tests were successful"
