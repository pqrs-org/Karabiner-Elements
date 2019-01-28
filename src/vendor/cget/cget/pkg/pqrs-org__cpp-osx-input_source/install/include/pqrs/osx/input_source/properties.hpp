#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "input_source.hpp"
#include <pqrs/hash.hpp>

namespace pqrs {
namespace osx {
namespace input_source {
class properties final {
public:
  properties(void) {
  }

  properties(TISInputSourceRef s) : input_source_id_(make_input_source_id(s)),
                                    localized_name_(make_localized_name(s)),
                                    input_mode_id_(make_input_mode_id(s)),
                                    languages_(make_languages(s)),
                                    first_language_(make_first_language(s)) {
  }

  const std::optional<std::string>& get_input_source_id(void) const {
    return input_source_id_;
  }

  properties& set_input_source_id(const std::optional<std::string>& value) {
    input_source_id_ = value;
    return *this;
  }

  const std::optional<std::string>& get_localized_name(void) const {
    return localized_name_;
  }

  properties& set_localized_name(const std::optional<std::string>& value) {
    localized_name_ = value;
    return *this;
  }

  const std::optional<std::string>& get_input_mode_id(void) const {
    return input_mode_id_;
  }

  properties& set_input_mode_id(const std::optional<std::string>& value) {
    input_mode_id_ = value;
    return *this;
  }

  const std::vector<std::string>& get_languages(void) const {
    return languages_;
  }

  properties& set_languages(const std::vector<std::string>& value) {
    languages_ = value;
    return *this;
  }

  const std::optional<std::string>& get_first_language(void) const {
    return first_language_;
  }

  properties& set_first_language(const std::optional<std::string>& value) {
    first_language_ = value;
    return *this;
  }

  bool operator==(const properties& other) const {
    return input_source_id_ == other.input_source_id_ &&
           localized_name_ == other.localized_name_ &&
           input_mode_id_ == other.input_mode_id_ &&
           languages_ == other.languages_ &&
           first_language_ == other.first_language_;
  }

  bool operator!=(const properties& other) const {
    return !(*this == other);
  }

private:
  std::optional<std::string> input_source_id_;
  std::optional<std::string> localized_name_;
  std::optional<std::string> input_mode_id_;
  std::vector<std::string> languages_;
  std::optional<std::string> first_language_;
};
} // namespace input_source
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::input_source::properties> final {
  std::size_t operator()(const pqrs::osx::input_source::properties& value) const {
    size_t h = 0;

    if (auto& input_source_id = value.get_input_source_id()) {
      pqrs::hash_combine(h, *input_source_id);
    }

    return h;
  }
};
} // namespace std
