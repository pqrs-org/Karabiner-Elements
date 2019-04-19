#include <catch2/catch.hpp>

#include "../../share/json_helper.hpp"
#include "manipulator/manipulator_factory.hpp"

namespace {
void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "device") {
    krbn::manipulator::conditions::device(json.at("input"));
  } else if (c == "frontmost_application") {
    krbn::manipulator::conditions::frontmost_application(json.at("input"));
  } else if (c == "input_source") {
    krbn::manipulator::conditions::input_source(json.at("input"));
  } else if (c == "keyboard_type") {
    krbn::manipulator::conditions::keyboard_type(json.at("input"));
  } else if (c == "variable") {
    krbn::manipulator::conditions::variable(json.at("input"));
  } else {
    REQUIRE(false);
  }
}
} // namespace

TEST_CASE("errors") {
  namespace basic = krbn::manipulator::manipulators::basic;

  auto json = krbn::unit_testing::json_helper::load_jsonc("json/errors.jsonc");
  for (const auto& j : json) {
    auto error_json = krbn::unit_testing::json_helper::load_jsonc("json/" + j.get<std::string>());
    for (const auto& e : error_json) {
      REQUIRE_THROWS_AS(
          handle_json(e),
          pqrs::json::unmarshal_error);
      REQUIRE_THROWS_WITH(
          handle_json(e),
          e.at("error").get<std::string>());
    }
  }
}
