#include <catch2/catch.hpp>

#include "manipulator/types.hpp"

TEST_CASE("modifier_definition.modifier json") {
  using krbn::manipulator::modifier_definition;

  std::vector<std::pair<modifier_definition::modifier, std::string>> pairs{
      std::make_pair(modifier_definition::modifier::any, "any"),
      std::make_pair(modifier_definition::modifier::caps_lock, "caps_lock"),
      std::make_pair(modifier_definition::modifier::command, "command"),
      std::make_pair(modifier_definition::modifier::control, "control"),
      std::make_pair(modifier_definition::modifier::fn, "fn"),
      std::make_pair(modifier_definition::modifier::left_command, "left_command"),
      std::make_pair(modifier_definition::modifier::left_control, "left_control"),
      std::make_pair(modifier_definition::modifier::left_option, "left_option"),
      std::make_pair(modifier_definition::modifier::left_shift, "left_shift"),
      std::make_pair(modifier_definition::modifier::option, "option"),
      std::make_pair(modifier_definition::modifier::right_command, "right_command"),
      std::make_pair(modifier_definition::modifier::right_control, "right_control"),
      std::make_pair(modifier_definition::modifier::right_option, "right_option"),
      std::make_pair(modifier_definition::modifier::right_shift, "right_shift"),
      std::make_pair(modifier_definition::modifier::shift, "shift"),
      std::make_pair(modifier_definition::modifier::end_, "end_"),
  };
  for (const auto& [m, name] : pairs) {
    // from_json
    {
      nlohmann::json json = name;
      REQUIRE(json.get<modifier_definition::modifier>() == m);
    }

    // to_json
    {
      nlohmann::json json = m;
      REQUIRE(json.get<modifier_definition::modifier>() == m);
    }
  }

  // json is not string.

  {
    auto json = nlohmann::json::array();
    json.push_back("hello");
    REQUIRE_THROWS_AS(
        json.get<modifier_definition::modifier>(),
        krbn::json_unmarshal_error);
    REQUIRE_THROWS_WITH(
        json.get<modifier_definition::modifier>(),
        Catch::Equals("complex_modifications json error: modifier should be string form: [\"hello\"]"));
  }

  // json is invalid string.

  {
    nlohmann::json json = "hello";
    REQUIRE_THROWS_AS(
        json.get<modifier_definition::modifier>(),
        krbn::json_unmarshal_error);
    REQUIRE_THROWS_WITH(
        json.get<modifier_definition::modifier>(),
        Catch::Equals("complex_modifications json error: unknown modifier: hello"));
  }
}

TEST_CASE("modifier_definition.make_modifiers") {
  using krbn::manipulator::event_definition;
  using krbn::manipulator::modifier_definition;

  {
    nlohmann::json json({"left_command", "left_shift", "fn", "any"});
    auto actual = modifier_definition::make_modifiers(json);
    auto expected = std::unordered_set<modifier_definition::modifier>({
        modifier_definition::modifier::left_command,
        modifier_definition::modifier::left_shift,
        modifier_definition::modifier::fn,
        modifier_definition::modifier::any,
    });
    REQUIRE(actual == expected);
  }

  {
    nlohmann::json json("left_command");
    auto actual = modifier_definition::make_modifiers(json);
    auto expected = std::unordered_set<modifier_definition::modifier>({
        modifier_definition::modifier::left_command,
    });
    REQUIRE(actual == expected);
  }

  {
    nlohmann::json json("unknown");
    auto actual = modifier_definition::make_modifiers(json);
    auto expected = std::unordered_set<modifier_definition::modifier>({});
    REQUIRE(actual == expected);
  }

  {
    nlohmann::json json(nullptr);
    auto actual = modifier_definition::make_modifiers(json);
    auto expected = std::unordered_set<modifier_definition::modifier>({});
    REQUIRE(actual == expected);
  }
}

TEST_CASE("modifier_definition.get_modifier") {
  using krbn::manipulator::modifier_definition;

  REQUIRE(modifier_definition::get_modifier(krbn::modifier_flag::zero) == modifier_definition::modifier::end_);
  REQUIRE(modifier_definition::get_modifier(krbn::modifier_flag::left_shift) == modifier_definition::modifier::left_shift);
}
