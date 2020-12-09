#include <catch2/catch.hpp>

#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"

TEST_CASE("to_after_key_up") {
  namespace basic = krbn::manipulator::manipulators::basic;
  using krbn::manipulator::event_definition;

  // object

  {
    auto json = nlohmann::json::object({
        {"to_after_key_up", nlohmann::json::object({
                                {"key_code", "tab"},
                            })},
    });

    basic::basic b(json,
                   krbn::core_configuration::details::complex_modifications_parameters());
    REQUIRE(b.get_to_after_key_up().size() == 1);
    {
      auto& d = b.get_to_after_key_up()[0].get_event_definition();
      REQUIRE(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
              pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
    }
  }

  // array

  {
    auto json = nlohmann::json::object({
        {"to_after_key_up", nlohmann::json::array({
                                nlohmann::json::object({{"key_code", "tab"}}),
                                nlohmann::json::object({{"key_code", "spacebar"}}),
                            })},
    });

    basic::basic b(json,
                   krbn::core_configuration::details::complex_modifications_parameters());
    REQUIRE(b.get_to_after_key_up().size() == 2);
    {
      auto& d = b.get_to_after_key_up()[0].get_event_definition();
      REQUIRE(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
              pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
    }
    {
      auto& d = b.get_to_after_key_up()[1].get_event_definition();
      REQUIRE(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
              pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
    }
  }
}
