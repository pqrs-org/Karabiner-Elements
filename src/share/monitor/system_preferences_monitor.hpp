#pragma once

// `krbn::system_preferences_monitor` can be used safely in a multi-threaded environment.

#include "boost_utility.hpp"
#include "configuration_monitor.hpp"
#include "logger.hpp"
#include "system_preferences_utility.hpp"
#include "thread_utility.hpp"

namespace krbn {
class system_preferences_monitor final {
public:
  // Signals

  boost::signals2::signal<void(const system_preferences& value)> system_preferences_changed;

  // Methods

  system_preferences_monitor(std::weak_ptr<configuration_monitor> weak_configuration_monitor) : weak_configuration_monitor_(weak_configuration_monitor) {
    dispatcher_ = std::make_unique<thread_utility::dispatcher>();

    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      // core_configuration_updated
      {
        auto c = configuration_monitor->core_configuration_updated.connect([this](auto&&) {
          dispatcher_->enqueue([this] {
            check_system_preferences();
          });
        });
        configuration_monitor_connections_.push_back(c);
      }
    }
  }

  ~system_preferences_monitor(void) {
    // Disconnect `configuration_monitor_connections_`.

    if (auto configuration_monitor = weak_configuration_monitor_.lock()) {
      configuration_monitor->get_run_loop_thread()->enqueue(^{
        configuration_monitor_connections_.disconnect_all_connections();
      });
    } else {
      configuration_monitor_connections_.disconnect_all_connections();
    }

    configuration_monitor_connections_.wait_disconnect_all_connections();

    // Destroy timer_

    dispatcher_->enqueue([this] {
      if (timer_) {
        timer_->cancel();
      }
      timer_ = nullptr;
    });

    // Destroy dispatcher_

    dispatcher_->terminate();
    dispatcher_ = nullptr;

    logger::get_logger().info("system_preferences_monitor is stopped.");
  }

  void async_start(void) {
    dispatcher_->enqueue([this] {
      if (timer_) {
        return;
      }

      timer_ = std::make_unique<thread_utility::timer>(
          [](auto&& count) {
            if (count == 0) {
              return std::chrono::milliseconds(0);
            } else {
              return std::chrono::milliseconds(3000);
            }
          },
          thread_utility::timer::mode::repeat,
          [this] {
            dispatcher_->enqueue([this] {
              check_system_preferences();
            });
          });

      logger::get_logger().info("system_preferences_monitor is started.");
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
    auto v = make_system_preferences();
    if (!last_system_preferences_ || *last_system_preferences_ != v) {
      logger::get_logger().info("system_preferences is updated.");

      last_system_preferences_ = v;
      system_preferences_changed(v);
    }
  }

  std::weak_ptr<configuration_monitor> weak_configuration_monitor_;

  std::unique_ptr<thread_utility::dispatcher> dispatcher_;
  boost_utility::signals2_connections configuration_monitor_connections_;
  std::unique_ptr<thread_utility::timer> timer_;
  boost::optional<system_preferences> last_system_preferences_;
};
} // namespace krbn
