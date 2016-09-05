#pragma once

#include <string>
#include <sys/stat.h>

class filesystem final {
public:
  static bool is_directory(const std::string& path) {
    struct stat s;
    if (stat(path.c_str(), &s) == 0) {
      return S_ISDIR(s.st_mode);
    }
    return false;
  }

  static std::string realpath(const std::string& path) {
    std::string result;
    if (auto p = ::realpath(path.c_str(), nullptr)) {
      result = p;
      free(p);
    }
    return result;
  }
};
