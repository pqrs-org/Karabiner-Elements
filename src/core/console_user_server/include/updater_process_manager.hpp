#pragma once

#include "configuration_monitor.hpp"
#include "update_utility.hpp"

namespace krbn {
class updater_process_manager final {
public:
  updater_process_manager(const updater_process_manager&) = delete;

  updater_process_manager(std::shared_ptr<configuration_monitor> configuration_monitor) : checked_(false) {
    // core_configuration_updated

    {
      auto c = configuration_monitor->core_configuration_updated.connect([this](auto&& core_configuration) {
        if (!checked_) {
          checked_ = true;

          if (core_configuration->get_global_configuration().get_check_for_updates_on_startup()) {
            logger::get_logger().info("Check for updates...");
            update_utility::check_for_updates_in_background();
          }
        }
      });

      connections_.push_back(std::make_unique<boost::signals2::scoped_connection>(c));
    }
  }

private:
  std::vector<std::unique_ptr<boost::signals2::scoped_connection>> connections_;
  bool checked_;
};
} // namespace krbn
