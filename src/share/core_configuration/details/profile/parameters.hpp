#pragma once

#include "types.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class parameters final {
public:
  parameters(void) : json_(nlohmann::json::object()),
                     delay_milliseconds_before_open_device_(1000) {
  }

  const nlohmann::json& get_json(void) const {
    return json_;
  }

  void set_json(const nlohmann::json& value) {
    json_ = value;
  }

  std::chrono::milliseconds get_delay_milliseconds_before_open_device(void) const {
    return delay_milliseconds_before_open_device_;
  }

  void set_delay_milliseconds_before_open_device(std::chrono::milliseconds value) {
    delay_milliseconds_before_open_device_ = value;
  }

  bool operator==(const parameters& other) const {
    return delay_milliseconds_before_open_device_ == other.delay_milliseconds_before_open_device_;
  }

private:
  nlohmann::json json_;
  std::chrono::milliseconds delay_milliseconds_before_open_device_;
};

inline void to_json(nlohmann::json& json, const parameters& value) {
  json = value.get_json();
  json["delay_milliseconds_before_open_device"] = value.get_delay_milliseconds_before_open_device().count();
}

inline void from_json(const nlohmann::json& json, parameters& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "delay_milliseconds_before_open_device") {
      pqrs::json::requires_number(v, "`" + k + "`");

      value.set_delay_milliseconds_before_open_device(std::chrono::milliseconds(v.get<int>()));

    } else {
      // Allow unknown keys in order to be able to load
      // newer version of karabiner.json with older Karabiner-Elements.
    }
  }

  value.set_json(json);
}
} // namespace details
} // namespace core_configuration
} // namespace krbn

namespace std {
template <>
struct hash<krbn::core_configuration::details::parameters> final {
  std::size_t operator()(const krbn::core_configuration::details::parameters& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_delay_milliseconds_before_open_device().count());

    return h;
  }
};
} // namespace std
