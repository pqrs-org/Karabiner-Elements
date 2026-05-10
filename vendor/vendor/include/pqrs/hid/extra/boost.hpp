#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/hid.hpp>

namespace pqrs::hid {
namespace detail {
template <typename T>
  requires type_safe::is_strong_typedef<T>::value
inline std::size_t hash_value(const T& value) {
  return std::hash<T>{}(value);
}
} // namespace detail

//
// number values
//

namespace country_code {
using detail::hash_value;
} // namespace country_code

namespace product_id {
using detail::hash_value;
} // namespace product_id

namespace report_id {
using detail::hash_value;
} // namespace report_id

namespace usage_page {
using detail::hash_value;
} // namespace usage_page

namespace usage {
using detail::hash_value;
} // namespace usage

namespace vendor_id {
using detail::hash_value;
} // namespace vendor_id

//
// string values
//

namespace manufacturer_string {
using detail::hash_value;
} // namespace manufacturer_string

namespace product_string {
using detail::hash_value;
} // namespace product_string

//
// usage_pair
//

inline std::size_t hash_value(const usage_pair& value) {
  return std::hash<usage_pair>{}(value);
}

inline std::size_t hash_value(const std::pair<usage_page::value_t, usage::value_t>& value) {
  return std::hash<std::pair<usage_page::value_t, usage::value_t>>{}(value);
}

inline std::size_t hash_value(const std::pair<vendor_id::value_t, product_id::value_t>& value) {
  return std::hash<std::pair<vendor_id::value_t, product_id::value_t>>{}(value);
}
} // namespace pqrs::hid
