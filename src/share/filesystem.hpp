#pragma once

#include "boost_defs.hpp"

#include <array>
#include <boost/optional.hpp>
#include <climits>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace krbn {
class filesystem final {
public:
  static bool exists(const std::string& path) {
    struct stat s;
    return (stat(path.c_str(), &s) == 0);
  }

  static bool create_directory_with_intermediate_directories(const std::string& path, mode_t mode) {
    std::vector<std::string> parents;
    auto directory = path;
    while (!exists(directory)) {
      parents.push_back(directory);
      directory = dirname(directory);
    }

    for (auto it = std::rbegin(parents); it != std::rend(parents); std::advance(it, 1)) {
      if (mkdir(it->c_str(), mode) != 0) {
        return false;
      }
    }

    if (!is_directory(path)) {
      return false;
    }

    chmod(path.c_str(), mode);

    return true;
  }

  static void copy(const std::string& from_file_path,
                   const std::string& to_file_path) {
    std::ifstream ifstream(from_file_path);
    if (!ifstream) {
      return;
    }
    std::ofstream ofstream(to_file_path);
    if (!ofstream) {
      return;
    }
    ofstream << ifstream.rdbuf();
  }

  static boost::optional<mode_t> file_access_permissions(const std::string& path) {
    struct stat s;
    if (stat(path.c_str(), &s) != 0) {
      return boost::none;
    }
    return s.st_mode & ACCESSPERMS;
  }

  static boost::optional<off_t> file_size(const std::string& path) {
    struct stat s;
    if (stat(path.c_str(), &s) != 0) {
      return boost::none;
    }
    return s.st_size;
  }

  static bool is_directory(const std::string& path) {
    struct stat s;
    if (stat(path.c_str(), &s) == 0) {
      return S_ISDIR(s.st_mode);
    }
    return false;
  }

  static bool is_owned(const std::string& path, uid_t uid) {
    struct stat s;
    if (stat(path.c_str(), &s) == 0) {
      return s.st_uid == uid;
    }
    return false;
  }

  static std::string dirname(const std::string& path) {
    size_t pos = get_dirname_position(path);
    if (pos == 0) {
      return ".";
    }
    return path.substr(0, pos);
  }

  static void normalize_file_path(std::string& path) {
    if (path.empty()) {
      path += '.';
      return;
    }

    size_t end = path.size();
    size_t dest = 1;
    for (size_t src = 1; src < end; ++src) {
      // Skip multiple slashes.
      if (path[dest - 1] == '/' && path[src] == '/') {
        continue;
      }

      // Handling . and ..
      if (path[src] == '/') {
        dest = process_dot(path, dest);
        dest = process_dotdot(path, dest);

        if (dest == 0) {
          if (path[0] != '/') {
            continue;
          }
        } else {
          if (path[dest - 1] == '/') {
            continue;
          }
        }
      }

      if (dest != src) {
        path[dest] = path[src];
      }

      ++dest;
    }

    dest = process_dot(path, dest);
    dest = process_dotdot(path, dest);

    if (dest == 0) {
      path[0] = '.';
      dest = 1;
    }

    path.resize(dest);
  }

  static boost::optional<std::string> realpath(const std::string& path) {
    std::array<char, PATH_MAX> resolved_path;
    if (!::realpath(path.c_str(), &(resolved_path[0]))) {
      return boost::none;
    }
    return std::string(&(resolved_path[0]));
  }

private:
  static size_t get_dirname_position(const std::string& path, size_t pos = std::string::npos) {
    if (path.empty()) return 0;

    if (pos == std::string::npos) {
      pos = path.size() - 1;
    }

    if (path.size() <= pos) return 0;

    if (pos == 0) {
      // We retain the first slash for dirname("/") == "/".
      if (path[pos] == '/') {
        return 1;
      } else {
        return 0;
      }
    }

    if (path[pos] == '/') {
      --pos;
    }

    size_t i = path.rfind('/', pos);
    if (i == std::string::npos) {
      return 0;
    }
    if (i == 0) {
      // path starts with "/".
      return 1;
    }
    return i;
  }

  static size_t process_dot(const std::string& path, size_t pos) {
    if (path.empty()) return pos;

    // foo/bar/./
    //          ^
    //         pos
    //
    if (pos > 2 &&
        path[pos - 2] == '/' &&
        path[pos - 1] == '.') {
      return pos - 2;
    }

    // ./foo/bar
    //  ^
    //  pos
    //
    if (pos == 1 &&
        path[0] == '.') {
      return 0;
    }

    return pos;
  }

  static size_t process_dotdot(const std::string& path, size_t pos) {
    // Ignore ../../
    if (pos > 4 &&
        path[pos - 5] == '.' &&
        path[pos - 4] == '.' &&
        path[pos - 3] == '/' &&
        path[pos - 2] == '.' &&
        path[pos - 1] == '.') {
      return pos;
    }

    // foo/bar/../
    //           ^
    //          pos
    //
    if (pos > 2 &&
        path[pos - 3] == '/' &&
        path[pos - 2] == '.' &&
        path[pos - 1] == '.') {
      pos = get_dirname_position(path, pos - 3);

      // foo/bar/../
      //    ^
      //   pos

      return pos;
    }

    return pos;
  }
};
} // namespace krbn
