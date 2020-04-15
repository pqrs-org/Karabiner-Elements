#pragma once

#include <pqrs/osx/iokit_types.hpp>
#include <pqrs/osx/iokit_types/extra/nlohmann_json.hpp>

namespace krbn {
using product_id = pqrs::osx::iokit_hid_product_id::value_t;

constexpr product_id product_id_karabiner_virtual_hid_keyboard(0x27db);
constexpr product_id product_id_karabiner_virtual_hid_pointing(0x27da);
} // namespace krbn
