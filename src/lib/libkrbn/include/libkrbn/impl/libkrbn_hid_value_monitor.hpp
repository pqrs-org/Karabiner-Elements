#pragma once

#include "event_queue.hpp"
#include "libkrbn/libkrbn.h"
#include "libkrbn_callback_manager.hpp"
#include <atomic>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>

class libkrbn_hid_value_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  libkrbn_hid_value_monitor(const libkrbn_hid_value_monitor&) = delete;

  libkrbn_hid_value_monitor(void)
      : dispatcher_client(),
        observed_(false) {
    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::keyboard),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::mouse),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::pointer),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::joystick),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::game_pad),

        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::consumer,
            pqrs::hid::usage::consumer::consumer_control),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                  pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
                                                                  matching_dictionaries);

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      if (device_ptr) {
        auto device_id = krbn::make_device_id(registry_entry_id);
        auto device_properties = krbn::device_properties::make_device_properties(device_id,
                                                                                 *device_ptr);

        auto hid_queue_value_monitor = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                                                  pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread(),
                                                                                                  *device_ptr);
        hid_queue_value_monitors_[device_id] = hid_queue_value_monitor;

        hid_queue_value_monitor->started.connect([this] {
          observed_ = true;
        });

        hid_queue_value_monitor->values_arrived.connect([this, device_id, device_properties](auto&& values_ptr) {
          values_arrived(device_id,
                         device_properties,
                         values_ptr);
        });

        hid_queue_value_monitor->async_start(kIOHIDOptionsTypeNone,
                                             std::chrono::milliseconds(3000));
      }
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      auto device_id = krbn::make_device_id(registry_entry_id);

      hid_queue_value_monitors_.erase(device_id);

      krbn::hat_switch_converter::get_global_hat_switch_converter()->erase_device(device_id);
    });

    hid_manager_->error_occurred.connect([](auto&& message, auto&& kern_return) {
      krbn::logger::get_logger()->error("{0}: {1}", message, kern_return.to_string());
    });

    hid_manager_->async_start();
  }

  ~libkrbn_hid_value_monitor(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;
      hid_queue_value_monitors_.clear();
    });
  }

  bool get_observed(void) const {
    return observed_;
  }

  void register_libkrbn_hid_value_arrived_callback(libkrbn_hid_value_arrived callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.register_callback(callback);
    });
  }

  void unregister_libkrbn_hid_value_arrived_callback(libkrbn_hid_value_arrived callback) {
    enqueue_to_dispatcher([this, callback] {
      callback_manager_.unregister_callback(callback);
    });
  }

private:
  void values_arrived(krbn::device_id device_id,
                      gsl::not_null<std::shared_ptr<krbn::device_properties>> device_properties,
                      std::shared_ptr<std::vector<pqrs::cf::cf_ptr<IOHIDValueRef>>> values) {
    for (const auto& value : *values) {
      auto v = pqrs::osx::iokit_hid_value(*value);

      if (auto usage_page = v.get_usage_page()) {
        if (auto usage = v.get_usage()) {
          if (auto logical_max = v.get_logical_max()) {
            if (auto logical_min = v.get_logical_min()) {
              for (const auto& c : callback_manager_.get_callbacks()) {
                c(type_safe::get(device_id),
                  device_properties->get_device_identifiers().get_is_keyboard(),
                  device_properties->get_device_identifiers().get_is_pointing_device(),
                  device_properties->get_device_identifiers().get_is_game_pad(),
                  type_safe::get(*usage_page),
                  type_safe::get(*usage),
                  *logical_max,
                  *logical_min,
                  v.get_integer_value());
              }
            }
          }
        }
      }
    }
  }

  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<krbn::device_id, std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor>> hid_queue_value_monitors_;
  std::atomic<bool> observed_;
  libkrbn_callback_manager<libkrbn_hid_value_arrived> callback_manager_;
};
