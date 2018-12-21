#pragma once

// pqrs::cf::number v2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <cstdint>
#include <pqrs/cf/cf_ptr.hpp>

namespace pqrs {
namespace cf {
template <typename T>
inline cf_ptr<CFNumberRef> make_cf_number(T value, CFNumberType type) {
  cf_ptr<CFNumberRef> result;

  if (auto number = CFNumberCreate(kCFAllocatorDefault, type, &value)) {
    result = number;
    CFRelease(number);
  }

  return result;
}

inline cf_ptr<CFNumberRef> make_cf_number(int8_t value) {
  return make_cf_number<int8_t>(value, kCFNumberSInt8Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(int16_t value) {
  return make_cf_number<int16_t>(value, kCFNumberSInt16Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(int32_t value) {
  return make_cf_number<int32_t>(value, kCFNumberSInt32Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(int64_t value) {
  return make_cf_number<int64_t>(value, kCFNumberSInt64Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(float value) {
  return make_cf_number<float>(value, kCFNumberFloat32Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(double value) {
  return make_cf_number<double>(value, kCFNumberFloat64Type);
}
} // namespace cf
} // namespace pqrs
