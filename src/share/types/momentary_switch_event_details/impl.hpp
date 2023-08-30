#pragma once

#include <pqrs/hid.hpp>
#include <pqrs/string.hpp>
#include <sstream>

namespace krbn {
namespace momentary_switch_event_details {
namespace impl {
//
// name and value pairs
//

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

  return "unsupported";
}

template <typename T>
inline std::optional<pqrs::hid::usage::value_t> find_usage(const T& name_value_map,
                                                           const std::string& name) {
  auto it = name_value_map.find(name.c_str());
  if (it != std::end(name_value_map)) {
    return it->second;
  }

  return std::nullopt;
}

//
// json
//

template <typename T>
inline pqrs::hid::usage_pair make_usage_pair(const T& name_value_map,
                                             pqrs::hid::usage_page::value_t usage_page,
                                             const std::string& key,
                                             const nlohmann::json& json) {
  if (json.is_string()) {
    if (auto usage = find_usage(name_value_map, json.get<std::string>())) {
      return pqrs::hid::usage_pair(usage_page, *usage);
    } else {
      std::stringstream ss;
      ss << "unknown " << key << ": `" << pqrs::json::dump_for_error_message(json) << "`";
      throw pqrs::json::unmarshal_error(ss.str());
    }

  } else if (json.is_number()) {
    return pqrs::hid::usage_pair(usage_page, json.get<pqrs::hid::usage::value_t>());

  } else {
    std::stringstream ss;
    ss << "`" << key << "` must be string or number, but is `" << pqrs::json::dump_for_error_message(json) << "`";
    throw pqrs::json::unmarshal_error(ss.str());
  }
}
} // namespace impl
} // namespace momentary_switch_event_details
} // namespace krbn
