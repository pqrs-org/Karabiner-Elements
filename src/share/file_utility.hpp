#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace krbn {
class file_utility final {
public:
  static std::shared_ptr<std::vector<uint8_t>> read_file(const std::string& path) {
    std::ifstream ifstream(path);
    if (ifstream) {
      ifstream.seekg(0, std::fstream::end);
      auto size = ifstream.tellg();
      ifstream.seekg(0, std::fstream::beg);

      auto buffer = std::make_shared<std::vector<uint8_t>>(size);
      ifstream.read(reinterpret_cast<char*>(&((*buffer)[0])), size);

      return buffer;
    }
    return nullptr;
  }
};
} // namespace krbn
