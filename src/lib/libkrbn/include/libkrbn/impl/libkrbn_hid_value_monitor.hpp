#pragma once

#include "event_queue.hpp"
#include "hid_queue_values_converter.hpp"
#include "libkrbn/libkrbn.h"
#include <atomic>
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>

class libkrbn_hid_value_monitor final {
public:
  libkrbn_hid_value_monitor(const libkrbn_hid_value_monitor&) = delete;

  libkrbn_hid_value_monitor(libkrbn_hid_value_monitor_callback callback,
                            void* refcon) : observed_(false) {
    krbn::logger::get_logger()->info(__func__);

    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::hid::usage_page::generic_desktop,
            pqrs::hid::usage::generic_desktop::keyboard),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                  matching_dictionaries,
                                                                  std::chrono::milliseconds(1000));

    hid_manager_->device_matched.connect([this, callback, refcon](auto&& registry_entry_id, auto&& device_ptr) {
      if (device_ptr) {
        auto device_id = krbn::make_device_id(registry_entry_id);
        auto device_properties = std::make_shared<krbn::device_properties>(device_id,
                                                                           *device_ptr);

        auto hid_queue_value_monitor = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                                                  *device_ptr);
        hid_queue_value_monitors_[device_id] = hid_queue_value_monitor;

        hid_queue_value_monitor->started.connect([this] {
          observed_ = true;
        });

        hid_queue_value_monitor->values_arrived.connect([this, callback, refcon, device_id](auto&& values_ptr) {
          auto event_queue = krbn::event_queue::utility::make_queue(device_id,
                                                                    hid_queue_values_converter_.make_hid_values(device_id,
                                                                                                                values_ptr));
          values_arrived(callback, refcon, event_queue);
        });

        hid_queue_value_monitor->async_start(kIOHIDOptionsTypeNone,
                                             std::chrono::milliseconds(3000));
      }
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      auto device_id = krbn::make_device_id(registry_entry_id);

      hid_queue_value_monitors_.erase(device_id);
      hid_queue_values_converter_.erase_device(device_id);
    });

    hid_manager_->error_occurred.connect([](auto&& message, auto&& kern_return) {
      krbn::logger::get_logger()->error("{0}: {1}", message, kern_return.to_string());
    });

    hid_manager_->async_start();
  }

  ~libkrbn_hid_value_monitor(void) {
    krbn::logger::get_logger()->info(__func__);

    hid_manager_ = nullptr;
    hid_queue_value_monitors_.clear();
  }

  bool get_observed(void) const {
    return observed_;
  }

private:
  void values_arrived(libkrbn_hid_value_monitor_callback callback,
                      void* refcon,
                      std::shared_ptr<krbn::event_queue::queue> event_queue) {
    for (const auto& entry : event_queue->get_entries()) {
      libkrbn_hid_value_event_type event_type = libkrbn_hid_value_event_type_key_down;
      switch (entry.get_event_type()) {
        case krbn::event_type::key_down:
          event_type = libkrbn_hid_value_event_type_key_down;
          break;
        case krbn::event_type::key_up:
          event_type = libkrbn_hid_value_event_type_key_up;
          break;
        case krbn::event_type::single:
          event_type = libkrbn_hid_value_event_type_single;
          break;
      }

      if (auto e = entry.get_event().get_if<krbn::momentary_switch_event>()) {
        if (auto usage_pair = e->make_usage_pair()) {
          if (callback) {
            callback(type_safe::get(entry.get_device_id()),
                     type_safe::get(usage_pair->get_usage_page()),
                     type_safe::get(usage_pair->get_usage()),
                     event_type,
                     refcon);
          }
        }
      }
    }
  }

  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<krbn::device_id, std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor>> hid_queue_value_monitors_;
  krbn::hid_queue_values_converter hid_queue_values_converter_;
  std::atomic<bool> observed_;
};
