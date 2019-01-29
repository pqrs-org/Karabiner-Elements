#pragma once

#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/iokit_types/extra/nlohmann_json.hpp>
#include <pqrs/osx/iokit_types/extra/boost.hpp>

namespace krbn {
using device_id = pqrs::osx::iokit_registry_entry_id;

inline device_id make_device_id(const pqrs::osx::iokit_registry_entry_id& value) {
  return value;
}
} // namespace krbn
