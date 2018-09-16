#pragma once

#include "stream_utility.hpp"
#include <cstdint>

namespace krbn {
enum class pointing_button : uint32_t {
  zero,

  button1,
  button2,
  button3,
  button4,
  button5,
  button6,
  button7,
  button8,

  button9,
  button10,
  button11,
  button12,
  button13,
  button14,
  button15,
  button16,

  button17,
  button18,
  button19,
  button20,
  button21,
  button22,
  button23,
  button24,

  button25,
  button26,
  button27,
  button28,
  button29,
  button30,
  button31,
  button32,

  end_,
};

inline std::ostream& operator<<(std::ostream& stream, const pointing_button& value) {
  return stream_utility::output_enum(stream, value);
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<pointing_button, std::allocator<pointing_button>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<pointing_button, std::hash<pointing_button>, std::equal_to<pointing_button>, std::allocator<pointing_button>>& values) {
  return stream_utility::output_enums(stream, values);
}
} // namespace krbn
