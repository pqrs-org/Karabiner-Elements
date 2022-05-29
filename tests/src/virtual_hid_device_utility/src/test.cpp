#include "virtual_hid_device_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::modifiers"_test = [] {
    {
      namespace hid_report = pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report;

      hid_report::modifiers modifiers;
      modifiers.insert(hid_report::modifier::left_shift);
      modifiers.insert(hid_report::modifier::right_command);

      nlohmann::json expected = nlohmann::json::array();
      expected.push_back("left_shift");
      expected.push_back("right_command");

      expect(nlohmann::json(modifiers).dump() == expected.dump());
    }
  };

  "karabiner_virtual_hid_device::hid_report::keys"_test = [] {
    namespace hid_report = pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report;

    // keyboard_or_keypad
    {
      hid_report::keys keys;
      keys.insert(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      keys.insert(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_b));
      keys.insert(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_c));
      keys.erase(type_safe::get(pqrs::hid::usage::keyboard_or_keypad::keyboard_a));

      nlohmann::json expected = nlohmann::json::array();
      expected.push_back(nlohmann::json({{"key_code", "b"}}));
      expected.push_back(nlohmann::json({{"key_code", "c"}}));

      expect(krbn::virtual_hid_device_utility::to_json(keys, pqrs::hid::usage_page::keyboard_or_keypad).dump() == expected.dump());
    }
    // consumer
    {
      hid_report::keys keys;
      keys.insert(type_safe::get(pqrs::hid::usage::consumer::rewind));
      keys.insert(type_safe::get(pqrs::hid::usage::consumer::eject));
      keys.insert(type_safe::get(pqrs::hid::usage::consumer::mute));

      nlohmann::json expected = nlohmann::json::array();
      expected.push_back(nlohmann::json::object({{"consumer_key_code", "rewind"}}));
      expected.push_back(nlohmann::json::object({{"consumer_key_code", "eject"}}));
      expected.push_back(nlohmann::json::object({{"consumer_key_code", "mute"}}));

      expect(krbn::virtual_hid_device_utility::to_json(keys, pqrs::hid::usage_page::consumer).dump() == expected.dump());
    }
    // apple_vendor_top_case
    {
      hid_report::keys keys;
      keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::keyboard_fn));
      keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::brightness_up));
      keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_top_case::illumination_up));

      nlohmann::json expected = nlohmann::json::array();
      expected.push_back(nlohmann::json::object({{"apple_vendor_top_case_key_code", "keyboard_fn"}}));
      expected.push_back(nlohmann::json::object({{"apple_vendor_top_case_key_code", "brightness_up"}}));
      expected.push_back(nlohmann::json::object({{"apple_vendor_top_case_key_code", "illumination_up"}}));

      expect(krbn::virtual_hid_device_utility::to_json(keys, pqrs::hid::usage_page::apple_vendor_top_case).dump() == expected.dump());
    }
    // apple_vendor_keyboard
    {
      hid_report::keys keys;
      keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::expose_all));
      keys.insert(type_safe::get(pqrs::hid::usage::apple_vendor_keyboard::launchpad));

      nlohmann::json expected = nlohmann::json::array();
      expected.push_back(nlohmann::json::object({{"apple_vendor_keyboard_key_code", "mission_control"}}));
      expected.push_back(nlohmann::json::object({{"apple_vendor_keyboard_key_code", "launchpad"}}));

      expect(krbn::virtual_hid_device_utility::to_json(keys, pqrs::hid::usage_page::apple_vendor_keyboard).dump() == expected.dump());
    }
  };

  "karabiner_virtual_hid_device::hid_report::buttons"_test = [] {
    {
      pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report::buttons buttons;
      buttons.insert(1);
      buttons.insert(32);

      nlohmann::json expected = nlohmann::json::array();
      expected.push_back("button1");
      expected.push_back("button32");

      expect(nlohmann::json(buttons).dump() == expected.dump());
    }
  };

  return 0;
}
