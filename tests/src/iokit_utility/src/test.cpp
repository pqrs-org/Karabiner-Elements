#include "iokit_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "iokit_utility::is_karabiner_virtual_hid_device"_test = [] {
    {
      auto actual = krbn::iokit_utility::is_karabiner_virtual_hid_device(
          pqrs::hid::manufacturer_string::value_t("pqrs.org"),
          pqrs::hid::product_string::value_t("Karabiner DriverKit VirtualHIDKeyboard 1.7.0"));
      expect(true == actual);
    }

    {
      auto actual = krbn::iokit_utility::is_karabiner_virtual_hid_device(
          pqrs::hid::manufacturer_string::value_t("Apple Inc."),
          pqrs::hid::product_string::value_t("Magic Trackpad"));
      expect(false == actual);
    }
  };

  return 0;
}
