#include "archiver.hpp"
#include "format.hpp"
#include "utils.hpp"

#include "../include/crc32.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>

// ============================================
// 公开接口实现
// ============================================

void Archiver::pack(const fs::path &source_dir, const fs::path &archive_path) {
  if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
    throw std::runtime_error("Source directory does not exist");
  }

  std::ofstream archive(archive_path, std::ios::binary);
  if (!archive) {
    throw std::runtime_error("Cannot create archive file");
  }

  // 收集所有文件路径
  std::vector<fs::path> files;
  for (const auto &entry : fs::recursive_directory_iterator(source_dir)) {
    if (entry.is_regular_file()) {
      files.push_back(entry.path());
    }
  }

  // 写入全局 Header
  FileHeader global_header{.magic = KAR_MAGIC,
                           .version = 1,
                           .entry_count = static_cast<uint32_t>(files.size()),
                           .created_at = current_timestamp(),
                           .reserved = 0};
  archive.write(reinterpret_cast<const char *>(&global_header),
                sizeof(global_header));

  // 逐个写入文件，显示进度条
  const size_t total_files = files.size();
  for (size_t i = 0; i < total_files; ++i) {
    const auto &file_path = files[i];
    write_entry(archive, source_dir, file_path);

    // 计算进度
    int progress = static_cast<int>((i + 1) * 100 / total_files);
    int bar_width = 30;
    int pos = static_cast<int>(bar_width * (i + 1) / total_files);

    // 构建进度条
    std::cout << "\r[";
    for (int j = 0; j < bar_width; ++j) {
      if (j < pos)
        std::cout << "█";
      else
        std::cout << "░";
    }
    std::cout << "] " << std::setw(3) << progress << "% ";
    std::cout << "(" << (i + 1) << "/" << total_files << ") ";
    std::cout << fs::relative(file_path, source_dir).string();
    std::cout.flush();
  }

  std::cout << "\n\nArchive created: " << archive_path << " (" << total_files
            << " files)\n";
}

void Archiver::unpack(const fs::path &archive_path,
                      const fs::path &target_dir) {
  std::ifstream archive(archive_path, std::ios::binary);
  if (!archive) {
    throw std::runtime_error("Cannot open archive file");
  }

  // 读取并验证全局 Header
  FileHeader global_header;
  archive.read(reinterpret_cast<char *>(&global_header), sizeof(global_header));

  if (global_header.magic != KAR_MAGIC) {
    throw std::runtime_error("Invalid archive format (wrong magic number)");
  }

  std::cout << "Archive version: " << global_header.version << "\n";
  std::cout << "Total entries: " << global_header.entry_count << "\n\n";

  const uint32_t total_entries = global_header.entry_count;

  // 逐个读取文件，显示进度条
  for (uint32_t i = 0; i < total_entries; ++i) {
    // 先读取 entry header 获取文件名
    EntryHeader entry;
    archive.read(reinterpret_cast<char *>(&entry), sizeof(entry));

    std::string rel_path(entry.path_length, '\0');
    archive.read(rel_path.data(), entry.path_length);

    // 计算并显示进度
    int progress = static_cast<int>((i + 1) * 100 / total_entries);
    int bar_width = 30;
    int pos = static_cast<int>(bar_width * (i + 1) / total_entries);

    std::cout << "\r[";
    for (int j = 0; j < bar_width; ++j) {
      if (j < pos)
        std::cout << "█";
      else
        std::cout << "░";
    }
    std::cout << "] " << std::setw(3) << progress << "% ";
    std::cout << "(" << (i + 1) << "/" << total_entries << ") ";
    std::cout << rel_path;
    std::cout.flush();

    // 读取内容并解压
    std::vector<char> buffer(entry.content_size);
    archive.read(buffer.data(), entry.content_size);

    // 验证 CRC32 校验和
    CRC32 crc32;
    uint32_t calculated_crc = crc32.calculate(buffer);
    if (calculated_crc != entry.checksum) {
      throw std::runtime_error("CRC32 mismatch for file: " + rel_path +
                               " (expected: " + std::to_string(entry.checksum) +
                               ", got: " + std::to_string(calculated_crc) + ")");
    }

    // 创建目标路径并写入文件
    fs::path out_path = target_dir / rel_path;
    fs::create_directories(out_path.parent_path());

    std::ofstream out_file(out_path, std::ios::binary);
    out_file.write(buffer.data(), entry.content_size);

    // 恢复权限
    fs::permissions(out_path, static_cast<fs::perms>(entry.permissions));
  }

  std::cout << "\n\nExtracted to: " << target_dir << "\n";
}

void Archiver::list(const fs::path &archive_path) {
  std::ifstream archive(archive_path, std::ios::binary);
  FileHeader global_header;
  archive.read(reinterpret_cast<char *>(&global_header), sizeof(global_header));

  std::cout << "Archive: " << archive_path << "\n";
  std::cout << "Entries: " << global_header.entry_count << "\n";
  std::cout << "------------------------\n";

  for (uint32_t i = 0; i < global_header.entry_count; ++i) {
    EntryHeader entry;
    archive.read(reinterpret_cast<char *>(&entry), sizeof(entry));

    std::string rel_path(entry.path_length, '\0');
    archive.read(rel_path.data(), entry.path_length);

    // 跳过内容
    archive.seekg(entry.content_size, std::ios::cur);

    std::cout << rel_path << " (" << format_size(entry.content_size) << ")\n";
  }
}

// ============================================
// 私有方法实现
// ============================================

void Archiver::write_entry(std::ofstream &archive, const fs::path &base_dir,
                           const fs::path &file_path) {
  // 计算相对路径（关键：保持目录结构）
  std::string rel_path = fs::relative(file_path, base_dir).string();

  // 读取文件内容
  std::ifstream file(file_path, std::ios::binary | std::ios::ate);
  auto size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> buffer(static_cast<size_t>(size));
  file.read(buffer.data(), size);

  // 计算 CRC32 校验和
  CRC32 crc32;
  uint32_t checksum = crc32.calculate(buffer);

  // 填充 Entry Header
  EntryHeader entry{.path_length = static_cast<uint32_t>(rel_path.size()),
                    .content_size = static_cast<uint64_t>(size),
                    .modified_time =
                        current_timestamp(), // 简化处理，实际应取文件 mtime
                    .checksum = checksum,    // CRC32 校验和
                    .permissions = 0644};

  // 写入：Header -> 路径 -> 内容
  archive.write(reinterpret_cast<const char *>(&entry), sizeof(entry));
  archive.write(rel_path.data(), rel_path.size());
  archive.write(buffer.data(), size);
}

void Archiver::read_entry(std::ifstream &archive, const fs::path &target_dir) {
  EntryHeader entry;
  archive.read(reinterpret_cast<char *>(&entry), sizeof(entry));

  // 读取相对路径
  std::string rel_path(entry.path_length, '\0');
  archive.read(rel_path.data(), entry.path_length);

  // 读取文件内容
  std::vector<char> buffer(entry.content_size);
  archive.read(buffer.data(), entry.content_size);

  // 验证 CRC32 校验和
  CRC32 crc32;
  uint32_t calculated_crc = crc32.calculate(buffer);
  if (calculated_crc != entry.checksum) {
    throw std::runtime_error("CRC32 mismatch for file: " + rel_path +
                             " (expected: " + std::to_string(entry.checksum) +
                             ", got: " + std::to_string(calculated_crc) + ")");
  }

  // 创建目标路径（自动创建父目录）
  fs::path out_path = target_dir / rel_path;
  fs::create_directories(out_path.parent_path());

  // 写入文件
  std::ofstream out_file(out_path, std::ios::binary);
  out_file.write(buffer.data(), entry.content_size);

  // 恢复权限（简化版）
  fs::permissions(out_path, static_cast<fs::perms>(entry.permissions));

  std::cout << "Extracted: " << rel_path << " (CRC32 OK)\n";
}
