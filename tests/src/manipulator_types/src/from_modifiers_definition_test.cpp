#include <catch2/catch.hpp>

#include "manipulator/types.hpp"
#include "modifier_flag_manager.hpp"

namespace {
krbn::modifier_flag_manager::active_modifier_flag left_command_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::left_command,
    krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_control_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::left_control,
    krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_option_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::left_option,
    krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag left_shift_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::left_shift,
    krbn::device_id(1));
krbn::modifier_flag_manager::active_modifier_flag right_shift_1(
    krbn::modifier_flag_manager::active_modifier_flag::type::increase,
    krbn::modifier_flag::right_shift,
    krbn::device_id(1));
} // namespace

TEST_CASE("from_modifiers_definition::test_modifier") {
  namespace modifier_definition = krbn::manipulator::modifier_definition;
  using krbn::manipulator::from_modifiers_definition;

  {
    krbn::modifier_flag_manager modifier_flag_manager;
    modifier_flag_manager.push_back_active_modifier_flag(left_command_1);

    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager,
                                                             modifier_definition::modifier::left_shift);
      auto expected = std::make_pair(false, krbn::modifier_flag::zero);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager,
                                                             modifier_definition::modifier::left_command);
      auto expected = std::make_pair(true, krbn::modifier_flag::left_command);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::right_command);
      auto expected = std::make_pair(false, krbn::modifier_flag::zero);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::command);
      auto expected = std::make_pair(true, krbn::modifier_flag::left_command);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::shift);
      auto expected = std::make_pair(false, krbn::modifier_flag::zero);
      REQUIRE(actual == expected);
    }
  }
  {
    krbn::modifier_flag_manager modifier_flag_manager;
    modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);

    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::left_shift);
      auto expected = std::make_pair(false, krbn::modifier_flag::zero);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::right_shift);
      auto expected = std::make_pair(true, krbn::modifier_flag::right_shift);
      REQUIRE(actual == expected);
    }
    {
      auto actual = from_modifiers_definition::test_modifier(modifier_flag_manager, modifier_definition::modifier::shift);
      auto expected = std::make_pair(true, krbn::modifier_flag::right_shift);
      REQUIRE(actual == expected);
    }
  }
}

TEST_CASE("from_event_definition.test_modifiers") {
  namespace modifier_definition = krbn::manipulator::modifier_definition;
  using krbn::manipulator::from_modifiers_definition;

  // empty

  {
    from_modifiers_definition d;

    REQUIRE(d.get_mandatory_modifiers() == std::set<modifier_definition::modifier>{});
    REQUIRE(d.get_optional_modifiers() == std::set<modifier_definition::modifier>{});

    {
      krbn::modifier_flag_manager modifier_flag_manager;
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({}));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::nullopt);
    }
  }

  // mandatory_modifiers any

  {
    auto json = nlohmann::json::object({
        {"modifiers", nlohmann::json::object({
                          {"mandatory", nlohmann::json::array({"any"})},
                      })},
    });

    from_modifiers_definition d;
    std::string key("modifiers");
    REQUIRE(d.handle_json(key, json.at(key), json));

    REQUIRE(d.get_mandatory_modifiers() == std::set<modifier_definition::modifier>{
                                               modifier_definition::modifier::any,
                                           });
    REQUIRE(d.get_optional_modifiers() == std::set<modifier_definition::modifier>{});

    {
      krbn::modifier_flag_manager modifier_flag_manager;
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({}));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_command_1);
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_command,
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
  }

  // optional_modifiers any

  {
    auto json = nlohmann::json::object({
        {"modifiers", nlohmann::json::object({
                          {"optional", nlohmann::json::array({"any"})},
                      })},
    });

    from_modifiers_definition d;
    std::string key("modifiers");
    REQUIRE(d.handle_json(key, json.at(key), json));

    REQUIRE(d.get_mandatory_modifiers() == std::set<modifier_definition::modifier>{});
    REQUIRE(d.get_optional_modifiers() == std::set<modifier_definition::modifier>{
                                              modifier_definition::modifier::any,
                                          });

    {
      krbn::modifier_flag_manager modifier_flag_manager;
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({}));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({}));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_command_1);
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({}));
    }
  }

  // mandatory_modifiers and optional_modifiers

  {
    auto json = nlohmann::json::object({
        {"modifiers", nlohmann::json::object({
                          {"mandatory", nlohmann::json::array({"control"})},
                          {"optional", nlohmann::json::array({"shift"})},
                      })},
    });

    from_modifiers_definition d;
    std::string key("modifiers");
    REQUIRE(d.handle_json(key, json.at(key), json));

    REQUIRE(d.get_mandatory_modifiers() == std::set<modifier_definition::modifier>{
                                               modifier_definition::modifier::control,
                                           });
    REQUIRE(d.get_optional_modifiers() == std::set<modifier_definition::modifier>{
                                              modifier_definition::modifier::shift,
                                          });

    {
      krbn::modifier_flag_manager modifier_flag_manager;
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::nullopt);
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_control_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_control,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_control_1);
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_control,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_control_1);
      modifier_flag_manager.push_back_active_modifier_flag(left_option_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::nullopt);
    }
  }

  // optional_modifiers any with mandatory_modifiers

  {
    auto json = nlohmann::json::object({
        {"key_code", "spacebar"},
        {"modifiers", nlohmann::json::object({
                          {"mandatory", nlohmann::json::array({"left_shift"})},
                          {"optional", nlohmann::json::array({"any"})},
                      })},
    });

    from_modifiers_definition d;
    std::string key("modifiers");
    REQUIRE(d.handle_json(key, json.at(key), json));

    REQUIRE(d.get_mandatory_modifiers() == std::set<modifier_definition::modifier>{
                                               modifier_definition::modifier::left_shift,
                                           });
    REQUIRE(d.get_optional_modifiers() == std::set<modifier_definition::modifier>{
                                              modifier_definition::modifier::any,
                                          });

    {
      krbn::modifier_flag_manager modifier_flag_manager;
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::nullopt);
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_command_1);
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
  }

  // mandatory_modifiers strict matching

  {
    auto json = nlohmann::json::object({
        {"modifiers", nlohmann::json::object({
                          {"mandatory", nlohmann::json::array({"left_shift"})},
                      })},
    });

    from_modifiers_definition d;
    std::string key("modifiers");
    REQUIRE(d.handle_json(key, json.at(key), json));

    REQUIRE(d.get_mandatory_modifiers() == std::set<modifier_definition::modifier>{
                                               modifier_definition::modifier::left_shift,
                                           });
    REQUIRE(d.get_optional_modifiers() == std::set<modifier_definition::modifier>{});

    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_command_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::nullopt);
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::nullopt);
    }
  }

  // mandatory_modifiers (modifier::shift)

  {
    auto json = nlohmann::json::object({
        {"modifiers", nlohmann::json::object({
                          {"mandatory", nlohmann::json::array({"shift"})},
                          {"optional", nlohmann::json::array({"any"})},
                      })},
    });

    from_modifiers_definition d;
    std::string key("modifiers");
    REQUIRE(d.handle_json(key, json.at(key), json));

    REQUIRE(d.get_mandatory_modifiers() == std::set<modifier_definition::modifier>{
                                               modifier_definition::modifier::shift,
                                           });
    REQUIRE(d.get_optional_modifiers() == std::set<modifier_definition::modifier>{
                                              modifier_definition::modifier::any,
                                          });

    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_command_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::nullopt);
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::right_shift,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_command_1);
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
  }

  // mandatory_modifiers strict matching (modifier::shift)

  {
    auto json = nlohmann::json::object({
        {"modifiers", nlohmann::json::object({
                          {"mandatory", nlohmann::json::array({"shift"})},
                      })},
    });

    from_modifiers_definition d;
    std::string key("modifiers");
    REQUIRE(d.handle_json(key, json.at(key), json));

    REQUIRE(d.get_mandatory_modifiers() == std::set<modifier_definition::modifier>{
                                               modifier_definition::modifier::shift,
                                           });
    REQUIRE(d.get_optional_modifiers() == std::set<modifier_definition::modifier>{});

    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_command_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::nullopt);
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::right_shift,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::unordered_set<krbn::modifier_flag>({
                                                             krbn::modifier_flag::left_shift,
                                                         }));
    }
    {
      krbn::modifier_flag_manager modifier_flag_manager;
      modifier_flag_manager.push_back_active_modifier_flag(left_command_1);
      modifier_flag_manager.push_back_active_modifier_flag(left_shift_1);
      modifier_flag_manager.push_back_active_modifier_flag(right_shift_1);
      REQUIRE(d.test_modifiers(modifier_flag_manager) == std::nullopt);
    }
  }
}
