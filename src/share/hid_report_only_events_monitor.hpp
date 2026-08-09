#pragma once

#include "device_properties.hpp"
#include "hid_report_only_events.hpp"
#include <mach/mach_time.h>
#include <nod/nod.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>
#include <pqrs/thread_wait.hpp>
#include <span>
#include <vector>

namespace krbn {
// Publishes events recovered from a device's raw input reports as ordinary HID values,
// so they flow through the rest of Karabiner normally.
//
// This class is the only part of the feature that touches IOKit; decoding stays in
// the device-specific implementation so it can be tested without hardware.
class hid_report_only_events_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  //
  // Signals (invoked from the shared dispatcher thread)
  //

  nod::signal<void(pqrs::not_null_shared_ptr_t<std::vector<pqrs::osx::iokit_hid_value>>)> values_arrived;

  //
  // Methods
  //

  hid_report_only_events_monitor(const hid_report_only_events_monitor&) = delete;

  hid_report_only_events_monitor(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
                                 pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread,
                                 IOHIDDeviceRef device,
                                 pqrs::not_null_shared_ptr_t<hid_report_only_events::report_handler> report_handler)
      : dispatcher_client(weak_dispatcher),
        run_loop_thread_(run_loop_thread),
        hid_device_(device),
        report_handler_(report_handler),
        registered_(false) {
    // IOHIDDeviceRegisterInputReportCallback writes into a buffer we own, so it has to be
    // large enough for anything the device can send.
    auto size = hid_device_.find_max_input_report_size().value_or(64);
    if (size < 1) {
      size = 64;
    }
    buffer_.resize(static_cast<size_t>(size));
  }

  ~hid_report_only_events_monitor() override {
    detach_from_dispatcher();

    // IOKit retains the report buffer even after the callback is cleared, so owners must
    // close the IOHIDDevice before destroying this monitor. Synchronize the callback
    // change here as well so no callback body remains in flight.
    auto wait = pqrs::make_thread_wait();

    run_loop_thread_->enqueue(^{
      unregister_callback();

      wait->notify();
    });

    wait->wait_notice();
  }

  // Build a monitor only for known devices with the expected report descriptor.
  [[nodiscard]] static std::shared_ptr<hid_report_only_events_monitor> make_if_target(
      std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
      pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread,
      IOHIDDeviceRef device,
      const device_properties& device_properties) {
    const auto& identifiers = device_properties.get_device_identifiers();

    if (hid_report_only_events::is_target_device(identifiers)) {
      // The descriptor is read only for a target device interface.
      auto report_descriptor = find_report_descriptor(device);
      if (auto report_handler =
              hid_report_only_events::make_report_handler(
                  identifiers,
                  report_descriptor)) {
        return std::make_shared<hid_report_only_events_monitor>(
            weak_dispatcher,
            run_loop_thread,
            device,
            pqrs::not_null_shared_ptr_t<hid_report_only_events::report_handler>(
                report_handler));
      }
    }

    return nullptr;
  }

  void async_start() {
    run_loop_thread_->enqueue(^{
      if (registered_) {
        return;
      }

      if (auto d = hid_device_.get_device()) {
        IOHIDDeviceRegisterInputReportCallback(*d,
                                               buffer_.data(),
                                               static_cast<CFIndex>(buffer_.size()),
                                               static_input_report_callback,
                                               this);
        registered_ = true;
      }
    });
  }

  void async_stop() {
    run_loop_thread_->enqueue(^{
      unregister_callback();
    });
  }

private:
  [[nodiscard]] static std::vector<uint8_t> find_report_descriptor(IOHIDDeviceRef device) {
    std::vector<uint8_t> result;

    if (auto property = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDReportDescriptorKey))) {
      if (CFGetTypeID(property) == CFDataGetTypeID()) {
        auto data = static_cast<CFDataRef>(property);
        auto length = CFDataGetLength(data);
        if (length > 0) {
          result.resize(static_cast<size_t>(length));
          CFDataGetBytes(data, CFRangeMake(0, length), result.data());
        }
      }
    }

    return result;
  }

  // This method should be called in run_loop_thread_.
  void unregister_callback() {
    if (!registered_) {
      return;
    }

    if (auto d = hid_device_.get_device()) {
      IOHIDDeviceRegisterInputReportCallback(*d,
                                             buffer_.data(),
                                             static_cast<CFIndex>(buffer_.size()),
                                             nullptr,
                                             nullptr);
    }

    registered_ = false;
    report_handler_->reset();
  }

  static void static_input_report_callback(void* context,
                                           IOReturn result,
                                           void* sender,
                                           IOHIDReportType type,
                                           uint32_t report_id,
                                           uint8_t* report,
                                           CFIndex report_length) {
    if (auto self = static_cast<hid_report_only_events_monitor*>(context)) {
      self->input_report_callback(result,
                                  type,
                                  report_id,
                                  report,
                                  report_length);
    }
  }

  // This method is called in run_loop_thread_.
  void input_report_callback(IOReturn result,
                             IOHIDReportType type,
                             uint32_t report_id,
                             uint8_t* report,
                             CFIndex report_length) {
    if (result != kIOReturnSuccess ||
        type != kIOHIDReportTypeInput ||
        report == nullptr ||
        report_length < 0) {
      return;
    }

    auto events = report_handler_->handle(
        report_id,
        std::span<const uint8_t>(report,
                                 static_cast<size_t>(report_length)));
    if (events.empty()) {
      return;
    }

    auto time_stamp = pqrs::osx::chrono::absolute_time_point(mach_absolute_time());
    auto values = std::make_shared<std::vector<pqrs::osx::iokit_hid_value>>();

    for (const auto& event : events) {
      values->push_back(pqrs::osx::iokit_hid_value(time_stamp,
                                                   event.value,
                                                   event.usage_page,
                                                   event.usage,
                                                   event.logical_max,
                                                   event.logical_min));
    }

    enqueue_to_dispatcher([this, values] {
      values_arrived(values);
    });
  }

  pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread_;
  pqrs::osx::iokit_hid_device hid_device_;
  std::vector<uint8_t> buffer_;

  // The following are touched only in run_loop_thread_.
  pqrs::not_null_shared_ptr_t<hid_report_only_events::report_handler> report_handler_;
  bool registered_;
};
} // namespace krbn
