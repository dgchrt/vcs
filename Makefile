CC = gcc
CFLAGS = -Wall -Wextra -O2
BUILD_DIR = build
DIST_DIR = dist

SRCS = $(wildcard *.c)
HDRS = $(wildcard *.h)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET = $(DIST_DIR)/vcs

.PHONY: all clean format lint

all: $(TARGET)

format:
	clang-format -i *.c *.h

lint:
	cppcheck --enable=all --std=c11 --suppress=missingIncludeSystem .

$(TARGET): $(OBJS) | $(DIST_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.c $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(DIST_DIR):
	mkdir -p $(DIST_DIR)

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
