# Compiler settings
CC       = gcc
CFLAGS   = -Wall -Wextra -std=gnu11
SRC      = src/ls-v1.5.0.c
BIN_DIR  = bin
TARGET   = $(BIN_DIR)/ls

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Remove compiled binary
clean:
	rm -f $(TARGET)

# Phony targets (not real files)
.PHONY: all clean
