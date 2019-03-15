#pragma once

#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/iokit_types/extra/nlohmann_json.hpp>

namespace krbn {
using vendor_id = pqrs::osx::iokit_hid_vendor_id;

constexpr vendor_id vendor_id_karabiner_virtual_hid_device(0x16c0);
} // namespace krbn
