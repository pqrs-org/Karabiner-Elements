#pragma once

#include <pqrs/hid.hpp>
#include <pqrs/string.hpp>
#include <sstream>

namespace krbn {
namespace momentary_switch_event_details {
namespace impl {
inline std::string make_unnamed_name(pqrs::hid::usage::value_t usage) {
  std::stringstream ss;
  ss << "(number:" << type_safe::get(usage) << ")";
  return ss.str();
}

inline std::optional<pqrs::hid::usage::value_t> find_unnamed_usage(const std::string& name) {
  std::string_view prefix("(number:");

  if (pqrs::string::starts_with(name, prefix)) {
    try {
      return pqrs::hid::usage::value_t(std::stoi(name.substr(prefix.size())));
    } catch (const std::exception& e) {
    }
  }

  return std::nullopt;
}

template <typename T>
inline auto find_pair(const T& name_value_pairs,
                      pqrs::hid::usage::value_t usage) {
  return std::find_if(std::begin(name_value_pairs),
                      std::end(name_value_pairs),
                      [&](const auto& pair) {
                        return pair.second == usage;
                      });
}

template <typename T>
inline std::string make_name(const T& name_value_pairs,
                             pqrs::hid::usage::value_t usage) {
  auto it = find_pair(name_value_pairs, usage);
  if (it != std::end(name_value_pairs)) {
    return it->first.c_str();
  }

  // fallback
  return make_unnamed_name(usage);
}

template <typename T>
inline std::optional<pqrs::hid::usage::value_t> find_usage(const T& name_value_map,
                                                           const std::string& name) {
  auto it = name_value_map.find(name.c_str());
  if (it != std::end(name_value_map)) {
    return it->second;
  }

  // fallback
  return find_unnamed_usage(name);
}
} // namespace impl
} // namespace momentary_switch_event_details
} // namespace krbn
