#pragma once

// `krbn::hid_manager` can be used safely in a multi-threaded environment.

#include "boost_utility.hpp"
#include "cf_utility.hpp"
#include "device_detail.hpp"
#include "human_interface_device.hpp"
#include "logger.hpp"
#include "thread_utility.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDManager.h>
#include <unordered_map>

namespace krbn {
class hid_manager final {
public:
  // Signals

  // Return false to ignore device.
  boost::signals2::signal<bool(IOHIDDeviceRef _Nonnull),
                          boost_utility::signals2_combiner_call_while_true>
      device_detecting;

  boost::signals2::signal<void(std::weak_ptr<human_interface_device>)> device_detected;
  boost::signals2::signal<void(std::weak_ptr<human_interface_device>)> device_removed;

  // Methods

  hid_manager(const hid_manager&) = delete;

  hid_manager(const std::vector<std::pair<hid_usage_page, hid_usage>>& usage_pairs) : usage_pairs_(usage_pairs),
                                                                                      manager_(nullptr) {
    run_loop_thread_ = std::make_shared<cf_utility::run_loop_thread>();
  }

  ~hid_manager(void) {
    stop();

    run_loop_thread_ = nullptr;
  }

  void start(void) {
    run_loop_thread_->enqueue(^{
      if (manager_) {
        return;
      }

      manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
      if (!manager_) {
        logger::get_logger().error("{0}: failed to IOHIDManagerCreate", __PRETTY_FUNCTION__);
        return;
      }

      if (auto device_matching_dictionaries = iokit_utility::create_device_matching_dictionaries(usage_pairs_)) {
        IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
        CFRelease(device_matching_dictionaries);
      }

      IOHIDManagerRegisterDeviceMatchingCallback(manager_, static_device_matching_callback, this);
      IOHIDManagerRegisterDeviceRemovalCallback(manager_, static_device_removal_callback, this);

      IOHIDManagerScheduleWithRunLoop(manager_,
                                      run_loop_thread_->get_run_loop(),
                                      kCFRunLoopDefaultMode);

      refresh_timer_ = std::make_unique<thread_utility::timer>(
          std::chrono::milliseconds(5000),
          true,
          [this] {
            run_loop_thread_->enqueue(^{
              refresh_if_needed();
            });
          });

      logger::get_logger().info("hid_manager is started.");
    });
  }

  void stop(void) {
    run_loop_thread_->enqueue(^{
      if (!manager_) {
        return;
      }

      // refresh_timer_

      refresh_timer_ = nullptr;

      // manager_

      if (manager_) {
        IOHIDManagerUnscheduleFromRunLoop(manager_,
                                          run_loop_thread_->get_run_loop(),
                                          kCFRunLoopDefaultMode);
        CFRelease(manager_);
        manager_ = nullptr;
      }

      // Other variables

      registry_entry_ids_.clear();

      {
        std::lock_guard<std::mutex> lock(hids_mutex_);

        hids_.clear();
      }

      logger::get_logger().info("hid_manager is stopped.");
    });
  }

  std::vector<std::shared_ptr<human_interface_device>> copy_hids(void) const {
    std::lock_guard<std::mutex> lock(hids_mutex_);

    return hids_;
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
    // `static_device_matching_callback` is called by multiple threads.
    // (It is not called from `run_loop_thread_`.)
    // Thus, we have to synchronize by using `run_loop_thread_`.

    run_loop_thread_->enqueue(^{
      if (!device) {
        return;
      }

      if (!device_detecting(device)) {
        return;
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
            return;
          }

          hid = std::make_shared<human_interface_device>(device, *registry_entry_id);
          hids_.push_back(hid);
        }

        device_detected(hid);
      }
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
    // `static_device_removal_callback` is called by multiple threads.
    // (It is not called from `run_loop_thread_`.)
    // Thus, we have to synchronize by using `run_loop_thread_`.

    run_loop_thread_->enqueue(^{
      if (!device) {
        return;
      }

      // ----------------------------------------

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
          device_removed(hid);
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
    });
  }

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

  std::shared_ptr<cf_utility::run_loop_thread> run_loop_thread_;

  std::vector<std::pair<hid_usage_page, hid_usage>> usage_pairs_;

  IOHIDManagerRef _Nullable manager_;
  std::unique_ptr<thread_utility::timer> refresh_timer_;

  std::unordered_map<IOHIDDeviceRef, registry_entry_id> registry_entry_ids_;
  // We do not need to use registry_entry_ids_mutex_ since it is modified only in run_loop_thread_.
  std::vector<std::shared_ptr<human_interface_device>> hids_;
  mutable std::mutex hids_mutex_;
};
} // namespace krbn
