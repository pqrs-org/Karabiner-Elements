#pragma once

#include "chrono_utility.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include <glob/glob.hpp>
#include <pqrs/filesystem.hpp>
#include <string_view>

namespace krbn {
namespace filesystem_utility {
inline void mkdir_tmp_directory(void) {
  pqrs::filesystem::create_directory_with_intermediate_directories(
      constants::get_tmp_directory(),
      0755);

  // Reset owner and permission just in case.

  chown(constants::get_tmp_directory().c_str(), 0, 0);
  chmod(constants::get_tmp_directory().c_str(), 0755);
}

inline void mkdir_rootonly_directory(void) {
  mkdir_tmp_directory();

  pqrs::filesystem::create_directory_with_intermediate_directories(
      constants::get_rootonly_directory(),
      0700);

  // Reset owner and permission just in case.

  chown(constants::get_rootonly_directory().c_str(), 0, 0);
  chmod(constants::get_rootonly_directory().c_str(), 0700);
}

inline void mkdir_system_user_directory(uid_t uid) {
  mkdir_tmp_directory();

  {
    auto directory_path = constants::get_system_user_directory();

    pqrs::filesystem::create_directory_with_intermediate_directories(
        directory_path.string(),
        0755);

    chown(directory_path.c_str(), 0, 0);
    chmod(directory_path.c_str(), 0755);
  }

  {
    auto directory_path = constants::get_system_user_directory(uid);

    pqrs::filesystem::create_directory_with_intermediate_directories(
        directory_path.string(),
        0700);

    chown(directory_path.c_str(), uid, 0);
    chmod(directory_path.c_str(), 0700);
  }
}

inline void remove_files_by_glob(const std::string_view& pattern) {
  for (auto path : glob::glob(pattern.data())) {
    logger::get_logger()->info("Remove {0}", path.c_str());

    std::error_code error_code;
    std::filesystem::remove(path, error_code);
  }
}

inline std::filesystem::path make_socket_file_basename(void) {
  std::stringstream ss;
  ss << std::hex
     << chrono_utility::nanoseconds_since_epoch()
     << ".sock";

  return ss.str();
}

inline std::filesystem::path find_socket_file_path(const std::filesystem::path& directory) {
  auto pattern = (directory / "*.sock").string();
  auto paths = glob::glob(pattern);
  std::sort(std::begin(paths), std::end(paths));

  if (!paths.empty()) {
    return paths.back();
  }

  return directory / "not_found.sock";
}
} // namespace filesystem_utility
} // namespace krbn
