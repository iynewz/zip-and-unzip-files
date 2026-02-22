# KAR (Kimi Archive) Tool

## Project Overview

This is a C++17 command-line archive tool that packs/unpacks directories into a custom binary archive format (`.kar` files). The project implements a simple archiver with support for recursive directory handling, preserving file permissions and directory structures.

## Technology Stack

- **Language**: C++17
- **Standard Library**: `std::filesystem` for file operations
- **Compiler**: Clang/GCC with C++17 support
- **Platform**: macOS (ARM64), but code is portable to other Unix-like systems

## Project Structure

```
.
├── src/                   # Source code
│   ├── main.cpp           # CLI entry point
│   ├── archiver.hpp       # Archiver class declaration
│   ├── archiver.cpp       # Archiver class implementation
│   ├── format.hpp         # File format structures (FileHeader, EntryHeader)
│   └── utils.hpp          # Utility functions (format_size, timestamp)
├── tests/                 # Test suite
│   ├── test_crc32.cpp     # CRC32 unit tests
│   └── fixtures/          # Test data
│       ├── a.txt
│       └── subdir/
│           └── b.txt
├── include/
│   └── crc32.hpp          # Shared CRC32 header
├── kar                    # Compiled executable
├── backup.kar             # Sample archive file for testing
├── .clangd                # LSP configuration
└── docs/
    ├── prd.md             # Product requirements document (Chinese)
    └── tasks.md           # Tasks tracking
```

## Build Instructions

项目已配置 Makefile，推荐使用方法：

```bash
# 编译主程序（默认使用 clang++）
make

# 使用 g++ 编译
make CXX=g++

# 运行测试
make test

# 清理构建产物
make clean

# 重新构建
make rebuild
```

如需手动编译（不使用 Makefile）：

```bash
# Using clang++
clang++ -std=c++17 -o kar src/main.cpp src/archiver.cpp

# Using g++
g++ -std=c++17 -o kar src/main.cpp src/archiver.cpp
```

The `.clangd` file contains LSP configuration specifying C++17 standard:
```yaml
CompileFlags:
  Add: [-std=c++17]
```

## Usage

The `kar` executable provides three commands:

```bash
# Pack a directory into a .kar archive
./kar pack <source_dir> <archive.kar>

# Unpack a .kar archive to a directory
./kar unpack <archive.kar> <target_dir>

# List contents of a .kar archive without extracting
./kar list <archive.kar>
```

### Example Usage

```bash
# Create archive from tests/fixtures
./kar pack tests/fixtures backup.kar

# List archive contents
./kar list backup.kar

# Extract to output directory
./kar unpack backup.kar output_dir
```

## Archive File Format

The `.kar` format uses a custom binary protocol:

### Global File Header (24 bytes)
```c
struct FileHeader {
    uint32_t magic;       // 'KAAR' (0x5241414B) - little-endian
    uint16_t version;     // Version 1
    uint32_t entry_count; // Number of files in archive
    uint64_t created_at;  // Unix timestamp
    uint32_t reserved;    // Reserved for future use
};
```

### Entry Header (26 bytes per file)
```c
struct EntryHeader {
    uint32_t path_length;   // Length of relative path string
    uint64_t content_size;  // File content size in bytes
    uint64_t modified_time; // Last modified timestamp
    uint32_t checksum;      // CRC32 (reserved, currently 0)
    uint16_t permissions;   // Unix file permissions (e.g., 0644)
};
```

### Data Layout
```
[FileHeader] + [EntryHeader + path_string + file_content] * N
```

**Note**: `#pragma pack(push, 1)` is used to ensure packed structure layout for cross-platform consistency.

## Code Organization

代码按功能拆分到 `src/` 目录下：

1. **format.hpp**: Binary format structures (`FileHeader`, `EntryHeader`) with `#pragma pack`
2. **utils.hpp**: Utility functions (`current_timestamp()`, `format_size()`)
3. **archiver.hpp/.cpp**: Core `Archiver` class
   - `pack()`: Recursively collect files and write archive
   - `unpack()`: Read archive and restore files
   - `list()`: Display archive contents
   - `write_entry()`: Serialize single file to archive
   - `read_entry()`: Deserialize single file from archive
4. **main.cpp**: CLI interface with argument parsing

## Development Roadmap

Per `docs/prd.md`, the project is planned in three phases:

### Phase 1: Basic Archive ✅ (Completed)
- [x] Binary file format protocol design
- [x] Serialization/deserialization
- [x] Recursive directory traversal
- [x] Unpack functionality

### Phase 2: Robustness (TODO)
- [ ] CRC32 or Adler32 checksum validation
- [ ] Progress bar for large files
- [ ] Incremental updates (append to existing archive)

### Phase 3: Compression (TODO)
- [ ] Huffman encoding for entropy compression
- [ ] Dictionary compression (LZ77/LZ78)

## Testing

### 基础测试 (小规模)
使用 `tests/fixtures/` 进行快速验证：
```bash
# Create archive
./kar pack tests/fixtures test.kar

# Verify contents
./kar list test.kar

# Extract and compare
./kar unpack test.kar test_output
diff -r tests/fixtures test_output
```

### 大规模性能测试
使用 `scripts/generate_test_data.py` 生成大量测试数据：

```bash
# 标准测试数据 (~80MB，用于快速回归测试)
make test-data

# 压力测试数据 (~500MB-1GB，用于专业性能评估)
make stress-data

# 或直接使用脚本
python3 scripts/generate_test_data.py

# 自定义测试数据规模
python3 scripts/generate_test_data.py --small-files 1000 --large-files 10

# 清理并重新生成
make test-data-clean && make test-data
make stress-data-clean && make stress-data
```

### 基准测试
完整性能测试流程（结果自动保存到 `benchmark_results.txt`）：

```bash
# 标准基准测试 (~80MB 数据，适合快速回归测试)
make benchmark

# 压力基准测试 (~500MB-1GB 数据，适合专业性能评估)
make benchmark-stress

# 查看历史结果
cat benchmark_results.txt

# 清理基准测试产物
make benchmark-clean

# 清理所有 (包括测试数据)
make clean-all
```

**专业建议**：
- **标准模式** (`make benchmark`): 数据量约 80MB，打包耗时 ~0.5s，适合快速回归测试
- **压力模式** (`make benchmark-stress`): 数据量约 500MB-1GB，打包耗时 5-30s，减少计时误差，适合：
  - 压缩算法性能对比
  - IO 瓶颈分析
  - 多轮测试取平均值

**基准测试输出示例：**
```
测试时间: 2026-02-21 17:15:30
测试数据: tests/large_fixtures
文件数量: 188
数据大小: 77M
================================

1. 打包测试...
[进度条...]
Archive created: "benchmark.kar" (188 files)
      0.47 real         0.15 user         0.31 sys

2. 列出归档内容...
[输出...]
      0.01 real         0.00 user         0.00 sys

3. 解包测试...
[进度条...]
Extracted to: "benchmark_output"
      0.35 real         0.05 user         0.28 sys

4. 验证文件一致性...
✓ 文件一致
```

### 测试数据结构
生成的 `tests/large_fixtures/` 包含以下场景：
- `small_files/`: 小文件密集场景 (1-10 KB)，分散在多个子目录
- `medium_files/`: 中等文件 (100 KB - 1 MB)
- `large_files/`: 大文件 (5-20 MB)
- `deep_structure/`: 深层嵌套目录 (10层)
- `mixed_project/`: 模拟真实项目结构 (源码、配置、日志)

## Code Style Guidelines

- **Naming**: 
  - Classes: `PascalCase` (e.g., `Archiver`, `FileHeader`)
  - Methods: `snake_case` (e.g., `pack()`, `write_entry()`)
  - Variables: `snake_case` (e.g., `file_path`, `rel_path`)
- **Indentation**: 2 spaces
- **Comments**: Chinese comments in code, indicating original author preference
- **Error Handling**: Exceptions (`std::runtime_error`) for error conditions
- **Namespace**: `fs` alias for `std::filesystem`

## Security Considerations

1. **Path Traversal**: The code uses `std::filesystem::relative()` to maintain directory structure, but doesn't explicitly sanitize paths. Be cautious when extracting untrusted archives.
2. **File Permissions**: Permissions are stored and restored using Unix-style mode bits.
3. **Checksum**: The `checksum` field in EntryHeader is currently a placeholder (always 0), not providing data integrity verification.

## Future Enhancements

1. Implement CRC32 checksum calculation and verification
2. Add compression algorithms (Huffman, LZ77)
3. Add progress bar for large archive operations
4. Support incremental archive updates
5. Add unit tests using a testing framework
6. Create proper build system (CMake or Makefile)
