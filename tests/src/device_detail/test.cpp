#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "boost_defs.hpp"

#include "device_detail.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("to_json") {
  using namespace std::string_literals;

  {
    krbn::device_detail device_detail(pqrs::osx::iokit_registry_entry_id(0),
                                      boost::none,
                                      boost::none,
                                      boost::none,
                                      boost::none,
                                      boost::none,
                                      boost::none,
                                      boost::none,
                                      true,
                                      false);
    nlohmann::json json;
    json["registry_entry_id"] = 0;
    json["is_keyboard"] = true;
    json["is_pointing_device"] = false;
    REQUIRE(device_detail.to_json() == json);
  }
  {
    krbn::device_detail device_detail(pqrs::osx::iokit_registry_entry_id(98765),
                                      krbn::vendor_id(123),
                                      krbn::product_id(234),
                                      krbn::location_id(345),
                                      "m"s,
                                      "p"s,
                                      "s"s,
                                      "t"s,
                                      false,
                                      true);
    nlohmann::json json;
    json["registry_entry_id"] = 98765;
    json["vendor_id"] = 123;
    json["product_id"] = 234;
    json["location_id"] = 345;
    json["manufacturer"] = "m";
    json["product"] = "p";
    json["serial_number"] = "s";
    json["transport"] = "t";
    json["is_keyboard"] = false;
    json["is_pointing_device"] = true;

    REQUIRE(device_detail.to_json() == json);
  }
}

TEST_CASE("compare") {
  using namespace std::string_literals;

  krbn::device_detail device_detail0(pqrs::osx::iokit_registry_entry_id(0),
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     false,
                                     false);

  krbn::device_detail device_detail1(pqrs::osx::iokit_registry_entry_id(98765),
                                     krbn::vendor_id(123),
                                     krbn::product_id(234),
                                     krbn::location_id(345),
                                     "m1"s,
                                     "p1"s,
                                     "s1"s,
                                     "t1"s,
                                     true,
                                     true);

  krbn::device_detail device_detail2(pqrs::osx::iokit_registry_entry_id(98765),
                                     krbn::vendor_id(123),
                                     krbn::product_id(234),
                                     krbn::location_id(345),
                                     "m2"s,
                                     "p1"s,
                                     "s1"s,
                                     "t1"s,
                                     true,
                                     false);

  krbn::device_detail device_detail3(pqrs::osx::iokit_registry_entry_id(98765),
                                     krbn::vendor_id(123),
                                     krbn::product_id(234),
                                     krbn::location_id(345),
                                     "m1"s,
                                     "p2"s,
                                     "s1"s,
                                     "t1"s,
                                     false,
                                     true);

  krbn::device_detail device_detail4(pqrs::osx::iokit_registry_entry_id(98765),
                                     krbn::vendor_id(123),
                                     krbn::product_id(234),
                                     krbn::location_id(345),
                                     "m2"s,
                                     "p2"s,
                                     "s1"s,
                                     "t1"s,
                                     false,
                                     false);

  REQUIRE(device_detail0.compare(device_detail0) == false);
  REQUIRE(device_detail1.compare(device_detail1) == false);
  // 0,1 (["",""], [p1,m1])
  REQUIRE(device_detail1.compare(device_detail0) == false);
  REQUIRE(device_detail0.compare(device_detail1) == true);
  // 1,2 ([p1,m1], [p1,m2])
  REQUIRE(device_detail1.compare(device_detail2) == true);
  REQUIRE(device_detail2.compare(device_detail1) == false);
  // 1,3 ([p1,m1], [p2,m1])
  REQUIRE(device_detail1.compare(device_detail3) == true);
  REQUIRE(device_detail3.compare(device_detail1) == false);
  // 3,4 ([p2,m1], [p2,m2])
  REQUIRE(device_detail3.compare(device_detail4) == true);
  REQUIRE(device_detail4.compare(device_detail3) == false);
}
