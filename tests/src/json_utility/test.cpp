#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "json_utility.hpp"

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
  REQUIRE(krbn::json_utility::find_optional<int>(json, "dummy") == std::nullopt);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "string") == std::nullopt);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "array") == std::nullopt);
  REQUIRE(krbn::json_utility::find_optional<int>(json, "object") == std::nullopt);
  REQUIRE(krbn::json_utility::find_optional<int>(nlohmann::json(), "key") == std::nullopt);

  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "number") == std::nullopt);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "dummy") == std::nullopt);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "string") == "abc"s);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "array") == std::nullopt);
  REQUIRE(krbn::json_utility::find_optional<std::string>(json, "object") == std::nullopt);

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
