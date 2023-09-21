#pragma once

#include "core_configuration/core_configuration.hpp"
#include "json_utility.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulators/basic/basic.hpp"

namespace krbn {
namespace grabber {
namespace device_grabber_details {
class simple_modifications_manipulator_manager final {
public:
  simple_modifications_manipulator_manager(void) {
    manipulator_manager_ = std::make_shared<manipulator::manipulator_manager>();
  }

  std::shared_ptr<manipulator::manipulator_manager> get_manipulator_manager(void) const {
    return manipulator_manager_;
  }

  void update(const core_configuration::details::profile& profile) {
    manipulator_manager_->invalidate_manipulators();

    for (const auto& device : profile.get_devices()) {
      //
      // Add profile.device.simple_modifications
      //

      for (const auto& pair : device.get_simple_modifications().get_pairs()) {
        try {
          if (auto m = make_manipulator(pair)) {
            auto c = manipulator::manipulator_factory::make_device_if_condition(device);
            m->push_back_condition(c);
            manipulator_manager_->push_back_manipulator(m);
          }

        } catch (const pqrs::json::unmarshal_error& e) {
          logger::get_logger()->error(fmt::format("karabiner.json error: {0}", e.what()));

        } catch (const std::exception& e) {
          logger::get_logger()->error(e.what());
        }
      }

      //
      // Add profile.device.mouse_flip_*
      //

      auto flip = nlohmann::json::array();
      if (device.get_mouse_flip_x()) {
        flip.push_back("x");
      }
      if (device.get_mouse_flip_y()) {
        flip.push_back("y");
      }
      if (device.get_mouse_flip_vertical_wheel()) {
        flip.push_back("vertical_wheel");
      }
      if (device.get_mouse_flip_horizontal_wheel()) {
        flip.push_back("horizontal_wheel");
      }
      if (flip.size() > 0) {
        try {
          auto json = nlohmann::json::object({
              {"type", "mouse_basic"},
              {"flip", flip},
          });
          auto parameters = krbn::core_configuration::details::complex_modifications_parameters();
          auto m = std::make_shared<manipulator::manipulators::mouse_basic::mouse_basic>(json,
                                                                                         parameters);
          auto c = manipulator::manipulator_factory::make_device_if_condition(device);
          m->push_back_condition(c);
          manipulator_manager_->push_back_manipulator(m);
        } catch (const std::exception& e) {
          logger::get_logger()->error(e.what());
        }
      }
    }

    for (const auto& pair : profile.get_simple_modifications().get_pairs()) {
      if (auto m = make_manipulator(pair)) {
        manipulator_manager_->push_back_manipulator(m);
      }
    }
  }

private:
  std::shared_ptr<manipulator::manipulators::base> make_manipulator(const std::pair<std::string, std::string>& pair) const {
    if (!pair.first.empty() && !pair.second.empty()) {
      try {
        auto from_json = json_utility::parse_jsonc(pair.first);
        from_json["modifiers"]["optional"] = nlohmann::json::array();
        from_json["modifiers"]["optional"].push_back("any");

        auto to_json = json_utility::parse_jsonc(pair.second);
        std::vector<manipulator::to_event_definition> to_event_definitions;
        for (auto&& j : to_json) {
          to_event_definitions.emplace_back(j);
        }

        return std::make_shared<manipulator::manipulators::basic::basic>(manipulator::manipulators::basic::from_event_definition(from_json),
                                                                         to_event_definitions);

      } catch (const pqrs::json::unmarshal_error& e) {
        logger::get_logger()->error(fmt::format("karabiner.json error: {0}", e.what()));

      } catch (const std::exception& e) {
        logger::get_logger()->error(e.what());
      }
    }
    return nullptr;
  }

  std::shared_ptr<manipulator::manipulator_manager> manipulator_manager_;
};
} // namespace device_grabber_details
} // namespace grabber
} // namespace krbn
