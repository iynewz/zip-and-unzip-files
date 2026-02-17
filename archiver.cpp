#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ============================================
// 1. 文件格式协议设计（关键！）
// ============================================
#pragma pack(push, 1) // 确保内存对齐，保证跨平台一致性

struct FileHeader {
  uint32_t magic;       // 魔数 'KAAR' (Kimi Archive)
  uint16_t version;     // 版本号，目前为 1
  uint32_t entry_count; // 文件条目数量
  uint64_t created_at;  // 创建时间戳
  uint32_t reserved;    // 预留字段
};

struct EntryHeader {
  uint32_t path_length;   // 文件路径长度（支持长路径）
  uint64_t content_size;  // 文件内容大小（支持大文件）
  uint64_t modified_time; // 修改时间
  uint32_t checksum;      // CRC32 校验和（预留，目前可填0）
  uint16_t permissions;   // 文件权限（Unix style）
};

#pragma pack(pop)

const uint32_t MAGIC = 0x5241414B; // 'KAAR' in little-endian

// ============================================
// 2. 核心 Archive 类
// ============================================
class Archiver {
public:
  // 打包文件夹到 archive 文件
  void pack(const fs::path &source_dir, const fs::path &archive_path) {
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
    FileHeader global_header{.magic = MAGIC,
                             .version = 1,
                             .entry_count = static_cast<uint32_t>(files.size()),
                             .created_at = current_timestamp(),
                             .reserved = 0};
    archive.write(reinterpret_cast<const char *>(&global_header),
                  sizeof(global_header));

    // 逐个写入文件
    for (const auto &file_path : files) {
      write_entry(archive, source_dir, file_path);
      std::cout << "Added: " << fs::relative(file_path, source_dir) << "\n";
    }

    std::cout << "\nArchive created: " << archive_path << " (" << files.size()
              << " files)\n";
  }

  // 解包 archive 到指定目录
  void unpack(const fs::path &archive_path, const fs::path &target_dir) {
    std::ifstream archive(archive_path, std::ios::binary);
    if (!archive) {
      throw std::runtime_error("Cannot open archive file");
    }

    // 读取并验证全局 Header
    FileHeader global_header;
    archive.read(reinterpret_cast<char *>(&global_header),
                 sizeof(global_header));

    if (global_header.magic != MAGIC) {
      throw std::runtime_error("Invalid archive format (wrong magic number)");
    }

    std::cout << "Archive version: " << global_header.version << "\n";
    std::cout << "Total entries: " << global_header.entry_count << "\n\n";

    // 逐个读取文件
    for (uint32_t i = 0; i < global_header.entry_count; ++i) {
      read_entry(archive, target_dir);
    }

    std::cout << "\nExtracted to: " << target_dir << "\n";
  }

  // 查看 archive 内容（不解压）
  void list(const fs::path &archive_path) {
    std::ifstream archive(archive_path, std::ios::binary);
    FileHeader global_header;
    archive.read(reinterpret_cast<char *>(&global_header),
                 sizeof(global_header));

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

private:
  void write_entry(std::ofstream &archive, const fs::path &base_dir,
                   const fs::path &file_path) {
    // 计算相对路径（关键：保持目录结构）
    std::string rel_path = fs::relative(file_path, base_dir).string();
    // 读取文件内容
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);

    // 填充 Entry Header
    EntryHeader entry{.path_length = static_cast<uint32_t>(rel_path.size()),
                      .content_size = static_cast<uint64_t>(size),
                      .modified_time =
                          current_timestamp(), // 简化处理，实际应取文件 mtime
                      .checksum = 0,           // TODO: 计算 CRC32
                      .permissions = 0644};

    // 写入：Header -> 路径 -> 内容
    archive.write(reinterpret_cast<const char *>(&entry), sizeof(entry));
    archive.write(rel_path.data(), rel_path.size());
    archive.write(buffer.data(), size);
  }

  void read_entry(std::ifstream &archive, const fs::path &target_dir) {
    EntryHeader entry;
    archive.read(reinterpret_cast<char *>(&entry), sizeof(entry));

    // 读取相对路径
    std::string rel_path(entry.path_length, '\0');
    archive.read(rel_path.data(), entry.path_length);

    // 读取文件内容
    std::vector<char> buffer(entry.content_size);
    archive.read(buffer.data(), entry.content_size);

    // 创建目标路径（自动创建父目录）
    fs::path out_path = target_dir / rel_path;
    fs::create_directories(out_path.parent_path());

    // 写入文件
    std::ofstream out_file(out_path, std::ios::binary);
    out_file.write(buffer.data(), entry.content_size);

    // 恢复权限（简化版）
    fs::permissions(out_path, static_cast<fs::perms>(entry.permissions));

    std::cout << "Extracted: " << rel_path << "\n";
  }

  uint64_t current_timestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  std::string format_size(uint64_t bytes) {
    const char *units[] = {"B", "KB", "MB", "GB"};
    int unit_idx = 0;
    double size = bytes;
    while (size >= 1024 && unit_idx < 3) {
      size /= 1024;
      unit_idx++;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f %s", size, units[unit_idx]);
    return std::string(buf);
  }
};

// ============================================
// 3. CLI 接口
// ============================================
void print_usage(const char *prog) {
  std::cout << "Usage:\n"
            << "  " << prog << " pack <source_dir> <archive.kar>\n"
            << "  " << prog << " unpack <archive.kar> <target_dir>\n"
            << "  " << prog << " list <archive.kar>\n";
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  try {
    Archiver ar;
    std::string cmd = argv[1];

    if (cmd == "pack") {
      ar.pack(argv[2], argv[3]);
    } else if (cmd == "unpack") {
      ar.unpack(argv[2], argv[3]);
    } else if (cmd == "list" && argc == 3) {
      ar.list(argv[2]);
    } else {
      print_usage(argv[0]);
      return 1;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}