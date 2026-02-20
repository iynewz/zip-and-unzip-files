#ifndef CRC32_HPP
#define CRC32_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

/**
 * CRC32 校验和计算类
 * 
 * 使用 IEEE 802.3 标准多项式: 0xEDB88320
 * 查表法实现，时间复杂度 O(n)
 */
class CRC32 {
public:
    // 构造函数：初始化查找表
    CRC32() {
        init_table();
    }

    /**
     * 计算数据的 CRC32 校验和
     * @param data 数据指针
     * @param len 数据长度（字节）
     * @return CRC32 校验和
     */
    uint32_t calculate(const uint8_t* data, size_t len) const {
        uint32_t crc = 0xFFFFFFFF;  // 初始值
        
        for (size_t i = 0; i < len; ++i) {
            // 核心算法：查表更新 CRC
            uint8_t table_index = (crc ^ data[i]) & 0xFF;
            crc = (crc >> 8) ^ table_[table_index];
        }
        
        return crc ^ 0xFFFFFFFF;  // 最终结果取反
    }

    /**
     * 计算 vector<char> 的 CRC32
     */
    uint32_t calculate(const std::vector<char>& data) const {
        return calculate(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    /**
     * 计算 string 的 CRC32
     */
    uint32_t calculate(const std::string& data) const {
        return calculate(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

private:
    // CRC32 查找表（256 项）
    uint32_t table_[256];

    /**
     * 初始化 CRC32 查找表
     * 
     * 原理：对 0-255 的每个字节，预计算其对应的 CRC32 值
     * 多项式：0xEDB88320 (IEEE 802.3 标准)
     */
    void init_table() {
        const uint32_t POLYNOMIAL = 0xEDB88320;
        
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            // 对每个字节，执行 8 次位运算（模拟除法）
            for (int j = 0; j < 8; ++j) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ POLYNOMIAL;
                } else {
                    crc >>= 1;
                }
            }
            table_[i] = crc;
        }
    }
};

#endif // CRC32_HPP
