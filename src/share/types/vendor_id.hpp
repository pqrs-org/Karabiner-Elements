#pragma once

#include <pqrs/hid.hpp>
#include <pqrs/hid/extra/nlohmann_json.hpp>

namespace krbn {
using vendor_id = pqrs::hid::vendor_id::value_t;

constexpr vendor_id vendor_id_karabiner_virtual_hid_device(0x16c0);
} // namespace krbn
