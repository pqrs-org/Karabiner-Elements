#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>

void run_simultaneous_options_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "simultaneous_options"_test = [] {
    namespace basic = krbn::manipulator::manipulators::basic;

    // from_json

    {
      basic::from_event_definition event_definition(nlohmann::json::object({
          {"key_code", "escape"},
          {"simultaneous_options", nlohmann::json::object({
                                       {"detect_key_down_uninterruptedly", true},
                                       {"key_down_order", "strict"},
                                       {"key_up_order", "strict_inverse"},
                                       {"key_up_when", "all"},
                                   })},
      }));

      expect(event_definition.get_simultaneous_options().get_detect_key_down_uninterruptedly() == true);
      expect(event_definition.get_simultaneous_options().get_key_down_order() == basic::simultaneous_options::key_order::strict);
      expect(event_definition.get_simultaneous_options().get_key_up_order() == basic::simultaneous_options::key_order::strict_inverse);
      expect(event_definition.get_simultaneous_options().get_key_up_when() == basic::simultaneous_options::key_up_when::all);
    }
  };

  "simultaneous_options.to_after_key_up"_test = [] {
    namespace basic = krbn::manipulator::manipulators::basic;
    using krbn::manipulator::event_definition;

    // object

    {
      auto json = nlohmann::json::object({
          {"to_after_key_up", nlohmann::json::object({
                                  {"key_code", "tab"},
                              })},
      });

      auto o = json.get<basic::simultaneous_options>();
      expect(o.get_to_after_key_up().size() == 1);
      {
        auto& d = o.get_to_after_key_up()[0].get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
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

      auto o = json.get<basic::simultaneous_options>();
      expect(o.get_to_after_key_up().size() == 2);
      {
        auto& d = o.get_to_after_key_up()[0].get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
      }
      {
        auto& d = o.get_to_after_key_up()[1].get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
      }
    }
  };
}
