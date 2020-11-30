#include <catch2/catch.hpp>

#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"

TEST_CASE("to_delayed_action") {
  namespace basic = krbn::manipulator::manipulators::basic;
  using krbn::manipulator::event_definition;

  // object

  {
    auto json = nlohmann::json::object({
        {"to_delayed_action", nlohmann::json::object({
                                  {"to_if_invoked", nlohmann::json::object({
                                                        {"key_code", "tab"},
                                                    })},
                                  {"to_if_canceled", nlohmann::json::object({
                                                         {"key_code", "a"},
                                                     })},
                              })},
    });

    basic::basic b(json,
                   krbn::core_configuration::details::complex_modifications_parameters());
    REQUIRE(b.get_to_delayed_action());

    // to_if_invoked

    REQUIRE(b.get_to_delayed_action()->get_to_if_invoked().size() == 1);
    {
      auto& d = b.get_to_delayed_action()->get_to_if_invoked()[0].get_event_definition();
      REQUIRE(d.get_if<krbn::momentary_switch_event>()->make_usage_pair() ==
              pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
    }

    // to_if_canceled

    REQUIRE(b.get_to_delayed_action()->get_to_if_canceled().size() == 1);
    {
      auto& d = b.get_to_delayed_action()->get_to_if_canceled()[0].get_event_definition();
      REQUIRE(d.get_if<krbn::momentary_switch_event>()->make_usage_pair() ==
              pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
    }
  }

  // array

  {
    auto json = nlohmann::json::object({
        {"to_delayed_action", nlohmann::json::object({
                                  {"to_if_invoked", nlohmann::json::array({
                                                        nlohmann::json::object({{"key_code", "tab"}}),
                                                        nlohmann::json::object({{"key_code", "spacebar"}}),
                                                    })},
                                  {"to_if_canceled", nlohmann::json::array({
                                                         nlohmann::json::object({{"key_code", "a"}}),
                                                         nlohmann::json::object({{"key_code", "b"}}),
                                                     })},
                              })},
    });

    basic::basic b(json,
                   krbn::core_configuration::details::complex_modifications_parameters());
    REQUIRE(b.get_to_delayed_action());

    // to_if_invoked

    REQUIRE(b.get_to_delayed_action()->get_to_if_invoked().size() == 2);
    {
      auto& d = b.get_to_delayed_action()->get_to_if_invoked()[0].get_event_definition();
      REQUIRE(d.get_if<krbn::momentary_switch_event>()->make_usage_pair() ==
              pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
    }
    {
      auto& d = b.get_to_delayed_action()->get_to_if_invoked()[1].get_event_definition();
      REQUIRE(d.get_if<krbn::momentary_switch_event>()->make_usage_pair() ==
              pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
    }

    // to_if_canceled

    REQUIRE(b.get_to_delayed_action()->get_to_if_canceled().size() == 2);
    {
      auto& d = b.get_to_delayed_action()->get_to_if_canceled()[0].get_event_definition();
      REQUIRE(d.get_if<krbn::momentary_switch_event>()->make_usage_pair() ==
              pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
    }
    {
      auto& d = b.get_to_delayed_action()->get_to_if_canceled()[1].get_event_definition();
      REQUIRE(d.get_if<krbn::momentary_switch_event>()->make_usage_pair() ==
              pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                    pqrs::hid::usage::keyboard_or_keypad::keyboard_b));
    }
  }
}
