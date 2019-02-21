#include <catch2/catch.hpp>

#include "manipulator/types.hpp"

namespace {
void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "to_event_definition") {
    krbn::manipulator::to_event_definition(json.at("input"));
  } else {
    REQUIRE(false);
  }
}
} // namespace

TEST_CASE("errors") {
  std::ifstream json_file("json/errors.json");
  auto json = nlohmann::json::parse(json_file);
  for (const auto& j : json) {
    std::ifstream error_json_input("json/" + j.get<std::string>());
    auto error_json = nlohmann::json::parse(error_json_input);
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
