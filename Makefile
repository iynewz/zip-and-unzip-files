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
TEST_DATA_DIR := $(TEST_DIR)/large_fixtures
STRESS_DATA_DIR := $(TEST_DIR)/stress_fixtures
GENERATE_SCRIPT := scripts/generate_test_data.py

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

# 生成大量测试数据 (标准模式: ~80MB, 用于快速测试)
test-data:
	python3 $(GENERATE_SCRIPT)

test-data-clean:
	python3 $(GENERATE_SCRIPT) --clean

# 生成压力测试数据 (Stress模式: ~500MB-1GB, 用于专业性能测试)
stress-data:
	python3 $(GENERATE_SCRIPT) --stress

stress-data-clean:
	python3 $(GENERATE_SCRIPT) --stress --clean

# 完整测试流程 (生成数据 + 打包 + 解包 + 验证)
# 结果保存到 benchmark_results.txt 用于后续对比
BENCHMARK_LOG := benchmark_results.txt

benchmark: all test-data
	@echo "=== KAR 性能基准测试 (标准模式) ==="
	@echo "说明: 使用 ~80MB 测试数据，适合快速回归测试"
	@$(MAKE) run-benchmark BENCHMARK_DATA_DIR=$(TEST_DATA_DIR)

# 压力测试 (用于专业性能评估)
benchmark-stress: all stress-data
	@echo "=== KAR 性能基准测试 (压力模式) ==="
	@echo "说明: 使用 ~500MB-1GB 测试数据，适合专业性能评估"
	@$(MAKE) run-benchmark BENCHMARK_DATA_DIR=$(STRESS_DATA_DIR)

# 内部使用的基准测试执行逻辑
run-benchmark:
	@echo "测试时间: $$(date '+%Y-%m-%d %H:%M:%S')" | tee $(BENCHMARK_LOG)
	@echo "测试数据: $(BENCHMARK_DATA_DIR)" | tee -a $(BENCHMARK_LOG)
	@echo "文件数量: $$(find $(BENCHMARK_DATA_DIR) -type f | wc -l)" | tee -a $(BENCHMARK_LOG)
	@echo "数据大小: $$(du -sh $(BENCHMARK_DATA_DIR) | cut -f1)" | tee -a $(BENCHMARK_LOG)
	@echo "================================" | tee -a $(BENCHMARK_LOG)
	@echo "" | tee -a $(BENCHMARK_LOG)
	@echo "1. 打包测试..." | tee -a $(BENCHMARK_LOG)
	@{ time ./$(TARGET) pack $(BENCHMARK_DATA_DIR) benchmark.kar ; } 2>&1 | tee -a $(BENCHMARK_LOG)
	@echo "" | tee -a $(BENCHMARK_LOG)
	@echo "2. 列出归档内容..." | tee -a $(BENCHMARK_LOG)
	@{ time ./$(TARGET) list benchmark.kar ; } 2>&1 | tee -a $(BENCHMARK_LOG)
	@echo "" | tee -a $(BENCHMARK_LOG)
	@echo "3. 解包测试..." | tee -a $(BENCHMARK_LOG)
	@rm -rf benchmark_output
	@{ time ./$(TARGET) unpack benchmark.kar benchmark_output ; } 2>&1 | tee -a $(BENCHMARK_LOG)
	@echo "" | tee -a $(BENCHMARK_LOG)
	@echo "4. 验证文件一致性..." | tee -a $(BENCHMARK_LOG)
	@diff -r $(BENCHMARK_DATA_DIR) benchmark_output && echo "✓ 文件一致" | tee -a $(BENCHMARK_LOG) || echo "✗ 文件不一致" | tee -a $(BENCHMARK_LOG)
	@echo "" | tee -a $(BENCHMARK_LOG)
	@echo "================================" | tee -a $(BENCHMARK_LOG)
	@echo "结果已保存到: $(BENCHMARK_LOG)"

benchmark-clean:
	rm -rf benchmark.kar benchmark_output benchmark_results.txt

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET)

clean-all: clean benchmark-clean
	@python3 $(GENERATE_SCRIPT) --clean 2>/dev/null || true
	@python3 $(GENERATE_SCRIPT) --stress --clean 2>/dev/null || true

# Rebuild
rebuild: clean all
