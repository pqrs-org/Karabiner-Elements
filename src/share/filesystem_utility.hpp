#pragma once

#include "constants.hpp"
#include "logger.hpp"
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>
#include <system_error>
#include <unistd.h>

namespace krbn::filesystem_utility {

inline constexpr auto permissions_0600 =
    std::filesystem::perms::owner_read |
    std::filesystem::perms::owner_write;
inline constexpr auto permissions_0644 =
    std::filesystem::perms::owner_read |
    std::filesystem::perms::owner_write |
    std::filesystem::perms::group_read |
    std::filesystem::perms::others_read;
inline constexpr auto permissions_0666 =
    std::filesystem::perms::owner_read |
    std::filesystem::perms::owner_write |
    std::filesystem::perms::group_read |
    std::filesystem::perms::group_write |
    std::filesystem::perms::others_read |
    std::filesystem::perms::others_write;
inline constexpr auto permissions_0700 =
    std::filesystem::perms::owner_all;
inline constexpr auto permissions_0755 =
    std::filesystem::perms::owner_all |
    std::filesystem::perms::group_read |
    std::filesystem::perms::group_exec |
    std::filesystem::perms::others_read |
    std::filesystem::perms::others_exec;

//
// std::filesystem wrappers
//

inline bool create_directories(const std::filesystem::path& directory_path) {
  std::error_code error_code;
  std::filesystem::create_directories(directory_path,
                                      error_code);
  if (error_code) {
    logger::get_logger()->error("create_directories failed: {0}: {1}",
                                directory_path.string(),
                                error_code.message());
    return false;
  }

  return true;
}

inline bool is_directory(const std::filesystem::path& directory_path) {
  std::error_code error_code;
  auto result = std::filesystem::is_directory(directory_path,
                                              error_code);
  if (error_code) {
    logger::get_logger()->error("is_directory failed: {0}: {1}",
                                directory_path.string(),
                                error_code.message());
    return false;
  }

  return result;
}

inline bool exists(const std::filesystem::path& path) {
  std::error_code error_code;
  auto result = std::filesystem::exists(path,
                                        error_code);
  if (error_code) {
    logger::get_logger()->error("exists failed: {0}: {1}",
                                path.string(),
                                error_code.message());
    return false;
  }

  return result;
}

inline std::optional<std::filesystem::file_status> status(const std::filesystem::path& path) {
  std::error_code error_code;
  auto result = std::filesystem::status(path,
                                        error_code);
  if (error_code) {
    logger::get_logger()->error("status failed: {0}: {1}",
                                path.string(),
                                error_code.message());
    return std::nullopt;
  }

  return result;
}

inline std::optional<std::filesystem::file_time_type> last_write_time(const std::filesystem::path& path) {
  std::error_code error_code;
  auto result = std::filesystem::last_write_time(path,
                                                 error_code);
  if (error_code) {
    logger::get_logger()->error("last_write_time failed: {0}: {1}",
                                path.string(),
                                error_code.message());
    return std::nullopt;
  }

  return result;
}

inline std::optional<std::filesystem::path> canonical(const std::filesystem::path& path) {
  std::error_code error_code;
  auto result = std::filesystem::canonical(path,
                                           error_code);
  if (error_code) {
    logger::get_logger()->error("canonical failed: {0}: {1}",
                                path.string(),
                                error_code.message());
    return std::nullopt;
  }

  return result;
}

inline bool copy(const std::filesystem::path& from,
                 const std::filesystem::path& to,
                 std::filesystem::copy_options options = std::filesystem::copy_options::none) {
  std::error_code error_code;
  std::filesystem::copy(from,
                        to,
                        options,
                        error_code);
  if (error_code) {
    logger::get_logger()->error("copy failed: {0} -> {1}: {2}",
                                from.string(),
                                to.string(),
                                error_code.message());
    return false;
  }

  return true;
}

inline bool remove(const std::filesystem::path& path) {
  std::error_code error_code;
  std::filesystem::remove(path,
                          error_code);
  if (error_code) {
    logger::get_logger()->error("remove failed: {0}: {1}",
                                path.string(),
                                error_code.message());
    return false;
  }

  return true;
}

inline bool rename(const std::filesystem::path& from,
                   const std::filesystem::path& to) {
  std::error_code error_code;
  std::filesystem::rename(from,
                          to,
                          error_code);
  if (error_code) {
    logger::get_logger()->error("rename failed: {0} -> {1}: {2}",
                                from.string(),
                                to.string(),
                                error_code.message());
    return false;
  }

  return true;
}

inline bool permissions(const std::filesystem::path& path,
                        std::filesystem::perms permissions) {
  std::error_code error_code;
  std::filesystem::permissions(path,
                               permissions,
                               std::filesystem::perm_options::replace,
                               error_code);
  if (error_code) {
    logger::get_logger()->error("permissions failed: {0}: {1}",
                                path.string(),
                                error_code.message());
    return false;
  }

  return true;
}

inline bool chown(const std::filesystem::path& path,
                  uid_t uid,
                  gid_t gid) {
  if (::chown(path.c_str(),
              uid,
              gid) != 0) {
    logger::get_logger()->error("chown failed: {0}: {1}",
                                path.string(),
                                std::error_code(errno, std::generic_category()).message());
    return false;
  }

  return true;
}

//
// Utilities
//

namespace impl {

inline void prepare_system_tmp_directory() {
  // /Library/Application Support/org.pqrs/tmp
  auto directory_path = constants::get_tmp_directory();

  if (!krbn::filesystem_utility::create_directories(directory_path)) {
    return;
  }

  // Reset owner and permission just in case.
  if (!krbn::filesystem_utility::chown(directory_path,
                                       0,
                                       0)) {
    return;
  }
  if (!krbn::filesystem_utility::permissions(directory_path,
                                             permissions_0755)) {
    return;
  }
}

inline void prepare_system_rootonly_directory() {
  prepare_system_tmp_directory();

  // /Library/Application Support/org.pqrs/tmp/rootonly
  auto directory_path = constants::get_rootonly_directory();

  if (!krbn::filesystem_utility::create_directories(directory_path)) {
    return;
  }

  // Reset owner and permission just in case.
  if (!krbn::filesystem_utility::chown(directory_path,
                                       0,
                                       0)) {
    return;
  }
  if (!krbn::filesystem_utility::permissions(directory_path,
                                             permissions_0700)) {
    return;
  }
}

inline void prepare_system_user_directory(uid_t uid) {
  prepare_system_tmp_directory();

  {
    // /Library/Application Support/org.pqrs/tmp/user
    auto directory_path = constants::get_system_user_directory();

    if (!krbn::filesystem_utility::create_directories(directory_path)) {
      return;
    }

    if (!krbn::filesystem_utility::chown(directory_path,
                                         0,
                                         0)) {
      return;
    }
    if (!krbn::filesystem_utility::permissions(directory_path,
                                               permissions_0755)) {
      return;
    }
  }

  {
    // /Library/Application Support/org.pqrs/tmp/user/<uid>
    auto directory_path = constants::get_system_user_directory(uid);

    if (!krbn::filesystem_utility::create_directories(directory_path)) {
      return;
    }

    if (!krbn::filesystem_utility::chown(directory_path,
                                         uid,
                                         0)) {
      return;
    }
    if (!krbn::filesystem_utility::permissions(directory_path,
                                               permissions_0700)) {
      return;
    }
  }
}

} // namespace impl

inline void prepare_system_directories(std::optional<uid_t> uid) {
  krbn::filesystem_utility::impl::prepare_system_tmp_directory();
  krbn::filesystem_utility::impl::prepare_system_rootonly_directory();
  if (uid) {
    krbn::filesystem_utility::impl::prepare_system_user_directory(*uid);
  }
}

inline void prepare_user_directories() {
  {
    // ~/.config/karabiner
    auto directory_path = constants::get_user_configuration_directory();

    if (krbn::filesystem_utility::create_directories(directory_path) &&
        krbn::filesystem_utility::is_directory(directory_path)) {
      if (!krbn::filesystem_utility::permissions(directory_path,
                                                 permissions_0700)) {
        return;
      }
    }
  }

  {
    // ~/.local/share/karabiner
    auto directory_path = constants::get_user_data_directory();

    if (krbn::filesystem_utility::create_directories(directory_path) &&
        krbn::filesystem_utility::is_directory(directory_path)) {
      if (!krbn::filesystem_utility::permissions(directory_path,
                                                 permissions_0700)) {
        return;
      }
    }
  }

  {
    // ~/.config/karabiner/assets/complex_modifications
    auto directory_path = constants::get_user_complex_modifications_assets_directory();

    if (krbn::filesystem_utility::create_directories(directory_path) &&
        krbn::filesystem_utility::is_directory(directory_path)) {
      if (!krbn::filesystem_utility::permissions(directory_path,
                                                 permissions_0700)) {
        return;
      }
    }
  }

  {
    // ~/.local/share/karabiner/log
    auto directory_path = constants::get_user_log_directory();

    if (krbn::filesystem_utility::create_directories(directory_path) &&
        krbn::filesystem_utility::is_directory(directory_path)) {
      if (!krbn::filesystem_utility::permissions(directory_path,
                                                 permissions_0700)) {
        return;
      }
    }
  }

  {
    // ~/.local/share/karabiner/tmp
    auto directory_path = constants::get_user_tmp_directory();

    if (krbn::filesystem_utility::create_directories(directory_path) &&
        krbn::filesystem_utility::is_directory(directory_path)) {
      if (!krbn::filesystem_utility::permissions(directory_path,
                                                 permissions_0700)) {
        return;
      }
    }
  }
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
