#include <catch2/catch.hpp>

#include "device_properties.hpp"

TEST_CASE("to_json") {
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
    REQUIRE(device_properties.to_json() == json);
  }
  {
    auto device_properties = krbn::device_properties()
                                 .set(krbn::device_id(98765))
                                 .set(krbn::vendor_id(123))
                                 .set(krbn::product_id(234))
                                 .set(krbn::location_id(345))
                                 .set_manufacturer("m"s)
                                 .set_product("p"s)
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

    REQUIRE(device_properties.to_json() == json);
  }
}
