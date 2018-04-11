#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "device_detail.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("to_json") {
  using namespace std::string_literals;

  {
    krbn::device_detail device_detail(boost::none,
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
    json["is_keyboard"] = true;
    json["is_pointing_device"] = false;
    REQUIRE(device_detail.to_json() == json);
  }
  {
    krbn::device_detail device_detail(krbn::vendor_id(123),
                                      krbn::product_id(234),
                                      krbn::location_id(345),
                                      "m"s,
                                      "p"s,
                                      "s"s,
                                      "t"s,
                                      krbn::registry_entry_id(98765),
                                      false,
                                      true);
    nlohmann::json json;
    json["vendor_id"] = 123;
    json["product_id"] = 234;
    json["location_id"] = 345;
    json["manufacturer"] = "m";
    json["product"] = "p";
    json["serial_number"] = "s";
    json["transport"] = "t";
    json["registry_entry_id"] = 98765;
    json["is_keyboard"] = false;
    json["is_pointing_device"] = true;

    REQUIRE(device_detail.to_json() == json);
  }
}

TEST_CASE("from_json") {
  {
    nlohmann::json json;
    json["vendor_id"] = 123;
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
  {
    nlohmann::json json;
    json["product_id"] = 123;
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
  {
    nlohmann::json json;
    json["location_id"] = 123;
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
  {
    nlohmann::json json;
    json["manufacturer"] = "m";
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
  {
    nlohmann::json json;
    json["product"] = "p";
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
  {
    nlohmann::json json;
    json["serial_number"] = "s";
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
  {
    nlohmann::json json;
    json["transport"] = "t";
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
  {
    nlohmann::json json;
    json["registry_entry_id"] = 123;
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
  {
    nlohmann::json json;
    json["is_keyboard"] = true;
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
  {
    nlohmann::json json;
    json["is_pointing_device"] = true;
    REQUIRE(krbn::device_detail(json).to_json() == json);
  }
}

TEST_CASE("compare") {
  using namespace std::string_literals;

  krbn::device_detail device_detail0(boost::none,
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     boost::none,
                                     false,
                                     false);

  krbn::device_detail device_detail1(krbn::vendor_id(123),
                                     krbn::product_id(234),
                                     krbn::location_id(345),
                                     "m1"s,
                                     "p1"s,
                                     "s1"s,
                                     "t1"s,
                                     krbn::registry_entry_id(98765),
                                     true,
                                     true);

  krbn::device_detail device_detail2(krbn::vendor_id(123),
                                     krbn::product_id(234),
                                     krbn::location_id(345),
                                     "m2"s,
                                     "p1"s,
                                     "s1"s,
                                     "t1"s,
                                     krbn::registry_entry_id(98765),
                                     true,
                                     false);

  krbn::device_detail device_detail3(krbn::vendor_id(123),
                                     krbn::product_id(234),
                                     krbn::location_id(345),
                                     "m1"s,
                                     "p2"s,
                                     "s1"s,
                                     "t1"s,
                                     krbn::registry_entry_id(98765),
                                     false,
                                     true);

  krbn::device_detail device_detail4(krbn::vendor_id(123),
                                     krbn::product_id(234),
                                     krbn::location_id(345),
                                     "m2"s,
                                     "p2"s,
                                     "s1"s,
                                     "t1"s,
                                     krbn::registry_entry_id(98765),
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
