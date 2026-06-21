#pragma once

// pqrs::regex v1.2.0

// (C) Copyright Takayama Fumihiko 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>
#include <regex>
#include <string>

namespace pqrs {
class regex final {
public:
  regex() = default;

  regex(const std::string& s,
        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript)
      : string_(s),
        regex_(s, flags),
        flags_(flags) {
  }

  [[nodiscard]] const std::regex& get_regex() const noexcept {
    return regex_;
  }

  [[nodiscard]] const std::string& get_string() const noexcept {
    return string_;
  }

  [[nodiscard]] std::regex_constants::syntax_option_type get_flags() const noexcept {
    return flags_;
  }

  [[nodiscard]] bool operator==(const regex& other) const {
    return string_ == other.string_ &&
           flags_ == other.flags_;
  }

private:
  std::string string_;
  std::regex regex_;
  std::regex_constants::syntax_option_type flags_ = std::regex_constants::ECMAScript;
};

inline void to_json(nlohmann::json& json, const regex& value) {
  json = value.get_string();
}

inline void from_json(const nlohmann::json& json, regex& value) {
  pqrs::json::requires_string(json, "json");

  auto s = json.get<std::string>();

  try {
    value = regex(s);
  } catch (const std::exception& e) {
    throw pqrs::json::unmarshal_error(std::string(e.what()) + ": `" + s + "`");
  }
}
} // namespace pqrs

namespace std {
template <>
struct hash<pqrs::regex> final {
  std::size_t operator()(const pqrs::regex& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_string());
    pqrs::hash::combine(h, value.get_flags());

    return h;
  }
};
} // namespace std
