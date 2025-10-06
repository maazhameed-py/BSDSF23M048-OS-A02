# Makefile for ls-v1.0.0

CC = gcc
CFLAGS = -Wall
SRC = src/ls-v1.0.0.c
BIN_DIR = bin
TARGET = $(BIN_DIR)/ls

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

