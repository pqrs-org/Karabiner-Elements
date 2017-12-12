#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "json_utility.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("find_optional") {
  using namespace std::string_literals;

  nlohmann::json json;
  json["number"] = 123;
  json["string"] = "abc";
  json["array"] = nlohmann::json::array();
  json["array"].push_back(1);
  json["array"].push_back(2);
  json["array"].push_back(3);
  json["object"] = nlohmann::json::object();
  json["object"]["a"] = 1;
  json["object"]["b"] = 2;
  json["object"]["c"] = 3;

  REQUIRE(krbn::json_utility::find_optional<int>(json, "number") == 123);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "dummy") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "string") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "array") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "object") == boost::none);

  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "number") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "dummy") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "string") == "abc"s);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "array") == boost::none);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "object") == boost::none);

  REQUIRE(krbn::json_utility::find_array(json, "number") == nullptr);
  REQUIRE(krbn::json_utility::find_array(json, "dummy") == nullptr);
  REQUIRE(krbn::json_utility::find_array(json, "string") == nullptr);
  REQUIRE(krbn::json_utility::find_array(json, "array"));
  REQUIRE(krbn::json_utility::find_array(json, "array")->dump() == json["array"].dump());
  REQUIRE(krbn::json_utility::find_array(json, "object") == nullptr);

  REQUIRE(krbn::json_utility::find_object(json, "number") == nullptr);
  REQUIRE(krbn::json_utility::find_object(json, "dummy") == nullptr);
  REQUIRE(krbn::json_utility::find_object(json, "string") == nullptr);
  REQUIRE(krbn::json_utility::find_object(json, "array") == nullptr);
  REQUIRE(krbn::json_utility::find_object(json, "object"));
  REQUIRE(krbn::json_utility::find_object(json, "object")->dump() == json["object"].dump());

  REQUIRE(krbn::json_utility::find_copy(json, "number", nlohmann::json("fallback_value")) == nlohmann::json(123));
  REQUIRE(krbn::json_utility::find_copy(json, "string", nlohmann::json("fallback_value")) == nlohmann::json("abc"));
  REQUIRE(krbn::json_utility::find_copy(json, "array", nlohmann::json("fallback_value")) == json["array"]);
  REQUIRE(krbn::json_utility::find_copy(json, "object", nlohmann::json("fallback_value")) == json["object"]);
  REQUIRE(krbn::json_utility::find_copy(json, "unknown", nlohmann::json("fallback_value")) == nlohmann::json("fallback_value"));
}
