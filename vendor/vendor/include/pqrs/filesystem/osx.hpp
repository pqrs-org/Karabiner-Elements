#pragma once

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX

#include <cerrno>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <sys/attr.h>
#include <sys/mount.h>
#include <system_error>
#include <unistd.h>

namespace pqrs::filesystem::osx {
struct volume_capabilities final {
  // nullopt means that the filesystem did not report this capability as valid.
  std::optional<bool> supports_permissions;
  std::optional<bool> supports_symbolic_links;
  std::optional<bool> supports_hard_links;
  std::optional<bool> case_sensitive;
  std::optional<bool> case_preserving;
  std::optional<bool> supports_extended_security;
};

// Current mount settings, rather than filesystem format capabilities.
struct volume_mount_flags final {
  bool ownership_ignored;
  bool read_only;
  bool no_exec;
  bool no_suid;
  bool local;
};

struct volume_properties final {
  volume_capabilities capabilities;
  volume_mount_flags mount_flags;
};

namespace detail {
[[nodiscard]] inline volume_capabilities make_volume_capabilities(const vol_capabilities_attr_t& volume) {
  auto capability = [&](size_t group, uint32_t flag) -> std::optional<bool> {
    if (!(volume.valid[group] & flag)) {
      return std::nullopt;
    }
    return (volume.capabilities[group] & flag) != 0;
  };
  auto no_permissions = capability(VOL_CAPABILITIES_FORMAT, VOL_CAP_FMT_NO_PERMISSIONS);
  return volume_capabilities{
      .supports_permissions = no_permissions ? std::optional(!*no_permissions) : std::nullopt,
      .supports_symbolic_links = capability(VOL_CAPABILITIES_FORMAT, VOL_CAP_FMT_SYMBOLICLINKS),
      .supports_hard_links = capability(VOL_CAPABILITIES_FORMAT, VOL_CAP_FMT_HARDLINKS),
      .case_sensitive = capability(VOL_CAPABILITIES_FORMAT, VOL_CAP_FMT_CASE_SENSITIVE),
      .case_preserving = capability(VOL_CAPABILITIES_FORMAT, VOL_CAP_FMT_CASE_PRESERVING),
      .supports_extended_security = capability(VOL_CAPABILITIES_INTERFACES, VOL_CAP_INT_EXTENDED_SECURITY),
  };
}
} // namespace detail

// Follows symlinks. The path must exist. This only queries metadata and never writes.
[[nodiscard]] inline std::expected<volume_properties, std::error_code> get_volume_properties(const std::filesystem::path& path) {
  struct statfs mount{};
  if (::statfs(path.c_str(), &mount) != 0) {
    return std::unexpected(std::error_code(errno, std::generic_category()));
  }

  struct attrlist attributes{};
  attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
  attributes.volattr = ATTR_VOL_INFO | ATTR_VOL_CAPABILITIES;

  struct {
    uint32_t length;
    vol_capabilities_attr_t capabilities;
  } volume{};
  if (::getattrlist(path.c_str(), &attributes, &volume, sizeof(volume), 0) != 0) {
    return std::unexpected(std::error_code(errno, std::generic_category()));
  }

  return volume_properties{
      .capabilities = detail::make_volume_capabilities(volume.capabilities),
      .mount_flags = {
          .ownership_ignored = (mount.f_flags & MNT_IGNORE_OWNERSHIP) != 0,
          .read_only = (mount.f_flags & MNT_RDONLY) != 0,
          .no_exec = (mount.f_flags & MNT_NOEXEC) != 0,
          .no_suid = (mount.f_flags & MNT_NOSUID) != 0,
          .local = (mount.f_flags & MNT_LOCAL) != 0,
      },
  };
}
} // namespace pqrs::filesystem::osx
#endif
#endif
