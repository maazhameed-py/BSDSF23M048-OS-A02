# Compiler
CC = gcc
CFLAGS = -Wall
SRC = src/ls-v1.1.0.c
BIN_DIR = bin
TARGET = $(BIN_DIR)/ls   # simple name "ls"

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Clean compiled files
clean:
	rm -f $(TARGET)

.PHONY: all clean
