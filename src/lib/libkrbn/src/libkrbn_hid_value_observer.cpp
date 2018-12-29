#include "device_properties_manager.hpp"
#include "event_queue.hpp"
#include "libkrbn.h"
#include "libkrbn_cpp.hpp"
#include <pqrs/osx/iokit_hid_manager.hpp>
#include <pqrs/osx/iokit_hid_queue_value_monitor.hpp>
#include <unordered_set>

namespace {
class libkrbn_hid_value_observer_class final {
public:
  libkrbn_hid_value_observer_class(const libkrbn_hid_value_observer_class&) = delete;

  libkrbn_hid_value_observer_class(libkrbn_hid_value_observer_callback callback,
                                   void* refcon) : callback_(callback),
                                                   refcon_(refcon) {
    std::vector<pqrs::cf::cf_ptr<CFDictionaryRef>> matching_dictionaries{
        pqrs::osx::iokit_hid_manager::make_matching_dictionary(
            pqrs::osx::iokit_hid_usage_page_generic_desktop,
            pqrs::osx::iokit_hid_usage_generic_desktop_keyboard),
    };

    hid_manager_ = std::make_unique<pqrs::osx::iokit_hid_manager>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                  matching_dictionaries);

    hid_manager_->device_matched.connect([this](auto&& registry_entry_id, auto&& device_ptr) {
      if (device_ptr) {
        auto device_id = krbn::make_device_id(registry_entry_id);
        auto device_properties = std::make_shared<krbn::device_properties>(device_id,
                                                                           *device_ptr);

        auto hid_queue_value_monitor = std::make_shared<pqrs::osx::iokit_hid_queue_value_monitor>(pqrs::dispatcher::extra::get_shared_dispatcher(),
                                                                                                  *device_ptr);
        hid_queue_value_monitors_[device_id] = hid_queue_value_monitor;

        hid_queue_value_monitor->started.connect([this, device_id] {
          std::lock_guard<std::mutex> lock(observed_devices_mutex_);

          observed_devices_.insert(device_id);
        });

        hid_queue_value_monitor->values_arrived.connect([this, device_id](auto&& values_ptr) {
          auto event_queue = krbn::event_queue::utility::make_queue(device_id,
                                                                    krbn::iokit_utility::make_hid_values(values_ptr));
          values_arrived(event_queue);
        });

        hid_queue_value_monitor->async_start(kIOHIDOptionsTypeNone,
                                             std::chrono::milliseconds(3000));
      }
    });

    hid_manager_->device_terminated.connect([this](auto&& registry_entry_id) {
      auto device_id = krbn::make_device_id(registry_entry_id);

      hid_queue_value_monitors_.erase(device_id);

      {
        std::lock_guard<std::mutex> lock(observed_devices_mutex_);

        observed_devices_.erase(device_id);
      }
    });

    hid_manager_->error_occurred.connect([](auto&& message, auto&& iokit_return) {
      krbn::logger::get_logger()->error("{0}: {1}", message, iokit_return.to_string());
    });

    hid_manager_->async_start();
  }

  ~libkrbn_hid_value_observer_class(void) {
    hid_manager_ = nullptr;
    hid_queue_value_monitors_.clear();
  }

  size_t calculate_observed_device_count(void) const {
    std::lock_guard<std::mutex> lock(observed_devices_mutex_);

    return observed_devices_.size();
  }

private:
  void values_arrived(std::shared_ptr<krbn::event_queue::queue> event_queue) {
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

      switch (entry.get_event().get_type()) {
        case krbn::event_queue::event::type::key_code:
          if (auto key_code = entry.get_event().get_key_code()) {
            callback_(libkrbn_hid_value_type_key_code,
                      static_cast<uint32_t>(*key_code),
                      event_type,
                      refcon_);
          }
          break;

        case krbn::event_queue::event::type::consumer_key_code:
          if (auto consumer_key_code = entry.get_event().get_consumer_key_code()) {
            callback_(libkrbn_hid_value_type_consumer_key_code,
                      static_cast<uint32_t>(*consumer_key_code),
                      event_type,
                      refcon_);
          }
          break;

        default:
          break;
      }
    }
  }

  libkrbn_hid_value_observer_callback callback_;
  void* refcon_;

  std::unique_ptr<pqrs::osx::iokit_hid_manager> hid_manager_;
  std::unordered_map<krbn::device_id, std::shared_ptr<pqrs::osx::iokit_hid_queue_value_monitor>> hid_queue_value_monitors_;

  std::unordered_set<krbn::device_id> observed_devices_;
  mutable std::mutex observed_devices_mutex_;
};
} // namespace

bool libkrbn_hid_value_observer_initialize(libkrbn_hid_value_observer** out, libkrbn_hid_value_observer_callback callback, void* refcon) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_hid_value_observer*>(new libkrbn_hid_value_observer_class(callback, refcon));
  return true;
}

void libkrbn_hid_value_observer_terminate(libkrbn_hid_value_observer** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_hid_value_observer_class*>(*p);
    *p = nullptr;
  }
}

size_t libkrbn_hid_value_observer_calculate_observed_device_count(libkrbn_hid_value_observer* p) {
  if (p) {
    if (auto o = reinterpret_cast<libkrbn_hid_value_observer_class*>(p)) {
      return o->calculate_observed_device_count();
    }
  }
  return 0;
}
