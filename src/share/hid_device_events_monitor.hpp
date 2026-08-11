#pragma once

#include "device_properties.hpp"
#include "hid_report_only_events.hpp"
#include <chrono>
#include <memory>
#include <nod/nod.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/gsl.hpp>
#include <pqrs/osx/iokit_hid_device_events_monitor.hpp>
#include <pqrs/osx/iokit_hid_value.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <string>
#include <vector>

namespace krbn {
// Combines the values exposed by IOHIDQueue with events recovered directly from
// raw input reports, and publishes both through one chronologically consistent stream.
class hid_device_events_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  //
  // Signals (invoked from the shared dispatcher thread)
  //

  nod::signal<void()> started;
  nod::signal<void()> stopped;
  nod::signal<void(pqrs::not_null_shared_ptr_t<std::vector<pqrs::osx::iokit_hid_value>>)> values_arrived;
  nod::signal<void(const std::string&, pqrs::osx::iokit_return)> error_occurred;

  //
  // Methods
  //

  hid_device_events_monitor(const hid_device_events_monitor&) = delete;

  hid_device_events_monitor(
      std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
      pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread,
      IOHIDDeviceRef device,
      const device_properties& device_properties)
      : dispatcher_client(weak_dispatcher),
        last_time_stamp_(0) {
    pqrs::osx::iokit_hid_device_events_monitor::parameters parameters;

    const auto& identifiers = device_properties.get_device_identifiers();
    if (hid_report_only_events::is_target_device(identifiers)) {
      // Reading and parsing the descriptor is unnecessary for all other devices.
      auto report_descriptor = find_report_descriptor(device);

      input_report_handler_ =
          hid_report_only_events::make_report_handler(
              identifiers,
              report_descriptor);

      if (input_report_handler_) {
        parameters.observe_input_reports = true;

        parameters.input_report_filter =
            [handler = input_report_handler_](auto report_id, auto report) {
              return handler->should_accept_report(report_id, report);
            };
        parameters.input_report_filter_started =
            [handler = input_report_handler_] {
              handler->reset_filter_state();
            };
      }
    }

    device_events_monitor_ =
        std::make_shared<pqrs::osx::iokit_hid_device_events_monitor>(
            weak_dispatcher,
            run_loop_thread,
            device,
            parameters);

    device_events_monitor_->started.connect([this] {
      if (input_report_handler_) {
        // The vendor monitor enqueues started before any input_report_arrived
        // signal from the newly opened device.
        input_report_handler_->reset();
      }

      started();
    });

    device_events_monitor_->stopped.connect([this] {
      stopped();
    });

    device_events_monitor_->input_values_arrived.connect([this](auto&& values) {
      auto hid_values = std::make_shared<std::vector<pqrs::osx::iokit_hid_value>>();
      hid_values->reserve(values->size());

      for (const auto& value : *values) {
        hid_values->emplace_back(*value);
      }

      input_values_arrived(hid_values);
    });

    device_events_monitor_->input_report_arrived.connect(
        [this](auto report_id, auto report, auto time_stamp) {
          if (!input_report_handler_) {
            return;
          }

          auto hid_values = std::make_shared<std::vector<pqrs::osx::iokit_hid_value>>(
              input_report_handler_->handle(
                  report_id,
                  report,
                  time_stamp));
          if (hid_values->empty()) {
            return;
          }

          input_values_arrived(hid_values);
        });

    device_events_monitor_->error_occurred.connect([this](auto&& message, auto&& result) {
      error_occurred(message, result);
    });
  }

  ~hid_device_events_monitor() override {
    detach_from_dispatcher([this] {
      device_events_monitor_ = nullptr;
    });
  }

  void async_start(IOOptionBits open_options,
                   std::chrono::milliseconds open_timer_interval) {
    device_events_monitor_->async_start(open_options,
                                        open_timer_interval);
  }

  void async_stop() {
    device_events_monitor_->async_stop();
  }

  [[nodiscard]] bool seized() const {
    return device_events_monitor_->seized();
  }

private:
  void input_values_arrived(
      pqrs::not_null_shared_ptr_t<std::vector<pqrs::osx::iokit_hid_value>> hid_values) {
    normalize_time_stamps(*hid_values);
    values_arrived(hid_values);
  }

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

  void normalize_time_stamps(
      std::vector<pqrs::osx::iokit_hid_value>& values) {
    for (auto& value : values) {
      // Some devices send events with a timestamp that is zero, fixed, or moves
      // backwards, possibly due to macOS input event handling.
      //
      // For example, Swiftpoint ProPoint sends pointing events with normal
      // timestamps, but consumer key-up events with a zero or fixed timestamp.
      // Correct backward movement so time-dependent features such as to_if_alone
      // continue to work correctly.
      //
      // The IOHIDQueue and raw-report callbacks can also arrive in either order,
      // even when their events originated in the same input report. Applying the
      // same correction to both sources keeps their combined stream monotonic.
      auto time_stamp = value.get_time_stamp();
      if (time_stamp < last_time_stamp_) {
        value.set_time_stamp(last_time_stamp_);
      } else {
        last_time_stamp_ = time_stamp;
      }
    }
  }

  std::shared_ptr<pqrs::osx::iokit_hid_device_events_monitor> device_events_monitor_;

  // should_accept_report and reset_filter_state access filter state from run_loop_thread, while
  // handle and reset access handler state from the shared dispatcher thread.
  std::shared_ptr<hid_report_only_events::report_handler> input_report_handler_;
  pqrs::osx::chrono::absolute_time_point last_time_stamp_;
};
} // namespace krbn
