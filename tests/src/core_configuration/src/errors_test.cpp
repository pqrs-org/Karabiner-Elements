#include <catch2/catch.hpp>

#include "../../share/json_helper.hpp"
#include "manipulator/manipulator_factory.hpp"

namespace {
void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "complex_modifications") {
    krbn::core_configuration::details::complex_modifications(json.at("input"));
  } else if (c == "devices") {
    krbn::core_configuration::details::device(json.at("input"));
  } else if (c == "profile") {
    krbn::core_configuration::details::profile(json.at("input"));
  } else if (c == "simple_modifications") {
    krbn::core_configuration::details::simple_modifications m;
    m.update(json.at("input"));
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
      if (!e.at("error").is_null()) {
        REQUIRE_THROWS_AS(
            handle_json(e),
            pqrs::json::unmarshal_error);
        REQUIRE_THROWS_WITH(
            handle_json(e),
            e.at("error").get<std::string>());
      }
    }
  }
}
