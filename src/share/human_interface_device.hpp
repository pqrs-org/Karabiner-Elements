#pragma once

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "cf_utility.hpp"
#include "connected_devices.hpp"
#include "core_configuration.hpp"
#include "event_queue.hpp"
#include "gcd_utility.hpp"
#include "iokit_utility.hpp"
#include "spdlog_utility.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hid/IOHIDValue.h>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <functional>
#include <list>
#include <mach/mach_time.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

namespace krbn {
class human_interface_device final {
public:
  enum class grabbable_state {
    grabbable,
    ungrabbable_temporarily,
    ungrabbable_permanently,
  };

  typedef std::function<void(human_interface_device& device,
                             event_queue& event_queue)>
      value_callback;

  typedef std::function<void(human_interface_device& device,
                             IOHIDReportType type,
                             uint32_t report_id,
                             uint8_t* _Nonnull report,
                             CFIndex report_length)>
      report_callback;

  typedef std::function<grabbable_state(human_interface_device& device)> is_grabbable_callback;

  typedef std::function<void(human_interface_device& device)> grabbed_callback;
  typedef std::function<void(human_interface_device& device)> ungrabbed_callback;
  typedef std::function<void(human_interface_device& device)> disabled_callback;

  human_interface_device(const human_interface_device&) = delete;

  human_interface_device(spdlog::logger& logger,
                         IOHIDDeviceRef _Nonnull device) : logger_(logger),
                                                           device_(device),
                                                           device_id_(types::get_new_device_id()),
                                                           queue_(nullptr),
                                                           is_grabbable_callback_log_reducer_(logger),
                                                           observed_(false),
                                                           grabbed_(false),
                                                           disabled_(false) {
    // ----------------------------------------
    // Retain device_

    CFRetain(device_);

    // Create connected_device_.
    {
      std::string manufacturer;
      std::string product;
      if (auto m = iokit_utility::get_manufacturer(device_)) {
        manufacturer = *m;
      }
      if (auto p = iokit_utility::get_product(device_)) {
        product = *p;
      }
      connected_devices::device::descriptions descriptions(manufacturer, product);

      auto vendor_id = vendor_id::zero;
      auto product_id = product_id::zero;
      bool is_keyboard = IOHIDDeviceConformsTo(device_, kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
      bool is_pointing_device = IOHIDDeviceConformsTo(device_, kHIDPage_GenericDesktop, kHIDUsage_GD_Pointer) ||
                                IOHIDDeviceConformsTo(device_, kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse);
      if (auto v = iokit_utility::get_vendor_id(device_)) {
        vendor_id = *v;
      }
      if (auto p = iokit_utility::get_product_id(device_)) {
        product_id = *p;
      }
      core_configuration::profile::device::identifiers identifiers(vendor_id,
                                                                   product_id,
                                                                   is_keyboard,
                                                                   is_pointing_device);

      bool is_built_in_keyboard = false;
      if (descriptions.get_product().find("Apple Internal ") != std::string::npos) {
        is_built_in_keyboard = true;
      }

      connected_device_ = std::make_unique<connected_devices::device>(descriptions,
                                                                      identifiers,
                                                                      is_built_in_keyboard);
    }

    // ----------------------------------------
    // Setup elements_

    if (auto elements = IOHIDDeviceCopyMatchingElements(device_, nullptr, kIOHIDOptionsTypeNone)) {
      for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
        // Add to elements_.
        auto element = cf_utility::get_value<IOHIDElementRef>(elements, i);
        auto usage_page = hid_usage_page(IOHIDElementGetUsagePage(element));
        auto usage = hid_usage(IOHIDElementGetUsage(element));

        auto key = elements_key(usage_page, usage);
        if (elements_.find(key) == elements_.end()) {
          CFRetain(element);
          elements_[key] = element;
        }
      }
      CFRelease(elements);
    }

    // ----------------------------------------
    // setup queue_

    const CFIndex depth = 1024;
    queue_ = IOHIDQueueCreate(kCFAllocatorDefault, device_, depth, kIOHIDOptionsTypeNone);
    if (!queue_) {
      logger_.error("IOHIDQueueCreate error @ {0}", __PRETTY_FUNCTION__);
    } else {
      // Add elements into queue_.
      for (const auto& it : elements_) {
        IOHIDQueueAddElement(queue_, it.second);
      }
      IOHIDQueueRegisterValueAvailableCallback(queue_, static_queue_value_available_callback, this);
    }
  }

  ~human_interface_device(void) {
    // Release device_ and queue_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      // Unregister all callbacks.
      unschedule();
      unregister_report_callback();
      queue_stop();
      close();

      // ----------------------------------------
      // Release queue_

      if (queue_) {
        CFRelease(queue_);
        queue_ = nullptr;
      }

      // ----------------------------------------
      // Release elements_

      for (const auto& it : elements_) {
        CFRelease(it.second);
      }
      elements_.clear();

      // ----------------------------------------
      // Release device_

      CFRelease(device_);
    });
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  IOReturn open(IOOptionBits options = kIOHIDOptionsTypeNone) {
    IOReturn __block r;
    gcd_utility::dispatch_sync_in_main_queue(^{
      r = IOHIDDeviceOpen(device_, options);
    });
    return r;
  }

  IOReturn close(void) {
    IOReturn __block r;
    gcd_utility::dispatch_sync_in_main_queue(^{
      r = IOHIDDeviceClose(device_, kIOHIDOptionsTypeNone);
    });
    return r;
  }

  void schedule(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      IOHIDDeviceScheduleWithRunLoop(device_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
      if (queue_) {
        IOHIDQueueScheduleWithRunLoop(queue_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
      }
    });
  }

  void unschedule(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (queue_) {
        IOHIDQueueUnscheduleFromRunLoop(queue_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
      }
      IOHIDDeviceUnscheduleFromRunLoop(device_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    });
  }

  void register_report_callback(const report_callback& callback) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      report_callback_ = callback;

      resize_report_buffer();
      IOHIDDeviceRegisterInputReportCallback(device_, &(report_buffer_[0]), report_buffer_.size(), static_input_report_callback, this);
    });
  }

  void unregister_report_callback(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      resize_report_buffer();
      IOHIDDeviceRegisterInputReportCallback(device_, &(report_buffer_[0]), report_buffer_.size(), nullptr, this);

      report_callback_ = nullptr;
    });
  }

  void queue_start(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (queue_) {
        IOHIDQueueStart(queue_);
      }
    });
  }

  void queue_stop(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (queue_) {
        IOHIDQueueStop(queue_);
      }
    });
  }

  // High-level utility method.
  void observe(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (is_pqrs_device()) {
        return;
      }

      if (observed_) {
        return;
      }

      auto r = open();
      if (r != kIOReturnSuccess) {
        logger_.error("IOHIDDeviceOpen error: {1} @ {0}", __PRETTY_FUNCTION__, r);
        return;
      }

      queue_start();
      schedule();

      observed_ = true;
    });
  }

  // High-level utility method.
  void unobserve(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (is_pqrs_device()) {
        return;
      }

      if (!observed_) {
        return;
      }

      unschedule();
      queue_stop();
      close();

      observed_ = false;
    });
  }

  // High-level utility method.
  void grab(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (is_pqrs_device()) {
        return;
      }

      if (grabbed_) {
        return;
      }

      cancel_grab_timer();

      is_grabbable_callback_log_reducer_.reset();

      grab_timer_ = std::make_unique<gcd_utility::main_queue_timer>(
          // We have to set an initial wait since OS X will lost the device if we called IOHIDDeviceOpen(kIOHIDOptionsTypeSeizeDevice) in device_matching_callback.
          // (The device will be unusable after karabiner_grabber is quitted if we don't wait here.)
          dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC),
          100 * NSEC_PER_MSEC,
          0,
          ^{
            switch (is_grabbable()) {
              case grabbable_state::grabbable:
                break;

              case grabbable_state::ungrabbable_temporarily:
                return;

              case grabbable_state::ungrabbable_permanently:
                cancel_grab_timer();
                return;
            }

            // ----------------------------------------
            grabbed_ = true;

            unobserve();

            // ----------------------------------------
            auto r = open(kIOHIDOptionsTypeSeizeDevice);
            if (r != kIOReturnSuccess) {
              logger_.error("IOHIDDeviceOpen error: {1} @ {0}", __PRETTY_FUNCTION__, r);
              return;
            }

            if (grabbed_callback_) {
              grabbed_callback_(*this);
            }

            queue_start();
            schedule();

            // ----------------------------------------
            logger_.info("{0} is grabbed", get_name_for_log());

            cancel_grab_timer();
          });
    });
  }

  // High-level utility method.
  void ungrab(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (is_pqrs_device()) {
        return;
      }

      if (!grabbed_) {
        return;
      }

      grabbed_ = false;

      cancel_grab_timer();

      unschedule();
      queue_stop();
      close();

      if (ungrabbed_callback_) {
        ungrabbed_callback_(*this);
      }

      observe();
    });
  }

  bool get_disabled(void) const {
    return disabled_;
  }

  void disable(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (is_pqrs_device()) {
        return;
      }

      if (disabled_) {
        return;
      }

      disabled_ = true;

      if (disabled_callback_) {
        disabled_callback_(*this);
      }

      logger_.info("{0} is disabled", get_name_for_log());
    });
  }

  void enable(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (is_pqrs_device()) {
        return;
      }

      if (!disabled_) {
        return;
      }

      disabled_ = false;

      logger_.info("{0} is enabled", get_name_for_log());
    });
  }

  boost::optional<long> get_max_input_report_size(void) const {
    return iokit_utility::get_max_input_report_size(device_);
  }

  boost::optional<vendor_id> get_vendor_id(void) const {
    return iokit_utility::get_vendor_id(device_);
  }

  boost::optional<product_id> get_product_id(void) const {
    return iokit_utility::get_product_id(device_);
  }

  boost::optional<location_id> get_location_id(void) const {
    return iokit_utility::get_location_id(device_);
  }

  boost::optional<std::string> get_manufacturer(void) const {
    return iokit_utility::get_manufacturer(device_);
  }

  boost::optional<std::string> get_product(void) const {
    return iokit_utility::get_product(device_);
  }

  boost::optional<std::string> get_serial_number(void) const {
    return iokit_utility::get_serial_number(device_);
  }

  boost::optional<std::string> get_transport(void) const {
    return iokit_utility::get_transport(device_);
  }

  std::string get_name_for_log(void) const {
    if (auto product_name = get_product()) {
      return boost::trim_copy(*product_name);
    }
    if (auto vendor_id = get_vendor_id()) {
      if (auto product_id = get_product_id()) {
        std::stringstream stream;
        stream << std::hex
               << "(vendor_id:0x" << static_cast<uint32_t>(*vendor_id)
               << ", product_id:0x" << static_cast<uint32_t>(*product_id)
               << ")"
               << std::dec;
        return stream.str();
      }
    }

    std::stringstream stream;
    stream << "(device_id:" << static_cast<uint32_t>(device_id_) << ")";
    return stream.str();
  }

  void set_is_grabbable_callback(const is_grabbable_callback& callback) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      is_grabbable_callback_ = callback;
    });
  }

  void set_grabbed_callback(const grabbed_callback& callback) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      grabbed_callback_ = callback;
    });
  }

  void set_ungrabbed_callback(const ungrabbed_callback& callback) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      ungrabbed_callback_ = callback;
    });
  }

  void set_disabled_callback(const disabled_callback& callback) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      disabled_callback_ = callback;
    });
  }

  void set_value_callback(const value_callback& callback) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      value_callback_ = callback;
    });
  }

  grabbable_state is_grabbable(void) {
    if (is_grabbable_callback_) {
      auto state = is_grabbable_callback_(*this);
      if (state != grabbable_state::grabbable) {
        return state;
      }
    }

    // ----------------------------------------
    // Ungrabbable while pointing button is pressed.

    if (!pressed_pointing_buttons_.empty()) {
      // We should not grab the device while a button is pressed since we cannot release the button.
      // (To release the button, we have to send a hid report to the device. But we cannot do it.)

      is_grabbable_callback_log_reducer_.warn(std::string("We cannot grab ") + get_name_for_log() + " while mouse buttons are pressed.");
      return human_interface_device::grabbable_state::ungrabbable_temporarily;
    }

    // ----------------------------------------

    return grabbable_state::grabbable;
  }

  bool is_grabbed(void) const {
    bool __block r = false;
    gcd_utility::dispatch_sync_in_main_queue(^{
      r = grabbed_;
    });
    return r;
  }

#pragma mark - usage specific utilities

  // This method requires root privilege to use IOHIDDeviceGetValue for kHIDPage_LEDs usage.
  boost::optional<led_state> get_caps_lock_led_state(void) const {
    boost::optional<led_state> __block state = boost::none;

    gcd_utility::dispatch_sync_in_main_queue(^{
      if (auto element = get_element(hid_usage_page::leds, hid_usage::led_caps_lock)) {
        auto max = IOHIDElementGetLogicalMax(element);

        IOHIDValueRef value;
        auto r = IOHIDDeviceGetValue(device_, element, &value);
        if (r != kIOReturnSuccess) {
          logger_.error("IOHIDDeviceGetValue error: {1} @ {0}", __PRETTY_FUNCTION__, r);
        } else {
          auto integer_value = IOHIDValueGetIntegerValue(value);
          if (integer_value == max) {
            state = led_state::on;
          } else {
            state = led_state::off;
          }
        }
      }
    });

    return state;
  }

  // This method requires root privilege to use IOHIDDeviceSetValue for kHIDPage_LEDs usage.
  IOReturn set_caps_lock_led_state(led_state state) {
    // `IOHIDDeviceSetValue` will block forever with some buggy devices. (eg. Bit Touch)
    // This, we use a blacklist.
    if (auto v = get_vendor_id()) {
      if (auto p = get_product_id()) {
        if ((*v == vendor_id(0x22ea) && *p == product_id(0xf)) /* Bit Touch (Bit Trade One LTD.) */ ||
            false) {
          return kIOReturnSuccess;
        }
      }
    }

    IOReturn __block r = kIOReturnError;

    gcd_utility::dispatch_sync_in_main_queue(^{
      if (auto element = get_element(hid_usage_page::leds, hid_usage::led_caps_lock)) {
        CFIndex integer_value = 0;
        if (state == led_state::on) {
          integer_value = IOHIDElementGetLogicalMax(element);
        } else {
          integer_value = IOHIDElementGetLogicalMin(element);
        }

        if (auto value = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, mach_absolute_time(), integer_value)) {
          r = IOHIDDeviceSetValue(device_, element, value);

          if (r != kIOReturnSuccess) {
            logger_.error("IOHIDDeviceSetValue error {1} for {2} @ {0}", __PRETTY_FUNCTION__, r, get_name_for_log());
          }

          CFRelease(value);

        } else {
          logger_.error("IOHIDValueCreateWithIntegerValue error @ {0}", __PRETTY_FUNCTION__);
        }
      }
    });

    return r;
  }

  const connected_devices::device get_connected_device(void) const {
    return *connected_device_;
  }

  bool is_keyboard(void) const {
    return connected_device_->get_identifiers().get_is_keyboard();
  }

  bool is_pointing_device(void) const {
    return connected_device_->get_identifiers().get_is_pointing_device();
  }

  bool is_built_in_keyboard(void) const {
    return connected_device_->get_is_built_in_keyboard();
  }

  bool is_pqrs_device(void) const {
    if (auto manufacturer = get_manufacturer()) {
      if (*manufacturer == "pqrs.org") {
        return true;
      }
    }

    return false;
  }

private:
  static void static_queue_value_available_callback(void* _Nullable context, IOReturn result, void* _Nullable sender) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<human_interface_device*>(context);
    if (!self) {
      return;
    }

    self->queue_value_available_callback();
  }

  void queue_value_available_callback(void) {
    while (true) {
      if (auto value = IOHIDQueueCopyNextValueWithTimeout(queue_, 0.)) {
        auto element = IOHIDValueGetElement(value);
        if (element) {
          auto time_stamp = IOHIDValueGetTimeStamp(value);
          auto usage_page = hid_usage_page(IOHIDElementGetUsagePage(element));
          auto usage = hid_usage(IOHIDElementGetUsage(element));
          auto integer_value = IOHIDValueGetIntegerValue(value);

          if (input_event_queue_.emplace_back_event(device_id_, time_stamp, usage_page, usage, integer_value)) {
            // We need to check whether event is emplaced into `input_event_queue_` since
            // `emplace_back_event` does not add an event in some usage_page and usage.

            auto& element_event = input_event_queue_.get_events().back();

            // Send `device_keys_are_released` event if needed.

            if (auto key_code = element_event.get_event().get_key_code()) {
              if (integer_value) {
                pressed_keys_.insert(elements_key(usage_page, usage));
              } else {
                size_t size = pressed_keys_.size();
                pressed_keys_.erase(elements_key(usage_page, usage));
                if (size > 0 && pressed_keys_.empty()) {
                  auto event = event_queue::queued_event::event(event_queue::queued_event::event::type::device_keys_are_released, 1);
                  input_event_queue_.emplace_back_event(device_id_,
                                                        time_stamp,
                                                        event,
                                                        event_type::key_down,
                                                        event);
                }
              }
            }

            // Send `device_pointing_buttons_are_released` event if needed.

            if (auto pointing_button = element_event.get_event().get_pointing_button()) {
              if (integer_value) {
                pressed_pointing_buttons_.insert(elements_key(usage_page, usage));
              } else {
                size_t size = pressed_pointing_buttons_.size();
                pressed_pointing_buttons_.erase(elements_key(usage_page, usage));
                if (size > 0 && pressed_pointing_buttons_.empty()) {
                  auto event = event_queue::queued_event::event(event_queue::queued_event::event::type::device_pointing_buttons_are_released, 1);
                  input_event_queue_.emplace_back_event(device_id_,
                                                        time_stamp,
                                                        event,
                                                        event_type::key_down,
                                                        event);
                }
              }
            }
          }
        }

        CFRelease(value);

      } else {
        break;
      }
    }

    // Call value_callback_.
    if (value_callback_) {
      value_callback_(*this, input_event_queue_);
    }

    input_event_queue_.clear_events();
  }

  static void static_input_report_callback(void* _Nullable context,
                                           IOReturn result,
                                           void* _Nullable sender,
                                           IOHIDReportType type,
                                           uint32_t report_id,
                                           uint8_t* _Nullable report,
                                           CFIndex report_length) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<human_interface_device*>(context);
    if (!self) {
      return;
    }

    self->input_report_callback(type, report_id, report, report_length);
  }

  void input_report_callback(IOHIDReportType type,
                             uint32_t report_id,
                             uint8_t* _Nullable report,
                             CFIndex report_length) {
    if (report_callback_) {
      report_callback_(*this, type, report_id, report, report_length);
    }
  }

  uint64_t elements_key(hid_usage_page usage_page, hid_usage usage) const {
    return ((static_cast<uint64_t>(usage_page) << 32) | static_cast<uint32_t>(usage));
  }

  IOHIDElementRef _Nullable get_element(hid_usage_page usage_page, hid_usage usage) const {
    auto key = elements_key(usage_page, usage);
    auto it = elements_.find(key);
    if (it == elements_.end()) {
      return nullptr;
    } else {
      return it->second;
    }
  }

  void resize_report_buffer(void) {
    size_t buffer_size = 32; // use this provisional value if we cannot get max input report size from device.
    if (auto size = get_max_input_report_size()) {
      buffer_size = *size;
    }

    report_buffer_.resize(buffer_size);
  }

  void cancel_grab_timer(void) {
    grab_timer_ = nullptr;
  }

  spdlog::logger& logger_;

  IOHIDDeviceRef _Nonnull device_;
  device_id device_id_;
  IOHIDQueueRef _Nullable queue_;
  std::unordered_map<uint64_t, IOHIDElementRef> elements_;

  event_queue input_event_queue_;

  value_callback value_callback_;
  report_callback report_callback_;
  std::vector<uint8_t> report_buffer_;

  is_grabbable_callback is_grabbable_callback_;
  spdlog_utility::log_reducer is_grabbable_callback_log_reducer_;

  grabbed_callback grabbed_callback_;
  ungrabbed_callback ungrabbed_callback_;
  disabled_callback disabled_callback_;
  std::unique_ptr<gcd_utility::main_queue_timer> grab_timer_;
  bool observed_;
  bool grabbed_;
  // `disabled_` is ignoring input events from this device.
  // (== `grabbed_` and does not call `value_callback_`)
  bool disabled_;

  std::unique_ptr<connected_devices::device> connected_device_;

  std::unordered_set<uint64_t> pressed_keys_;
  std::unordered_set<uint64_t> pressed_pointing_buttons_;
};
} // namespace krbn
