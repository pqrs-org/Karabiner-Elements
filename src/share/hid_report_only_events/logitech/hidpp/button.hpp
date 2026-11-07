#pragma once

// HID++ reports are vendor-defined input reports, so macOS does not expose
// their diverted controls through IOHIDElement. This handler converts a
// controls' pressed-state changes into logical logitech_hidpp_button HID values.

#include "hid_report_only_events/logitech/hidpp/session.hpp"
#include "hid_report_only_events/report_handler.hpp"
#include "types/momentary_switch_event_details/logitech_hidpp_button.hpp"
#include <functional>
#include <optional>
#include <pqrs/hid.hpp>
#include <span>
#include <utility>
#include <vector>

namespace krbn::hid_report_only_events::logitech::hidpp::button {
inline std::vector<pqrs::osx::iokit_hid_value> make_hid_values(
    const std::vector<krbn::logitech::hidpp::session::result::button_event>& events,
    pqrs::osx::chrono::absolute_time_point time_stamp) {
  std::vector<pqrs::osx::iokit_hid_value> result;

  for (const auto& event : events) {
    result.emplace_back(time_stamp,
                        event.pressed ? 1 : 0, // integer_value
                        momentary_switch_event_details::logitech_hidpp_button::usage_page,
                        pqrs::hid::usage::value_t(event.control_id),
                        1, // logical_max
                        0  // logical_min
    );
  }

  return result;
}

class handler final : public krbn::hid_report_only_events::report_handler {
public:
  using input_report_handler =
      std::function<std::vector<krbn::logitech::hidpp::session::result::button_event>(std::span<const uint8_t>)>;

  // These methods should be called in the shared dispatcher thread.
  void set_input_report_handler(
      input_report_handler input_report_handler) {
    input_report_handler_ = std::move(input_report_handler);
  }

  void clear_input_report_handler() {
    input_report_handler_ = nullptr;
  }

  // This method is called in the run loop thread and must not access
  // input_report_handler_, which belongs to the shared dispatcher thread.
  [[nodiscard]] bool should_accept_report(
      uint32_t report_id,
      std::span<const uint8_t> report) override {
    return krbn::logitech::hidpp::is_hidpp_report(report_id, report);
  }

  // This method is called in the shared dispatcher thread.
  [[nodiscard]] std::vector<pqrs::osx::iokit_hid_value> handle(
      uint32_t report_id,
      std::span<const uint8_t> report,
      pqrs::osx::chrono::absolute_time_point time_stamp) override {
    if (input_report_handler_) {
      return make_hid_values(input_report_handler_(report),
                             time_stamp);
    }

    return {};
  }

  // The HID++ session lifecycle is controlled explicitly by the owner.
  void reset() override {
  }

private:
  // Only the shared dispatcher thread reads or writes these members.
  input_report_handler input_report_handler_;
};
} // namespace krbn::hid_report_only_events::logitech::hidpp::button
