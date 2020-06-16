#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <optional>
#include <pqrs/hash.hpp>
#include <pqrs/osx/input_source.hpp>
#include <regex>
#include <string>

namespace pqrs {
namespace osx {
namespace input_source_selector {
class specifier {
public:
  const std::optional<std::string>& get_language_string(void) const {
    return language_string_;
  }

  specifier& set_language(const std::optional<std::string>& value) {
    language_string_ = value;

    if (language_string_) {
      language_regex_ = std::regex(*language_string_);
    } else {
      language_regex_ = std::nullopt;
    }

    return *this;
  }

  const std::optional<std::string>& get_input_source_id_string(void) const {
    return input_source_id_string_;
  }

  specifier& set_input_source_id(const std::optional<std::string>& value) {
    input_source_id_string_ = value;

    if (input_source_id_string_) {
      input_source_id_regex_ = std::regex(*input_source_id_string_);
    } else {
      input_source_id_regex_ = std::nullopt;
    }

    return *this;
  }

  const std::optional<std::string>& get_input_mode_id_string(void) const {
    return input_mode_id_string_;
  }

  specifier& set_input_mode_id(const std::optional<std::string>& value) {
    input_mode_id_string_ = value;

    if (input_mode_id_string_) {
      input_mode_id_regex_ = std::regex(*input_mode_id_string_);
    } else {
      input_mode_id_regex_ = std::nullopt;
    }

    return *this;
  }

  bool test(const input_source::properties& properties) const {
    if (language_regex_) {
      if (auto& v = properties.get_first_language()) {
        if (!regex_search(std::begin(*v),
                          std::end(*v),
                          *language_regex_)) {
          return false;
        }
      } else {
        return false;
      }
    }

    if (input_source_id_regex_) {
      if (auto& v = properties.get_input_source_id()) {
        if (!regex_search(std::begin(*v),
                          std::end(*v),
                          *input_source_id_regex_)) {
          return false;
        }
      } else {
        return false;
      }
    }

    if (input_mode_id_regex_) {
      if (auto& v = properties.get_input_mode_id()) {
        if (!regex_search(std::begin(*v),
                          std::end(*v),
                          *input_mode_id_regex_)) {
          return false;
        }
      } else {
        return false;
      }
    }

    return true;
  }

  bool operator==(const specifier& other) const {
    return language_string_ == other.language_string_ &&
           input_source_id_string_ == other.input_source_id_string_ &&
           input_mode_id_string_ == other.input_mode_id_string_;
  }

  bool operator!=(const specifier& other) const {
    return !(*this == other);
  }

private:
  std::optional<std::string> language_string_;
  std::optional<std::string> input_source_id_string_;
  std::optional<std::string> input_mode_id_string_;

  std::optional<std::regex> language_regex_;
  std::optional<std::regex> input_source_id_regex_;
  std::optional<std::regex> input_mode_id_regex_;
};
} // namespace input_source_selector
} // namespace osx
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::osx::input_source_selector::specifier> final {
  std::size_t operator()(const pqrs::osx::input_source_selector::specifier& value) const {
    size_t h = 0;

    if (auto& s = value.get_language_string()) {
      pqrs::hash::combine(h, *s);
    } else {
      pqrs::hash::combine(h, 0);
    }

    if (auto& s = value.get_input_source_id_string()) {
      pqrs::hash::combine(h, *s);
    } else {
      pqrs::hash::combine(h, 0);
    }

    if (auto& s = value.get_input_mode_id_string()) {
      pqrs::hash::combine(h, *s);
    } else {
      pqrs::hash::combine(h, 0);
    }

    return h;
  }
};
} // namespace std
