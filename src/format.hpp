#pragma once

#include <cstdint>

// ============================================
// 文件格式协议定义（关键！）
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

constexpr uint32_t KAR_MAGIC = 0x5241414B; // 'KAAR' in little-endian
