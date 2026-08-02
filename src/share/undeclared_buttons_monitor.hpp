#pragma once

#include "device_properties.hpp"
#include "types.hpp"
#include "undeclared_buttons.hpp"
#include <mach/mach_time.h>
#include <nod/nod.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/iokit_hid_device.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>
#include <pqrs/thread_wait.hpp>
#include <vector>

namespace krbn {
// Publishes the pointing buttons that krbn::undeclared_buttons recovers from a device's
// raw input reports as ordinary HID values, so they flow through the rest of Karabiner as
// regular pointing buttons.
//
// This class is the only part of the feature that touches IOKit; decoding stays in
// undeclared_buttons.hpp so it can be tested without hardware.
class undeclared_buttons_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  //
  // Signals (invoked from the shared dispatcher thread)
  //

  nod::signal<void(pqrs::not_null_shared_ptr_t<std::vector<pqrs::osx::iokit_hid_value>>)> values_arrived;

  //
  // Methods
  //

  undeclared_buttons_monitor(const undeclared_buttons_monitor&) = delete;

  undeclared_buttons_monitor(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
                             pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread,
                             IOHIDDeviceRef device,
                             const undeclared_buttons::configuration& configuration)
      : dispatcher_client(weak_dispatcher),
        run_loop_thread_(run_loop_thread),
        hid_device_(device),
        decoder_(configuration),
        registered_(false) {
    // IOHIDDeviceRegisterInputReportCallback writes into a buffer we own, so it has to be
    // large enough for anything the device can send.
    auto size = hid_device_.find_max_input_report_size().value_or(64);
    if (size < 1) {
      size = 64;
    }
    buffer_.resize(static_cast<size_t>(size));
  }

  ~undeclared_buttons_monitor() override {
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
  [[nodiscard]] static std::shared_ptr<undeclared_buttons_monitor> make(
      std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
      pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread,
      IOHIDDeviceRef device,
      const device_properties& device_properties) {
    const auto& identifiers = device_properties.get_device_identifiers();

    // The spec describes the mouse collection's report, so it must not be applied to the
    // keyboard or vendor-defined interfaces of the same physical device.
    if (!identifiers.get_is_pointing_device()) {
      return nullptr;
    }

    auto report_descriptor = find_report_descriptor(device);
    auto configuration = undeclared_buttons::find_configuration(identifiers.get_vendor_id(),
                                                                identifiers.get_product_id(),
                                                                report_descriptor);
    if (!configuration) {
      return nullptr;
    }

    return std::make_shared<undeclared_buttons_monitor>(weak_dispatcher,
                                                        run_loop_thread,
                                                        device,
                                                        *configuration);
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
    decoder_.reset();
  }

  static void static_input_report_callback(void* context,
                                           IOReturn result,
                                           void* sender,
                                           IOHIDReportType type,
                                           uint32_t report_id,
                                           uint8_t* report,
                                           CFIndex report_length) {
    if (auto self = static_cast<undeclared_buttons_monitor*>(context)) {
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

    auto changes = decoder_.update(static_cast<uint8_t>(report_id),
                                   std::span<const uint8_t>(report,
                                                            static_cast<size_t>(report_length)));
    if (changes.empty()) {
      return;
    }

    auto time_stamp = pqrs::osx::chrono::absolute_time_point(mach_absolute_time());
    auto values = std::make_shared<std::vector<pqrs::osx::iokit_hid_value>>();

    for (const auto& c : changes) {
      values->push_back(pqrs::osx::iokit_hid_value(time_stamp,
                                                   c.pressed ? 1 : 0,
                                                   pqrs::hid::usage_page::button,
                                                   pqrs::hid::usage::value_t(c.button),
                                                   1,
                                                   0));
    }

    enqueue_to_dispatcher([this, values] {
      values_arrived(values);
    });
  }

  pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread_;
  pqrs::osx::iokit_hid_device hid_device_;
  std::vector<uint8_t> buffer_;

  // The following are touched only in run_loop_thread_.
  undeclared_buttons::decoder decoder_;
  bool registered_;
};
} // namespace krbn
