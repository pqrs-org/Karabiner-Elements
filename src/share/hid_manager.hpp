#pragma once

// `krbn::hid_manager` can be used safely in a multi-threaded environment.

#include "boost_utility.hpp"
#include "cf_utility.hpp"
#include "device_detail.hpp"
#include "dispatcher.hpp"
#include "human_interface_device.hpp"
#include "logger.hpp"
#include "monitor/service_monitor.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDManager.h>
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
  boost::signals2::signal<bool(IOHIDDeviceRef _Nonnull), boost_utility::signals2_combiner_call_while_true> device_detecting;
  boost::signals2::signal<void(std::weak_ptr<human_interface_device>)> device_detected;
  boost::signals2::signal<void(std::weak_ptr<human_interface_device>)> device_removed;

  // Methods

  hid_manager(const hid_manager&) = delete;

  hid_manager(const std::vector<std::pair<hid_usage_page, hid_usage>>& usage_pairs) : dispatcher_client(),
                                                                                      usage_pairs_(usage_pairs),
                                                                                      refresh_timer_(*this) {
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

        auto monitor = std::make_shared<monitor::service_monitor::service_monitor>(dictionary);

        monitor->service_detected.connect([this](auto&& services) {
          for (const auto& service : services->get_services()) {
            if (devices_.find(service) == std::end(devices_)) {
              if (auto device = IOHIDDeviceCreate(kCFAllocatorDefault, service)) {
                devices_[service] = cf_utility::cf_ptr<IOHIDDeviceRef>(device);

                device_matching_callback(device);

                CFRelease(device);
              }
            }
          }
        });

        monitor->service_removed.connect([this](auto&& services) {
          for (const auto& service : services->get_services()) {
            auto it = devices_.find(service);
            if (it != std::end(devices_)) {
              device_removal_callback(*(it->second));

              devices_.erase(it);
            }
          }
        });

        monitor->async_start();

        service_monitors_.push_back(monitor);

        CFRelease(dictionary);
      }
    }

    refresh_timer_.start(
        [this] {
          refresh_if_needed();
        },
        std::chrono::milliseconds(5000));

    logger::get_logger().info("hid_manager is started.");
  }

  // This method is executed in the dispatcher thread.
  void stop(void) {
    refresh_timer_.stop();
    service_monitors_.clear();
    hids_.clear();
    registry_entry_ids_.clear();
    devices_.clear();
  }

  // This method is executed in the dispatcher thread.
  void refresh_if_needed(void) {
    // `device_removal_callback` is sometimes missed
    // and invalid human_interface_devices are remained into hids_.
    // (The problem is likely occur just after wake up from sleep.)
    //
    // We validate the human_interface_device,
    // and then reload devices if there is an invalid human_interface_device.

    bool needs_refresh = false;

    {
      std::lock_guard<std::mutex> lock(hids_mutex_);

      for (const auto& hid : hids_) {
        if (!hid->validate()) {
          logger::get_logger().warn("Refreshing hid_manager since a dangling human_interface_device is found. ({0})",
                                    hid->get_name_for_log());
          needs_refresh = true;
          break;
        }
      }
    }

    if (needs_refresh) {
      stop();
      start();
    }
  }

  boost::optional<registry_entry_id> find_registry_entry_id(IOHIDDeviceRef _Nonnull device) {
    auto it = registry_entry_ids_.find(device);
    if (it != std::end(registry_entry_ids_)) {
      return it->second;
    }
    return boost::none;
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    CFRetain(device);

    enqueue_to_dispatcher([this, device] {
      if (!device_detecting(device)) {
        goto finish;
      }

      if (auto registry_entry_id = iokit_utility::find_registry_entry_id(device)) {
        // iokit_utility::find_registry_entry_id will be failed in device_removal_callback at least on macOS 10.13.
        // Thus, we have to memory device and registry_entry_id correspondence by myself.
        registry_entry_ids_[device] = *registry_entry_id;

        // Skip if same device is already matched.
        // (Multiple usage device (e.g. usage::pointer and usage::mouse) will be matched twice.)

        std::shared_ptr<human_interface_device> hid;

        {
          std::lock_guard<std::mutex> lock(hids_mutex_);

          if (std::any_of(std::begin(hids_),
                          std::end(hids_),
                          [&](auto&& h) {
                            return *registry_entry_id == h->get_registry_entry_id();
                          })) {
            // logger::get_logger().info("registry_entry_id:{0} already exists", static_cast<uint64_t>(*registry_entry_id));
            goto finish;
          }

          hid = std::make_shared<human_interface_device>(device, *registry_entry_id);
          hids_.push_back(hid);
        }

        enqueue_to_dispatcher([this, hid] {
          device_detected(hid);
        });
      }

    finish:
      CFRelease(device);
    });
  }

  static void static_device_removal_callback(void* _Nullable context,
                                             IOReturn result,
                                             void* _Nullable sender,
                                             IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<hid_manager*>(context);
    if (!self) {
      return;
    }

    self->device_removal_callback(device);
  }

  void device_removal_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    CFRetain(device);

    enqueue_to_dispatcher([this, device] {
      if (auto registry_entry_id = find_registry_entry_id(device)) {
        std::shared_ptr<human_interface_device> hid;

        {
          std::lock_guard<std::mutex> lock(hids_mutex_);

          auto it = std::find_if(std::begin(hids_),
                                 std::end(hids_),
                                 [&](auto&& h) {
                                   return *registry_entry_id == h->get_registry_entry_id();
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

        // There might be multiple devices for one registry_entry_id.
        // (For example, keyboard and mouse combined device.)
        // And there is possibility that removal callback is not called with some device.
        //
        // For example, there are 3 devices for 1 registry_entry_id (4294974284).
        //   - device1 { device:0x7fb9d64078a0 => registry_entry_id:4294974284 }
        //   - device2 { device:0x7fb9d8301390 => registry_entry_id:4294974284 }
        //   - device3 { device:0x7fb9d830a630 => registry_entry_id:4294974284 }
        // And device_removal_callback are called only with device1 and device2.
        //
        // We should remove device1, device2 and device3 at the same time in order to deal with this case.

        for (auto it = std::begin(registry_entry_ids_); it != std::end(registry_entry_ids_);) {
          if (it->second == *registry_entry_id) {
            it = registry_entry_ids_.erase(it);
          } else {
            std::advance(it, 1);
          }
        }
      }

      CFRelease(device);
    });
  }

  std::vector<std::pair<hid_usage_page, hid_usage>> usage_pairs_;

  std::vector<std::shared_ptr<monitor::service_monitor::service_monitor>> service_monitors_;
  pqrs::dispatcher::extra::timer refresh_timer_;

  std::unordered_map<io_service_t, cf_utility::cf_ptr<IOHIDDeviceRef>> devices_;
  std::unordered_map<IOHIDDeviceRef, registry_entry_id> registry_entry_ids_;

  std::vector<std::shared_ptr<human_interface_device>> hids_;
  mutable std::mutex hids_mutex_;
}; // namespace krbn
} // namespace krbn
