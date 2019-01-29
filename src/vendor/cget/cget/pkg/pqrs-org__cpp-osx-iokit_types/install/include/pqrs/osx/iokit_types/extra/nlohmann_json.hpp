#pragma once

// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <nlohmann/json.hpp>
#include <pqrs/osx/iokit_types/iokit_hid_location_id.hpp>
#include <pqrs/osx/iokit_types/iokit_hid_product_id.hpp>
#include <pqrs/osx/iokit_types/iokit_hid_usage.hpp>
#include <pqrs/osx/iokit_types/iokit_hid_usage_page.hpp>
#include <pqrs/osx/iokit_types/iokit_hid_vendor_id.hpp>
#include <pqrs/osx/iokit_types/iokit_registry_entry_id.hpp>

namespace pqrs {
namespace osx {
// iokit_hid_location_id

inline void to_json(nlohmann::json& j, const iokit_hid_location_id& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, iokit_hid_location_id& value) {
  value = iokit_hid_location_id(j.get<uint64_t>());
}

// iokit_hid_product_id

inline void to_json(nlohmann::json& j, const iokit_hid_product_id& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, iokit_hid_product_id& value) {
  value = iokit_hid_product_id(j.get<uint64_t>());
}

// iokit_hid_usage

inline void to_json(nlohmann::json& j, const iokit_hid_usage& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, iokit_hid_usage& value) {
  value = iokit_hid_usage(j.get<int32_t>());
}

// iokit_hid_usage_page

inline void to_json(nlohmann::json& j, const iokit_hid_usage_page& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, iokit_hid_usage_page& value) {
  value = iokit_hid_usage_page(j.get<int32_t>());
}

// iokit_hid_vendor_id

inline void to_json(nlohmann::json& j, const iokit_hid_vendor_id& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, iokit_hid_vendor_id& value) {
  value = iokit_hid_vendor_id(j.get<uint64_t>());
}

// iokit_registry_entry_id

inline void to_json(nlohmann::json& j, const iokit_registry_entry_id& value) {
  j = type_safe::get(value);
}

inline void from_json(const nlohmann::json& j, iokit_registry_entry_id& value) {
  value = iokit_registry_entry_id(j.get<uint64_t>());
}
} // namespace osx
} // namespace pqrs
