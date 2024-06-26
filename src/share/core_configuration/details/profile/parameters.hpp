#pragma once

#include "../../configuration_json_helper.hpp"
#include "types.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class parameters final {
public:
  parameters(const parameters&) = delete;

  parameters(void)
      : parameters(nlohmann::json::object(),
                   krbn::core_configuration::error_handling::loose) {
  }

  parameters(const nlohmann::json& json,
             error_handling error_handling)
      : json_(json) {
    helper_values_.push_back_value<std::chrono::milliseconds>("delay_milliseconds_before_open_device",
                                                              delay_milliseconds_before_open_device_,
                                                              std::chrono::milliseconds(1000));

    pqrs::json::requires_object(json, "json");

    helper_values_.update_value(json, error_handling);
  }

  nlohmann::json to_json(void) const {
    auto j = json_;

    helper_values_.update_json(j);

    return j;
  }

  const std::chrono::milliseconds& get_delay_milliseconds_before_open_device(void) const {
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
  configuration_json_helper::helper_values helper_values_;
};
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
