#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "device_properties.hpp"

TEST_CASE("device_properties") {
  auto device_properties = krbn::device_properties(krbn::device_id(0),
                                                   nullptr);
  REQUIRE(device_properties.get_device_id());
  REQUIRE(*(device_properties.get_device_id()) == krbn::device_id(0));
}

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

TEST_CASE("compare") {
  using namespace std::string_literals;

  krbn::device_properties device_properties0;
  device_properties0
      .set(krbn::device_id(0))
      .set_is_keyboard(false)
      .set_is_pointing_device(false);

  auto device_properties1 = krbn::device_properties()
                                .set(krbn::device_id(98765))
                                .set(krbn::vendor_id(123))
                                .set(krbn::product_id(234))
                                .set(krbn::location_id(345))
                                .set_manufacturer("m1"s)
                                .set_product("p1"s)
                                .set_serial_number("s1"s)
                                .set_transport("t1"s)
                                .set_is_keyboard(true)
                                .set_is_pointing_device(true);

  auto device_properties2 = krbn::device_properties()
                                .set(krbn::device_id(98765))
                                .set(krbn::vendor_id(123))
                                .set(krbn::product_id(234))
                                .set(krbn::location_id(345))
                                .set_manufacturer("m2"s)
                                .set_product("p1"s)
                                .set_serial_number("s1"s)
                                .set_transport("t1"s)
                                .set_is_keyboard(true)
                                .set_is_pointing_device(false);

  auto device_properties3 = krbn::device_properties()
                                .set(krbn::device_id(98765))
                                .set(krbn::vendor_id(123))
                                .set(krbn::product_id(234))
                                .set(krbn::location_id(345))
                                .set_manufacturer("m1"s)
                                .set_product("p2"s)
                                .set_serial_number("s1"s)
                                .set_transport("t1"s)
                                .set_is_keyboard(false)
                                .set_is_pointing_device(true);

  auto device_properties4 = krbn::device_properties()
                                .set(krbn::device_id(98765))
                                .set(krbn::vendor_id(123))
                                .set(krbn::product_id(234))
                                .set(krbn::location_id(345))
                                .set_manufacturer("m2"s)
                                .set_product("p2"s)
                                .set_serial_number("s1"s)
                                .set_transport("t1"s)
                                .set_is_keyboard(false)
                                .set_is_pointing_device(false);

  REQUIRE(device_properties0.compare(device_properties0) == false);
  REQUIRE(device_properties1.compare(device_properties1) == false);
  // 0,1 (["",""], [p1,m1])
  REQUIRE(device_properties1.compare(device_properties0) == false);
  REQUIRE(device_properties0.compare(device_properties1) == true);
  // 1,2 ([p1,m1], [p1,m2])
  REQUIRE(device_properties1.compare(device_properties2) == true);
  REQUIRE(device_properties2.compare(device_properties1) == false);
  // 1,3 ([p1,m1], [p2,m1])
  REQUIRE(device_properties1.compare(device_properties3) == true);
  REQUIRE(device_properties3.compare(device_properties1) == false);
  // 3,4 ([p2,m1], [p2,m2])
  REQUIRE(device_properties3.compare(device_properties4) == true);
  REQUIRE(device_properties4.compare(device_properties3) == false);
}
