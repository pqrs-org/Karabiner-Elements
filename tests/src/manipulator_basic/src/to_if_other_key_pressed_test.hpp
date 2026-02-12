#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>

void run_to_if_other_key_pressed_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "to_if_other_key_pressed"_test = [] {
    namespace basic = krbn::manipulator::manipulators::basic;

    // other_keys object

    {
      auto json = nlohmann::json::object({
          {"to_if_other_key_pressed", nlohmann::json::object({
                                          {"other_keys", nlohmann::json::object({
                                                              {"key_code", "tab"},
                                                          })},
                                          {"to", nlohmann::json::object({
                                                     {"key_code", "left_command"},
                                                 })},
                                      })},
      });

      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      basic::basic b(json,
                     parameters);
      expect(b.get_to_if_other_key_pressed().get() != nullptr);
      expect(b.get_to_if_other_key_pressed()->get_entries().size() == 1);
      expect(b.get_to_if_other_key_pressed()->get_entries()[0].get_other_keys().size() == 1);
      expect(b.get_to_if_other_key_pressed()->get_entries()[0].get_to().size() == 1);
      {
        auto& d = b.get_to_if_other_key_pressed()->get_entries()[0].get_other_keys()[0].get_event_definitions()[0];
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
      }
      {
        auto& d = b.get_to_if_other_key_pressed()->get_entries()[0].get_to()[0]->get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_left_gui));
      }
    }

    // other_keys array

    {
      auto json = nlohmann::json::object({
          {"to_if_other_key_pressed", nlohmann::json::object({
                                          {"other_keys", nlohmann::json::array({
                                                              nlohmann::json::object({{"key_code", "tab"}}),
                                                              nlohmann::json::object({{"key_code", "spacebar"}}),
                                                          })},
                                          {"to", nlohmann::json::array({
                                                     nlohmann::json::object({{"key_code", "left_command"}}),
                                                     nlohmann::json::object({{"key_code", "left_shift"}}),
                                                 })},
                                      })},
      });

      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      basic::basic b(json,
                     parameters);
      expect(b.get_to_if_other_key_pressed().get() != nullptr);
      expect(b.get_to_if_other_key_pressed()->get_entries().size() == 1);
      expect(b.get_to_if_other_key_pressed()->get_entries()[0].get_other_keys().size() == 2);
      expect(b.get_to_if_other_key_pressed()->get_entries()[0].get_to().size() == 2);
      {
        auto& d = b.get_to_if_other_key_pressed()->get_entries()[0].get_other_keys()[1].get_event_definitions()[0];
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
      }
      {
        auto& d = b.get_to_if_other_key_pressed()->get_entries()[0].get_to()[1]->get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift));
      }
    }

    // array of entries

    {
      auto json = nlohmann::json::object({
          {"to_if_other_key_pressed", nlohmann::json::array({
                                          nlohmann::json::object({
                                              {"other_keys", nlohmann::json::object({{"key_code", "tab"}})},
                                              {"to", nlohmann::json::object({{"key_code", "left_command"}})},
                                          }),
                                          nlohmann::json::object({
                                              {"other_keys", nlohmann::json::array({
                                                  nlohmann::json::object({{"key_code", "spacebar"}}),
                                                  nlohmann::json::object({{"key_code", "escape"}}),
                                              })},
                                              {"to", nlohmann::json::array({
                                                  nlohmann::json::object({{"key_code", "left_option"}}),
                                                  nlohmann::json::object({{"key_code", "left_shift"}}),
                                              })},
                                          }),
                                      })},
      });

      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      basic::basic b(json,
                     parameters);
      expect(b.get_to_if_other_key_pressed().get() != nullptr);
      expect(b.get_to_if_other_key_pressed()->get_entries().size() == 2);
      {
        auto& d = b.get_to_if_other_key_pressed()->get_entries()[0].get_other_keys()[0].get_event_definitions()[0];
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
      }
      {
        auto& d = b.get_to_if_other_key_pressed()->get_entries()[1].get_to()[1]->get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift));
      }
    }
  };
}
