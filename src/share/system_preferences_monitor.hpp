#pragma once

#include "boost_defs.hpp"

#include "configuration_monitor.hpp"
#include "gcd_utility.hpp"
#include "logger.hpp"
#include "system_preferences_utility.hpp"
#include <boost/optional.hpp>

namespace krbn {
class system_preferences_monitor final {
public:
  typedef std::function<void(const system_preferences& value)> system_preferences_updated_callback;

  system_preferences_monitor(const system_preferences_updated_callback& callback,
                             std::weak_ptr<configuration_monitor> configuration_monitor) : callback_(callback),
                                                                                           configuration_monitor_(configuration_monitor) {
    timer_ = std::make_unique<gcd_utility::main_queue_timer>(
        dispatch_time(DISPATCH_TIME_NOW, 3.0 * NSEC_PER_SEC),
        3.0 * NSEC_PER_SEC,
        0,
        ^{
          auto v = make_system_preferences();
          if (!system_preferences_ || *system_preferences_ != v) {
            logger::get_logger().info("system_preferences is updated.");

            system_preferences_ = v;
            if (callback_) {
              callback_(*system_preferences_);
            }
          }
        });
  }

  ~system_preferences_monitor(void) {
    timer_ = nullptr;
  }

private:
  system_preferences make_system_preferences(void) const {
    system_preferences system_preferences;

    if (auto configuration_monitor_ptr = configuration_monitor_.lock()) {
      auto core_configuration = configuration_monitor_ptr->get_core_configuration();
      auto country_code = core_configuration->get_selected_profile().get_virtual_hid_keyboard().get_country_code();

      system_preferences.set_keyboard_fn_state(system_preferences_utility::get_keyboard_fn_state());
      system_preferences.set_swipe_scroll_direction(system_preferences_utility::get_swipe_scroll_direction());
      system_preferences.set_keyboard_type(system_preferences_utility::get_keyboard_type(vendor_id(0x16c0),
                                                                                         product_id(0x27db),
                                                                                         country_code));
    }

    return system_preferences;
  }

  system_preferences_updated_callback callback_;
  std::weak_ptr<configuration_monitor> configuration_monitor_;

  std::unique_ptr<gcd_utility::main_queue_timer> timer_;

  boost::optional<system_preferences> system_preferences_;
};
} // namespace krbn
