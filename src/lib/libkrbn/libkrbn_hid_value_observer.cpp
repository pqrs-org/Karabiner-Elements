#include "boost_defs.hpp"

#include "human_interface_device.hpp"
#include "libkrbn.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/bind.hpp>

namespace {
class libkrbn_hid_value_observer_class final {
public:
  libkrbn_hid_value_observer_class(const libkrbn_hid_value_observer_class&) = delete;

  libkrbn_hid_value_observer_class(libkrbn_hid_value_observer_callback callback) : callback_(callback) {
    manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager_) {
      return;
    }

    auto device_matching_dictionaries = krbn::iokit_utility::create_device_matching_dictionaries({
        std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_keyboard),
        // std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_mouse),
        // std::make_pair(krbn::hid_usage_page::generic_desktop, krbn::hid_usage::gd_pointer),
    });
    if (device_matching_dictionaries) {
      IOHIDManagerSetDeviceMatchingMultiple(manager_, device_matching_dictionaries);
      CFRelease(device_matching_dictionaries);

      IOHIDManagerRegisterDeviceMatchingCallback(manager_, static_device_matching_callback, this);
      IOHIDManagerRegisterDeviceRemovalCallback(manager_, static_device_removal_callback, this);

      IOHIDManagerScheduleWithRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    }
  }

  ~libkrbn_hid_value_observer_class(void) {
    if (manager_) {
      IOHIDManagerUnscheduleFromRunLoop(manager_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
      CFRelease(manager_);
      manager_ = nullptr;
    }
  }

private:
  static void static_device_matching_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess || !device) {
      return;
    }

    auto self = static_cast<libkrbn_hid_value_observer_class*>(context);
    if (!self) {
      return;
    }

    self->device_matching_callback(device);
  }

  void device_matching_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    krbn::iokit_utility::log_matching_device(device);

    hids_[device] = std::make_unique<krbn::human_interface_device>(device);
    auto& dev = hids_[device];
    dev->set_value_callback(boost::bind(&libkrbn_hid_value_observer_class::value_callback, this, _1, _2));
    dev->observe();
  }

  static void static_device_removal_callback(void* _Nullable context, IOReturn result, void* _Nullable sender, IOHIDDeviceRef _Nonnull device) {
    if (result != kIOReturnSuccess || !device) {
      return;
    }

    auto self = static_cast<libkrbn_hid_value_observer_class*>(context);
    if (!self) {
      return;
    }

    self->device_removal_callback(device);
  }

  void device_removal_callback(IOHIDDeviceRef _Nonnull device) {
    if (!device) {
      return;
    }

    krbn::iokit_utility::log_removal_device(device);

    auto it = hids_.find(device);
    if (it != hids_.end()) {
      auto& dev = it->second;
      if (dev) {
        hids_.erase(it);
      }
    }
  }

  void value_callback(krbn::human_interface_device& device,
                      krbn::event_queue& event_queue) {
    for (const auto& queued_event : event_queue.get_events()) {
      libkrbn_hid_value_event_type event_type = libkrbn_hid_value_event_type_key_down;
      switch (queued_event.get_event_type()) {
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

      switch (queued_event.get_event().get_type()) {
        case krbn::event_queue::queued_event::event::type::key_code:
          if (auto key_code = queued_event.get_event().get_key_code()) {
            callback_(libkrbn_hid_value_type_key_code,
                      static_cast<uint32_t>(*key_code),
                      event_type);
          }
          break;

        case krbn::event_queue::queued_event::event::type::consumer_key_code:
          if (auto consumer_key_code = queued_event.get_event().get_consumer_key_code()) {
            callback_(libkrbn_hid_value_type_consumer_key_code,
                      static_cast<uint32_t>(*consumer_key_code),
                      event_type);
          }
          break;

        case krbn::event_queue::queued_event::event::type::none:
        case krbn::event_queue::queued_event::event::type::pointing_button:
        case krbn::event_queue::queued_event::event::type::pointing_motion:
        case krbn::event_queue::queued_event::event::type::shell_command:
        case krbn::event_queue::queued_event::event::type::select_input_source:
        case krbn::event_queue::queued_event::event::type::set_variable:
        case krbn::event_queue::queued_event::event::type::mouse_key:
        case krbn::event_queue::queued_event::event::type::stop_keyboard_repeat:
        case krbn::event_queue::queued_event::event::type::device_keys_and_pointing_buttons_are_released:
        case krbn::event_queue::queued_event::event::type::device_ungrabbed:
        case krbn::event_queue::queued_event::event::type::caps_lock_state_changed:
        case krbn::event_queue::queued_event::event::type::event_from_ignored_device:
        case krbn::event_queue::queued_event::event::type::pointing_device_event_from_event_tap:
        case krbn::event_queue::queued_event::event::type::frontmost_application_changed:
        case krbn::event_queue::queued_event::event::type::input_source_changed:
        case krbn::event_queue::queued_event::event::type::keyboard_type_changed:
          break;
      }
    }

    event_queue.clear_events();
  }

  libkrbn_hid_value_observer_callback callback_;

  IOHIDManagerRef _Nullable manager_;
  std::unordered_map<IOHIDDeviceRef, std::unique_ptr<krbn::human_interface_device>> hids_;
};
} // namespace

bool libkrbn_hid_value_observer_initialize(libkrbn_hid_value_observer** out, libkrbn_hid_value_observer_callback callback) {
  if (!out) return false;
  // return if already initialized.
  if (*out) return false;

  *out = reinterpret_cast<libkrbn_hid_value_observer*>(new libkrbn_hid_value_observer_class(callback));
  return true;
}

void libkrbn_hid_value_observer_terminate(libkrbn_hid_value_observer** p) {
  if (p && *p) {
    delete reinterpret_cast<libkrbn_hid_value_observer_class*>(*p);
    *p = nullptr;
  }
}
