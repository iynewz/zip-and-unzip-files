#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ============================================
// 核心 Archive 类
// ============================================
class Archiver {
public:
  // 打包文件夹到 archive 文件
  void pack(const fs::path &source_dir, const fs::path &archive_path);

  // 解包 archive 到指定目录
  void unpack(const fs::path &archive_path, const fs::path &target_dir);

  // 查看 archive 内容（不解压）
  void list(const fs::path &archive_path);

private:
  void write_entry(std::ofstream &archive, const fs::path &base_dir,
                   const fs::path &file_path);

  void read_entry(std::ifstream &archive, const fs::path &target_dir);
};
