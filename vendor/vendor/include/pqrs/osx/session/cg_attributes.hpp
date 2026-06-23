#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <CoreGraphics/CoreGraphics.h>
#include <optional>
#include <pqrs/cf/boolean.hpp>
#include <pqrs/cf/number.hpp>
#include <pqrs/cf/string.hpp>

namespace pqrs::osx::session {
class cg_attributes final {
public:
  explicit cg_attributes(CFDictionaryRef dictionary) {
    if (auto v = pqrs::cf::make_number<int64_t>(CFDictionaryGetValue(dictionary, kCGSessionConsoleSetKey))) {
      console_set_ = static_cast<uint32_t>(*v);
    }

    login_done_ = pqrs::cf::make_bool(CFDictionaryGetValue(dictionary, kCGSessionLoginDoneKey));

    on_console_ = pqrs::cf::make_bool(CFDictionaryGetValue(dictionary, kCGSessionOnConsoleKey));

    if (auto v = pqrs::cf::make_number<int64_t>(CFDictionaryGetValue(dictionary, kCGSessionUserIDKey))) {
      user_id_ = static_cast<uid_t>(*v);
    }

    user_name_ = pqrs::cf::make_string(CFDictionaryGetValue(dictionary, kCGSessionUserNameKey));
  }

  [[nodiscard]] const std::optional<uint32_t>& get_console_set() const noexcept {
    return console_set_;
  }

  [[nodiscard]] const std::optional<bool>& get_login_done() const noexcept {
    return login_done_;
  }

  [[nodiscard]] const std::optional<bool>& get_on_console() const noexcept {
    return on_console_;
  }

  [[nodiscard]] const std::optional<uid_t>& get_user_id() const noexcept {
    return user_id_;
  }

  [[nodiscard]] const std::optional<std::string>& get_user_name() const noexcept {
    return user_name_;
  }

private:
  std::optional<uint32_t> console_set_;
  std::optional<bool> login_done_;
  std::optional<bool> on_console_;
  std::optional<uid_t> user_id_;
  std::optional<std::string> user_name_;
};
} // namespace pqrs::osx::session
