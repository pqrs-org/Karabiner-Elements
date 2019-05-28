#pragma once

#include "monitor/configuration_monitor.hpp"
#include "update_utility.hpp"
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace console_user_server {
class updater_process_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  updater_process_manager(const updater_process_manager&) = delete;

  updater_process_manager(std::weak_ptr<configuration_monitor> weak_configuration_monitor) : dispatcher_client(),
                                                                                             weak_configuration_monitor_(weak_configuration_monitor),
                                                                                             checked_(false) {
    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      external_signal_connections_.emplace_back(
          configuration_monitor->core_configuration_updated.connect([this](auto&& weak_core_configuration) {
            if (auto core_configuration = weak_core_configuration.lock()) {
              if (!checked_) {
                checked_ = true;

                if (core_configuration->get_global_configuration().get_check_for_updates_on_startup()) {
                  logger::get_logger()->info("Check for updates...");
                  update_utility::check_for_updates_in_background();
                }
              }
            }
          }));
    }
  }

  ~updater_process_manager(void) {
    detach_from_dispatcher([this] {
      external_signal_connections_.clear();
    });
  }

private:
  std::weak_ptr<configuration_monitor> weak_configuration_monitor_;

  std::vector<nod::scoped_connection> external_signal_connections_;
  bool checked_;
};
} // namespace console_user_server
} // namespace krbn
