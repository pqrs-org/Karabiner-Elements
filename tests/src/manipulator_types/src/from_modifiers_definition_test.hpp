#include "../../share/json_helper.hpp"
#include "manipulator/types.hpp"
#include "modifier_flag_manager.hpp"
#include <boost/ut.hpp>

namespace {
krbn::modifier_flag_manager::active_modifier_flag left_command_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::left_command,
    krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag right_shift_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::right_shift,
    krbn::device_id(1));
} // namespace

void run_from_modifiers_definition_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "from_event_definition"_test = [] {
    namespace modifier_definition = krbn::manipulator::modifier_definition;
    using krbn::manipulator::from_modifiers_definition;

    {
      auto json = nlohmann::json::object({
          {"mandatory", nlohmann::json::array({"left_shift"})},
          {"optional", nlohmann::json::array({"any"})},
      });

      auto d = json.get<from_modifiers_definition>();

      expect(d.get_mandatory_modifiers() == std::set<modifier_definition::modifier>{
                                                modifier_definition::modifier::left_shift,
                                            });
      expect(d.get_optional_modifiers() == std::set<modifier_definition::modifier>{
                                               modifier_definition::modifier::any,
                                           });
    }
  };

  "from_modifiers_definition::test_modifier"_test = [] {
    namespace modifier_definition = krbn::manipulator::modifier_definition;
    using krbn::manipulator::from_modifiers_definition;

    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_command_1);

      {
        auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager,
                                                               modifier_definition::modifier::left_shift);
        auto expected = std::make_pair(false, krbn::modifier_flag::zero);
        expect(actual == expected);
      }
      {
        auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager,
                                                               modifier_definition::modifier::left_command);
        auto expected = std::make_pair(true, krbn::modifier_flag::left_command);
        expect(actual == expected);
      }
      {
        auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::right_command);
        auto expected = std::make_pair(false, krbn::modifier_flag::zero);
        expect(actual == expected);
      }
      {
        auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::command);
        auto expected = std::make_pair(true, krbn::modifier_flag::left_command);
        expect(actual == expected);
      }
      {
        auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::shift);
        auto expected = std::make_pair(false, krbn::modifier_flag::zero);
        expect(actual == expected);
      }
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);

      {
        auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::left_shift);
        auto expected = std::make_pair(false, krbn::modifier_flag::zero);
        expect(actual == expected);
      }
      {
        auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::right_shift);
        auto expected = std::make_pair(true, krbn::modifier_flag::right_shift);
        expect(actual == expected);
      }
      {
        auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::shift);
        auto expected = std::make_pair(true, krbn::modifier_flag::right_shift);
        expect(actual == expected);
      }
    }
  };

  "from_event_definition.test_modifiers"_test = [] {
    auto json = krbn::unit_testing::json_helper::load_jsonc("json/from_modifiers_definitions.jsonc");
    for (const auto& j : json) {
      auto d = j.at("input").get<krbn::manipulator::from_modifiers_definition>();

      for (const auto& t : j.at("tests")) {
        krbn::modifier_flag_manager modifier_flag_manager;

        for (const auto& value : t.at("input")) {
          modifier_flag_manager.push_back_active_modifier_flag(
              krbn::modifier_flag_manager::active_modifier_flag(
                  krbn::modifier_flag_manager::active_modifier_flag::type::increase,
                  value.get<krbn::modifier_flag>(),
                  krbn::device_id(1)));
        }

        auto actual = d.test_modifiers(modifier_flag_manager);
        if (t.at("expected").is_null()) {
          expect(actual == nullptr);
        } else {
          expect(*actual == t.at("expected").get<std::unordered_set<krbn::modifier_flag>>());
        }
      }
    }
  };
}
