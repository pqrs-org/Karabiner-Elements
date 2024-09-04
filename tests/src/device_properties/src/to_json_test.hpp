#include "../../share/ut_helper.hpp"
#include "device_properties.hpp"
#include <boost/ut.hpp>

void run_to_json_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "to_json"_test = [] {
    using namespace std::string_literals;

    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .device_address = "ec-ba-73-21-e6-f4",
          .is_keyboard = true});

      auto json = R"(

{
  "device_id": 0,
  "device_identifiers": {
    "device_address": "ec-ba-73-21-e6-f4",
    "is_keyboard": true
  }
}

)"_json;

      expect(device_properties.to_json() == json) << UT_SHOW_LINE;
    }
    {
      auto device_properties = krbn::device_properties(krbn::device_properties::initialization_parameters{
          .device_id = krbn::device_id(98765),
          .vendor_id = pqrs::hid::vendor_id::value_t(123),
          .product_id = pqrs::hid::product_id::value_t(234),
          .location_id = krbn::location_id(345),
          .manufacturer = pqrs::hid::manufacturer_string::value_t("m"),
          .product = pqrs::hid::product_string::value_t("p"),
          .serial_number = "s"s,
          .transport = "t"s,
          .is_pointing_device = true,
      });

      auto json = R"(

{
  "device_id": 98765,
  "device_identifiers": {
    "vendor_id": 123,
    "product_id": 234,
    "is_pointing_device": true
  },
  "location_id": 345,
  "manufacturer": "m",
  "product": "p",
  "serial_number": "s",
  "transport": "t"
}

)"_json;

      expect(device_properties.to_json() == json) << UT_SHOW_LINE;
    }
  };
}
