#pragma once

// HID++ reports are vendor-defined input reports, so macOS does not expose
// their diverted controls through IOHIDElement. This handler converts a
// control's pressed-state changes into logical logitech_hidpp_button HID values.

#include "hid_report_only_events/report_handler.hpp"
#include "hid_report_only_events/logitech/hidpp/session.hpp"
#include "types/momentary_switch_event_details/logitech_hidpp_button.hpp"
#include <functional>
#include <optional>
#include <pqrs/hid.hpp>
#include <span>
#include <utility>
#include <vector>

namespace krbn::hid_report_only_events::logitech::hidpp::button {
class handler final : public krbn::hid_report_only_events::report_handler {
public:
  using input_report_handler =
      std::function<std::optional<bool>(std::span<const uint8_t>)>;

  // These methods should be called in the shared dispatcher thread.
  void set_input_report_handler(
      input_report_handler input_report_handler,
      uint16_t control_id) {
    input_report_handler_ = std::move(input_report_handler);
    control_id_ = control_id;
  }

  void clear_input_report_handler() {
    input_report_handler_ = nullptr;
  }

  // This method is called in the run loop thread and must not access
  // input_report_handler_, which belongs to the shared dispatcher thread.
  [[nodiscard]] bool should_accept_report(
      uint32_t report_id,
      std::span<const uint8_t> report) override {
    return krbn::logitech::hidpp::is_long_report(report_id, report);
  }

  // This method is called in the shared dispatcher thread.
  [[nodiscard]] std::vector<pqrs::osx::iokit_hid_value> handle(
      uint32_t report_id,
      std::span<const uint8_t> report,
      pqrs::osx::chrono::absolute_time_point time_stamp) override {
    std::vector<pqrs::osx::iokit_hid_value> result;

    if (input_report_handler_) {
      if (auto pressed = input_report_handler_(report)) {
        result.emplace_back(time_stamp,
                            *pressed ? 1 : 0,
                            momentary_switch_event_details::logitech_hidpp_button::usage_page,
                            pqrs::hid::usage::value_t(control_id_),
                            1,
                            0);
      }
    }

    return result;
  }

  // The HID++ session lifecycle is controlled explicitly by the owner.
  void reset() override {
  }

private:
  // Only the shared dispatcher thread reads or writes these members.
  input_report_handler input_report_handler_;
  uint16_t control_id_ = 0;
};
} // namespace krbn::hid_report_only_events::logitech::hidpp::button
