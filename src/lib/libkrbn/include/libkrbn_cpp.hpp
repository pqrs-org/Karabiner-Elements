#pragma once

#include "dispatcher.hpp"
#include "libkrbn.h"
#include "monitor/configuration_monitor.hpp"

class libkrbn_cpp final {
public:
  static std::weak_ptr<pqrs::dispatcher::dispatcher> get_weak_dispatcher(void);

  static krbn::device_identifiers make_device_identifiers(const libkrbn_device_identifiers& device_identifiers) {
    krbn::device_identifiers identifiers(krbn::vendor_id(device_identifiers.vendor_id),
                                         krbn::product_id(device_identifiers.product_id),
                                         device_identifiers.is_keyboard,
                                         device_identifiers.is_pointing_device);
    return identifiers;
  }

  class libkrbn_core_configuration_class final {
  public:
    libkrbn_core_configuration_class(std::shared_ptr<krbn::core_configuration> core_configuration) : core_configuration_(core_configuration) {
    }

    krbn::core_configuration& get_core_configuration(void) {
      return *core_configuration_;
    }

  private:
    std::shared_ptr<krbn::core_configuration> core_configuration_;
  };

  class libkrbn_configuration_monitor_class final {
  public:
    libkrbn_configuration_monitor_class(const libkrbn_configuration_monitor_class&) = delete;

    libkrbn_configuration_monitor_class(libkrbn_configuration_monitor_callback callback, void* refcon) {
      configuration_monitor_ = std::make_shared<krbn::configuration_monitor>(
          krbn::constants::get_user_core_configuration_file_path());

      configuration_monitor_->core_configuration_updated.connect([callback, refcon](auto&& weak_core_configuration) {
        if (auto core_configuration = weak_core_configuration.lock()) {
          if (callback) {
            auto* p = new libkrbn_core_configuration_class(core_configuration);
            callback(p, refcon);
          }
        }
      });

      configuration_monitor_->async_start();
    }

    std::shared_ptr<krbn::configuration_monitor> get_configuration_monitor(void) const {
      return configuration_monitor_;
    }

  private:
    std::shared_ptr<krbn::configuration_monitor> configuration_monitor_;
  };
};
