#include "device_properties.hpp"
#include <boost/ut.hpp>

void run_to_json_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "to_json"_test = [] {
    using namespace std::string_literals;

    {
      auto device_properties = krbn::device_properties()
                                   .set(krbn::device_id(0))
                                   .set_is_keyboard(true)
                                   .set_is_pointing_device(false);

      nlohmann::json json;
      json["device_id"] = 0;
      json["is_keyboard"] = true;
      json["is_pointing_device"] = false;
      expect(device_properties.to_json() == json);
    }
    {
      auto device_properties = krbn::device_properties()
                                   .set(krbn::device_id(98765))
                                   .set(pqrs::hid::vendor_id::value_t(123))
                                   .set(pqrs::hid::product_id::value_t(234))
                                   .set(krbn::location_id(345))
                                   .set_manufacturer(pqrs::hid::manufacturer_string::value_t("m"))
                                   .set_product(pqrs::hid::product_string::value_t("p"))
                                   .set_serial_number("s"s)
                                   .set_transport("t"s)
                                   .set_is_keyboard(false)
                                   .set_is_pointing_device(true);

      nlohmann::json json;
      json["device_id"] = 98765;
      json["vendor_id"] = 123;
      json["product_id"] = 234;
      json["location_id"] = 345;
      json["manufacturer"] = "m";
      json["product"] = "p";
      json["serial_number"] = "s";
      json["transport"] = "t";
      json["is_keyboard"] = false;
      json["is_pointing_device"] = true;

      expect(device_properties.to_json() == json);
    }
  };
}
