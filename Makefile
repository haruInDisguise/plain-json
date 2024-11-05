# TODO: Replace Make with meson
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
INC_PATH = json.h

DEBUG_BIN = $(BUILD_DIR)/dump_state
DEBUG_SRC = tools/dump_state.c
DEBUG_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(DEBUG_SRC))

TEST_BIN = $(BUILD_DIR)/run_tests
TEST_SRC = test/test_main.c \
		   test/test_structure.c \
		   test/test_parsing.c \
		   test/test_utf8.c
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))

DEP := $(patsubst %.c,$(BUILD_DIR)/%.d,$(TEST_SRC) $(DEBUG_SRC))

all: build_tests build_bin

build_bin: $(DEBUG_BIN)

build_tests: $(TEST_BIN)

fuzz:
	$(MAKE) -C fuzzing

dev: clean
	@mkdir -p $(BUILD_DIR)
	bear --append --output "$(BUILD_DIR)/compile_commands.json" -- $(MAKE)

rebuild: clean all

clean:
	rm -rf $(BUILD_DIR)

.PHONY: build_bin build_tests fuzz dev rebuild clean
SUFFIXES:

# ----

$(TEST_BIN): $(DEP) $(TEST_OBJ)
	$(CC) $(LDFLAGS) $(TEST_OBJ) -o $@

$(DEBUG_BIN): $(DEP) $(DEBUG_OBJ)
	$(CC) $(LDFLAGS) $(DEBUG_OBJ) -o $@

-include $(DEP)

$(DEP): $(BUILD_DIR)/%.d: %.c $(INC_PATH)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $< -MM | \
		sed 's#$(notdir $*)\.o[ :]*#$(BUILD_PATH)/$*.o $(BUILD_PATH)/$*.d: #g' > $@

$(TEST_OBJ) $(DEBUG_OBJ): $(BUILD_DIR)/%.o: %.c $(INC_PATH)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

