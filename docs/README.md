# KAR (Kimi Archive) Tool

一个基于 C++17 的命令行归档工具，用于将目录打包/解包为自定义二进制归档格式（`.kar` 文件）。

## 功能特性

- **打包目录**：将目录递归打包为 `.kar` 归档文件
- **解包归档**：从 `.kar` 文件还原目录结构和文件
- **列出内容**：无需解压即可查看归档内容
- **保留属性**：保留文件权限和修改时间

## 快速开始

### 编译

项目使用 Makefile 构建，确保你的系统已安装支持 C++17 的编译器（推荐 Clang 或 GCC）：

```bash
# 编译主程序
make

# 或使用 g++ 编译
make CXX=g++

# 完整重新构建
make rebuild
```

编译完成后会生成 `kar` 可执行文件。

### 基本用法

#### 1. 打包目录

```bash
./kar pack <源目录> <归档文件.kar>
```

示例：
```bash
./kar pack test_dir backup.kar
```

#### 2. 列出归档内容

```bash
./kar list <归档文件.kar>
```

示例：
```bash
./kar list backup.kar
```

输出示例：
```
KAR Archive: backup.kar
================
Entries: 2
Created: 2024-01-15 10:30:45
================
  1. a.txt		12 bytes	2024-01-15 10:25:30
  2. subdir/b.txt	24 bytes	2024-01-15 10:26:15
```

#### 3. 解包归档

```bash
./kar unpack <归档文件.kar> <目标目录>
```

示例：
```bash
./kar unpack backup.kar output_dir
```

## Makefile 指令参考

| 指令 | 说明 |
|------|------|
| `make` 或 `make all` | 编译生成 `kar` 可执行文件 |
| `make test` | 编译并运行 CRC32 测试程序 |
| `make clean` | 删除编译生成的文件 |
| `make rebuild` | 清理后重新编译 |
| `make CXX=g++` | 使用指定编译器（默认 clang++）|

## 归档文件格式

`.kar` 格式使用自定义二进制协议：

**文件头** (24 字节)
- Magic: `KAAR` (0x5241414B)
- 版本: 1
- 条目数量
- 创建时间戳

**条目头** (26 字节 + 路径 + 内容)
- 路径长度
- 内容大小
- 修改时间
- CRC32 校验和（预留）
- 文件权限

## 项目结构

```
.
├── archiver.cpp      # 主程序源码
├── crc32.hpp         # CRC32 校验头文件
├── kar               # 编译后的可执行文件
├── Makefile          # 构建配置
├── backup.kar        # 示例归档文件
├── test_dir/         # 测试目录
├── docs/             # 项目文档
└── README.md         # 本文件
```

## 测试

项目包含 `test_dir` 目录用于手动测试：

```bash
# 创建测试归档
make
./kar pack test_dir test.kar

# 查看内容
./kar list test.kar

# 解压并对比
./kar unpack test.kar test_output
diff -r test_dir test_output
```

运行单元测试：

```bash
make test
```

## 系统要求

- C++17 兼容的编译器（Clang 或 GCC）
- 支持 `std::filesystem` 的标准库
- macOS / Linux / 其他 Unix-like 系统

## 注意事项

1. **路径安全**：解压时请注意归档文件来源，避免解压不受信任的归档文件
2. **校验和**：当前版本的 CRC32 校验和字段为预留状态（值为 0），不提供数据完整性验证
3. **压缩**：当前版本不支持压缩，仅做打包归档

## 许可证

本项目为示例项目，可自由使用。
