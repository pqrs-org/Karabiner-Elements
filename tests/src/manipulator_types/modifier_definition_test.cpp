#include <catch2/catch.hpp>

#include "manipulator/types.hpp"

TEST_CASE("modifier_definition.modifier json") {
  using krbn::manipulator::modifier_definition::modifier;

  std::vector<std::pair<modifier, std::string>> pairs{
      std::make_pair(modifier::any, "any"),
      std::make_pair(modifier::caps_lock, "caps_lock"),
      std::make_pair(modifier::command, "command"),
      std::make_pair(modifier::control, "control"),
      std::make_pair(modifier::fn, "fn"),
      std::make_pair(modifier::left_command, "left_command"),
      std::make_pair(modifier::left_control, "left_control"),
      std::make_pair(modifier::left_option, "left_option"),
      std::make_pair(modifier::left_shift, "left_shift"),
      std::make_pair(modifier::option, "option"),
      std::make_pair(modifier::right_command, "right_command"),
      std::make_pair(modifier::right_control, "right_control"),
      std::make_pair(modifier::right_option, "right_option"),
      std::make_pair(modifier::right_shift, "right_shift"),
      std::make_pair(modifier::shift, "shift"),
      std::make_pair(modifier::end_, "end_"),
  };
  for (const auto& [m, name] : pairs) {
    // from_json
    {
      nlohmann::json json = name;
      REQUIRE(json.get<modifier>() == m);
    }

    // to_json
    {
      nlohmann::json json = m;
      REQUIRE(json.get<modifier>() == m);
    }
  }

  // json is not string.

  {
    auto json = nlohmann::json::array();
    json.push_back("hello");
    REQUIRE_THROWS_AS(
        json.get<modifier>(),
        krbn::json_unmarshal_error);
    REQUIRE_THROWS_WITH(
        json.get<modifier>(),
        Catch::Equals("modifier must be string, but is `[\"hello\"]`"));
  }

  // json is invalid string.

  {
    nlohmann::json json = "hello";
    REQUIRE_THROWS_AS(
        json.get<modifier>(),
        krbn::json_unmarshal_error);
    REQUIRE_THROWS_WITH(
        json.get<modifier>(),
        Catch::Equals("unknown modifier: `hello`"));
  }
}

TEST_CASE("modifier_definition.make_modifiers") {
  namespace modifier_definition = krbn::manipulator::modifier_definition;
  using krbn::manipulator::event_definition;

  // array

  {
    nlohmann::json json({"left_command", "left_shift", "fn", "any"});
    auto actual = modifier_definition::make_modifiers(json, "modifiers");
    auto expected = std::unordered_set<modifier_definition::modifier>({
        modifier_definition::modifier::left_command,
        modifier_definition::modifier::left_shift,
        modifier_definition::modifier::fn,
        modifier_definition::modifier::any,
    });
    REQUIRE(actual == expected);
  }

  // string

  {
    nlohmann::json json("left_command");
    auto actual = modifier_definition::make_modifiers(json, "modifiers");
    auto expected = std::unordered_set<modifier_definition::modifier>({
        modifier_definition::modifier::left_command,
    });
    REQUIRE(actual == expected);
  }

  // null

  {
    nlohmann::json json(nullptr);
    auto actual = modifier_definition::make_modifiers(json, "modifiers");
    auto expected = std::unordered_set<modifier_definition::modifier>({});
    REQUIRE(actual == expected);
  }

  // type error

  {
    auto json = nlohmann::json::object();
    REQUIRE_THROWS_AS(
        modifier_definition::make_modifiers(json, "modifiers"),
        krbn::json_unmarshal_error);
    REQUIRE_THROWS_WITH(
        modifier_definition::make_modifiers(json, "modifiers"),
        "`modifiers` must be array or string, but is `{}`");
  }

  // unknown modifier

  {
    nlohmann::json json("unknown");
    REQUIRE_THROWS_AS(
        modifier_definition::make_modifiers(json, "modifiers"),
        krbn::json_unmarshal_error);
    REQUIRE_THROWS_WITH(
        modifier_definition::make_modifiers(json, "modifiers"),
        "unknown modifier: `unknown`");
  }
}

TEST_CASE("modifier_definition.get_modifier") {
  namespace modifier_definition = krbn::manipulator::modifier_definition;

  REQUIRE(modifier_definition::get_modifier(krbn::modifier_flag::zero) == modifier_definition::modifier::end_);
  REQUIRE(modifier_definition::get_modifier(krbn::modifier_flag::left_shift) == modifier_definition::modifier::left_shift);
}
