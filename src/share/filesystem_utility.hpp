#pragma once

#include "constants.hpp"
#include <filesystem>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace krbn::filesystem_utility {
inline void mkdir_tmp_directory() {
  // /Library/Application Support/org.pqrs/tmp
  auto directory_path = constants::get_tmp_directory();

  std::error_code error_code;
  std::filesystem::create_directories(directory_path,
                                      error_code);

  // Reset owner and permission just in case.
  chown(directory_path.c_str(),
        0,
        0);
  chmod(directory_path.c_str(),
        0755);
}

inline void mkdir_rootonly_directory() {
  mkdir_tmp_directory();

  // /Library/Application Support/org.pqrs/tmp/rootonly
  auto directory_path = constants::get_rootonly_directory();

  std::error_code error_code;
  std::filesystem::create_directories(directory_path,
                                      error_code);

  // Reset owner and permission just in case.
  chown(directory_path.c_str(),
        0,
        0);
  chmod(directory_path.c_str(),
        0700);
}

inline void mkdir_system_user_directory(uid_t uid) {
  mkdir_tmp_directory();

  {
    // /Library/Application Support/org.pqrs/tmp/user
    auto directory_path = constants::get_system_user_directory();

    std::error_code error_code;
    std::filesystem::create_directories(directory_path,
                                        error_code);

    chown(directory_path.c_str(),
          0,
          0);
    chmod(directory_path.c_str(),
          0755);
  }

  {
    // /Library/Application Support/org.pqrs/tmp/user/<uid>
    auto directory_path = constants::get_system_user_directory(uid);

    std::error_code error_code;
    std::filesystem::create_directories(directory_path,
                                        error_code);

    chown(directory_path.c_str(),
          uid,
          0);
    chmod(directory_path.c_str(),
          0700);
  }
}

inline void mkdir_user_directories() {
  std::error_code error_code;

  {
    // ~/.config/karabiner
    auto directory_path = constants::get_user_configuration_directory();

    std::filesystem::create_directories(directory_path,
                                        error_code);
    if (std::filesystem::is_directory(directory_path,
                                      error_code)) {
      chmod(directory_path.c_str(),
            0700);
    }
  }

  {
    // ~/.local/share/karabiner
    auto directory_path = constants::get_user_data_directory();

    std::filesystem::create_directories(directory_path,
                                        error_code);
    if (std::filesystem::is_directory(directory_path,
                                      error_code)) {
      chmod(directory_path.c_str(),
            0700);
    }
  }

  {
    // ~/.config/karabiner/assets/complex_modifications
    auto directory_path = constants::get_user_complex_modifications_assets_directory();

    std::filesystem::create_directories(directory_path,
                                        error_code);
    if (std::filesystem::is_directory(directory_path,
                                      error_code)) {
      chmod(directory_path.c_str(),
            0700);
    }
  }

  {
    // ~/.local/share/karabiner/log
    auto directory_path = constants::get_user_log_directory();

    std::filesystem::create_directories(directory_path,
                                        error_code);
    if (std::filesystem::is_directory(directory_path,
                                      error_code)) {
      chmod(directory_path.c_str(),
            0700);
    }
  }

  {
    // ~/.local/share/karabiner/tmp
    auto directory_path = constants::get_user_tmp_directory();

    std::filesystem::create_directories(directory_path,
                                        error_code);
    if (std::filesystem::is_directory(directory_path,
                                      error_code)) {
      chmod(directory_path.c_str(),
            0700);
    }
  }
}

inline void create_base_directories(std::optional<uid_t> uid) {
  filesystem_utility::mkdir_tmp_directory();
  filesystem_utility::mkdir_rootonly_directory();
  if (uid) {
    filesystem_utility::mkdir_system_user_directory(*uid);
  }
}

[[nodiscard]] inline std::filesystem::path find_socket_file_path(const std::filesystem::path& directory) {
  auto paths = std::vector<std::filesystem::path>();

  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
    if (ec) {
      break;
    }

    if (entry.path().extension() == ".sock") {
      paths.push_back(entry.path());
    }
  }

  std::ranges::sort(paths);

  if (!paths.empty()) {
    return paths.back();
  }

  return directory / "not_found.sock";
}

[[nodiscard]] inline std::optional<std::string> read_file(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (input) {
    std::string code;

    input.seekg(0, std::ios::end);
    code.reserve(input.tellg());
    input.seekg(0, std::ios::beg);

    code.assign(std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>());

    return code;
  }

  return std::nullopt;
}
} // namespace krbn::filesystem_utility
