#pragma once

#include "stream_utility.hpp"
#include <cstdint>

namespace krbn {
enum class modifier_flag : uint32_t {
  zero,
  caps_lock,
  left_control,
  left_shift,
  left_option,
  left_command,
  right_control,
  right_shift,
  right_option,
  right_command,
  fn,
  end_,
};

inline std::ostream& operator<<(std::ostream& stream, const modifier_flag& value) {
#define KRBN_MODIFIER_FLAG_STREAM_OUTPUT(NAME) \
  case modifier_flag::NAME:                    \
    stream << #NAME;                           \
    break;

  switch (value) {
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(zero);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(caps_lock);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(left_control);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(left_shift);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(left_option);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(left_command);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(right_control);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(right_shift);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(right_option);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(right_command);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(fn);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(end_);
  }

#undef KRBN_MODIFIER_FLAG_STREAM_OUTPUT

  return stream;
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<modifier_flag, std::allocator<modifier_flag>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<modifier_flag, std::hash<modifier_flag>, std::equal_to<modifier_flag>, std::allocator<modifier_flag>>& values) {
  return stream_utility::output_enums(stream, values);
}
} // namespace krbn
