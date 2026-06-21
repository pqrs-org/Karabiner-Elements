#pragma once

// pqrs::cf::number v2.2.0

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include "number/impl.hpp"
#include <cstdint>

namespace pqrs::cf {
[[nodiscard]] inline cf_ptr<CFNumberRef> make_cf_number(int8_t value) noexcept {
  return impl::make_cf_number<int8_t>(value, kCFNumberSInt8Type);
}

[[nodiscard]] inline cf_ptr<CFNumberRef> make_cf_number(int16_t value) noexcept {
  return impl::make_cf_number<int16_t>(value, kCFNumberSInt16Type);
}

[[nodiscard]] inline cf_ptr<CFNumberRef> make_cf_number(int32_t value) noexcept {
  return impl::make_cf_number<int32_t>(value, kCFNumberSInt32Type);
}

[[nodiscard]] inline cf_ptr<CFNumberRef> make_cf_number(int64_t value) noexcept {
  return impl::make_cf_number<int64_t>(value, kCFNumberSInt64Type);
}

[[nodiscard]] inline cf_ptr<CFNumberRef> make_cf_number(float value) noexcept {
  return impl::make_cf_number<float>(value, kCFNumberFloat32Type);
}

[[nodiscard]] inline cf_ptr<CFNumberRef> make_cf_number(double value) noexcept {
  return impl::make_cf_number<double>(value, kCFNumberFloat64Type);
}

template <typename T>
[[nodiscard]] std::optional<T> make_number(CFTypeRef value) noexcept = delete;

template <>
[[nodiscard]] inline std::optional<int8_t> make_number<int8_t>(CFTypeRef value) noexcept {
  return impl::make_number<int8_t>(value, kCFNumberSInt8Type);
}

template <>
[[nodiscard]] inline std::optional<int16_t> make_number<int16_t>(CFTypeRef value) noexcept {
  return impl::make_number<int16_t>(value, kCFNumberSInt16Type);
}

template <>
[[nodiscard]] inline std::optional<int32_t> make_number<int32_t>(CFTypeRef value) noexcept {
  return impl::make_number<int32_t>(value, kCFNumberSInt32Type);
}

template <>
[[nodiscard]] inline std::optional<int64_t> make_number<int64_t>(CFTypeRef value) noexcept {
  return impl::make_number<int64_t>(value, kCFNumberSInt64Type);
}

template <>
[[nodiscard]] inline std::optional<float> make_number<float>(CFTypeRef value) noexcept {
  return impl::make_number<float>(value, kCFNumberFloat32Type);
}

template <>
[[nodiscard]] inline std::optional<double> make_number<double>(CFTypeRef value) noexcept {
  return impl::make_number<double>(value, kCFNumberFloat64Type);
}
} // namespace pqrs::cf
