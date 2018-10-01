#pragma once

// `krbn::system_preferences_monitor` can be used safely in a multi-threaded environment.

#include "boost_utility.hpp"
#include "configuration_monitor.hpp"
#include "dispatcher.hpp"
#include "logger.hpp"
#include "system_preferences_utility.hpp"
#include "thread_utility.hpp"

namespace krbn {
class system_preferences_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals

  boost::signals2::signal<void(const system_preferences& value)> system_preferences_changed;

  // Methods

  system_preferences_monitor(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
                             std::weak_ptr<configuration_monitor> weak_configuration_monitor) : dispatcher_client(weak_dispatcher),
                                                                                                weak_configuration_monitor_(weak_configuration_monitor),
                                                                                                enabled_(false) {
    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      // core_configuration_updated
      {
        auto c = configuration_monitor->core_configuration_updated.connect([this](auto&&) {
          enqueue_to_dispatcher([this] {
            check_system_preferences();
          });
        });
        configuration_monitor_connections_.push_back(c);
      }
    }
  }

  virtual ~system_preferences_monitor(void) {
    // Disconnect `configuration_monitor_connections_`.

    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      configuration_monitor->get_run_loop_thread()->enqueue(^{
        configuration_monitor_connections_.disconnect_all_connections();
      });
    } else {
      configuration_monitor_connections_.disconnect_all_connections();
    }

    configuration_monitor_connections_.wait_disconnect_all_connections();

    // dispatcher

    detach_from_dispatcher([] {
    });

    logger::get_logger().info("system_preferences_monitor is stopped.");
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (enabled_) {
        return;
      }

      enabled_ = true;

      logger::get_logger().info("system_preferences_monitor is started.");

      check_system_preferences();
    });
  }

private:
  system_preferences make_system_preferences(void) const {
    system_preferences system_preferences;

    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      if (auto core_configuration = configuration_monitor->get_core_configuration()) {
        auto country_code = core_configuration->get_selected_profile().get_virtual_hid_keyboard().get_country_code();

        system_preferences.set_keyboard_fn_state(system_preferences_utility::get_keyboard_fn_state());
        system_preferences.set_swipe_scroll_direction(system_preferences_utility::get_swipe_scroll_direction());
        system_preferences.set_keyboard_type(system_preferences_utility::get_keyboard_type(vendor_id(0x16c0),
                                                                                           product_id(0x27db),
                                                                                           country_code));
      }
    }

    return system_preferences;
  }

  void check_system_preferences(void) {
    if (!enabled_) {
      return;
    }

    auto v = make_system_preferences();
    if (!last_system_preferences_ || *last_system_preferences_ != v) {
      logger::get_logger().info("system_preferences is updated.");

      last_system_preferences_ = v;
      system_preferences_changed(v);
    }

    // Enqueue next check

    enqueue_to_dispatcher(
        [this] {
          check_system_preferences();
        },
        when_now() + std::chrono::milliseconds(3000));
  }

  std::weak_ptr<configuration_monitor> weak_configuration_monitor_;

  boost_utility::signals2_connections configuration_monitor_connections_;
  bool enabled_;
  boost::optional<system_preferences> last_system_preferences_;
};
} // namespace krbn
