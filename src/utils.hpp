#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>

// ============================================
// 工具函数
// ============================================

inline uint64_t current_timestamp() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

inline std::string format_size(uint64_t bytes) {
  const char *units[] = {"B", "KB", "MB", "GB"};
  int unit_idx = 0;
  double size = static_cast<double>(bytes);
  while (size >= 1024 && unit_idx < 3) {
    size /= 1024;
    unit_idx++;
  }
  char buf[32];
  snprintf(buf, sizeof(buf), "%.2f %s", size, units[unit_idx]);
  return std::string(buf);
}
