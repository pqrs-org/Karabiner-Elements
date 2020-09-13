#include <catch2/catch.hpp>

#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"

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

TEST_CASE("manipulator.details.basic::from_event_definition") {
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
    REQUIRE(event_definition.get_event_definitions().size() == 1);
    REQUIRE(event_definition.get_event_definitions().front().get_type() == krbn::manipulator::event_definition::type::key_code);
    REQUIRE(event_definition.get_event_definitions().front().get_key_code() == krbn::key_code::keyboard_spacebar);
    REQUIRE(event_definition.get_event_definitions().front().get_pointing_button() == std::nullopt);
    REQUIRE(event_definition.get_from_modifiers_definition().get_mandatory_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
                                                                                              krbn::manipulator::modifier_definition::modifier::shift,
                                                                                              krbn::manipulator::modifier_definition::modifier::left_command,
                                                                                          }));
    REQUIRE(event_definition.get_from_modifiers_definition().get_optional_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
                                                                                             krbn::manipulator::modifier_definition::modifier::any,
                                                                                         }));
    REQUIRE(event_definition.get_event_definitions().front().to_event() == krbn::event_queue::event(krbn::key_code::keyboard_spacebar));
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
    REQUIRE(event_definition.get_event_definitions().size() == 1);
    REQUIRE(event_definition.get_event_definitions().front().get_type() == krbn::manipulator::event_definition::type::key_code);
    REQUIRE(event_definition.get_event_definitions().front().get_key_code() == krbn::key_code::keyboard_right_option);
    REQUIRE(event_definition.get_event_definitions().front().get_pointing_button() == std::nullopt);
    REQUIRE(event_definition.get_from_modifiers_definition().get_mandatory_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
                                                                                              krbn::manipulator::modifier_definition::modifier::shift,
                                                                                              krbn::manipulator::modifier_definition::modifier::left_command,
                                                                                          }));
    REQUIRE(event_definition.get_from_modifiers_definition().get_optional_modifiers() == std::set<krbn::manipulator::modifier_definition::modifier>({
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
    REQUIRE(event_definition.get_event_definitions().size() == 2);
    REQUIRE(event_definition.get_event_definitions()[0].get_key_code() == krbn::key_code::keyboard_left_shift);
    REQUIRE(event_definition.get_event_definitions()[1].get_key_code() == krbn::key_code::keyboard_right_shift);
  }
}

TEST_CASE("basic::from_event_definition.test_event") {
  namespace basic = krbn::manipulator::manipulators::basic;

  // Empty json

  {
    basic::from_event_definition d(nlohmann::json::object({
        {"key_code", "spacebar"},
    }));

    REQUIRE(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::key_code::keyboard_spacebar), d));

    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::key_code::keyboard_a), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::consumer_key_code::mute), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::pointing_button::button1), d));
  }

  {
    basic::from_event_definition d(nlohmann::json::object({
        {"consumer_key_code", "rewind"},
    }));

    REQUIRE(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::consumer_key_code::rewind), d));

    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::key_code::keyboard_a), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::consumer_key_code::mute), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::pointing_button::button1), d));
  }

  {
    basic::from_event_definition d(nlohmann::json::object({
        {"pointing_button", "button2"},
    }));

    REQUIRE(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::pointing_button::button2), d));

    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::key_code::keyboard_a), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::consumer_key_code::mute), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::pointing_button::button1), d));
  }

  {
    basic::from_event_definition d(nlohmann::json::object({
        {"any", "key_code"},
    }));

    REQUIRE(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::key_code::keyboard_a), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::consumer_key_code::mute), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::pointing_button::button1), d));
  }

  {
    basic::from_event_definition d(nlohmann::json::object({
        {"any", "consumer_key_code"},
    }));

    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::key_code::keyboard_a), d));
    REQUIRE(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::consumer_key_code::mute), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::pointing_button::button1), d));
  }

  {
    basic::from_event_definition d(nlohmann::json::object({
        {"any", "pointing_button"},
    }));

    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::key_code::keyboard_a), d));
    REQUIRE(!basic::from_event_definition::test_event(krbn::event_queue::event(krbn::consumer_key_code::mute), d));
    REQUIRE(basic::from_event_definition::test_event(krbn::event_queue::event(krbn::pointing_button::button1), d));
  }
}

TEST_CASE("to") {
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
    REQUIRE(b.get_to().size() == 1);
    {
      auto& d = b.get_to()[0].get_event_definition();
      REQUIRE(d.get_type() == event_definition::type::key_code);
      REQUIRE(std::get<krbn::key_code::value_t>(d.get_value()) == krbn::key_code::keyboard_tab);
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
    REQUIRE(b.get_to().size() == 2);
    {
      auto& d = b.get_to()[0].get_event_definition();
      REQUIRE(d.get_type() == event_definition::type::key_code);
      REQUIRE(std::get<krbn::key_code::value_t>(d.get_value()) == krbn::key_code::keyboard_tab);
    }
    {
      auto& d = b.get_to()[1].get_event_definition();
      REQUIRE(d.get_type() == event_definition::type::key_code);
      REQUIRE(std::get<krbn::key_code::value_t>(d.get_value()) == krbn::key_code::keyboard_spacebar);
    }
  }
}
