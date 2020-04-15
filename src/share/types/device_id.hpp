#pragma once

#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/iokit_types/extra/nlohmann_json.hpp>

namespace krbn {
using device_id = pqrs::osx::iokit_registry_entry_id::value_t;

inline device_id make_device_id(const pqrs::osx::iokit_registry_entry_id::value_t& value) {
  return value;
}
} // namespace krbn
