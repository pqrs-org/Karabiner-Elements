#include "../../share/ut_helper.hpp"
#include "device_properties.hpp"
#include <boost/ut.hpp>

void run_json_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "from_json"_test = [] {
    {
      auto json = R"(

{
  "device_id": 1,
  "vendor_id": 2,
  "product_id": 3,
  "location_id": 4,
  "manufacturer": "m",
  "product": "p",
  "serial_number": "s",
  "transport": "t",
  "device_address": "d",
  "is_keyboard": true,
  "is_pointing_device": true,
  "is_game_pad": true
}

)"_json;

      auto device_properties = krbn::device_properties::make_device_properties(json);
      expect(krbn::device_id(1) == device_properties->get_device_id());
      expect(pqrs::hid::vendor_id::value_t(2) == device_properties->get_device_identifiers().get_vendor_id());
      expect(pqrs::hid::product_id::value_t(3) == device_properties->get_device_identifiers().get_product_id());
      expect(krbn::location_id(4) == device_properties->get_location_id());
      expect(pqrs::hid::manufacturer_string::value_t("m") == device_properties->get_manufacturer());
      expect(pqrs::hid::product_string::value_t("p") == device_properties->get_product());
      expect("s" == device_properties->get_serial_number());
      expect("t" == device_properties->get_transport());
      expect("" == device_properties->get_device_identifiers().get_device_address()); // device_address is ignored
      expect(true == device_properties->get_device_identifiers().get_is_keyboard());
      expect(true == device_properties->get_device_identifiers().get_is_pointing_device());
      expect(true == device_properties->get_device_identifiers().get_is_game_pad());
    }

    {
      auto json = R"(

{
  "device_address": "d"
}

)"_json;

      auto device_properties = krbn::device_properties::make_device_properties(json);
      expect("d" == device_properties->get_device_identifiers().get_device_address());
    }
  };

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
