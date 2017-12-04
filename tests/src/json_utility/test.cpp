#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "json_utility.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("get_optional") {
  nlohmann::json json;
  json["number"] = 123;
  json["string"] = "abc";
  json["array"] = nlohmann::json::array();
  json["object"] = nlohmann::json::object();

  REQUIRE(krbn::json_utility::find_optional<int>(json, "number") == 123);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "dummy") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "string") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "array") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "object") == boost::none);

  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "number") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "dummy") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "string") == std::string("abc"));
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "array") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "object") == boost::none);
}
