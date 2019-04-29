#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <SystemConfiguration/SystemConfiguration.h>
#include <optional>

namespace pqrs {
namespace osx {
namespace session {
class attributes final {
public:
  attributes(SessionAttributeBits attributes) : is_root_(false),
                                                has_graphic_access_(false),
                                                has_tty_(false),
                                                is_remote_(false) {
    is_root_ = attributes & sessionIsRoot;
    has_graphic_access_ = attributes & sessionHasGraphicAccess;
    has_tty_ = attributes & sessionHasTTY;
    is_remote_ = attributes & sessionIsRemote;
  }

  bool get_is_root(void) const {
    return is_root_;
  }

  bool get_has_graphic_access(void) const {
    return has_graphic_access_;
  }

  bool get_has_tty(void) const {
    return has_tty_;
  }

  bool get_is_remote(void) const {
    return is_remote_;
  }

private:
  bool is_root_;
  bool has_graphic_access_;
  bool has_tty_;
  bool is_remote_;
};
} // namespace session
} // namespace osx
} // namespace pqrs
