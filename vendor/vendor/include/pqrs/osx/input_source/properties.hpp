#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "input_source.hpp"
#include <pqrs/hash.hpp>

namespace pqrs::osx::input_source {
class properties final {
public:
  properties() noexcept = default;

  properties(TISInputSourceRef s)
      : input_source_id_(make_input_source_id(s)),
        localized_name_(make_localized_name(s)),
        input_mode_id_(make_input_mode_id(s)),
        languages_(make_languages(s)),
        first_language_(make_first_language(s)) {
  }

  [[nodiscard]] const std::optional<std::string>& get_input_source_id() const noexcept {
    return input_source_id_;
  }

  properties& set_input_source_id(const std::optional<std::string>& value) {
    input_source_id_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<std::string>& get_localized_name() const noexcept {
    return localized_name_;
  }

  properties& set_localized_name(const std::optional<std::string>& value) {
    localized_name_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<std::string>& get_input_mode_id() const noexcept {
    return input_mode_id_;
  }

  properties& set_input_mode_id(const std::optional<std::string>& value) {
    input_mode_id_ = value;
    return *this;
  }

  [[nodiscard]] const std::vector<std::string>& get_languages() const noexcept {
    return languages_;
  }

  properties& set_languages(const std::vector<std::string>& value) {
    languages_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<std::string>& get_first_language() const noexcept {
    return first_language_;
  }

  properties& set_first_language(const std::optional<std::string>& value) {
    first_language_ = value;
    return *this;
  }

  [[nodiscard]] bool operator==(const properties& other) const = default;

  [[nodiscard]] bool operator!=(const properties& other) const = default;

private:
  std::optional<std::string> input_source_id_;
  std::optional<std::string> localized_name_;
  std::optional<std::string> input_mode_id_;
  std::vector<std::string> languages_;
  std::optional<std::string> first_language_;
};
} // namespace pqrs::osx::input_source

namespace std {
template <>
struct hash<pqrs::osx::input_source::properties> final {
  [[nodiscard]] std::size_t operator()(const pqrs::osx::input_source::properties& value) const {
    size_t h = 0;

    if (auto& input_source_id = value.get_input_source_id()) {
      pqrs::hash::combine(h, *input_source_id);
    } else {
      pqrs::hash::combine(h, 0);
    }

    return h;
  }
};
} // namespace std
