#pragma once

// pqrs::cf::number v2.1

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include "number/impl.hpp"
#include <cstdint>

namespace pqrs {
namespace cf {
inline cf_ptr<CFNumberRef> make_cf_number(int8_t value) {
  return impl::make_cf_number<int8_t>(value, kCFNumberSInt8Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(int16_t value) {
  return impl::make_cf_number<int16_t>(value, kCFNumberSInt16Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(int32_t value) {
  return impl::make_cf_number<int32_t>(value, kCFNumberSInt32Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(int64_t value) {
  return impl::make_cf_number<int64_t>(value, kCFNumberSInt64Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(float value) {
  return impl::make_cf_number<float>(value, kCFNumberFloat32Type);
}

inline cf_ptr<CFNumberRef> make_cf_number(double value) {
  return impl::make_cf_number<double>(value, kCFNumberFloat64Type);
}

template <typename T>
std::optional<T> make_number(CFTypeRef value) = delete;

template <>
inline std::optional<int8_t> make_number<int8_t>(CFTypeRef value) {
  return impl::make_number<int8_t>(value, kCFNumberSInt8Type);
}

template <>
inline std::optional<int16_t> make_number<int16_t>(CFTypeRef value) {
  return impl::make_number<int16_t>(value, kCFNumberSInt16Type);
}

template <>
inline std::optional<int32_t> make_number<int32_t>(CFTypeRef value) {
  return impl::make_number<int32_t>(value, kCFNumberSInt32Type);
}

template <>
inline std::optional<int64_t> make_number<int64_t>(CFTypeRef value) {
  return impl::make_number<int64_t>(value, kCFNumberSInt64Type);
}

template <>
inline std::optional<float> make_number<float>(CFTypeRef value) {
  return impl::make_number<float>(value, kCFNumberFloat32Type);
}

template <>
inline std::optional<double> make_number<double>(CFTypeRef value) {
  return impl::make_number<double>(value, kCFNumberFloat64Type);
}
} // namespace cf
} // namespace pqrs
