#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("virtual_hid_devices_state") {
  {
    krbn::virtual_hid_devices_state virtual_hid_devices_state;

    REQUIRE(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == false);

    virtual_hid_devices_state.set_virtual_hid_keyboard_ready(true);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == true);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == false);

    virtual_hid_devices_state.set_virtual_hid_keyboard_ready(false);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == false);

    virtual_hid_devices_state.set_virtual_hid_pointing_ready(true);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == true);

    virtual_hid_devices_state.set_virtual_hid_pointing_ready(false);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == false);

    virtual_hid_devices_state.set_virtual_hid_keyboard_ready(true);

    auto json = nlohmann::json::object({
        {"virtual_hid_keyboard_ready", true},
        {"virtual_hid_pointing_ready", false},
    });

    REQUIRE(nlohmann::json(virtual_hid_devices_state) == json);
  }
  {
    auto json = nlohmann::json::object({
        {"virtual_hid_keyboard_ready", false},
        {"virtual_hid_pointing_ready", true},
    });

    auto virtual_hid_devices_state = json.get<krbn::virtual_hid_devices_state>();

    REQUIRE(virtual_hid_devices_state.get_virtual_hid_keyboard_ready() == false);
    REQUIRE(virtual_hid_devices_state.get_virtual_hid_pointing_ready() == true);
  }
}
