# KAR (Kimi Archive) Tool - Makefile

# Compiler settings
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude

# Directories
SRC_DIR := src

# Source files
SRCS := $(SRC_DIR)/main.cpp $(SRC_DIR)/archiver.cpp
TARGET := kar

# Test settings
TEST_DIR := tests
TEST_SRC := $(TEST_DIR)/test_crc32.cpp
TEST_TARGET := test_crc32

# Default target
.PHONY: all clean test

all: $(TARGET)

# Build main executable
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build and run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) include/crc32.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SRC)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET)

# Rebuild
rebuild: clean all
