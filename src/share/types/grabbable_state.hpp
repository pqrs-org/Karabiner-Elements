#pragma once

#include "types.hpp"
#include <cstdint>
#include <ostream>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
class grabbable_state final {
public:
  enum class state : uint32_t {
    none,
    grabbable,
    ungrabbable_temporarily,
    ungrabbable_permanently,
    device_error,
  };

  enum class ungrabbable_temporarily_reason : uint32_t {
    none,
    key_repeating,
    modifier_key_pressed,
    pointing_button_pressed,
  };

  grabbable_state(void) : grabbable_state(device_id(0),
                                          state::grabbable,
                                          ungrabbable_temporarily_reason::none,
                                          absolute_time_point(0)) {
  }

  grabbable_state(device_id device_id,
                  state state,
                  ungrabbable_temporarily_reason ungrabbable_temporarily_reason,
                  absolute_time_point time_stamp) : device_id_(device_id),
                                                    state_(state),
                                                    ungrabbable_temporarily_reason_(ungrabbable_temporarily_reason),
                                                    time_stamp_(time_stamp) {
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  state get_state(void) const {
    return state_;
  }

  ungrabbable_temporarily_reason get_ungrabbable_temporarily_reason(void) const {
    return ungrabbable_temporarily_reason_;
  }

  absolute_time_point get_time_stamp(void) const {
    return time_stamp_;
  }

  bool equals_except_time_stamp(const grabbable_state& other) const {
    return device_id_ == other.device_id_ &&
           state_ == other.state_ &&
           ungrabbable_temporarily_reason_ == other.ungrabbable_temporarily_reason_;
  }

  bool operator==(const grabbable_state& other) const {
    return device_id_ == other.device_id_ &&
           state_ == other.state_ &&
           ungrabbable_temporarily_reason_ == other.ungrabbable_temporarily_reason_ &&
           time_stamp_ == other.time_stamp_;
  }

  bool operator!=(const grabbable_state& other) const { return !(*this == other); }

private:
  device_id device_id_;
  state state_;
  ungrabbable_temporarily_reason ungrabbable_temporarily_reason_;
  absolute_time_point time_stamp_;
};

inline std::ostream& operator<<(std::ostream& stream, const grabbable_state::state& value) {
  switch (value) {
    case grabbable_state::state::none:
      stream << "state::none";
      break;
    case grabbable_state::state::grabbable:
      stream << "state::grabbable";
      break;
    case grabbable_state::state::ungrabbable_temporarily:
      stream << "state::ungrabbable_temporarily";
      break;
    case grabbable_state::state::ungrabbable_permanently:
      stream << "state::ungrabbable_permanently";
      break;
    case grabbable_state::state::device_error:
      stream << "state::device_error";
      break;
  }
  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const grabbable_state::ungrabbable_temporarily_reason& value) {
  switch (value) {
    case grabbable_state::ungrabbable_temporarily_reason::none:
      stream << "ungrabbable_temporarily_reason::none";
      break;
    case grabbable_state::ungrabbable_temporarily_reason::key_repeating:
      stream << "ungrabbable_temporarily_reason::key_repeating";
      break;
    case grabbable_state::ungrabbable_temporarily_reason::modifier_key_pressed:
      stream << "ungrabbable_temporarily_reason::modifier_key_pressed";
      break;
    case grabbable_state::ungrabbable_temporarily_reason::pointing_button_pressed:
      stream << "ungrabbable_temporarily_reason::pointing_button_pressed";
      break;
  }
  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const grabbable_state& value) {
  stream << "["
         << value.get_device_id() << ","
         << value.get_state() << ","
         << value.get_ungrabbable_temporarily_reason() << ","
         << value.get_time_stamp()
         << "]";
  return stream;
}
} // namespace krbn
