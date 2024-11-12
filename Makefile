# TODO: Replace make with meson
CFLAGS += -std=c99 -Itest/libtest/include
CFLAGS += -Wall -Wpedantic -Wextra -Wno-sign-conversion -Wno-implicit-fallthrough
LDFLAGS =

BUILD_CONFIG ?= debug

ifeq ($(BUILD_CONFIG),debug)
CFLAGS += -DJSON_DEBUG -O0 -gdwarf-5 -fno-omit-frame-pointer -fsanitize=undefined,address
LDFLAGS += -fsanitize=undefined,address

else ifeq ($(BUILD_CONFIG),release)
CFLAGS += -O3

else
$(error 'Invalid build config: $(BUILD_CONFIG). Possible values are debug/release')
endif

BUILD_DIR = build
INC_PATH = plain_json.h

TEST_BIN = $(BUILD_DIR)/run_tests
TEST_SRC = test/test_main.c \
		   test/test_structure.c \
		   test/test_parsing.c \
		   test/test_utf8.c
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))

DEP := $(patsubst %.c,$(BUILD_DIR)/%.d,$(TEST_SRC) $(TOOLS_SRC))

all: build/json_test_suit build/dump_state build/run_tests

build/json_test_suit: tools/json_test_suit.c $(INC_PATH)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(CFLAGS) tools/json_test_suit.c -o $@

build/dump_state: tools/dump_state.c $(INC_PATH)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(CFLAGS) tools/dump_state.c -o $@

build/run_tests: $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(TEST_OBJ) -o $@

dev: clean
	@mkdir -p $(BUILD_DIR)
	bear --output "$(BUILD_DIR)/compile_commands.json" -- $(MAKE)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: build_bin build_tests fuzz dev rebuild clean
.SUFFIXES:

# ----

-include $(DEP)

$(DEP): $(BUILD_DIR)/%.d: %.c $(INC_PATH)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $< -MM | \
		sed 's#$(notdir $*)\.o[ :]*#$(BUILD_PATH)/$*.o $(BUILD_PATH)/$*.d: #g' > $@

$(TEST_OBJ): $(BUILD_DIR)/%.o: %.c $(TEST_SRC) $(INC_PATH)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@
