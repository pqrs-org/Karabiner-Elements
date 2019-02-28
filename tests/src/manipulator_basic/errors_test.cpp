#include <catch2/catch.hpp>

#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"

namespace {
void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "from_event_definition") {
    json.at("input").get<krbn::manipulator::manipulators::basic::from_event_definition>();
  } else if (c == "simultaneous_options::key_order") {
    json.at("input").get<krbn::manipulator::manipulators::basic::simultaneous_options::key_order>();
  } else if (c == "simultaneous_options::key_up_when") {
    json.at("input").get<krbn::manipulator::manipulators::basic::simultaneous_options::key_up_when>();
  } else if (c == "simultaneous_options") {
    json.at("input").get<krbn::manipulator::manipulators::basic::simultaneous_options>();
  } else {
    REQUIRE(false);
  }
}
} // namespace

TEST_CASE("errors") {
  namespace basic = krbn::manipulator::manipulators::basic;

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
