#pragma once

#include <cstdint>
#include <pqrs/osx/iokit_hid_value.hpp>
#include <span>
#include <vector>

namespace krbn::hid_report_only_events {
// Converts raw input reports into the HID events which macOS does not expose
// through IOHIDElement. Implementations may keep state to emit only changes.
class report_handler {
public:
  virtual ~report_handler() = default;

  // should_accept_report and reset_filter_state are called serially in the run loop thread.
  // They may run concurrently with handle or reset in the dispatcher thread,
  // so implementations must keep filter state separate from handler state.
  [[nodiscard]] virtual bool should_accept_report(
      uint32_t report_id,
      std::span<const uint8_t> report) = 0;

  virtual void reset_filter_state() {
  }

  // report must have been accepted by should_accept_report.
  [[nodiscard]] virtual std::vector<pqrs::osx::iokit_hid_value> handle(
      uint32_t report_id,
      std::span<const uint8_t> report,
      pqrs::osx::chrono::absolute_time_point time_stamp) = 0;

  virtual void reset() = 0;
};
} // namespace krbn::hid_report_only_events
