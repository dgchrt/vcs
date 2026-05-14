CC = gcc
CFLAGS = -Wall -Wextra -O2
BUILD_DIR = build
DIST_DIR = dist

SRCS = vcs.c conio.c
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET = $(DIST_DIR)/vcs

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(DIST_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.c vcs.h conio.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(DIST_DIR):
	mkdir -p $(DIST_DIR)

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
