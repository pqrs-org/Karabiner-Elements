#pragma once

#include "constants.hpp"
#include <pqrs/filesystem.hpp>

namespace krbn {
namespace filesystem_utility {
inline void mkdir_tmp_directory(void) {
  pqrs::filesystem::create_directory_with_intermediate_directories(
      constants::get_tmp_directory(),
      0755);

  // Reset owner and permission just in case.

  chown(constants::get_tmp_directory(), 0, 0);
  chmod(constants::get_tmp_directory(), 0755);
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
} // namespace filesystem_utility
} // namespace krbn
