#pragma once

#include <cstdint>
#include <pqrs/hid.hpp>
#include <span>
#include <vector>

namespace krbn::hid_report_only_events {
struct event final {
  pqrs::hid::usage_page::value_t usage_page;
  pqrs::hid::usage::value_t usage;
  int64_t value;
  int64_t logical_max;
  int64_t logical_min;

  bool operator==(const event&) const = default;
};

// Converts raw input reports into the HID events which macOS does not expose
// through IOHIDElement. Implementations may keep state to emit only changes.
class report_handler {
public:
  virtual ~report_handler() = default;

  [[nodiscard]] virtual std::vector<event> handle(
      uint32_t report_id,
      std::span<const uint8_t> report) = 0;

  virtual void reset() = 0;
};
} // namespace krbn::hid_report_only_events
