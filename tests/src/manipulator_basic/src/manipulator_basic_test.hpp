#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>

namespace {
krbn::modifier_flag_manager::active_modifier_flag left_command_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                 krbn::modifier_flag::left_command,
                                                                 krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_control_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                 krbn::modifier_flag::left_control,
                                                                 krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_option_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                krbn::modifier_flag::left_option,
                                                                krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                               krbn::modifier_flag::left_shift,
                                                               krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag right_shift_1(krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                                                                krbn::modifier_flag::right_shift,
                                                                krbn::device_id(1));
} // namespace

void run_manipulator_basic_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "manipulator.details.basic::from_event_definition"_test = [] {
    namespace basic = krbn::manipulator::manipulators::basic;
    //

    {
      nlohmann::json json({
          {"key_code", "spacebar"},
          {"modifiers", {
                            {"mandatory", {
                                              "shift",
                                              "left_command",
                                          }},
                            {"optional", {
                                             "any",
                                         }},
                        }},
      });
      basic::from_event_definition event_definition(json);
      expect(event_definition.get_event_definitions().size() == 1);
      expect(event_definition.get_event_definitions().front().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
             pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
      expect(event_definition.get_from_modifiers_definition().get_mandatory_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
                                                                                               krbn::manipulator::modifier_definition::modifier::shift,
                                                                                               krbn::manipulator::modifier_definition::modifier::left_command,
                                                                                           }));
      expect(event_definition.get_from_modifiers_definition().get_optional_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
                                                                                              krbn::manipulator::modifier_definition::modifier::any,
                                                                                          }));
      expect(event_definition.get_event_definitions().front().to_event() ==
             krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar)));
    }
    {
      nlohmann::json json({
          {"key_code", "right_option"},
          {"modifiers", {
                            {"mandatory", {
                                              "shift",
                                              "left_command",
                                              // duplicated
                                              "shift",
                                              "left_command",
                                          }},
                            {"optional", {
                                             "any",
                                             // duplicated
                                             "any",
                                         }},
                        }},
      });
      basic::from_event_definition event_definition(json);
      expect(event_definition.get_event_definitions().size() == 1);
      expect(event_definition.get_event_definitions().front().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
             pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_right_alt));
      expect(event_definition.get_from_modifiers_definition().get_mandatory_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
                                                                                               krbn::manipulator::modifier_definition::modifier::shift,
                                                                                               krbn::manipulator::modifier_definition::modifier::left_command,
                                                                                           }));
      expect(event_definition.get_from_modifiers_definition().get_optional_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
                                                                                              krbn::manipulator::modifier_definition::modifier::any,
                                                                                          }));
    }
    {
      nlohmann::json json({
          // The top level event_definition is ignored if `simultaneous` exists.
          {"key_code", "spacebar"},

          {"simultaneous", nlohmann::json::array({
                               nlohmann::json::object({
                                   {"key_code", "left_shift"},
                               }),
                               nlohmann::json::object({
                                   {"key_code", "right_shift"},
                               }),
                           })},
      });
      basic::from_event_definition event_definition(json);
      expect(event_definition.get_event_definitions().size() == 2);
      expect(event_definition.get_event_definitions()[0].get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
             pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_left_shift));
      expect(event_definition.get_event_definitions()[1].get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
             pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_right_shift));
    }
  };

  "basic::from_event_definition.test_event"_test = [] {
    namespace basic = krbn::manipulator::manipulators::basic;

    // Empty json

    {
      basic::from_event_definition d(nlohmann::json::object({
          {"key_code", "spacebar"},
      }));

      expect(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar)),
                                                      d));

      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                             pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                                                                             pqrs::hid::usage::consumer::mute)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                                             pqrs::hid::usage::button::button_1)),
                                                       d));
    }

    {
      basic::from_event_definition d(nlohmann::json::object({
          {"consumer_key_code", "rewind"},
      }));

      expect(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                                                                            pqrs::hid::usage::consumer::rewind)),
                                                      d));

      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                             pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                                                                             pqrs::hid::usage::consumer::mute)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                                             pqrs::hid::usage::button::button_1)),
                                                       d));
    }

    {
      basic::from_event_definition d(nlohmann::json::object({
          {"pointing_button", "button2"},
      }));

      expect(basic::from_event_definition::test_event(krbn::event_queue::event(
                                                          krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                       pqrs::hid::usage::button::button_2)),
                                                      d));

      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                             pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                                                                             pqrs::hid::usage::consumer::mute)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                                             pqrs::hid::usage::button::button_1)),
                                                       d));
    }

    {
      basic::from_event_definition d(nlohmann::json::object({
          {"any", "key_code"},
      }));

      expect(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                            pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                                      d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                                                                             pqrs::hid::usage::consumer::mute)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(
                                                           krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                                                                        pqrs::hid::usage::apple_vendor_keyboard::expose_all)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(
                                                           krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_top_case,
                                                                                        pqrs::hid::usage::apple_vendor_top_case::keyboard_fn)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                                             pqrs::hid::usage::button::button_1)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                                                                                             pqrs::hid::usage::generic_desktop::dpad_up)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                                                                                             pqrs::hid::usage::generic_desktop::hat_switch)),
                                                       d));
    }

    {
      basic::from_event_definition d(nlohmann::json::object({
          {"any", "consumer_key_code"},
      }));

      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                             pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                                       d));
      expect(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                                                                            pqrs::hid::usage::consumer::mute)),
                                                      d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(
                                                           krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                                                                        pqrs::hid::usage::apple_vendor_keyboard::expose_all)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                                             pqrs::hid::usage::button::button_1)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                                                                                             pqrs::hid::usage::generic_desktop::dpad_up)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                                                                                             pqrs::hid::usage::generic_desktop::hat_switch)),
                                                       d));
    }

    {
      basic::from_event_definition d(nlohmann::json::object({
          {"any", "pointing_button"},
      }));

      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                             pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                                                                             pqrs::hid::usage::consumer::mute)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(
                                                           krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                                                                        pqrs::hid::usage::apple_vendor_keyboard::expose_all)),
                                                       d));
      expect(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                                            pqrs::hid::usage::button::button_1)),
                                                      d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                                                                                             pqrs::hid::usage::generic_desktop::dpad_up)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                                                                                             pqrs::hid::usage::generic_desktop::hat_switch)),
                                                       d));
    }

    {
      basic::from_event_definition d(nlohmann::json::object({
          {"any", "dpad"},
      }));

      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::keyboard_or_keypad,
                                                                                                             pqrs::hid::usage::keyboard_or_keypad::keyboard_a)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::consumer,
                                                                                                             pqrs::hid::usage::consumer::mute)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(
                                                           krbn::momentary_switch_event(pqrs::hid::usage_page::apple_vendor_keyboard,
                                                                                        pqrs::hid::usage::apple_vendor_keyboard::expose_all)),
                                                       d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                                             pqrs::hid::usage::button::button_1)),
                                                       d));
      expect(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                                                                                            pqrs::hid::usage::generic_desktop::dpad_up)),
                                                      d));
      expect(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::generic_desktop,
                                                                                                             pqrs::hid::usage::generic_desktop::hat_switch)),
                                                       d));
    }
  };

  "to"_test = [] {
    namespace basic = krbn::manipulator::manipulators::basic;
    using krbn::manipulator::event_definition;

    // object

    {
      auto json = nlohmann::json::object({
          {"to", nlohmann::json::object({
                     {"key_code", "tab"},
                 })},
      });
      basic::basic b(json,
                     krbn::core_configuration::details::complex_modifications_parameters());
      expect(b.get_to().size() == 1);
      {
        auto& d = b.get_to()[0].get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
      }
    }

    // array

    {
      auto json = nlohmann::json::object({
          {"to", nlohmann::json::array({
                     nlohmann::json::object({{"key_code", "tab"}}),
                     nlohmann::json::object({{"key_code", "spacebar"}}),
                 })},
      });
      basic::basic b(json,
                     krbn::core_configuration::details::complex_modifications_parameters());
      expect(b.get_to().size() == 2);
      {
        auto& d = b.get_to()[0].get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_tab));
      }
      {
        auto& d = b.get_to()[1].get_event_definition();
        expect(d.get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_spacebar));
      }
    }
  };
}
