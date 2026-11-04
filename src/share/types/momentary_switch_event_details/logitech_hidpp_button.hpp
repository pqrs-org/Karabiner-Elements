#pragma once

#include "types/internal_usage_page.hpp"
#include <cstdint>
#include <pqrs/json.hpp>

namespace krbn::momentary_switch_event_details::logitech_hidpp_button {
constexpr auto usage_page = internal_usage_page::logitech_hidpp_button;

inline bool target(pqrs::hid::usage_page::value_t candidate_usage_page,
                   pqrs::hid::usage::value_t usage) {
  auto value = type_safe::get(usage);
  return candidate_usage_page == usage_page &&
         1 <= value && value <= 0xffff;
}

inline pqrs::hid::usage_pair make_usage_pair(const std::string& key,
                                             const nlohmann::json& json) {
  if (!json.is_number_integer() ||
      json.get<int64_t>() < 1 ||
      json.get<int64_t>() > 0xffff) {
    throw pqrs::json::unmarshal_error(
        fmt::format("`{0}` must be an integer between 1 and 65535, but is `{1}`",
                    key,
                    pqrs::json::dump_for_error_message(json)));
  }

  return pqrs::hid::usage_pair(
      usage_page,
      pqrs::hid::usage::value_t(json.get<int32_t>()));
}
} // namespace krbn::momentary_switch_event_details::logitech_hidpp_button
