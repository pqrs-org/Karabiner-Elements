#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "virtual_hid_device_utility.hpp"

TEST_CASE("karabiner_virtual_hid_device::hid_report::modifiers") {
  {
    pqrs::karabiner_virtual_hid_device::hid_report::modifiers modifiers;
    modifiers.insert(pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_shift);
    modifiers.insert(pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_command);

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("left_shift");
    expected.push_back("right_command");

    REQUIRE(nlohmann::json(modifiers).dump() == expected.dump());
  }
}

TEST_CASE("karabiner_virtual_hid_device::hid_report::keys") {
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keys keys;
    keys.insert(static_cast<uint8_t>(*(krbn::types::make_hid_usage(krbn::key_code::a))));
    keys.insert(static_cast<uint8_t>(*(krbn::types::make_hid_usage(krbn::key_code::b))));
    keys.insert(static_cast<uint8_t>(*(krbn::types::make_hid_usage(krbn::key_code::c))));
    keys.erase(static_cast<uint8_t>(*(krbn::types::make_hid_usage(krbn::key_code::a))));

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("b");
    expected.push_back("c");

    REQUIRE(krbn::virtual_hid_device_utility::to_json(keys, krbn::hid_usage_page::keyboard_or_keypad).dump() == expected.dump());
  }
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keys keys;
    keys.insert(static_cast<uint8_t>(*(krbn::types::make_hid_usage(krbn::consumer_key_code::rewind))));
    keys.insert(static_cast<uint8_t>(*(krbn::types::make_hid_usage(krbn::consumer_key_code::eject))));
    keys.insert(static_cast<uint8_t>(*(krbn::types::make_hid_usage(krbn::consumer_key_code::mute))));

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("rewind");
    expected.push_back("eject");
    expected.push_back("mute");

    REQUIRE(krbn::virtual_hid_device_utility::to_json(keys, krbn::hid_usage_page::consumer).dump() == expected.dump());
  }
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keys keys;
    keys.insert(static_cast<uint8_t>(krbn::hid_usage::av_top_case_keyboard_fn));
    keys.insert(static_cast<uint8_t>(krbn::hid_usage::av_top_case_brightness_up));
    keys.insert(static_cast<uint8_t>(krbn::hid_usage::av_top_case_illumination_up));

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("fn");
    expected.push_back("apple_top_case_display_brightness_increment");
    expected.push_back("illumination_increment");

    REQUIRE(krbn::virtual_hid_device_utility::to_json(keys, krbn::hid_usage_page::apple_vendor_top_case).dump() == expected.dump());
  }
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keys keys;
    keys.insert(static_cast<uint8_t>(krbn::hid_usage::apple_vendor_keyboard_expose_all));
    keys.insert(static_cast<uint8_t>(krbn::hid_usage::apple_vendor_keyboard_launchpad));

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("mission_control");
    expected.push_back("launchpad");

    REQUIRE(krbn::virtual_hid_device_utility::to_json(keys, krbn::hid_usage_page::apple_vendor_keyboard).dump() == expected.dump());
  }
}

TEST_CASE("karabiner_virtual_hid_device::hid_report::buttons") {
  {
    pqrs::karabiner_virtual_hid_device::hid_report::buttons buttons;
    buttons.insert(1);
    buttons.insert(32);

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("button1");
    expected.push_back("button32");

    REQUIRE(nlohmann::json(buttons).dump() == expected.dump());
  }
}
