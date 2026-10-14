#pragma once

#include "hidpp/session.hpp"
#include "logger.hpp"
#include <IOKit/hid/IOHIDDevice.h>
#include <nod/nod.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/chrono.hpp>
#include <pqrs/osx/iokit_return.hpp>

namespace krbn::core_service::daemon::device_grabber_details {
//
// Routes one HID++ control of a seized device by using hidpp::session.
//
// Threads:
// - HID++ requests are written in the shared run loop thread, where the device is scheduled, opened and closed.
//   The run loop thread also handles input of all devices, so read-only discovery queries are written with
//   IOHIDDeviceSetReportWithCallback and a short timeout instead of blocking it.
//   Requests which change the reporting of the control (divert and restore) are written synchronously, so that a
//   restore is written after a preceding divert and before a later IOHIDDeviceClose.
//   Requests are written only while the control is activated and restored, never per button press.
// - Input reports are delivered by hid_device_events_monitor, and protocol state and signals run in the shared
//   dispatcher thread. The dispatcher never waits for a HID++ reply.
//
// Failure behavior:
// If the device rejects a request or does not reply (3 attempts, 2 seconds each; a failed write is not answered),
// the control is restored and left to the firmware until the next activation (reconnect, wake, restart or
// configuration change). Restoration is best effort. If Karabiner-Core-Service terminates abnormally or the device
// disconnects, the device itself resets temporary diversion when it reconnects.
// Exception: if only the discovery before the diversion (feature, control count, control info or reporting state)
// timed out, the session allows one automatic recovery for the same device. It restarts the discovery when
// handle_physical_activity is called, which the owner does when the device reports motion again. A refusal, a HID++
// error, a failed diversion, or a second timeout is not recovered, and the control stays with the firmware until the
// next activation.
//
class hidpp_button_controller final : public pqrs::dispatcher::extra::dispatcher_client {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  //
  // Signals (invoked from the shared dispatcher thread)
  //

  nod::signal<void(bool pressed, absolute_time_point time_stamp)> pressed_changed;

  //
  // Methods
  //

  hidpp_button_controller(const hidpp_button_controller&) = delete;

  // This method should be called in the shared dispatcher thread.
  hidpp_button_controller(pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread,
                          IOHIDDeviceRef device,
                          uint16_t control_id,
                          const std::string& log_prefix)
      : dispatcher_client(),
        run_loop_thread_(run_loop_thread),
        device_(device),
        log_prefix_(log_prefix),
        session_(control_id,
                 next_software_id(),
                 log_prefix) {
    dispatcher_client_constructor_guard_.initialize([] {});
  }

  // This method should be called in the shared dispatcher thread while the device is still opened.
  ~hidpp_button_controller() override {
    detach_from_dispatcher([this] {
      // Do not emit signals while the owner is being destroyed.
      // A held button is released by the owner (explicit release or device_ungrabbed).
      handle_requests(session_.stop());
    });
  }

  // This method should be called in the shared dispatcher thread.
  void start() {
    handle_result(session_.start(),
                  pqrs::osx::chrono::mach_absolute_time_point());
  }

  // This method should be called in the shared dispatcher thread.
  void handle_input_report(std::span<const uint8_t> report,
                           absolute_time_point time_stamp) {
    handle_result(session_.handle_input_report(report),
                  time_stamp);
  }

  [[nodiscard]] bool can_recover_on_physical_activity() const {
    return session_.can_recover_on_physical_activity();
  }

  // Restarts the discovery once if the session timed out before the diversion and has not recovered yet.
  // The owner calls this method when the device reports physical motion.
  // This method should be called in the shared dispatcher thread while the device is still opened.
  void handle_physical_activity() {
    if (!can_recover_on_physical_activity()) {
      return;
    }

    logger::get_logger()->info("{0} restarting discovery once because the device reported motion after a discovery timeout",
                               log_prefix_);

    // The returned request is the first discovery request, so start() must not be called.
    handle_result(session_.restart_after_discovery_timeout(next_software_id()),
                  pqrs::osx::chrono::mach_absolute_time_point());
  }

  // Restores the control and reports a release if the button is held.
  // This method should be called in the shared dispatcher thread while the device is still opened.
  void stop() {
    handle_result(session_.stop(),
                  pqrs::osx::chrono::mach_absolute_time_point());
  }

  // Forgets the session without writing to the device or emitting signals.
  // This method should be called in the shared dispatcher thread after the device has been closed.
  void abandon() {
    session_.stop();
  }

private:
  static uint8_t next_software_id() {
    // Use different software ids in consecutive sessions so that late replies to a previous session are ignored.
    // This method is called only in the shared dispatcher thread.
    static uint8_t last = 0;
    last = static_cast<uint8_t>(last % 15 + 1);
    return last;
  }

  void handle_result(const hidpp::session::result& r,
                     absolute_time_point time_stamp) {
    if (handle_requests(r)) {
      auto generation = session_.get_generation();
      enqueue_to_dispatcher(
          [this, generation] {
            handle_result(session_.handle_timeout(generation),
                          pqrs::osx::chrono::mach_absolute_time_point());
          },
          when_now() + hidpp::reply_timeout);
    }

    if (r.pressed) {
      pressed_changed(*r.pressed, time_stamp);
    }
  }

  // Returns true if a request which waits for a reply was sent.
  bool handle_requests(const hidpp::session::result& r) const {
    if (!r.request) {
      return false;
    }

    if (hidpp::is_set_reporting_request(*r.request)) {
      send_synchronously(*r.request);
    } else {
      send_query(*r.request);
    }

    return session_.waiting_reply();
  }

  struct query_context final {
    hidpp::long_report report;
    std::string log_prefix;
  };

  void send_query(const hidpp::long_report& request) const {
    auto device = device_;
    auto r = request;
    auto log_prefix = log_prefix_;

    run_loop_thread_->enqueue(^{
      // The context (including the report buffer) is deleted by the callback, which is also invoked on timeout.
      // If the device is removed while the request is in flight, the callback may not be invoked and the small
      // context is leaked.
      auto context = new query_context{r, log_prefix};
      pqrs::osx::iokit_return kr = IOHIDDeviceSetReportWithCallback(
          *device,
          kIOHIDReportTypeOutput,
          context->report[0],
          context->report.data(),
          context->report.size(),
          query_timeout,
          [](void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t report_id, uint8_t* report, CFIndex report_length) {
            auto c = static_cast<query_context*>(context);
            pqrs::osx::iokit_return r(result);
            if (!r) {
              logger::get_logger()->warn("{0} failed to send a HID++ request: {1}",
                                         c->log_prefix,
                                         r.to_string());
            }
            delete c;
          },
          context);
      if (!kr) {
        // The callback is not invoked when the request is not submitted.
        logger::get_logger()->warn("{0} failed to send a HID++ request: {1}",
                                   log_prefix,
                                   kr.to_string());
        delete context;
      }
    });
  }

  void send_synchronously(const hidpp::long_report& request) const {
    auto device = device_;
    auto r = request;
    auto log_prefix = log_prefix_;

    run_loop_thread_->enqueue(^{
      pqrs::osx::iokit_return kr = IOHIDDeviceSetReport(*device,
                                                        kIOHIDReportTypeOutput,
                                                        r[0],
                                                        r.data(),
                                                        r.size());
      if (!kr) {
        logger::get_logger()->warn("{0} failed to change the reporting of the control; it may stay diverted until the device reconnects: {1}",
                                   log_prefix,
                                   kr.to_string());
      }
    });
  }

  // Milliseconds (IOHIDDeviceSetReportWithCallback documents its CFTimeInterval timeout in milliseconds).
  static constexpr CFTimeInterval query_timeout = 500;

  pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread_;
  pqrs::cf::cf_ptr<IOHIDDeviceRef> device_;
  std::string log_prefix_;
  hidpp::session session_;
};
} // namespace krbn::core_service::daemon::device_grabber_details
