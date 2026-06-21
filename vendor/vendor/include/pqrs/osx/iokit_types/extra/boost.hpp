#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See https://www.boost.org/LICENSE_1_0.txt)

#include <pqrs/osx/iokit_types.hpp>

namespace pqrs::osx {
namespace iokit_hid_location_id {
[[nodiscard]] inline std::size_t hash_value(const value_t& value) noexcept {
  return std::hash<value_t>{}(value);
}
} // namespace iokit_hid_location_id

namespace iokit_keyboard_type {
[[nodiscard]] inline std::size_t hash_value(const value_t& value) noexcept {
  return std::hash<value_t>{}(value);
}
} // namespace iokit_keyboard_type

namespace iokit_registry_entry_id {
[[nodiscard]] inline std::size_t hash_value(const value_t& value) noexcept {
  return std::hash<value_t>{}(value);
}
} // namespace iokit_registry_entry_id

[[nodiscard]] inline std::size_t hash_value(const std::tuple<hid::vendor_id::value_t,
                                                             hid::product_id::value_t,
                                                             iokit_hid_location_id::value_t>& value) noexcept {
  return std::hash<std::tuple<hid::vendor_id::value_t,
                              hid::product_id::value_t,
                              iokit_hid_location_id::value_t>>{}(value);
}
} // namespace pqrs::osx
