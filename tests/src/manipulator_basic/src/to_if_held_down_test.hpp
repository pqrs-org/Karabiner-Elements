#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>

void run_to_if_held_down_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "to_if_held_down"_test = [] {
    namespace basic = krbn::manipulator::manipulators::basic;
    using krbn::manipulator::event_definition;

    // object

    {
      auto json = nlohmann::json::object({
          {"to_if_held_down", nlohmann::json::object({
                                  {"key_code", "tab"},
                              })},
      });

      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      basic::basic b(json,
                     parameters);
      expect(b.get_to_if_held_down().get() != nullptr);
      expect(b.get_to_if_held_down()->get_to().size() == 1);
      {
        auto& d = b.get_to_if_held_down()->get_to()[0].get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
      }
    }

    // array

    {
      auto json = nlohmann::json::object({
          {"to_if_held_down", nlohmann::json::array({
                                  nlohmann::json::object({{"key_code", "tab"}}),
                                  nlohmann::json::object({{"key_code", "spacebar"}}),
                              })},
      });

      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      basic::basic b(json,
                     parameters);
      expect(b.get_to_if_held_down().get() != nullptr);
      expect(b.get_to_if_held_down()->get_to().size() == 2);
      {
        auto& d = b.get_to_if_held_down()->get_to()[0].get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
      }
      {
        auto& d = b.get_to_if_held_down()->get_to()[1].get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
      }
    }
  };
}
