#!/usr/bin/env python3
"""
生成大量测试数据用于 KAR 归档工具的性能测试

用法:
    python3 scripts/generate_test_data.py [options]

选项:
    --output-dir DIR    输出目录 (默认: tests/large_fixtures)
    --small-files N     小文件数量 (默认: 100)
    --medium-files N    中等文件数量 (默认: 20)
    --large-files N     大文件数量 (默认: 5)
    --deep-dirs N       深层目录层级 (默认: 10)
    --clean             清理并重新生成
"""

import os
import sys
import random
import string
import argparse
from pathlib import Path

# 默认配置
DEFAULT_CONFIG = {
    'output_dir': 'tests/large_fixtures',
    'small_files': 100,      # 1-10 KB
    'medium_files': 20,      # 100 KB - 1 MB
    'large_files': 5,        # 5-20 MB
    'deep_dirs': 10,         # 深层目录层级
}

# 压力测试配置 (用于专业性能测试)
STRESS_CONFIG = {
    'output_dir': 'tests/stress_fixtures',
    'small_files': 1000,     # 1000 个小文件
    'medium_files': 100,     # 100 个中等文件
    'large_files': 20,       # 20 个大文件 (100-400MB)
    'deep_dirs': 20,         # 20 层深层目录
}

# 文件大小范围 (字节)
SIZE_RANGES = {
    'small': (1024, 10 * 1024),           # 1-10 KB
    'medium': (100 * 1024, 1024 * 1024),  # 100 KB - 1 MB
    'large': (5 * 1024 * 1024, 20 * 1024 * 1024),  # 5-20 MB
}


def random_content(size):
    """生成指定大小的随机文本内容"""
    # 使用可重复的种子确保内容可预测
    random.seed(size)
    # 生成随机文本而不是二进制，便于调试
    chars = string.ascii_letters + string.digits + string.punctuation + '\n '
    return ''.join(random.choices(chars, k=size)).encode('utf-8')


def generate_file(filepath, size_range):
    """生成一个随机文件"""
    min_size, max_size = size_range
    size = random.randint(min_size, max_size)
    content = random_content(size)
    
    filepath.parent.mkdir(parents=True, exist_ok=True)
    with open(filepath, 'wb') as f:
        f.write(content)
    
    return size


def generate_small_files(base_dir, count):
    """生成大量小文件 (模拟小文件密集场景)"""
    print(f"  生成 {count} 个小文件 (1-10 KB)...")
    total_size = 0
    
    # 分散到多个子目录
    subdirs = ['tiny', 'small', 'micro']
    
    for i in range(count):
        subdir = random.choice(subdirs)
        filename = f"file_{i:04d}.txt"
        filepath = base_dir / 'small_files' / subdir / filename
        total_size += generate_file(filepath, SIZE_RANGES['small'])
    
    return total_size


def generate_medium_files(base_dir, count):
    """生成中等大小文件"""
    print(f"  生成 {count} 个中等文件 (100 KB - 1 MB)...")
    total_size = 0
    
    for i in range(count):
        filename = f"medium_{i:03d}.dat"
        filepath = base_dir / 'medium_files' / filename
        total_size += generate_file(filepath, SIZE_RANGES['medium'])
    
    return total_size


def generate_large_files(base_dir, count):
    """生成大文件"""
    print(f"  生成 {count} 个大文件 (5-20 MB)...")
    total_size = 0
    
    for i in range(count):
        filename = f"large_{i:02d}.bin"
        filepath = base_dir / 'large_files' / filename
        total_size += generate_file(filepath, SIZE_RANGES['large'])
    
    return total_size


def generate_deep_structure(base_dir, depth):
    """生成深层目录结构"""
    print(f"  生成 {depth} 层深层目录结构...")
    
    current_dir = base_dir / 'deep_structure'
    for i in range(depth):
        current_dir = current_dir / f"level_{i:02d}"
    
    # 在最深层放置一些文件
    total_size = 0
    for i in range(5):
        filepath = current_dir / f"deep_file_{i}.txt"
        total_size += generate_file(filepath, SIZE_RANGES['small'])
    
    # 在每层也放置一些文件
    temp_dir = base_dir / 'deep_structure'
    for i in range(depth):
        filepath = temp_dir / f"level_file_{i}.txt"
        total_size += generate_file(filepath, (100, 1000))
        temp_dir = temp_dir / f"level_{i:02d}"
    
    return total_size


def generate_mixed_content(base_dir):
    """生成混合内容目录"""
    print("  生成混合内容目录...")
    total_size = 0
    
    # 模拟一个真实项目结构
    mixed_dir = base_dir / 'mixed_project'
    
    # 源代码文件
    src_dir = mixed_dir / 'src'
    for i in range(20):
        filepath = src_dir / f"module_{i:02d}.cpp"
        # 模拟源代码内容
        lines = [
            f"// Module {i}",
            "#include <iostream>",
            "",
            f"void function_{i}() {{",
            '    std::cout << "Hello from module ' + str(i) + '" << std::endl;',
            "}",
            "",
        ]
        # 添加一些随机填充
        content = '\n'.join(lines) + '\n' + random_content(random.randint(500, 2000)).decode('utf-8', errors='ignore')
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, 'w') as f:
            f.write(content)
        total_size += len(content)
    
    # 头文件
    include_dir = mixed_dir / 'include'
    for i in range(15):
        filepath = include_dir / f"header_{i:02d}.h"
        content = f"#pragma once\n// Header {i}\n\nclass Class{i} {{\n    int value;\n}};\n"
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, 'w') as f:
            f.write(content)
        total_size += len(content)
    
    # 配置文件
    config_dir = mixed_dir / 'config'
    config_dir.mkdir(parents=True, exist_ok=True)
    config_content = "# Configuration\n" + "key=value\n" * 100
    (config_dir / 'app.conf').write_text(config_content)
    total_size += len(config_content)
    
    # 日志文件
    log_dir = mixed_dir / 'logs'
    log_dir.mkdir(parents=True, exist_ok=True)
    for i in range(10):
        filepath = log_dir / f"app_{i:03d}.log"
        lines = [f"[{i}] Log entry at timestamp {random.randint(1000000, 9999999)}" for _ in range(100)]
        content = '\n'.join(lines)
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, 'w') as f:
            f.write(content)
        total_size += len(content)
    
    # 文档
    docs_dir = mixed_dir / 'docs'
    docs_dir.mkdir(parents=True, exist_ok=True)
    (docs_dir / 'README.md').write_text("# Project Documentation\n\n" + "Lorem ipsum.\n" * 50)
    (docs_dir / 'API.md').write_text("# API Reference\n\n" + "Function documentation.\n" * 30)
    
    return total_size


def count_files_and_size(directory):
    """统计目录中的文件数量和总大小"""
    total_files = 0
    total_size = 0
    
    for root, dirs, files in os.walk(directory):
        total_files += len(files)
        for file in files:
            filepath = os.path.join(root, file)
            try:
                total_size += os.path.getsize(filepath)
            except OSError:
                pass
    
    return total_files, total_size


def format_size(size_bytes):
    """格式化文件大小显示"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_bytes < 1024.0:
            return f"{size_bytes:.2f} {unit}"
        size_bytes /= 1024.0
    return f"{size_bytes:.2f} TB"


def main():
    parser = argparse.ArgumentParser(
        description='生成 KAR 归档工具的大量测试数据',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
    # 使用默认配置生成测试数据
    python3 scripts/generate_test_data.py

    # 生成大量小文件进行压力测试
    python3 scripts/generate_test_data.py --small-files 1000

    # 清理并重新生成
    python3 scripts/generate_test_data.py --clean

    # 自定义输出目录
    python3 scripts/generate_test_data.py --output-dir /tmp/kar_test_data
""")
    
    parser.add_argument('--output-dir', default=DEFAULT_CONFIG['output_dir'],
                        help='测试数据输出目录 (默认: tests/large_fixtures)')
    parser.add_argument('--small-files', type=int, default=DEFAULT_CONFIG['small_files'],
                        help=f'小文件数量，默认 {DEFAULT_CONFIG["small_files"]}')
    parser.add_argument('--medium-files', type=int, default=DEFAULT_CONFIG['medium_files'],
                        help=f'中等文件数量，默认 {DEFAULT_CONFIG["medium_files"]}')
    parser.add_argument('--large-files', type=int, default=DEFAULT_CONFIG['large_files'],
                        help=f'大文件数量，默认 {DEFAULT_CONFIG["large_files"]}')
    parser.add_argument('--deep-dirs', type=int, default=DEFAULT_CONFIG['deep_dirs'],
                        help=f'深层目录层级，默认 {DEFAULT_CONFIG["deep_dirs"]}')
    parser.add_argument('--clean', action='store_true',
                        help='清理现有测试数据并重新生成')
    parser.add_argument('--stress', action='store_true',
                        help='使用压力测试配置 (生成约 500MB-1GB 数据用于专业性能测试)')
    
    args = parser.parse_args()
    
    # 如果使用压力测试模式，覆盖配置
    if args.stress:
        print("启用压力测试模式 (Stress Mode)")
        args.output_dir = STRESS_CONFIG['output_dir']
        args.small_files = STRESS_CONFIG['small_files']
        args.medium_files = STRESS_CONFIG['medium_files']
        args.large_files = STRESS_CONFIG['large_files']
        args.deep_dirs = STRESS_CONFIG['deep_dirs']
    
    base_dir = Path(args.output_dir)
    
    # 清理模式
    if args.clean:
        if base_dir.exists():
            print(f"清理现有测试数据: {base_dir}")
            import shutil
            shutil.rmtree(base_dir)
            print("清理完成")
        else:
            print(f"测试数据目录不存在: {base_dir}")
        return 0
    
    # 检查是否已存在
    if base_dir.exists():
        print(f"测试数据目录已存在: {base_dir}")
        print("使用 --clean 选项可以重新生成")
        file_count, total_size = count_files_and_size(base_dir)
        print(f"当前状态: {file_count} 个文件, 总大小 {format_size(total_size)}")
        return 0
    
    print("=" * 60)
    print("KAR 测试数据生成器")
    print("=" * 60)
    print(f"输出目录: {base_dir.absolute()}")
    print()
    
    # 设置随机种子以确保可重复性
    random.seed(42)
    
    # 生成各类测试数据
    total_size = 0
    
    total_size += generate_small_files(base_dir, args.small_files)
    total_size += generate_medium_files(base_dir, args.medium_files)
    total_size += generate_large_files(base_dir, args.large_files)
    total_size += generate_deep_structure(base_dir, args.deep_dirs)
    total_size += generate_mixed_content(base_dir)
    
    # 统计结果
    file_count, actual_size = count_files_and_size(base_dir)
    
    print()
    print("=" * 60)
    print("生成完成!")
    print("=" * 60)
    print(f"文件总数: {file_count}")
    print(f"总大小: {format_size(actual_size)}")
    print(f"输出目录: {base_dir.absolute()}")
    print()
    print("现在可以使用以下命令测试:")
    print(f"  ./kar pack {base_dir} large_test.kar")
    print(f"  ./kar list large_test.kar")
    print(f"  ./kar unpack large_test.kar output_dir")
    print()
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
