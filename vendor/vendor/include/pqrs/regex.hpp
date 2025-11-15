#pragma once

// pqrs::regex v1.1

// (C) Copyright Takayama Fumihiko 2025.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>
#include <regex>

namespace pqrs {
class regex final {
public:
  regex(void)
      : flags_(std::regex_constants::ECMAScript) {
  }

  regex(const std::string& s,
        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript) {
    std::regex r(s, flags);
    string_ = s;
    regex_ = r;
    flags_ = flags;
  }

  const std::regex& get_regex(void) const {
    return regex_;
  }

  const std::string& get_string(void) const {
    return string_;
  }

  bool operator==(const regex& other) const {
    return string_ == other.string_;
  }

private:
  std::string string_;
  std::regex regex_;
  std::regex_constants::syntax_option_type flags_;
};

inline void to_json(nlohmann::json& json, const regex& value) {
  json = value.get_string();
}

inline void from_json(const nlohmann::json& json, regex& value) {
  pqrs::json::requires_string(json, "json");

  auto s = json.get<std::string>();

  try {
    value = regex(s);
  } catch (std::exception& e) {
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

    return h;
  }
};
} // namespace std
