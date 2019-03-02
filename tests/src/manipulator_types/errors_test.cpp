#include <catch2/catch.hpp>

#include "../share/json_helper.hpp"
#include "manipulator/types.hpp"

namespace {
void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "event_definition") {
    krbn::manipulator::event_definition event_definition;
    for (const auto& [key, value] : json.at("input").items()) {
      event_definition.handle_json(key, value, json.at("input"));
    }
  } else if (c == "modifier") {
    json.at("input").get<krbn::manipulator::modifier_definition::modifier>();
  } else if (c == "modifier_definition") {
    krbn::manipulator::modifier_definition::make_modifiers(json.at("input"));
  } else if (c == "to_event_definition") {
    json.at("input").get<krbn::manipulator::to_event_definition>();
  } else {
    REQUIRE(false);
  }
}
} // namespace

TEST_CASE("errors") {
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
