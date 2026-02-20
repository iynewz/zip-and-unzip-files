#include "archiver.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

// ============================================
// CLI 接口
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
      if (argc < 4) {
        print_usage(argv[0]);
        return 1;
      }
      ar.pack(argv[2], argv[3]);
    } else if (cmd == "unpack") {
      if (argc < 4) {
        print_usage(argv[0]);
        return 1;
      }
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
