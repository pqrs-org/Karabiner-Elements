#pragma once

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "cf_utility.hpp"
#include "connected_devices.hpp"
#include "core_configuration.hpp"
#include "device_detail.hpp"
#include "event_queue.hpp"
#include "gcd_utility.hpp"
#include "iokit_utility.hpp"
#include "keyboard_repeat_detector.hpp"
#include "logger.hpp"
#include "spdlog_utility.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hid/IOHIDValue.h>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <cstdint>
#include <functional>
#include <list>
#include <mach/mach_time.h>
#include <unordered_map>
#include <vector>

namespace krbn {
class human_interface_device final {
public:
  boost::signals2::signal<void(human_interface_device& device,
                               event_queue& event_queue)>
      values_arrived;

  boost::signals2::signal<void(human_interface_device& device,
                               IOHIDReportType type,
                               uint32_t report_id,
                               uint8_t* _Nonnull report,
                               CFIndex report_length)>
      report_arrived;

  typedef std::function<grabbable_state(human_interface_device& device)> is_grabbable_callback;

  boost::signals2::signal<void(human_interface_device&)> device_observed;
  boost::signals2::signal<void(human_interface_device&)> device_grabbed;
  boost::signals2::signal<void(human_interface_device&)> device_ungrabbed;
  boost::signals2::signal<void(human_interface_device&)> device_disabled;

  human_interface_device(const human_interface_device&) = delete;

  human_interface_device(IOHIDDeviceRef _Nonnull device) : device_(device),
                                                           device_id_(types::make_new_device_id(std::make_shared<device_detail>(device))),
                                                           queue_(nullptr),
                                                           removed_(false),
                                                           observed_(false),
                                                           grabbed_(false),
                                                           disabled_(false) {
    // ----------------------------------------
    // Retain device_

    CFRetain(device_);

    // Set name_for_log_
    {
      std::stringstream stream;

      if (auto product_name = find_product()) {
        stream << boost::trim_copy(*product_name);
      } else {
        if (auto vendor_id = find_vendor_id()) {
          if (auto product_id = find_product_id()) {
            stream << std::hex
                   << "(vendor_id:0x" << static_cast<uint32_t>(*vendor_id)
                   << ", product_id:0x" << static_cast<uint32_t>(*product_id)
                   << ")"
                   << std::dec;
          }
        }
      }

      stream << " (device_id:" << static_cast<uint32_t>(device_id_) << ")";
      name_for_log_ = stream.str();
    }

    // Create connected_device_.
    {
      std::string manufacturer;
      std::string product;
      if (auto m = iokit_utility::find_manufacturer(device_)) {
        manufacturer = *m;
      }
      if (auto p = iokit_utility::find_product(device_)) {
        product = *p;
      }
      connected_devices::device::descriptions descriptions(manufacturer, product);

      auto vendor_id = iokit_utility::find_vendor_id(device_);
      auto product_id = iokit_utility::find_product_id(device_);
      bool is_keyboard = iokit_utility::is_keyboard(device_);
      bool is_pointing_device = iokit_utility::is_pointing_device(device_);
      device_identifiers identifiers(vendor_id,
                                     product_id,
                                     is_keyboard,
                                     is_pointing_device);

      bool is_built_in_keyboard = false;
      if (is_keyboard &&
          !is_pointing_device &&
          descriptions.get_product().find("Apple Internal ") != std::string::npos) {
        is_built_in_keyboard = true;
      }

      bool is_built_in_trackpad = false;
      if (!is_keyboard &&
          is_pointing_device &&
          descriptions.get_product().find("Apple Internal ") != std::string::npos) {
        is_built_in_trackpad = true;
      }

      connected_device_ = std::make_unique<connected_devices::device>(descriptions,
                                                                      identifiers,
                                                                      is_built_in_keyboard,
                                                                      is_built_in_trackpad);
    }

    // ----------------------------------------
    // Setup elements_

    // Note:
    // Some devices has duplicated entries for same usage_page and usage.
    //
    // For example, there are entries of Microsoft Designer Mouse:
    //
    //   * Microsoft Designer Mouse usage_page 1 usage 2
    //   * Microsoft Designer Mouse usage_page 1 usage 2
    //   * Microsoft Designer Mouse usage_page 1 usage 1
    //   * Microsoft Designer Mouse usage_page 1 usage 56
    //   * Microsoft Designer Mouse usage_page 1 usage 56
    //   * Microsoft Designer Mouse usage_page 1 usage 568
    //   * Microsoft Designer Mouse usage_page 12 usage 568
    //   * Microsoft Designer Mouse usage_page 9 usage 1
    //   * Microsoft Designer Mouse usage_page 9 usage 2
    //   * Microsoft Designer Mouse usage_page 9 usage 3
    //   * Microsoft Designer Mouse usage_page 9 usage 4
    //   * Microsoft Designer Mouse usage_page 9 usage 5
    //   * Microsoft Designer Mouse usage_page 1 usage 48
    //   * Microsoft Designer Mouse usage_page 1 usage 49

    if (auto elements = IOHIDDeviceCopyMatchingElements(device_, nullptr, kIOHIDOptionsTypeNone)) {
      for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
        // Add to elements_.
        if (auto e = cf_utility::get_value<IOHIDElementRef>(elements, i)) {
          CFRetain(e);

#if 0
          logger::get_logger().info("{0} usage_page:{1} usage:{2} min:{3} max:{4}",
                                    name_for_log_,
                                    IOHIDElementGetUsagePage(e),
                                    IOHIDElementGetUsage(e),
                                    IOHIDElementGetLogicalMin(e),
                                    IOHIDElementGetLogicalMax(e));
#endif

          elements_.push_back(e);
        }
      }
      CFRelease(elements);
    }

    // ----------------------------------------
    // setup queue_

    const CFIndex depth = 1024;
    queue_ = IOHIDQueueCreate(kCFAllocatorDefault, device_, depth, kIOHIDOptionsTypeNone);
    if (!queue_) {
      logger::get_logger().error("IOHIDQueueCreate error @ {0}", __PRETTY_FUNCTION__);
    } else {
      // Add elements into queue_.
      for (const auto& e : elements_) {
        IOHIDQueueAddElement(queue_, e);
      }
      IOHIDQueueRegisterValueAvailableCallback(queue_, static_queue_value_available_callback, this);
    }
  }

  ~human_interface_device(void) {
    // Release device_ and queue_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      grab_timer_ = nullptr;
      observe_timer_ = nullptr;

      // Unregister all callbacks.
      unschedule();
      disable_report_callback();
      queue_stop();
      close();

      types::detach_device_id(device_id_);

      // ----------------------------------------
      // Release queue_

      if (queue_) {
        CFRelease(queue_);
        queue_ = nullptr;
      }

      // ----------------------------------------
      // Release elements_

      for (const auto& e : elements_) {
        CFRelease(e);
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

  bool get_observed(void) const {
    return observed_;
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

  void enable_report_callback(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      resize_report_buffer();
      IOHIDDeviceRegisterInputReportCallback(device_, &(report_buffer_[0]), report_buffer_.size(), static_input_report_callback, this);
    });
  }

  void disable_report_callback(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      resize_report_buffer();
      IOHIDDeviceRegisterInputReportCallback(device_, &(report_buffer_[0]), report_buffer_.size(), nullptr, this);
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

  IOReturn set_report(IOHIDReportType report_type,
                      CFIndex report_id,
                      const uint8_t* _Nonnull report,
                      CFIndex report_length) {
    IOReturn __block r;
    gcd_utility::dispatch_sync_in_main_queue(^{
      r = IOHIDDeviceSetReport(device_,
                               report_type,
                               report_id,
                               report,
                               report_length);
    });
    return r;
  }

  void set_removed(void) {
    removed_ = true;
  }

  // High-level utility method.
  void observe(void) {
    boost::optional<IOReturn> __block last_open_error;

    observe_timer_ = std::make_unique<gcd_utility::fire_while_false_timer>(
        3 * NSEC_PER_SEC,
        ^{
          if (removed_) {
            return true;
          }

          if (observed_) {
            return true;
          }

          auto r = open();
          if (r != kIOReturnSuccess) {
            if (last_open_error != r) {
              last_open_error = r;
              logger::get_logger().error("IOHIDDeviceOpen error: {0} ({1}) {2}", iokit_utility::get_error_name(r), r, name_for_log_);
            }
            return false;
          }

          queue_start();
          schedule();

          observed_ = true;

          logger::get_logger().info("{0} is observed.", get_name_for_log());

          device_observed(*this);

          return true;
        });
  }

  // High-level utility method.
  void unobserve(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      observe_timer_ = nullptr;

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
      if (removed_) {
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

              case grabbable_state::device_error:
                return;
            }

            // ----------------------------------------
            grabbed_ = true;

            unobserve();

            // ----------------------------------------
            auto r = open(kIOHIDOptionsTypeSeizeDevice);
            if (r != kIOReturnSuccess) {
              logger::get_logger().error("IOHIDDeviceOpen error: {0} ({1}) {2}", iokit_utility::get_error_name(r), r, name_for_log_);
              cancel_grab_timer();
              return;
            }

            device_grabbed(*this);

            queue_start();
            schedule();

            // ----------------------------------------
            logger::get_logger().info("{0} is grabbed", get_name_for_log());

            cancel_grab_timer();
          });
    });
  }

  // High-level utility method.
  void ungrab(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (!grabbed_) {
        return;
      }

      grabbed_ = false;

      cancel_grab_timer();

      unschedule();
      queue_stop();
      close();

      device_ungrabbed(*this);

      observe();
    });
  }

  bool get_disabled(void) const {
    return disabled_;
  }

  void disable(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (disabled_) {
        return;
      }

      disabled_ = true;

      device_disabled(*this);

      logger::get_logger().info("{0} is disabled", get_name_for_log());
    });
  }

  void enable(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      if (!disabled_) {
        return;
      }

      disabled_ = false;

      logger::get_logger().info("{0} is enabled", get_name_for_log());
    });
  }

  boost::optional<registry_entry_id> find_registry_entry_id(void) const {
    return iokit_utility::find_registry_entry_id(device_);
  }

  boost::optional<long> find_max_input_report_size(void) const {
    return iokit_utility::find_max_input_report_size(device_);
  }

  boost::optional<vendor_id> find_vendor_id(void) const {
    return iokit_utility::find_vendor_id(device_);
  }

  boost::optional<product_id> find_product_id(void) const {
    return iokit_utility::find_product_id(device_);
  }

  boost::optional<location_id> find_location_id(void) const {
    return iokit_utility::find_location_id(device_);
  }

  boost::optional<std::string> find_manufacturer(void) const {
    return iokit_utility::find_manufacturer(device_);
  }

  boost::optional<std::string> find_product(void) const {
    return iokit_utility::find_product(device_);
  }

  boost::optional<std::string> find_serial_number(void) const {
    return iokit_utility::find_serial_number(device_);
  }

  boost::optional<std::string> find_transport(void) const {
    return iokit_utility::find_transport(device_);
  }

  bool is_karabiner_virtual_hid_device(void) const {
    return iokit_utility::is_karabiner_virtual_hid_device(device_);
  }

  std::string get_name_for_log(void) const {
    return name_for_log_;
  }

  void set_is_grabbable_callback(const is_grabbable_callback& callback) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      is_grabbable_callback_ = callback;
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
    // Ungrabbable while key repeating

    if (keyboard_repeat_detector_.is_repeating()) {
      is_grabbable_callback_log_reducer_.warn(std::string("We cannot grab ") + get_name_for_log() + " while a key is repeating.");
      return grabbable_state::ungrabbable_temporarily;
    }

    // Ungrabbable while modifier keys are pressed
    //
    // We have to check the modifier keys state to avoid pressed physical modifiers affects in mouse events.
    // (See DEVELOPMENT.md > Modifier flags handling in kernel)

    if (!pressed_modifier_flags_.empty()) {
      std::stringstream ss;
      ss << "We cannot grab " << get_name_for_log() << " while any modifier flags are pressed. (";
      for (const auto& m : pressed_modifier_flags_) {
        ss << m << " ";
      }
      ss << ")";
      is_grabbable_callback_log_reducer_.warn(ss.str());
      return grabbable_state::ungrabbable_temporarily;
    }

    // ----------------------------------------
    // Ungrabbable while pointing button is pressed.

    if (!pressed_pointing_buttons_.empty()) {
      // We should not grab the device while a button is pressed since we cannot release the button.
      // (To release the button, we have to send a hid report to the device. But we cannot do it.)

      is_grabbable_callback_log_reducer_.warn(std::string("We cannot grab ") + get_name_for_log() + " while mouse buttons are pressed.");
      return grabbable_state::ungrabbable_temporarily;
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
      for (const auto& e : elements_) {
        auto usage_page = hid_usage_page(IOHIDElementGetUsagePage(e));
        auto usage = hid_usage(IOHIDElementGetUsage(e));

        if (usage_page == hid_usage_page::leds &&
            usage == hid_usage::led_caps_lock) {
          auto max = IOHIDElementGetLogicalMax(e);

          IOHIDValueRef value;
          auto r = IOHIDDeviceGetValue(device_, e, &value);
          if (r == kIOReturnSuccess) {
            auto integer_value = IOHIDValueGetIntegerValue(value);
            if (integer_value == max) {
              state = led_state::on;
            } else {
              state = led_state::off;
            }
            break;
          }
        }
      }
    });

    return state;
  }

  // This method requires root privilege to use IOHIDDeviceSetValue for kHIDPage_LEDs usage.
  IOReturn set_caps_lock_led_state(led_state state) {
    IOReturn __block r = kIOReturnError;

    gcd_utility::dispatch_sync_in_main_queue(^{
      for (const auto& e : elements_) {
        auto usage_page = hid_usage_page(IOHIDElementGetUsagePage(e));
        auto usage = hid_usage(IOHIDElementGetUsage(e));

        if (usage_page == hid_usage_page::leds &&
            usage == hid_usage::led_caps_lock) {
          CFIndex integer_value = 0;
          if (state == led_state::on) {
            integer_value = IOHIDElementGetLogicalMax(e);
          } else {
            integer_value = IOHIDElementGetLogicalMin(e);
          }

          if (auto value = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, e, mach_absolute_time(), integer_value)) {
            IOHIDDeviceSetValue(device_, e, value);

            CFRelease(value);
          }
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

  bool is_built_in_trackpad(void) const {
    return connected_device_->get_is_built_in_trackpad();
  }

  device_detail make_device_detail(void) const {
    return device_detail(device_);
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
    std::vector<hid_value> hid_values;

    while (auto value = IOHIDQueueCopyNextValueWithTimeout(queue_, 0.)) {
      hid_values.emplace_back(value);

      CFRelease(value);
    }

    for (const auto& pair : event_queue::make_queued_events(hid_values, device_id_)) {
      auto& hid_value = pair.first;
      auto& queued_event = pair.second;

      input_event_queue_.push_back_event(queued_event);

      if (hid_value) {
        if (auto hid_usage_page = hid_value->get_hid_usage_page()) {
          if (auto hid_usage = hid_value->get_hid_usage()) {
            if (queued_event.get_event().get_type() == event_queue::queued_event::event::type::key_code ||
                queued_event.get_event().get_type() == event_queue::queued_event::event::type::consumer_key_code ||
                queued_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_button) {

              // Update keyboard_repeat_detector_

              if (queued_event.get_event().get_type() == event_queue::queued_event::event::type::key_code ||
                  queued_event.get_event().get_type() == event_queue::queued_event::event::type::consumer_key_code) {
                keyboard_repeat_detector_.set(*hid_usage_page,
                                              *hid_usage,
                                              queued_event.get_event_type());
              }

              // Update pressed_modifier_flags_

              if (queued_event.get_event().get_type() == event_queue::queued_event::event::type::key_code) {
                if (auto m = types::make_modifier_flag(*hid_usage_page,
                                                       *hid_usage)) {
                  if (queued_event.get_event_type() == event_type::key_down) {
                    pressed_modifier_flags_.insert(*m);
                  } else if (queued_event.get_event_type() == event_type::key_up) {
                    pressed_modifier_flags_.erase(*m);
                  }
                }
              }

              // Update pressed_pointing_buttons_

              if (queued_event.get_event().get_type() == event_queue::queued_event::event::type::pointing_button) {
                if (queued_event.get_event_type() == event_type::key_down) {
                  pressed_pointing_buttons_.insert(elements_key(*hid_usage_page, *hid_usage));
                } else if (queued_event.get_event_type() == event_type::key_up) {
                  pressed_pointing_buttons_.erase(elements_key(*hid_usage_page, *hid_usage));
                }
              }

              // Send `device_keys_and_pointing_buttons_are_released` event if needed.

              if (queued_event.get_event_type() == event_type::key_down) {
                pressed_keys_.insert(elements_key(*hid_usage_page, *hid_usage));
              } else {
                size_t size = pressed_keys_.size();
                pressed_keys_.erase(elements_key(*hid_usage_page, *hid_usage));
                if (size > 0) {
                  post_device_keys_and_pointing_buttons_are_released_event_if_needed(hid_value->get_time_stamp());
                }
              }
            }
          }
        }
      }
    }

    values_arrived(*this, input_event_queue_);

    input_event_queue_.clear_events();
  }

  void post_device_keys_and_pointing_buttons_are_released_event_if_needed(uint64_t time_stamp) {
    if (pressed_keys_.empty()) {
      auto event = event_queue::queued_event::event::make_device_keys_and_pointing_buttons_are_released_event();
      input_event_queue_.emplace_back_event(device_id_,
                                            event_queue::queued_event::event_time_stamp(time_stamp),
                                            event,
                                            event_type::single,
                                            event);
    }
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
    report_arrived(*this, type, report_id, report, report_length);
  }

  uint64_t elements_key(hid_usage_page usage_page, hid_usage usage) const {
    return ((static_cast<uint64_t>(usage_page) << 32) | static_cast<uint32_t>(usage));
  }

  void resize_report_buffer(void) {
    size_t buffer_size = 32; // use this provisional value if we cannot get max input report size from device.
    if (auto size = find_max_input_report_size()) {
      buffer_size = *size;
    }

    report_buffer_.resize(buffer_size);
  }

  void cancel_grab_timer(void) {
    grab_timer_ = nullptr;
  }

  IOHIDDeviceRef _Nonnull device_;
  device_id device_id_;
  IOHIDQueueRef _Nullable queue_;
  std::vector<IOHIDElementRef> elements_;

  std::string name_for_log_;

  event_queue input_event_queue_;

  std::vector<uint8_t> report_buffer_;

  is_grabbable_callback is_grabbable_callback_;
  spdlog_utility::log_reducer is_grabbable_callback_log_reducer_;

  std::unique_ptr<gcd_utility::fire_while_false_timer> observe_timer_;
  std::unique_ptr<gcd_utility::main_queue_timer> grab_timer_;
  bool removed_;
  bool observed_;
  bool grabbed_;
  // `disabled_` is ignoring input events from this device.
  // (== `grabbed_` and does not call `values_arrived`)
  bool disabled_;

  std::unique_ptr<connected_devices::device> connected_device_;

  keyboard_repeat_detector keyboard_repeat_detector_;
  std::unordered_set<modifier_flag> pressed_modifier_flags_;

  std::unordered_set<uint64_t> pressed_keys_;
  std::unordered_set<uint64_t> pressed_pointing_buttons_;
};
} // namespace krbn
