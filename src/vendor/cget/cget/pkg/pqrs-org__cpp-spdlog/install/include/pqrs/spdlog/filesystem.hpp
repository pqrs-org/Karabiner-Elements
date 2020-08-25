#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <filesystem>

namespace pqrs {
namespace spdlog {
namespace filesystem {
//
// owner
//

constexpr std::filesystem::perms log_directory_perms_0700(
    std::filesystem::perms::owner_all);

//
// owner + group
//

constexpr std::filesystem::perms log_directory_perms_0770(
    std::filesystem::perms::owner_all |
    std::filesystem::perms::group_all);

constexpr std::filesystem::perms log_directory_perms_0750(
    std::filesystem::perms::owner_all |
    std::filesystem::perms::group_read | std::filesystem::perms::group_exec);

//
// owner + group + others
//

constexpr std::filesystem::perms log_directory_perms_0775(
    std::filesystem::perms::owner_all |
    std::filesystem::perms::group_all |
    std::filesystem::perms::others_read | std::filesystem::perms::others_exec);

constexpr std::filesystem::perms log_directory_perms_0755(
    std::filesystem::perms::owner_all |
    std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
    std::filesystem::perms::others_read | std::filesystem::perms::others_exec);
} // namespace filesystem
} // namespace spdlog
} // namespace pqrs
