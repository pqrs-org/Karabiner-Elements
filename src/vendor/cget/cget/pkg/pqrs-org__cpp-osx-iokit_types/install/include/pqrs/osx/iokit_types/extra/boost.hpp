#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/iokit_types/hash.hpp>
#include <pqrs/osx/iokit_types/iokit_hid_country_code.hpp>
#include <pqrs/osx/iokit_types/iokit_keyboard_type.hpp>
#include <pqrs/osx/iokit_types/iokit_registry_entry_id.hpp>

namespace pqrs {
namespace osx {
inline std::size_t hash_value(const iokit_hid_country_code::value_t& value) {
  return std::hash<iokit_hid_country_code::value_t>{}(value);
}

inline std::size_t hash_value(const iokit_hid_location_id::value_t& value) {
  return std::hash<iokit_hid_location_id::value_t>{}(value);
}

inline std::size_t hash_value(const iokit_hid_product_id::value_t& value) {
  return std::hash<iokit_hid_product_id::value_t>{}(value);
}

inline std::size_t hash_value(const iokit_hid_usage::value_t& value) {
  return std::hash<iokit_hid_usage::value_t>{}(value);
}

inline std::size_t hash_value(const iokit_hid_usage_page::value_t& value) {
  return std::hash<iokit_hid_usage_page::value_t>{}(value);
}

inline std::size_t hash_value(const iokit_hid_vendor_id::value_t& value) {
  return std::hash<iokit_hid_vendor_id::value_t>{}(value);
}

inline std::size_t hash_value(const iokit_keyboard_type::value_t& value) {
  return std::hash<iokit_keyboard_type::value_t>{}(value);
}

inline std::size_t hash_value(const iokit_registry_entry_id::value_t& value) {
  return std::hash<iokit_registry_entry_id::value_t>{}(value);
}

inline std::size_t hash_value(const std::pair<iokit_hid_usage_page::value_t, iokit_hid_usage::value_t>& value) {
  return std::hash<std::pair<iokit_hid_usage_page::value_t, iokit_hid_usage::value_t>>{}(value);
}

inline std::size_t hash_value(const std::pair<iokit_hid_vendor_id::value_t, iokit_hid_product_id::value_t>& value) {
  return std::hash<std::pair<iokit_hid_vendor_id::value_t, iokit_hid_product_id::value_t>>{}(value);
}

inline std::size_t hash_value(const std::tuple<iokit_hid_vendor_id::value_t, iokit_hid_product_id::value_t, iokit_hid_location_id::value_t>& value) {
  return std::hash<std::tuple<iokit_hid_vendor_id::value_t, iokit_hid_product_id::value_t, iokit_hid_location_id::value_t>>{}(value);
}
} // namespace osx
} // namespace pqrs
