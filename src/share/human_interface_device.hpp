#pragma once

// `krbn::human_interface_device` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "cf_utility.hpp"
#include "connected_devices/connected_devices.hpp"
#include "core_configuration/core_configuration.hpp"
#include "device_detail.hpp"
#include "event_queue.hpp"
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
#include <list>
#include <mach/mach_time.h>
#include <unordered_map>
#include <vector>

namespace krbn {
class human_interface_device final : pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  boost::signals2::signal<void(void)> opened;
  boost::signals2::signal<void(IOReturn error)> open_failed;

  boost::signals2::signal<void(void)> closed;
  boost::signals2::signal<void(IOReturn error)> close_failed;

  // `event_queue` is not owned by `human_interface_device`.
  boost::signals2::signal<void(std::shared_ptr<event_queue::queue>)> values_arrived;

  boost::signals2::signal<void(IOHIDReportType type, uint32_t report_id, std::shared_ptr<std::vector<uint8_t>>)> report_arrived;

  // Methods

  human_interface_device(const human_interface_device&) = delete;

  human_interface_device(IOHIDDeviceRef _Nonnull device,
                         registry_entry_id registry_entry_id) : dispatcher_client(),
                                                                device_(device),
                                                                registry_entry_id_(registry_entry_id),
                                                                device_id_(types::make_new_device_id(std::make_shared<device_detail>(device))),
                                                                removed_(false),
                                                                opened_(false),
                                                                scheduled_(false),
                                                                queue_(nullptr) {
    // ----------------------------------------

    run_loop_thread_ = std::make_unique<cf_utility::run_loop_thread>();

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
      connected_devices::details::descriptions descriptions(manufacturer, product);

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

      connected_device_ = std::make_shared<connected_devices::details::device>(descriptions,
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
      logger::get_logger().error("IOHIDQueueCreate is failed.");
    } else {
      // Add elements into queue_.
      for (const auto& e : elements_) {
        IOHIDQueueAddElement(queue_, e);
      }
      IOHIDQueueRegisterValueAvailableCallback(queue_, static_queue_value_available_callback, this);
    }
  }

  virtual ~human_interface_device(void) {
    detach_from_dispatcher([this] {
      unschedule();
      disable_report_callback();
      queue_stop();
      close();

      // ----------------------------------------

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

    run_loop_thread_->terminate();
    run_loop_thread_ = nullptr;

    logger::get_logger().info("human_interface_device:{0} is destroyed.", name_for_log_);
  }

  registry_entry_id get_registry_entry_id(void) const {
    return registry_entry_id_;
  }

  std::string get_name_for_log(void) const {
    return name_for_log_;
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  bool get_removed(void) const {
    return removed_;
  }

  void set_removed(void) {
    removed_ = true;
  }

  std::shared_ptr<connected_devices::details::device> get_connected_device(void) const {
    return connected_device_;
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

  bool validate(void) const {
    // `iokit_utility::find_registry_entry_id` is failed after `device_` is removed.

    if (!iokit_utility::find_registry_entry_id(device_)) {
      return false;
    }

    return true;
  }

  void async_open(IOOptionBits options = kIOHIDOptionsTypeNone) {
    enqueue_to_dispatcher([this, options] {
      open(options);
    });
  }

  void async_close(void) {
    enqueue_to_dispatcher([this] {
      close();
    });
  }

  void async_schedule(void) {
    enqueue_to_dispatcher([this] {
      schedule();
    });
  }

  void async_unschedule(void) {
    enqueue_to_dispatcher([this] {
      unschedule();
    });
  }

  void async_enable_report_callback(void) {
    enqueue_to_dispatcher([this] {
      enable_report_callback();
    });
  }

  void async_disable_report_callback(void) {
    enqueue_to_dispatcher([this] {
      disable_report_callback();
    });
  }

  void async_queue_start(void) {
    enqueue_to_dispatcher([this] {
      queue_start();
    });
  }

  void async_queue_stop(void) {
    enqueue_to_dispatcher([this] {
      queue_stop();
    });
  }

  void async_set_report(IOHIDReportType report_type,
                        CFIndex report_id,
                        std::shared_ptr<std::vector<uint8_t>> report) {
    enqueue_to_dispatcher([this, report_type, report_id, report] {
      set_report(report_type,
                 report_id,
                 report);
    });
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

#pragma mark - usage specific utilities

  // This method requires root privilege to use IOHIDDeviceSetValue for kHIDPage_LEDs usage.
  void async_set_caps_lock_led_state(led_state state) {
    enqueue_to_dispatcher([this, state] {
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

          if (auto value = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault,
                                                            e,
                                                            mach_absolute_time(),
                                                            integer_value)) {
            IOHIDDeviceSetValue(device_, e, value);

            CFRelease(value);
          }
        }
      }
    });
  }

private:
  // This method is executed in the dispatcher thread.
  void open(IOOptionBits options) {
    if (removed_) {
      return;
    }

    if (!opened_) {
      auto r = IOHIDDeviceOpen(device_, options);
      if (r == kIOReturnSuccess) {
        opened_ = true;

        enqueue_to_dispatcher([this] {
          opened();
        });
      } else {
        enqueue_to_dispatcher([this, r] {
          open_failed(r);
        });
      }
    }
  }

  // This method is executed in the dispatcher thread.
  void close(void) {
    if (removed_) {
      return;
    }

    if (opened_) {
      auto r = IOHIDDeviceClose(device_, kIOHIDOptionsTypeNone);
      if (r == kIOReturnSuccess) {
        enqueue_to_dispatcher([this] {
          closed();
        });
      } else {
        enqueue_to_dispatcher([this, r] {
          close_failed(r);
        });
      }
      opened_ = false;
    }
  }

  // This method is executed in the dispatcher thread.
  void schedule(void) {
    if (removed_) {
      return;
    }

    if (!scheduled_) {
      scheduled_ = true;

      IOHIDDeviceScheduleWithRunLoop(device_,
                                     run_loop_thread_->get_run_loop(),
                                     kCFRunLoopCommonModes);
      if (queue_) {
        IOHIDQueueScheduleWithRunLoop(queue_,
                                      run_loop_thread_->get_run_loop(),
                                      kCFRunLoopCommonModes);
      }

      run_loop_thread_->wake();
    }
  }

  // This method is executed in the dispatcher thread.
  void unschedule(void) {
    if (removed_) {
      return;
    }

    if (scheduled_) {
      if (queue_) {
        IOHIDQueueUnscheduleFromRunLoop(queue_,
                                        run_loop_thread_->get_run_loop(),
                                        kCFRunLoopCommonModes);
      }
      IOHIDDeviceUnscheduleFromRunLoop(device_,
                                       run_loop_thread_->get_run_loop(),
                                       kCFRunLoopCommonModes);

      scheduled_ = false;
    }
  }

  // This method is executed in the dispatcher thread.
  void enable_report_callback(void) {
    if (removed_) {
      return;
    }

    resize_report_buffer();
    IOHIDDeviceRegisterInputReportCallback(device_, &(report_buffer_[0]), report_buffer_.size(), static_input_report_callback, this);
  }

  // This method is executed in the dispatcher thread.
  void disable_report_callback(void) {
    if (removed_) {
      return;
    }

    resize_report_buffer();
    IOHIDDeviceRegisterInputReportCallback(device_, &(report_buffer_[0]), report_buffer_.size(), nullptr, this);
  }

  // This method is executed in the dispatcher thread.
  void queue_start(void) {
    if (removed_) {
      return;
    }

    if (queue_) {
      IOHIDQueueStart(queue_);
    }
  }

  // This method is executed in the dispatcher thread.
  void queue_stop(void) {
    if (removed_) {
      return;
    }

    if (queue_) {
      IOHIDQueueStop(queue_);
    }
  }

  // This method is executed in the dispatcher thread.
  void set_report(IOHIDReportType report_type,
                  CFIndex report_id,
                  std::shared_ptr<std::vector<uint8_t>> report) {
    if (removed_) {
      return;
    }

    if (report) {
      IOHIDDeviceSetReport(device_,
                           report_type,
                           report_id,
                           &((*report)[0]),
                           report->size());
    }
  }

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
    enqueue_to_dispatcher([this] {
      auto input_event_queue = std::make_shared<event_queue::queue>();
      std::vector<hid_value> hid_values;

      while (auto value = IOHIDQueueCopyNextValueWithTimeout(queue_, 0.)) {
        hid_values.emplace_back(value);

        CFRelease(value);
      }

      for (const auto& pair : event_queue::queue::make_entries(hid_values, device_id_)) {
        auto& hid_value = pair.first;
        auto& entry = pair.second;

        input_event_queue->push_back_event(entry);

        if (hid_value) {
          if (auto hid_usage_page = hid_value->get_hid_usage_page()) {
            if (auto hid_usage = hid_value->get_hid_usage()) {
              if (entry.get_event().get_type() == event_queue::event::type::key_code ||
                  entry.get_event().get_type() == event_queue::event::type::consumer_key_code ||
                  entry.get_event().get_type() == event_queue::event::type::pointing_button) {
                // Send `device_keys_and_pointing_buttons_are_released` event if needed.

                if (entry.get_event_type() == event_type::key_down) {
                  pressed_keys_.insert(elements_key(*hid_usage_page, *hid_usage));
                } else {
                  size_t size = pressed_keys_.size();
                  pressed_keys_.erase(elements_key(*hid_usage_page, *hid_usage));
                  if (size > 0) {
                    post_device_keys_and_pointing_buttons_are_released_event_if_needed(input_event_queue,
                                                                                       hid_value->get_time_stamp());
                  }
                }
              }
            }
          }
        }
      }

      enqueue_to_dispatcher([this, input_event_queue] {
        values_arrived(input_event_queue);
      });
    });
  }

  void post_device_keys_and_pointing_buttons_are_released_event_if_needed(std::shared_ptr<event_queue::queue> input_event_queue,
                                                                          absolute_time time_stamp) {
    if (pressed_keys_.empty()) {
      auto event = event_queue::event::make_device_keys_and_pointing_buttons_are_released_event();
      input_event_queue->emplace_back_event(device_id_,
                                            event_queue::event_time_stamp(time_stamp),
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
    auto r = std::make_shared<std::vector<uint8_t>>(report_length);
    memcpy(&((*r)[0]), report, report_length);

    enqueue_to_dispatcher([this, type, report_id, r] {
      report_arrived(type, report_id, r);
    });
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

  IOHIDDeviceRef _Nonnull device_;
  registry_entry_id registry_entry_id_;

  std::unique_ptr<cf_utility::run_loop_thread> run_loop_thread_;
  device_id device_id_;
  std::string name_for_log_;
  std::atomic<bool> removed_;
  bool opened_;
  bool scheduled_;
  std::shared_ptr<connected_devices::details::device> connected_device_;

  IOHIDQueueRef _Nullable queue_;
  std::vector<IOHIDElementRef> elements_;
  std::vector<uint8_t> report_buffer_;
  std::unordered_set<uint64_t> pressed_keys_;
};
} // namespace krbn
