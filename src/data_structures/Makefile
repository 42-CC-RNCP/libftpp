# src/data_structures/Makefile
BUILD_DIR = ../../build
ROOT_DIR  = ../..

TARGET = data_structures

.PHONY: all

all:
	@cmake -B$(BUILD_DIR) -S $(ROOT_DIR)
	@cmake --build $(BUILD_DIR) --target $(TARGET) -- -s
