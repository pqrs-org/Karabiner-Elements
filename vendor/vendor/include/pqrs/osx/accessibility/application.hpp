#pragma once

// (C) Copyright Takayama Fumihiko 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <string>
#include <unistd.h>

namespace pqrs {
namespace osx {
namespace accessibility {
class application final {
public:
  const std::optional<std::string>& get_name() const {
    return name_;
  }

  application& set_name(const std::optional<std::string>& value) {
    name_ = value;
    return *this;
  }

  const std::optional<std::string>& get_bundle_identifier() const {
    return bundle_identifier_;
  }

  application& set_bundle_identifier(const std::optional<std::string>& value) {
    bundle_identifier_ = value;
    return *this;
  }

  const std::optional<std::string>& get_bundle_path() const {
    return bundle_path_;
  }

  application& set_bundle_path(const std::optional<std::string>& value) {
    bundle_path_ = value;
    return *this;
  }

  const std::optional<std::string>& get_file_path() const {
    return file_path_;
  }

  application& set_file_path(const std::optional<std::string>& value) {
    file_path_ = value;
    return *this;
  }

  const std::optional<pid_t>& get_pid() const {
    return pid_;
  }

  application& set_pid(const std::optional<pid_t>& value) {
    pid_ = value;
    return *this;
  }

  bool operator==(const application& other) const = default;

private:
  std::optional<std::string> name_;
  std::optional<std::string> bundle_identifier_;
  std::optional<std::string> bundle_path_;
  std::optional<std::string> file_path_;
  std::optional<pid_t> pid_;
};
} // namespace accessibility
} // namespace osx
} // namespace pqrs
