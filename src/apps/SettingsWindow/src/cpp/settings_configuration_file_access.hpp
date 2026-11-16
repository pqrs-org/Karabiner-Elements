#pragma once

#include "settings.hpp"
#include <filesystem>
#include <optional>
#include <pqrs/filesystem.hpp>
#include <pqrs/filesystem/osx.hpp>
#include <system_error>

// The library query follows symlinks and inspects the volume of the resolved path.
[[nodiscard]] inline krbn_configuration_file_access check_configuration_volume_access(const std::filesystem::path& path) {
  if (auto volume = pqrs::filesystem::osx::get_volume_properties(path)) {
    if (volume->capabilities.supports_permissions == std::optional(false)) {
      return krbn_configuration_file_access_permissions_unsupported;
    }
    if (volume->mount_flags.ownership_ignored) {
      return krbn_configuration_file_access_ownership_ignored;
    }
  }

  return krbn_configuration_file_access_ok;
}

// Read-only preflight checks. These do not guarantee that a later save will succeed:
// permissions, mount state, or the file itself can change between checking and saving.
[[nodiscard]] inline krbn_configuration_file_access check_configuration_file_access(const std::filesystem::path& path) {
  // A new configuration directory may not exist yet. Check the nearest existing
  // ancestor, where the missing directories will have to be created.
  auto directory = path.parent_path();
  std::error_code error;
  while (!directory.empty() && !std::filesystem::exists(directory, error)) {
    if (error && error != std::errc::no_such_file_or_directory && error != std::errc::not_a_directory) {
      // Read/access errors are handled by the configuration load-state UI.
      return krbn_configuration_file_access_ok;
    }
    auto parent = directory.parent_path();
    if (parent == directory) {
      break;
    }
    directory = parent;
  }

  // Check both volumes: an existing karabiner.json may be a symlink to another
  // volume, while atomic replacement creates a temporary file in its parent directory.
  if (std::filesystem::exists(path, error)) {
    if (auto error = check_configuration_volume_access(path);
        error != krbn_configuration_file_access_ok) {
      return error;
    }
  }
  if (!directory.empty()) {
    if (auto error = check_configuration_volume_access(directory);
        error != krbn_configuration_file_access_ok) {
      return error;
    }
  }

  // Use effective credentials and ACL-aware access checks, rather than mode bits.
  if (auto writable = pqrs::filesystem::is_writable_by_current_process(path); writable && !*writable) {
    return krbn_configuration_file_access_file_not_writable;
  }
  if (!directory.empty()) {
    auto writable = pqrs::filesystem::is_writable_by_current_process(directory);
    auto executable = pqrs::filesystem::is_executable_by_current_process(directory);
    if ((writable && !*writable) || (executable && !*executable)) {
      return krbn_configuration_file_access_directory_not_writable;
    }
  }

  return krbn_configuration_file_access_ok;
}
