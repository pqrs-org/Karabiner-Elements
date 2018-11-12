#pragma once

// `krbn::hid_manager` can be used safely in a multi-threaded environment.

#include "boost_utility.hpp"
#include "cf_utility.hpp"
#include "device_detail.hpp"
#include "human_interface_device.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <pqrs/cf_ptr.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_service_monitor.hpp>
#include <unordered_map>

namespace krbn {

// We implement hid_manager without IOHIDManager.
//
// IOHIDManager opens connected device automatically, and we cannot stop it.
// We want to prevent opening device depending the `device_detecting` result.
// Thus, we cannot use IOHIDManager to this purpose.

class hid_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  // Return false to ignore device.
  boost::signals2::signal<bool(pqrs::osx::iokit_registry_entry_id, IOHIDDeviceRef _Nonnull), boost_utility::signals2_combiner_call_while_true> device_detecting;
  boost::signals2::signal<void(std::weak_ptr<human_interface_device>)> device_detected;
  boost::signals2::signal<void(std::weak_ptr<human_interface_device>)> device_removed;

  // Methods

  hid_manager(const hid_manager&) = delete;

  hid_manager(const std::vector<std::pair<hid_usage_page, hid_usage>>& usage_pairs) : dispatcher_client(),
                                                                                      usage_pairs_(usage_pairs) {
  }

  virtual ~hid_manager(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      start();
    });
  }

  std::vector<std::shared_ptr<human_interface_device>> copy_hids(void) const {
    std::lock_guard<std::mutex> lock(hids_mutex_);

    return hids_;
  }

private:
  // This method is executed in the dispatcher thread.
  void start(void) {
    for (const auto& pair : usage_pairs_) {
      if (auto dictionary = IOServiceMatching(kIOHIDDeviceKey)) {
        cf_utility::set_cfmutabledictionary_value(dictionary,
                                                  CFSTR(kIOHIDDeviceUsagePageKey),
                                                  static_cast<int64_t>(pair.first));
        cf_utility::set_cfmutabledictionary_value(dictionary,
                                                  CFSTR(kIOHIDDeviceUsageKey),
                                                  static_cast<int64_t>(pair.second));

        auto monitor = std::make_shared<pqrs::osx::iokit_service_monitor>(weak_dispatcher_,
                                                                          dictionary);

        monitor->service_detected.connect([this](auto&& registry_entry_id, auto&& service_ptr) {
          if (devices_.find(registry_entry_id) == std::end(devices_)) {
            if (auto device = IOHIDDeviceCreate(kCFAllocatorDefault, *service_ptr)) {
              devices_[registry_entry_id] = pqrs::cf_ptr<IOHIDDeviceRef>(device);

              if (device_detecting(registry_entry_id, device)) {
                auto hid = std::make_shared<human_interface_device>(device, registry_entry_id);

                {
                  std::lock_guard<std::mutex> lock(hids_mutex_);

                  hids_.push_back(hid);
                }

                enqueue_to_dispatcher([this, hid] {
                  device_detected(hid);
                });
              }
            }
          }
        });

        monitor->service_removed.connect([this](auto&& registry_entry_id) {
          auto it = devices_.find(registry_entry_id);
          if (it != std::end(devices_)) {
            std::shared_ptr<human_interface_device> hid;

            {
              std::lock_guard<std::mutex> lock(hids_mutex_);

              auto it = std::find_if(std::begin(hids_),
                                     std::end(hids_),
                                     [&](auto&& h) {
                                       return registry_entry_id == h->get_registry_entry_id();
                                     });
              if (it != hids_.end()) {
                hid = *it;
                hids_.erase(it);
              }
            }

            if (hid) {
              hid->set_removed();

              enqueue_to_dispatcher([this, hid] {
                device_removed(hid);
              });
            }

            devices_.erase(it);
          }
        });

        monitor->async_start();

        service_monitors_.push_back(monitor);

        CFRelease(dictionary);
      }
    }

    logger::get_logger().info("hid_manager is started.");
  }

  // This method is executed in the dispatcher thread.
  void stop(void) {
    service_monitors_.clear();
    devices_.clear();

    {
      std::lock_guard<std::mutex> lock(hids_mutex_);

      hids_.clear();
    }
  }

  std::vector<std::pair<hid_usage_page, hid_usage>> usage_pairs_;

  std::vector<std::shared_ptr<pqrs::osx::iokit_service_monitor>> service_monitors_;
  std::unordered_map<pqrs::osx::iokit_registry_entry_id, pqrs::cf_ptr<IOHIDDeviceRef>> devices_;
  std::vector<std::shared_ptr<human_interface_device>> hids_;
  mutable std::mutex hids_mutex_;
}; // namespace krbn
} // namespace krbn
