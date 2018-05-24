#pragma once

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "configuration_monitor.hpp"
#include "constants.hpp"
#include "device_detail.hpp"
#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "iokit_utility.hpp"
#include "json_utility.hpp"
#include "krbn_notification_center.hpp"
#include "logger.hpp"
#include "spdlog_utility.hpp"
#include "system_preferences_utility.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <time.h>

namespace krbn {
class device_observer final {
public:
  device_observer(const device_observer&) = delete;

  device_observer(void) {
    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      logger::get_logger().error("{0}: failed to IOHIDManagerCreate", __PRETTY_FUNCTION__);
      return;
    }

    auto device_matching_dictionaries = iokit_utility::create_device_matching_dictionaries({
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_keyboard),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_mouse),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_pointer),
    });
    if (device_matching_dictionaries) {
      IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
      CFRelease(device_matching_dictionaries);

      IOHIDManagerRegisterDeviceMatchingCallback(manager_, static_device_matching_callback, this);
      IOHIDManagerRegisterDeviceRemovalCallback(manager_, static_device_removal_callback, this);

      IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    }
  }

  ~device_observer(void) {
    // Release manager_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (manager_) {
        IOHIDManagerUnscheduleFromRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
        CFRelease(manager_);
        manager_ = nullptr;
      }
    });
  }

private:
  static void static_device_matching_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<device_observer*>(context);
    if (!self) {
      return;
    }

    self->device_matching_callback(device);
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    if (iokit_utility::is_karabiner_virtual_hid_device(device)) {
      return;
    }

    iokit_utility::log_matching_device(device);

    if (auto registry_entry_id = iokit_utility::find_registry_entry_id(device)) {
      registry_entry_ids_[device] = *registry_entry_id;

      // Skip if same device is already matched.
      // (Multiple usage device (e.g. usage::pointer and usage::mouse) will be matched twice.)
      auto it = hids_.find(*registry_entry_id);
      if (it != std::end(hids_)) {
        logger::get_logger().info("registry_entry_id:{0} already exists.", static_cast<uint64_t>(*registry_entry_id));
        return;
      }

      auto dev = std::make_unique<human_interface_device>(device);
      dev->set_value_callback(std::bind(&device_observer::value_callback,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));
      logger::get_logger().info("{0} is detected.", dev->get_name_for_log());

      dev->observe();

      hids_[*registry_entry_id] = std::move(dev);
    }
  }

  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<device_observer*>(context);
    if (!self) {
      return;
    }

    self->device_removal_callback(device);
  }

  void device_removal_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    iokit_utility::log_removal_device(device);

    // ----------------------------------------

    registry_entry_id registry_entry_id = registry_entry_id::zero;

    {
      auto it = registry_entry_ids_.find(device);
      if (it == std::end(registry_entry_ids_)) {
        return;
      }

      registry_entry_id = it->second;
      registry_entry_ids_.erase(device);
    }

    // ----------------------------------------

    {
      auto it = hids_.find(registry_entry_id);
      if (it != hids_.end()) {
        auto& dev = it->second;
        if (dev) {
          logger::get_logger().info("{0} is removed.", dev->get_name_for_log());
          dev->set_removed();
          hids_.erase(it);
        }
      }
    }
  }

  void value_callback(human_interface_device& device,
                      event_queue& event_queue) {
  }

  IOHIDManagerRef _Nullable manager_;

  std::unordered_map<IOHIDDeviceRef, registry_entry_id> registry_entry_ids_;

  std::unordered_map<registry_entry_id, std::unique_ptr<human_interface_device>> hids_;

  std::shared_ptr<event_queue> merged_input_event_queue_;
};
} // namespace krbn
