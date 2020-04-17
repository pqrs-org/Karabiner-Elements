#pragma once

// (C) Copyright Takayama Fumihiko 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/hid.hpp>

namespace pqrs {
namespace hid {
namespace country_code {
inline std::size_t hash_value(const value_t& value) {
  return std::hash<value_t>{}(value);
}
} // namespace country_code

namespace product_id {
inline std::size_t hash_value(const value_t& value) {
  return std::hash<value_t>{}(value);
}
} // namespace product_id

namespace usage_page {
inline std::size_t hash_value(const value_t& value) {
  return std::hash<value_t>{}(value);
}
} // namespace usage_page

namespace usage {
inline std::size_t hash_value(const value_t& value) {
  return std::hash<value_t>{}(value);
}
} // namespace usage

namespace vendor_id {
inline std::size_t hash_value(const value_t& value) {
  return std::hash<value_t>{}(value);
}
} // namespace vendor_id

inline std::size_t hash_value(const std::pair<usage_page::value_t, usage::value_t>& value) {
  return std::hash<std::pair<usage_page::value_t, usage::value_t>>{}(value);
}

inline std::size_t hash_value(const std::pair<vendor_id::value_t, product_id::value_t>& value) {
  return std::hash<std::pair<vendor_id::value_t, product_id::value_t>>{}(value);
}
} // namespace hid
} // namespace pqrs
