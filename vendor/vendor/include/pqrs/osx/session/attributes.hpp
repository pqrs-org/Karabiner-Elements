#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <SystemConfiguration/SystemConfiguration.h>

namespace pqrs::osx::session {
class attributes final {
public:
  explicit attributes(SessionAttributeBits attributes) noexcept
      : is_root_(attributes & sessionIsRoot),
        has_graphic_access_(attributes & sessionHasGraphicAccess),
        has_tty_(attributes & sessionHasTTY),
        is_remote_(attributes & sessionIsRemote) {
  }

  [[nodiscard]] bool get_is_root() const noexcept {
    return is_root_;
  }

  [[nodiscard]] bool get_has_graphic_access() const noexcept {
    return has_graphic_access_;
  }

  [[nodiscard]] bool get_has_tty() const noexcept {
    return has_tty_;
  }

  [[nodiscard]] bool get_is_remote() const noexcept {
    return is_remote_;
  }

private:
  bool is_root_;
  bool has_graphic_access_;
  bool has_tty_;
  bool is_remote_;
};
} // namespace pqrs::osx::session
