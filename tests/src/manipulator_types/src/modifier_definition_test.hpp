#include "manipulator/types.hpp"
#include <boost/ut.hpp>

void run_modifier_definition_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "modifier_definition.modifier json"_test = [] {
    using krbn::manipulator::modifier_definition::modifier;

    std::vector<std::pair<modifier, std::string>> pairs{
        std::make_pair(modifier::any, "any"),
        std::make_pair(modifier::caps_lock, "caps_lock"),
        std::make_pair(modifier::command, "command"),
        std::make_pair(modifier::control, "control"),
        std::make_pair(modifier::fn, "fn"),
        std::make_pair(modifier::left_command, "left_command"),
        std::make_pair(modifier::left_control, "left_control"),
        std::make_pair(modifier::left_option, "left_option"),
        std::make_pair(modifier::left_shift, "left_shift"),
        std::make_pair(modifier::option, "option"),
        std::make_pair(modifier::right_command, "right_command"),
        std::make_pair(modifier::right_control, "right_control"),
        std::make_pair(modifier::right_option, "right_option"),
        std::make_pair(modifier::right_shift, "right_shift"),
        std::make_pair(modifier::shift, "shift"),
        std::make_pair(modifier::end_, "end_"),
    };
    for (const auto& [m, name] : pairs) {
      // from_json
      {
        nlohmann::json json = name;
        expect(json.get<modifier>() == m);
      }

      // to_json
      {
        nlohmann::json json = m;
        expect(json.get<modifier>() == m);
      }
    }

    // aliases

    expect(nlohmann::json("left_gui").get<modifier>() == modifier::left_command);
    expect(nlohmann::json("right_gui").get<modifier>() == modifier::right_command);
    expect(nlohmann::json("left_alt").get<modifier>() == modifier::left_option);
    expect(nlohmann::json("right_alt").get<modifier>() == modifier::right_option);
  };

  "modifier_definition.make_modifiers"_test = [] {
    namespace modifier_definition = krbn::manipulator::modifier_definition;
    using krbn::manipulator::event_definition;

    // array

    {
      nlohmann::json json({"left_command", "left_shift", "fn", "any"});
      auto actual = modifier_definition::make_modifiers(json);
      auto expected = std::set<modifier_definition::modifier>({
          modifier_definition::modifier::left_command,
          modifier_definition::modifier::left_shift,
          modifier_definition::modifier::fn,
          modifier_definition::modifier::any,
      });
      expect(actual == expected);
    }

    // string

    {
      nlohmann::json json("left_command");
      auto actual = modifier_definition::make_modifiers(json);
      auto expected = std::set<modifier_definition::modifier>({
          modifier_definition::modifier::left_command,
      });
      expect(actual == expected);
    }
  };

  "modifier_definition.get_modifier"_test = [] {
    namespace modifier_definition = krbn::manipulator::modifier_definition;

    expect(modifier_definition::get_modifier(krbn::modifier_flag::zero) == modifier_definition::modifier::end_);
    expect(modifier_definition::get_modifier(krbn::modifier_flag::left_shift) == modifier_definition::modifier::left_shift);
  };
}
