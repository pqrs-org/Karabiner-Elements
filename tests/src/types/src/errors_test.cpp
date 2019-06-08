#include <catch2/catch.hpp>

#include "../../share/json_helper.hpp"
#include "types.hpp"

namespace {
void handle_json(const nlohmann::json& json) {
  auto c = json.at("class").get<std::string>();
  if (c == "device_identifiers") {
    json.at("input").get<krbn::device_identifiers>();
  } else if (c == "mouse_key") {
    json.at("input").get<krbn::mouse_key>();
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
