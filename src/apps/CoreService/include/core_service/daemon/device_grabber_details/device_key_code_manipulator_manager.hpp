#pragma once

#include "core_configuration/core_configuration.hpp"
#include "manipulator/condition_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulators/basic/basic.hpp"
#include <pqrs/gsl.hpp>

namespace krbn::core_service::daemon::device_grabber_details {
class device_key_code_manipulator_manager final {
public:
  device_key_code_manipulator_manager()
      : manipulator_manager_(std::make_shared<manipulator::manipulator_manager>()) {
  }

  [[nodiscard]] pqrs::not_null_shared_ptr_t<manipulator::manipulator_manager> get_manipulator_manager() const {
    return manipulator_manager_;
  }

  void update(const core_configuration::details::profile& profile) {
    manipulator_manager_->invalidate_manipulators();

    for (const auto& device : profile.get_devices()) {
      if (device->get_swap_grave_accent_and_non_us_backslash()) {
        push_back_manipulator(*device,
                              "grave_accent_and_tilde",
                              "non_us_backslash");
        push_back_manipulator(*device,
                              "non_us_backslash",
                              "grave_accent_and_tilde");
      }
    }
  }

private:
  void push_back_manipulator(const core_configuration::details::device& device,
                             const std::string& from_key_code,
                             const std::string& to_key_code) {
    auto from_json = nlohmann::json::object({
        {"key_code", from_key_code},
        {"modifiers", nlohmann::json::object({
                          {"optional", nlohmann::json::array({"any"})},
                      })},
    });
    auto to_json = nlohmann::json::object({
        {"key_code", to_key_code},
    });

    std::vector<pqrs::not_null_shared_ptr_t<manipulator::to_event_definition>> to_event_definitions{
        std::make_shared<manipulator::to_event_definition>(to_json),
    };
    auto m = std::make_shared<manipulator::manipulators::basic::basic>(
        manipulator::manipulators::basic::from_event_definition(from_json),
        to_event_definitions);
    m->push_back_condition(manipulator::condition_factory::make_device_if_condition(device));
    manipulator_manager_->push_back_manipulator(m);
  }

  pqrs::not_null_shared_ptr_t<manipulator::manipulator_manager> manipulator_manager_;
};
} // namespace krbn::core_service::daemon::device_grabber_details
