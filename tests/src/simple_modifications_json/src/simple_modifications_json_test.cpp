#include <catch2/catch.hpp>

#include "../../share/json_helper.hpp"
#include "manipulator/types.hpp"
#include <pqrs/json.hpp>

TEST_CASE("simple_modifications.json") {
  auto json = krbn::unit_testing::json_helper::load_jsonc("../../../src/apps/PreferencesWindow/Resources/simple_modifications.json");
  for (const auto& entry : json) {
    if (auto data = pqrs::json::find_object(entry, "data")) {
      krbn::manipulator::to_event_definition e(data->value());
    }
  }
}
