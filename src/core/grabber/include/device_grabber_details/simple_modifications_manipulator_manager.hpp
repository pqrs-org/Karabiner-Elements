#pragma once

#include "core_configuration/core_configuration.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "manipulator/manipulator_manager.hpp"
#include "manipulator/manipulators/basic/basic.hpp"

namespace krbn {
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
        auto from_json = nlohmann::json::parse(pair.first);
        from_json["modifiers"]["optional"] = nlohmann::json::array();
        from_json["modifiers"]["optional"].push_back("any");

        auto to_json = nlohmann::json::parse(pair.second);

        return std::make_shared<manipulator::manipulators::basic::basic>(manipulator::manipulators::basic::from_event_definition(from_json),
                                                                         manipulator::to_event_definition(to_json));

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
} // namespace krbn
