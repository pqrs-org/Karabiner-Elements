#pragma once

#include <pqrs/hid.hpp>
#include <pqrs/string.hpp>
#include <sstream>

namespace krbn {
namespace momentary_switch_event_details {
namespace unnamed {
inline std::string make_name(pqrs::hid::usage::value_t usage) {
  std::stringstream ss;
  ss << "(number:" << type_safe::get(usage) << ")";
  return ss.str();
}

inline std::optional<pqrs::hid::usage::value_t> find_usage(const std::string& name) {
  std::string_view prefix("(number:");

  if (pqrs::string::starts_with(name, prefix)) {
    try {
      return pqrs::hid::usage::value_t(std::stoi(name.substr(prefix.size())));
    } catch (const std::exception& e) {
    }
  }

  return std::nullopt;
}
} // namespace unnamed
} // namespace momentary_switch_event_details
} // namespace krbn
