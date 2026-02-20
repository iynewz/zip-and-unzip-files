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
├── archiver.cpp      # Main source file (all logic in single file)
├── kar               # Compiled executable (Mach-O 64-bit ARM64)
├── .clangd           # LSP configuration (specifies C++17 standard)
├── backup.kar        # Sample archive file for testing
├── docs/
│   ├── prd.md        # Product requirements document (Chinese)
│   └── tasks.md      # Tasks tracking (currently empty)
└── test_dir/         # Test directory for development
    ├── a.txt
    └── subdir/
        └── b.txt
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
clang++ -std=c++17 -o kar archiver.cpp

# Using g++
g++ -std=c++17 -o kar archiver.cpp
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
# Create archive from test_dir
./kar pack test_dir backup.kar

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

The entire implementation is contained in `archiver.cpp`:

1. **Structures** (lines 15-35): Binary format definitions with `#pragma pack`
2. **Archiver Class** (lines 40-204): Core functionality
   - `pack()`: Recursively collect files and write archive
   - `unpack()`: Read archive and restore files
   - `list()`: Display archive contents
   - `write_entry()`: Serialize single file to archive
   - `read_entry()`: Deserialize single file from archive
   - Helper methods: `current_timestamp()`, `format_size()`
3. **CLI Interface** (lines 209-242): Main function with argument parsing

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

Manual testing with `test_dir/`:
```bash
# Create archive
./kar pack test_dir test.kar

# Verify contents
./kar list test.kar

# Extract and compare
./kar unpack test.kar test_output
diff -r test_dir test_output
```

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
