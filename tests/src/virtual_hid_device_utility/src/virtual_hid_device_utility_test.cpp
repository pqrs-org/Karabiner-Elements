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
    keys.insert(type_safe::get(*(krbn::make_hid_usage(krbn::key_code::a))));
    keys.insert(type_safe::get(*(krbn::make_hid_usage(krbn::key_code::b))));
    keys.insert(type_safe::get(*(krbn::make_hid_usage(krbn::key_code::c))));
    keys.erase(type_safe::get(*(krbn::make_hid_usage(krbn::key_code::a))));

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("b");
    expected.push_back("c");

    REQUIRE(krbn::virtual_hid_device_utility::to_json(keys, pqrs::hid::usage_page::keyboard_or_keypad).dump() == expected.dump());
  }
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keys keys;
    keys.insert(type_safe::get(*(krbn::make_hid_usage(krbn::consumer_key_code::rewind))));
    keys.insert(type_safe::get(*(krbn::make_hid_usage(krbn::consumer_key_code::eject))));
    keys.insert(type_safe::get(*(krbn::make_hid_usage(krbn::consumer_key_code::mute))));

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("rewind");
    expected.push_back("eject");
    expected.push_back("mute");

    REQUIRE(krbn::virtual_hid_device_utility::to_json(keys, pqrs::hid::usage_page::consumer).dump() == expected.dump());
  }
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keys keys;
    keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::keyboard_fn));
    keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::brightness_up));
    keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::illumination_up));

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("fn");
    expected.push_back("apple_top_case_display_brightness_increment");
    expected.push_back("illumination_increment");

    REQUIRE(krbn::virtual_hid_device_utility::to_json(keys, pqrs::hid::usage_page::apple_vendor_top_case).dump() == expected.dump());
  }
  {
    pqrs::karabiner_virtual_hid_device::hid_report::keys keys;
    keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::expose_all));
    keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::launchpad));

    nlohmann::json expected = nlohmann::json::array();
    expected.push_back("mission_control");
    expected.push_back("launchpad");

    REQUIRE(krbn::virtual_hid_device_utility::to_json(keys, pqrs::hid::usage_page::apple_vendor_keyboard).dump() == expected.dump());
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
