#include <catch2/catch.hpp>

#include "manipulator/types.hpp"

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
