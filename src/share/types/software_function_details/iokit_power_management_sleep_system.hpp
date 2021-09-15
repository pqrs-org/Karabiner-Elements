#pragma once

#include <chrono>
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>
#include <string_view>

namespace krbn {
namespace software_function_details {
class iokit_power_management_sleep_system {
public:
  iokit_power_management_sleep_system(void)
      : delay_milliseconds_(500) {
  }

  std::chrono::milliseconds get_delay_milliseconds(void) const {
    return delay_milliseconds_;
  }

  void set_delay_milliseconds(std::chrono::milliseconds value) {
    delay_milliseconds_ = value;
  }

  constexpr bool operator==(const iokit_power_management_sleep_system&) const = default;

private:
  std::chrono::milliseconds delay_milliseconds_;
};

inline void to_json(nlohmann::json& json, const iokit_power_management_sleep_system& value) {
  json["delay_milliseconds"] = value.get_delay_milliseconds().count();
}

inline void from_json(const nlohmann::json& json, iokit_power_management_sleep_system& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "delay_milliseconds") {
      pqrs::json::requires_number(v, "`" + k + "`");
      value.set_delay_milliseconds(std::chrono::milliseconds(v.get<int>()));
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", k));
    }
  }
}
} // namespace software_function_details
} // namespace krbn

namespace std {
template <>
struct hash<krbn::software_function_details::iokit_power_management_sleep_system> final {
  std::size_t operator()(const krbn::software_function_details::iokit_power_management_sleep_system& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_delay_milliseconds().count());

    return h;
  }
};
} // namespace std
