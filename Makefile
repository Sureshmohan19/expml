CC = gcc
CFLAGS = -Wall -O2 -Isrc
LDFLAGS = -lncursesw -lcjson -lm
BUILD_DIR = build
TARGET = $(BUILD_DIR)/expml

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=$(BUILD_DIR)/%.o)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ============================
# Auto-build all tests
# ============================

TEST_SRCS := $(wildcard tests/test_*.c)
TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

TEST_SRCS_BASE := $(filter-out src/expml.c, $(SRCS))

# Build all test binaries
test: $(TEST_BINS)

# Pattern rule: test_X.c pairs with src/X.c
$(BUILD_DIR)/test_%: tests/test_%.c $(TEST_SRCS_BASE) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# ============================

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean
