#pragma once

#include "hid_report_only_events/logitech/hidpp/session.hpp"
#include "logger.hpp"
#include <IOKit/hid/IOHIDDevice.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <nod/nod.hpp>
#include <pqrs/cf/run_loop_thread.hpp>
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/chrono.hpp>
#include <pqrs/osx/iokit_return.hpp>
#include <unordered_map>

namespace krbn::core_service::daemon::device_grabber_details {
//
// Routes HID++ controls of a seized device by using logitech::hidpp::session.
//
// Thread responsibilities:
// - The shared run loop thread handles the IOKit device lifecycle and writes HID++ requests.
//   It also handles input from all devices, so read-only discovery queries use
//   IOHIDDeviceSetReportWithCallback and a short timeout instead of blocking the thread.
//   Requests which change the reporting of controls (divert and restore) are written synchronously, so that a
//   restore is written after a preceding divert and before a later IOHIDDeviceClose.
//   Requests are written only while controls are activated and restored, never per button press.
// - The shared dispatcher thread receives input reports from hid_device_events_monitor and updates the HID++
//   protocol state. It never waits for a HID++ reply.
//
// Failure behavior:
// If the device rejects a request or does not reply (3 attempts, 2 seconds each; a failed write is not answered),
// the controls are restored and left to the firmware until the next activation (reconnect, wake, restart or
// configuration change). Restoration is best effort. If Karabiner-Core-Service terminates abnormally or the device
// disconnects, the device itself resets temporary diversion when it reconnects.
// Exception: if only the discovery before the diversion (feature, control count, control info or reporting state)
// timed out, the session allows one automatic recovery for the same device. It restarts the discovery when
// handle_physical_activity is called, which the owner does when the device reports physical input again. A refusal,
// a HID++ error, a failed diversion, or a second timeout is not recovered, and the controls stay with the firmware
// until the next activation.
//
class logitech_hidpp_session_controller final : public pqrs::dispatcher::extra::dispatcher_client,
                                                public std::enable_shared_from_this<logitech_hidpp_session_controller> {
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard dispatcher_client_constructor_guard_{*this};

public:
  nod::signal<void(const std::vector<uint16_t>&)> divertable_control_ids_changed;

  logitech_hidpp_session_controller(const logitech_hidpp_session_controller&) = delete;

  // This method should be called in the shared dispatcher thread.
  logitech_hidpp_session_controller(pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread> run_loop_thread,
                                    IOHIDDeviceRef device,
                                    std::vector<uint16_t> control_ids,
                                    const std::string& log_prefix)
      : dispatcher_client(),
        run_loop_thread_(run_loop_thread),
        device_(device),
        log_prefix_(log_prefix),
        session_(std::move(control_ids),
                 next_software_id(),
                 log_prefix) {
    dispatcher_client_constructor_guard_.initialize([] {});
  }

  // This method should be called in the shared dispatcher thread while the device is still opened.
  ~logitech_hidpp_session_controller() override {
    detach_from_dispatcher([this] {
      queries_enabled_->store(false);

      // The owner releases a held button before destroying the controller, or
      // device_ungrabbed releases it after the device has already disappeared.
      handle_requests(session_.stop());
    });
  }

  // This method should be called in the shared dispatcher thread.
  void start() {
    static_cast<void>(handle_result(session_.start()));
  }

  // This method should be called in the shared dispatcher thread.
  [[nodiscard]] std::vector<logitech::hidpp::session::result::button_event> handle_input_report(std::span<const uint8_t> report) {
    return handle_result(session_.handle_input_report(report));
  }

  [[nodiscard]] bool can_recover_on_physical_activity() const {
    return session_.can_recover_on_physical_activity();
  }

  [[nodiscard]] bool can_reuse_for_configuration_update() const {
    return session_.can_reuse_for_configuration_update();
  }

  // Restarts the discovery once if the session timed out before the diversion and has not recovered yet.
  // The owner calls this method when the device reports physical input.
  // This method should be called in the shared dispatcher thread while the device is still opened.
  void handle_physical_activity() {
    if (!can_recover_on_physical_activity()) {
      return;
    }

    logger::get_logger()->info("{0} retrying HID++ discovery after a timeout",
                               log_prefix_);

    // The returned request is the first discovery request, so start() must not be called.
    static_cast<void>(handle_result(session_.restart_after_discovery_timeout(next_software_id())));
  }

  // Restores the controls and reports releases for held buttons.
  // This method should be called in the shared dispatcher thread while the device is still opened.
  [[nodiscard]] std::vector<logitech::hidpp::session::result::button_event> stop() {
    queries_enabled_->store(false);
    return handle_result(session_.stop());
  }

  // Forgets the session without writing to the device or emitting signals.
  // This method should be called in the shared dispatcher thread after the device has been closed.
  void abandon() {
    queries_enabled_->store(false);
    session_.stop();
    clear_query_contexts(*device_);
  }

private:
  static uint8_t next_software_id() {
    // Use different software ids in consecutive sessions so that late replies to a previous session are ignored.
    // This method is called only in the shared dispatcher thread.
    static uint8_t last = 0;
    last = static_cast<uint8_t>(last % 15 + 1);
    return last;
  }

  [[nodiscard]] std::vector<logitech::hidpp::session::result::button_event> handle_result(const logitech::hidpp::session::result& r) {
    if (handle_requests(r)) {
      schedule_reply_timeout(session_.get_request_generation());
    }

    if (session_.controls_discovered()) {
      auto control_ids = session_.get_divertable_control_ids();
      if (control_ids != published_divertable_control_ids_) {
        published_divertable_control_ids_ = control_ids;
        divertable_control_ids_changed(published_divertable_control_ids_);
      }
    }

    return r.button_events;
  }

  // Returns true if a read-only query which waits for a reply was sent.
  // A setCidReporting timeout is scheduled separately after its synchronous
  // IOKit write succeeds.
  bool handle_requests(const logitech::hidpp::session::result& r) {
    bool query_waits_for_reply = false;

    if (r.request) {
      if (logitech::hidpp::is_set_reporting_request(*r.request)) {
        send_synchronously(*r.request,
                           session_.waiting_reply()
                               ? std::optional<uint64_t>(session_.get_request_generation())
                               : std::nullopt);
      } else {
        send_query(*r.request);
        query_waits_for_reply = session_.waiting_reply();
      }
    }

    for (const auto& request : r.additional_requests) {
      if (logitech::hidpp::is_set_reporting_request(request)) {
        send_synchronously(request, std::nullopt);
      } else {
        send_query(request);
      }
    }

    return query_waits_for_reply;
  }

  void schedule_reply_timeout(uint64_t request_generation) {
    enqueue_to_dispatcher(
        [this, request_generation] {
          static_cast<void>(handle_result(session_.handle_timeout(request_generation)));
        },
        when_now() + logitech::hidpp::reply_timeout);
  }

  struct query_context final {
    IOHIDDeviceRef device;
    logitech::hidpp::long_report report;
    std::string log_prefix;
    std::shared_ptr<std::atomic<bool>> queries_enabled;
  };

  // Use an opaque monotonically increasing id rather than the query_context address.
  // A late callback after abandon() must not match a newly allocated context which reused that address.
  using query_context_id = uintptr_t;

  struct registered_query_context final {
    query_context_id id;
    query_context* context;
  };

  static registered_query_context register_query_context(IOHIDDeviceRef device,
                                                          const logitech::hidpp::long_report& report,
                                                          const std::string& log_prefix,
                                                          const std::shared_ptr<std::atomic<bool>>& queries_enabled) {
    auto context = std::make_unique<query_context>(query_context{
        .device = device,
        .report = report,
        .log_prefix = log_prefix,
        .queries_enabled = queries_enabled,
    });

    std::lock_guard lock(query_contexts_mutex());
    auto id = next_query_context_id();
    auto result = registered_query_context{
        .id = id,
        .context = context.get(),
    };
    query_contexts().emplace(id, std::move(context));

    return result;
  }

  [[nodiscard]] static std::unique_ptr<query_context> take_query_context(query_context_id id) {
    std::lock_guard lock(query_contexts_mutex());

    auto it = query_contexts().find(id);
    if (it == std::end(query_contexts())) {
      return nullptr;
    }

    auto result = std::move(it->second);
    query_contexts().erase(it);
    return result;
  }

  static void clear_query_contexts(IOHIDDeviceRef device) {
    std::lock_guard lock(query_contexts_mutex());

    std::erase_if(query_contexts(),
                  [device](const auto& pair) {
                    return pair.second->device == device;
                  });
  }

  // This method is called only while query_contexts_mutex is locked.
  [[nodiscard]] static query_context_id next_query_context_id() {
    static query_context_id value{0};

    do {
      ++value;
    } while (value == 0 || query_contexts().contains(value));

    return value;
  }

  [[nodiscard]] static std::unordered_map<query_context_id, std::unique_ptr<query_context>>& query_contexts() {
    static std::unordered_map<query_context_id, std::unique_ptr<query_context>> value;
    return value;
  }

  [[nodiscard]] static std::mutex& query_contexts_mutex() {
    static std::mutex value;
    return value;
  }

  void send_query(const logitech::hidpp::long_report& request) const {
    auto device = device_;
    auto r = request;
    auto log_prefix = log_prefix_;
    auto queries_enabled = queries_enabled_;

    run_loop_thread_->enqueue(^{
      if (!queries_enabled->load()) {
        return;
      }

      // Keep the report buffer alive until the callback. If device removal suppresses
      // the callback, abandon() clears contexts after the device has been closed.
      auto registered_context = register_query_context(*device, r, log_prefix, queries_enabled);
      pqrs::osx::iokit_return kr = IOHIDDeviceSetReportWithCallback(
          *device,
          kIOHIDReportTypeOutput,
          registered_context.context->report[0],
          registered_context.context->report.data(),
          registered_context.context->report.size(),
          query_timeout,
          [](void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t report_id, uint8_t* report, CFIndex report_length) {
            auto c = take_query_context(reinterpret_cast<query_context_id>(context));
            if (!c) {
              return;
            }

            if (!c->queries_enabled->load()) {
              return;
            }

            // The protocol-level reply timer determines whether a query
            // succeeded. Some BLE devices deliver that reply even when this
            // short IOKit write callback reports kIOReturnTimeout.
            pqrs::osx::iokit_return r(result);
            if (!r && result != kIOReturnTimeout) {
              logger::get_logger()->warn("{0} failed to send a HID++ request: {1}",
                                         c->log_prefix,
                                         r.to_string());
            }
          },
          reinterpret_cast<void*>(registered_context.id));
      if (!kr) {
        // The callback is not invoked when the request is not submitted.
        static_cast<void>(take_query_context(registered_context.id));
        if (queries_enabled->load()) {
          logger::get_logger()->warn("{0} failed to send a HID++ request: {1}",
                                     log_prefix,
                                     kr.to_string());
        }
      }
    });
  }

  void send_synchronously(const logitech::hidpp::long_report& request,
                          std::optional<uint64_t> request_generation) {
    auto device = device_;
    auto r = request;
    auto log_prefix = log_prefix_;
    auto weak_self = weak_from_this();

    run_loop_thread_->enqueue(^{
      pqrs::osx::iokit_return kr = IOHIDDeviceSetReport(*device,
                                                        kIOHIDReportTypeOutput,
                                                        r[0],
                                                        r.data(),
                                                        r.size());

      if (request_generation) {
        if (auto self = weak_self.lock()) {
          self->enqueue_to_dispatcher([weak_self, request_generation = *request_generation, r, kr] {
            if (auto self = weak_self.lock()) {
              if (kr) {
                // Start the protocol reply timeout only after IOKit has accepted
                // the request. Some devices apply setCidReporting without
                // returning the HID++ acknowledgement.
                if (self->session_.handle_request_write_success(request_generation, r)) {
                  self->schedule_reply_timeout(request_generation);
                }
              } else if (auto result = self->session_.handle_request_write_failure(
                             request_generation,
                             r,
                             kr.to_string())) {
                static_cast<void>(self->handle_result(*result));
              }
            }
          });
        }
      } else if (!kr) {
        logger::get_logger()->error("{0} failed to restore the reporting of a control; reconnect the device to ensure its firmware function is restored: {1}",
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
  std::shared_ptr<std::atomic<bool>> queries_enabled_{std::make_shared<std::atomic<bool>>(true)};
  logitech::hidpp::session session_;
  std::vector<uint16_t> published_divertable_control_ids_;
};
} // namespace krbn::core_service::daemon::device_grabber_details
