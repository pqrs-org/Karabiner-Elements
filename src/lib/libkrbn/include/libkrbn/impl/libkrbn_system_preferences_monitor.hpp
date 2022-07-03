#pragma once

#include "libkrbn/libkrbn.h"
#include <pqrs/osx/system_preferences_monitor.hpp>

class libkrbn_system_preferences_monitor final {
public:
  libkrbn_system_preferences_monitor(const libkrbn_system_preferences_monitor&) = delete;

  libkrbn_system_preferences_monitor(libkrbn_system_preferences_monitor_callback callback,
                                     void* refcon) {
    krbn::logger::get_logger()->info(__func__);

    monitor_ = std::make_unique<pqrs::osx::system_preferences_monitor>(
        pqrs::dispatcher::extra::get_shared_dispatcher());

    monitor_->system_preferences_changed.connect([callback, refcon](auto&& properties_ptr) {
      if (callback && properties_ptr) {
        libkrbn_system_preferences_properties p;

        p.use_fkeys_as_standard_function_keys = properties_ptr->get_use_fkeys_as_standard_function_keys();

        auto keyboard_types = properties_ptr->get_keyboard_types();
        for (int i = 0; i < std::size(p.keyboard_types); ++i) {
          auto it = keyboard_types.find(
              pqrs::osx::system_preferences::keyboard_type_key(
                  krbn::hid::vendor_id::karabiner_virtual_hid_device,
                  krbn::hid::product_id::karabiner_virtual_hid_keyboard,
                  pqrs::hid::country_code::value_t(i)));
          if (it != std::end(keyboard_types)) {
            p.keyboard_types[i] = static_cast<int32_t>(type_safe::get(it->second));
          } else {
            p.keyboard_types[i] = -1;
          }
        }

        callback(&p, refcon);
      }
    });

    monitor_->async_start(std::chrono::milliseconds(3000));
  }

  ~libkrbn_system_preferences_monitor(void) {
    krbn::logger::get_logger()->info(__func__);
  }

private:
  std::unique_ptr<pqrs::osx::system_preferences_monitor> monitor_;
};
