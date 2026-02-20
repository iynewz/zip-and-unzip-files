# KAR (Kimi Archive) Tool - Makefile

# Compiler settings
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

# Target executable
TARGET := kar
SRC := archiver.cpp

# Test settings
TEST_SRC := test_crc32.cpp
TEST_TARGET := test_crc32

# Default target
.PHONY: all clean test

all: $(TARGET)

# Build main executable
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build and run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) crc32.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SRC)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET)

# Rebuild
rebuild: clean all
