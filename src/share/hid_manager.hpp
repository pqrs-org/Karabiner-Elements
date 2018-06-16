#pragma once

#include "boost_defs.hpp"

#include "gcd_utility.hpp"
#include "human_interface_device.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <boost/signals2.hpp>
#include <unordered_map>

namespace krbn {
class hid_manager final {
public:
  boost::signals2::signal<void(human_interface_device&)> device_detected;
  boost::signals2::signal<void(human_interface_device&)> device_removed;

  hid_manager(const hid_manager&) = delete;

  hid_manager(void) : manager_(nullptr) {
  }

  ~hid_manager(void) {
    stop();
  }

  void start(const std::vector<std::pair<hid_usage_page, hid_usage>>& usage_pairs) {
    if (manager_) {
      stop();
    }

    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      logger::get_logger().error("{0}: failed to IOHIDManagerCreate", __PRETTY_FUNCTION__);
      return;
    }

    if (auto device_matching_dictionaries = iokit_utility::create_device_matching_dictionaries(usage_pairs)) {
      IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
      CFRelease(device_matching_dictionaries);
    }

    IOHIDManagerRegisterDeviceMatchingCallback(manager_, static_device_matching_callback, this);
    IOHIDManagerRegisterDeviceRemovalCallback(manager_, static_device_removal_callback, this);

    IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
  }

  void stop(void) {
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
  static void static_device_matching_callback(void* _Nullable context,
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

    self->device_matching_callback(device);
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
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

      auto hid = std::make_shared<human_interface_device>(device);
      hids_[*registry_entry_id] = hid;

      logger::get_logger().info("{0} is detected.", hid->get_name_for_log());

      device_detected(*hid);
    }
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

    iokit_utility::log_removal_device(device);

    // ----------------------------------------

    if (auto registry_entry_id = find_registry_entry_id(device)) {
      auto it = hids_.find(*registry_entry_id);
      if (it != hids_.end()) {
        if (auto hid = it->second) {
          logger::get_logger().info("{0} is removed.", hid->get_name_for_log());

          hid->set_removed();
          device_removed(*hid);
          hids_.erase(it);
        }
      }
    }

    registry_entry_ids_.erase(device);
  }

  boost::optional<registry_entry_id> find_registry_entry_id(IOHIDDeviceRef _Nonnull device) {
    auto it = registry_entry_ids_.find(device);
    if (it != std::end(registry_entry_ids_)) {
      return it->second;
    }
    return boost::none;
  }

  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, registry_entry_id> registry_entry_ids_;
  std::unordered_map<registry_entry_id, std::shared_ptr<human_interface_device>> hids_;
};
} // namespace krbn
