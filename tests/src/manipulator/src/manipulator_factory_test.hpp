#include "../../share/manipulator_conditions_helper.hpp"
#include "../../share/manipulator_helper.hpp"
#include <boost/ut.hpp>

namespace modifier_definition = krbn::manipulator::modifier_definition;
using krbn::manipulator::event_definition;
using krbn::manipulator::to_event_definition;

void run_manipulator_factory_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "manipulator.manipulator_factory"_test = [] {
    {
      nlohmann::json json({
          {"type", "basic"},
          {
              "from",
              {
                  {
                      "key_code",
                      "escape",
                  },
                  {
                      "modifiers",
                      {
                          {"mandatory", {
                                            "left_shift",
                                            "left_option",
                                        }},
                          {"optional", {
                                           "any",
                                       }},
                      },
                  },
              },
          },
          {
              "to",
              {
                  {
                      {
                          "pointing_button",
                          "button1",
                      },
                  },
              },
          },
      });
      auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
      auto manipulator = krbn::manipulator::manipulator_factory::make_manipulator(json,
                                                                                  parameters);
      auto p = krbn::memory_utility::unwrap_not_null(manipulator).get();
      expect(dynamic_cast<krbn::manipulator::manipulators::basic::basic*>(p) != nullptr);
      expect(dynamic_cast<krbn::manipulator::manipulators::nop*>(p) == nullptr);
      expect(manipulator->get_validity() == krbn::validity::valid);
      expect(manipulator->active() == false);

      auto basic = dynamic_cast<krbn::manipulator::manipulators::basic::basic*>(p);
      expect(basic->get_from().get_event_definitions().size() == 1);
      expect(basic->get_from().get_event_definitions().front().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
             pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                   pqrs::hid::usage::keyboard_or_keypad::keyboard_escape));
      expect(basic->get_from().get_from_modifiers_definition().get_mandatory_modifiers() == std::set<modifier_definition::modifier>({
                                                                                                modifier_definition::modifier::left_shift,
                                                                                                modifier_definition::modifier::left_option,
                                                                                            }));
      expect(basic->get_from().get_from_modifiers_definition().get_optional_modifiers() == std::set<modifier_definition::modifier>({
                                                                                               modifier_definition::modifier::any,
                                                                                           }));
      expect(basic->get_to().size() == 1);
      expect(basic->get_to()[0]->get_event_definition().get_type() == event_definition::type::momentary_switch_event);
      expect(basic->get_to()[0]->get_event_definition().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
             pqrs::hid::usage_pair(pqrs::hid::usage_page::button,
                                   pqrs::hid::usage::button::button_1));
      expect(basic->get_to()[0]->get_modifiers() == std::set<modifier_definition::modifier>());
    }
  };

  "errors"_test = [] {
    {
      try {
        nlohmann::json json;
        auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
        krbn::manipulator::manipulator_factory::make_manipulator(json, parameters);
        expect(false);
      } catch (pqrs::json::unmarshal_error& ex) {
        expect(std::string("`type` must be specified: null") == ex.what());
      } catch (...) {
        expect(false);
      }
    }
  };
}
