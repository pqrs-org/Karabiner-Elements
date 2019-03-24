#include <catch2/catch.hpp>

#include "manipulator/types.hpp"
#include "modifier_flag_manager.hpp"

namespace {
krbn::modifier_flag_manager::active_modifier_flag left_command_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::left_command,
    krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_control_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::left_control,
    krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_option_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::left_option,
    krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_shift_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::left_shift,
    krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag right_shift_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::right_shift,
    krbn::device_id(1));
} // namespace

TEST_CASE("from_modifiers_definition::test_modifier") {
  namespace modifier_definition = krbn::manipulator::modifier_definition;
  using krbn::manipulator::from_modifiers_definition;

  {
    krbn::modifier_flag_manager modifier_flag_manager;
    modifier_flag_manager.push_back_active_modifier_flag(left_command_1);

    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager,
                                                             modifier_definition::modifier::left_shift);
      auto expected = std::make_pair(false, krbn::modifier_flag::zero);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager,
                                                             modifier_definition::modifier::left_command);
      auto expected = std::make_pair(true, krbn::modifier_flag::left_command);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::right_command);
      auto expected = std::make_pair(false, krbn::modifier_flag::zero);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::command);
      auto expected = std::make_pair(true, krbn::modifier_flag::left_command);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::shift);
      auto expected = std::make_pair(false, krbn::modifier_flag::zero);
      REQUIRE(actual == expected);
    }
  }
  {
    krbn::modifier_flag_manager modifier_flag_manager;
    modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);

    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::left_shift);
      auto expected = std::make_pair(false, krbn::modifier_flag::zero);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::right_shift);
      auto expected = std::make_pair(true, krbn::modifier_flag::right_shift);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::shift);
      auto expected = std::make_pair(true, krbn::modifier_flag::right_shift);
      REQUIRE(actual == expected);
    }
  }
}
