#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "device_detail_json.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("constructor") {
  using namespace std::string_literals;

  {
    krbn::device_detail_json device_detail_json(boost::none,
                                                boost::none,
                                                boost::none,
                                                boost::none,
                                                boost::none,
                                                boost::none,
                                                boost::none,
                                                boost::none);
    nlohmann::json json;
    REQUIRE(device_detail_json.get_json() == json);
  }
  {
    krbn::device_detail_json device_detail_json(krbn::vendor_id(123),
                                                krbn::product_id(234),
                                                krbn::location_id(345),
                                                "m"s,
                                                "p"s,
                                                "s"s,
                                                "t"s,
                                                98765);
    nlohmann::json json;
    json["vendor_id"] = 123;
    json["product_id"] = 234;
    json["location_id"] = 345;
    json["manufacturer"] = "m";
    json["product"] = "p";
    json["serial_number"] = "s";
    json["transport"] = "t";
    json["registry_entry_id"] = 98765;

    REQUIRE(device_detail_json.get_json() == json);
  }
}
