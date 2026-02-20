/**
 * CRC32 功能测试
 *
 * 测试内容：
 * 1. CRC32 算法正确性（与已知校验值对比）
 * 2. 正常打包/解包流程
 * 3. 文件内容完整性验证
 * 4. CRC 校验失败检测
 *
 * 使用方法
 * # 编译并运行（从项目根目录）
 * clang++ -std=c++17 -Iinclude -o test_crc32 tests/test_crc32.cpp &&
 * ./test_crc32
 *
 * # 或使用 Makefile
 * make test
 */

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../include/crc32.hpp"

namespace fs = std::filesystem;

// ============================================
// 轻量级测试框架
// ============================================

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "  ❌ FAILED: " << msg << " (" << __FILE__ << ":"           \
                << __LINE__ << ")\n";                                          \
      g_tests_failed++;                                                        \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define RUN_TEST(name)                                                         \
  do {                                                                         \
    std::cout << "\n🧪 Running: " << #name << "\n";                            \
    name();                                                                    \
    if (g_tests_failed == 0 || g_tests_passed > 0) {                           \
      std::cout << "  ✅ PASSED\n";                                            \
      g_tests_passed++;                                                        \
    }                                                                          \
  } while (0)

// ============================================
// 测试工具函数
// ============================================

// 创建测试目录和文件
void setup_test_files(const fs::path &test_dir) {
  fs::remove_all(test_dir);
  fs::create_directories(test_dir / "subdir");

  // 创建测试文件 a.txt
  std::ofstream f1(test_dir / "a.txt");
  f1 << "hello"; // 5 bytes
  f1.close();

  // 创建测试文件 subdir/b.txt
  std::ofstream f2(test_dir / "subdir" / "b.txt");
  f2 << "world"; // 5 bytes
  f2.close();
}

// 读取文件内容为字符串
std::string read_file_string(const fs::path &path) {
  std::ifstream f(path, std::ios::binary);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// 读取文件内容为 bytes
std::vector<char> read_file_bytes(const fs::path &path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  auto size = f.tellg();
  f.seekg(0, std::ios::beg);
  std::vector<char> buffer(size);
  f.read(buffer.data(), size);
  return buffer;
}

// 篡改归档文件中的指定字节
void corrupt_archive_at(const fs::path &archive_path, size_t offset,
                        uint8_t new_byte) {
  std::fstream f(archive_path, std::ios::binary | std::ios::in | std::ios::out);
  f.seekp(offset, std::ios::beg);
  f.put(static_cast<char>(new_byte));
}

// ============================================
// 测试用例 1: CRC32 算法正确性
// ============================================

void test_crc32_known_values() {
  CRC32 crc32;

  // 测试用例来自 RFC 3309 和其他标准测试向量
  struct TestCase {
    std::string input;
    uint32_t expected_crc;
  };

  // 已知的 CRC32 测试向量 (IEEE 802.3，与 Python zlib 一致)
  // 验证命令: python3 -c "import zlib; print(hex(zlib.crc32(b'hello') &
  // 0xffffffff))"
  std::vector<TestCase> test_cases = {
      {"", 0x00000000},          // 空字符串
      {"a", 0xE8B7BE43},         // 单字符
      {"abc", 0x352441C2},       // 多字符
      {"hello", 0x3610A686},     // 测试文件内容 (a.txt)
      {"world", 0x3A771143},     // 测试文件内容 (subdir/b.txt)
      {"123456789", 0xCBF43926}, // 标准测试向量
  };

  for (const auto &tc : test_cases) {
    uint32_t calculated = crc32.calculate(tc.input);

    uint32_t expected = tc.expected_crc;

    if (calculated != expected) {
      std::cerr << "  Input: \"" << tc.input << "\"\n";
      std::cerr << "  Expected: 0x" << std::hex << expected << std::dec << "\n";
      std::cerr << "  Got: 0x" << std::hex << calculated << std::dec << "\n";
    }

    TEST_ASSERT(calculated == expected,
                "CRC32 mismatch for input: \"" + tc.input + "\"");
  }

  std::cout << "  ✓ " << test_cases.size() << " test vectors passed\n";
}

// ============================================
// 测试用例 2: 正常打包/解包流程
// ============================================

void test_pack_unpack_normal() {
  const fs::path test_dir = "test_crc_tmp/source";
  const fs::path archive_path = "test_crc_tmp/test.kar";
  const fs::path output_dir = "test_crc_tmp/output";

  // 准备测试文件
  setup_test_files(test_dir);

  // 打包（直接调用系统命令）
  std::string pack_cmd = "./kar pack " + test_dir.string() + " " +
                         archive_path.string() + " > /dev/null 2>&1";
  int pack_result = std::system(pack_cmd.c_str());
  TEST_ASSERT(pack_result == 0, "Pack command failed");
  TEST_ASSERT(fs::exists(archive_path), "Archive file not created");

  // 解包
  fs::create_directories(output_dir);
  std::string unpack_cmd = "./kar unpack " + archive_path.string() + " " +
                           output_dir.string() + " > /dev/null 2>&1";
  int unpack_result = std::system(unpack_cmd.c_str());
  TEST_ASSERT(unpack_result == 0, "Unpack command failed");

  // 验证文件存在
  TEST_ASSERT(fs::exists(output_dir / "a.txt"), "a.txt not extracted");
  TEST_ASSERT(fs::exists(output_dir / "subdir" / "b.txt"),
              "subdir/b.txt not extracted");

  // 清理
  fs::remove_all("test_crc_tmp");
}

// ============================================
// 测试用例 3: 文件内容完整性验证
// ============================================

void test_content_integrity() {
  const fs::path test_dir = "test_crc_tmp/source";
  const fs::path archive_path = "test_crc_tmp/test.kar";
  const fs::path output_dir = "test_crc_tmp/output";

  // 准备测试文件
  setup_test_files(test_dir);

  // 读取原始文件内容
  auto original_a = read_file_string(test_dir / "a.txt");
  auto original_b = read_file_string(test_dir / "subdir" / "b.txt");

  // 打包
  std::string pack_cmd = "./kar pack " + test_dir.string() + " " +
                         archive_path.string() + " > /dev/null 2>&1";
  std::system(pack_cmd.c_str());

  // 解包
  fs::create_directories(output_dir);
  std::string unpack_cmd = "./kar unpack " + archive_path.string() + " " +
                           output_dir.string() + " > /dev/null 2>&1";
  std::system(unpack_cmd.c_str());

  // 读取解压后的文件内容
  auto extracted_a = read_file_string(output_dir / "a.txt");
  auto extracted_b = read_file_string(output_dir / "subdir" / "b.txt");

  // 验证内容一致
  TEST_ASSERT(original_a == extracted_a,
              "Content mismatch for a.txt: original=\"" + original_a +
                  "\", extracted=\"" + extracted_a + "\"");
  TEST_ASSERT(original_b == extracted_b, "Content mismatch for subdir/b.txt");

  // 计算 CRC 验证
  CRC32 crc32;
  auto original_crc_a = crc32.calculate(original_a);
  auto extracted_crc_a = crc32.calculate(extracted_a);
  TEST_ASSERT(original_crc_a == extracted_crc_a, "CRC mismatch for a.txt");

  std::cout << "  ✓ File contents and CRC32 verified\n";

  // 清理
  fs::remove_all("test_crc_tmp");
}

// ============================================
// 测试用例 4: CRC 校验失败检测
// ============================================

void test_crc_mismatch_detection() {
  const fs::path test_dir = "test_crc_tmp/source";
  const fs::path archive_path = "test_crc_tmp/test.kar";
  const fs::path output_dir = "test_crc_tmp/output";

  // 准备测试文件
  setup_test_files(test_dir);

  // 打包
  std::string pack_cmd = "./kar pack " + test_dir.string() + " " +
                         archive_path.string() + " > /dev/null 2>&1";
  std::system(pack_cmd.c_str());

  // 查找 "hello" 在归档中的位置并篡改
  auto archive_data = read_file_bytes(archive_path);
  size_t hello_pos = 0;
  bool found = false;

  for (size_t i = 0; i < archive_data.size() - 4; ++i) {
    if (archive_data[i] == 'h' && archive_data[i + 1] == 'e' &&
        archive_data[i + 2] == 'l' && archive_data[i + 3] == 'l' &&
        archive_data[i + 4] == 'o') {
      hello_pos = i + 4; // 指向 'o' 的位置
      found = true;
      break;
    }
  }

  TEST_ASSERT(found, "Could not find 'hello' in archive to corrupt");

  // 篡改 'o' -> 'x'
  corrupt_archive_at(archive_path, hello_pos, 'x');
  std::cout << "  ✓ Corrupted byte at offset " << hello_pos
            << " (changed 'o' to 'x')\n";

  // 尝试解包，应该失败
  fs::create_directories(output_dir);
  std::string unpack_cmd = "./kar unpack " + archive_path.string() + " " +
                           output_dir.string() + " 2>&1";

  // 捕获命令输出
  FILE *pipe = popen(unpack_cmd.c_str(), "r");
  TEST_ASSERT(pipe != nullptr, "Failed to run unpack command");

  char buffer[256];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  int status = pclose(pipe);
  int exit_code = WEXITSTATUS(status);

  // 验证解包失败（非零退出码）
  TEST_ASSERT(
      exit_code != 0,
      "Expected unpack to fail with corrupted archive, but it succeeded");

  // 验证错误信息包含 CRC mismatch
  TEST_ASSERT(output.find("CRC32 mismatch") != std::string::npos,
              "Error message does not contain 'CRC32 mismatch'. Output: " +
                  output);

  std::cout << "  ✓ CRC mismatch correctly detected\n";
  std::cout << "  ✓ Error message: \""
            << output.substr(output.find("CRC32"), 40) << "...\"\n";

  // 清理
  fs::remove_all("test_crc_tmp");
}

// ============================================
// 测试用例 5: 空文件测试
// ============================================

void test_empty_file() {
  const fs::path test_dir = "test_crc_tmp/source";
  const fs::path archive_path = "test_crc_tmp/test.kar";
  const fs::path output_dir = "test_crc_tmp/output";

  // 准备测试文件（包含空文件）
  fs::remove_all(test_dir);
  fs::create_directories(test_dir);

  std::ofstream empty_file(test_dir / "empty.txt");
  empty_file.close(); // 创建空文件

  std::ofstream non_empty(test_dir / "data.txt");
  non_empty << "content";
  non_empty.close();

  // 打包
  std::string pack_cmd = "./kar pack " + test_dir.string() + " " +
                         archive_path.string() + " > /dev/null 2>&1";
  std::system(pack_cmd.c_str());

  // 解包
  fs::create_directories(output_dir);
  std::string unpack_cmd = "./kar unpack " + archive_path.string() + " " +
                           output_dir.string() + " > /dev/null 2>&1";
  int result = std::system(unpack_cmd.c_str());

  TEST_ASSERT(result == 0, "Unpack with empty file failed");
  TEST_ASSERT(fs::exists(output_dir / "empty.txt"), "Empty file not extracted");
  TEST_ASSERT(fs::file_size(output_dir / "empty.txt") == 0,
              "Empty file has non-zero size");

  std::cout << "  ✓ Empty file handled correctly\n";

  // 清理
  fs::remove_all("test_crc_tmp");
}

// ============================================
// 主函数
// ============================================

int main() {
  std::cout << "========================================\n";
  std::cout << "  CRC32 功能测试套件\n";
  std::cout << "========================================\n";

  // 确保 kar 可执行文件存在
  if (!fs::exists("./kar")) {
    std::cerr << "❌ Error: ./kar not found. Please compile first.\n";
    return 1;
  }

  // 运行所有测试
  RUN_TEST(test_crc32_known_values);
  RUN_TEST(test_pack_unpack_normal);
  RUN_TEST(test_content_integrity);
  RUN_TEST(test_crc_mismatch_detection);
  RUN_TEST(test_empty_file);

  // 输出总结
  std::cout << "\n========================================\n";
  std::cout << "  测试结果\n";
  std::cout << "========================================\n";
  std::cout << "  ✅ Passed: " << g_tests_passed << "\n";
  std::cout << "  ❌ Failed: " << g_tests_failed << "\n";
  std::cout << "========================================\n";

  return g_tests_failed > 0 ? 1 : 0;
}
