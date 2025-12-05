#pragma once

#include <pqrs/osx/iokit_hid_value.hpp>
#include <unordered_map>

namespace krbn {
class hid_queue_values_converter final {
public:
  hid_queue_values_converter(void)
      : last_time_stamp_(0) {
  }

  std::vector<pqrs::osx::iokit_hid_value> make_hid_values(device_id device_id,
                                                          std::shared_ptr<std::vector<pqrs::cf::cf_ptr<IOHIDValueRef>>> values) {
    std::vector<pqrs::osx::iokit_hid_value> hid_values;

    for (const auto& v : *values) {
      hid_values.emplace_back(*v);

      // Some devices send events with time_stamp == 0 or a fixed value.
      // (Maybe an issue around input event handling of macOS.)
      //
      // For example, Swiftpoint ProPoint sends pointing events with normal time_stamp,
      // but consumer_key_code key_up events with time_stamp == 0.
      // (Sometimes, time_stamp is not zero, but a fixed value.)
      //
      // To properly handle cases such as timeouts in to_if_alone,
      // we correct the timestamp if any backward time drift is detected.

      auto t = hid_values.back().get_time_stamp();
      if (t < last_time_stamp_) {
        hid_values.back().set_time_stamp(last_time_stamp_);
      } else {
        last_time_stamp_ = t;
      }
    }

    return hid_values;
  }

private:
  pqrs::osx::chrono::absolute_time_point last_time_stamp_;
}; // namespace krbn
} // namespace krbn
