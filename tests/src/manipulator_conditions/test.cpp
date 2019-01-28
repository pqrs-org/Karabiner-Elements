#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "../share/json_helper.hpp"
#include "dispatcher_utility.hpp"
#include "manipulator/condition_manager.hpp"
#include "manipulator/manipulator_factory.hpp"

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("manipulator.manipulator_factory") {
  {
    nlohmann::json json;
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::frontmost_application*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "frontmost_application_if"},
        {
            "bundle_identifiers",
            {
                "^com\\.apple\\.Terminal$",
                "^com\\.googlecode\\.iterm2$",
            },
        },
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::frontmost_application*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "frontmost_application_unless"},
        {
            "bundle_identifiers",
            {
                "^com\\.apple\\.Terminal$",
                "^com\\.googlecode\\.iterm2$",
                "broken([regex",
            },
        },
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::frontmost_application*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json;
    json["type"] = "frontmost_application_if";
    json["bundle_identifiers"] = nlohmann::json::array();
    json["bundle_identifiers"].push_back("invalid(regex");
    json["file_paths"] = nlohmann::json::array();
    json["file_paths"].push_back("invalid(regex");
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::frontmost_application*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "input_source_if"},
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::input_source*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "input_source_unless"},
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::input_source*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json;
    json["type"] = "input_source_if";
    json["input_sources"] = nlohmann::json::array();
    {
      nlohmann::json j;
      j["language"] = "invalid(regex";
      j["input_source_id"] = "invalid(regex";
      j["input_mode_id"] = "invalid(regex";
      json["input_sources"].push_back(j);
    }
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::input_source*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "variable_if"},
        {"name", "variable_name"},
        {"value", 1},
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::variable*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "variable_unless"},
        {"name", "variable_name"},
        {"value", 1},
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::variable*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json;
    json["type"] = "keyboard_type_if";
    json["keyboard_types"] = nlohmann::json::array({"iso"});
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::keyboard_type*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json;
    json["type"] = "keyboard_type_unless";
    json["keyboard_types"] = nlohmann::json::array({"iso"});
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::keyboard_type*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::details::conditions::nop*>(condition.get()) == nullptr);
  }
}

namespace {
class actual_examples_helper final {
public:
  actual_examples_helper(const std::string& file_name) {
    std::ifstream input(std::string("json/") + file_name);
    auto json = nlohmann::json::parse(input);
    for (const auto& j : json) {
      condition_manager_.push_back_condition(krbn::manipulator::manipulator_factory::make_condition(j));
    }
  }

  const krbn::manipulator::condition_manager& get_condition_manager(void) const {
    return condition_manager_;
  }

private:
  krbn::manipulator::condition_manager condition_manager_;
};
} // namespace

TEST_CASE("manipulator_environment.save_to_file") {
  krbn::manipulator_environment manipulator_environment;
  manipulator_environment.enable_json_output("tmp/manipulator_environment.json");
  manipulator_environment.set_frontmost_application({"com.apple.Terminal",
                                                     "/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal"});
  pqrs::osx::input_source::properties properties;
  properties.set_first_language("en");
  properties.set_input_source_id("com.apple.keylayout.US");
  manipulator_environment.set_input_source_identifiers(properties);
  manipulator_environment.set_variable("value1", 100);
  manipulator_environment.set_variable("value2", 200);
  manipulator_environment.set_keyboard_type("iso");

  krbn::async_file_writer::wait();

  REQUIRE(krbn::unit_testing::json_helper::compare_files("expected/manipulator_environment.json",
                                                         "tmp/manipulator_environment.json"));
}

TEST_CASE("conditions.frontmost_application") {
  actual_examples_helper helper("frontmost_application.json");
  krbn::manipulator_environment manipulator_environment;
  krbn::event_queue::entry entry(krbn::device_id(1),
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::a));

  // bundle_identifiers matching
  manipulator_environment.set_frontmost_application({"com.apple.Terminal",
                                                     "/not_found"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                      manipulator_environment) == true);
  // use cache
  REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                      manipulator_environment) == true);

  // Test regex escape works properly
  manipulator_environment.set_frontmost_application({"com/apple/Terminal",
                                                     "/not_found"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                      manipulator_environment) == false);
  // use cache
  REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                      manipulator_environment) == false);

  // file_path matching
  manipulator_environment.set_frontmost_application({"com/apple/Terminal",
                                                     "/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                      manipulator_environment) == true);

  // frontmost_application_unless
  manipulator_environment.set_frontmost_application({"com.googlecode.iterm2",
                                                     "/Applications/iTerm.app"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                      manipulator_environment) == true);
  manipulator_environment.set_frontmost_application({"com.googlecode.iterm2",
                                                     "/Users/tekezo/Applications/iTerm.app"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                      manipulator_environment) == false);
}

TEST_CASE("conditions.input_source") {
  actual_examples_helper helper("input_source.json");
  krbn::manipulator_environment manipulator_environment;
  krbn::event_queue::entry entry(krbn::device_id(1),
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::a));

  // language matching
  {
    pqrs::osx::input_source::properties properties;
    properties.set_first_language("en");
    properties.set_input_source_id("com.apple.keylayout.Australian");
    manipulator_environment.set_input_source_identifiers(properties);
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
    // use cache
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }

  // Test regex escape works properly
  {
    pqrs::osx::input_source::properties properties;
    properties.set_first_language("ja");
    properties.set_input_source_id("com/apple/keylayout/Australian");
    manipulator_environment.set_input_source_identifiers(properties);
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);
    // use cache
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);
  }

  // input_source_id matching
  {
    pqrs::osx::input_source::properties properties;
    properties.set_first_language("ja");
    properties.set_input_source_id("com.apple.keylayout.US");
    manipulator_environment.set_input_source_identifiers(properties);
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }

  // input_mode_id matching
  {
    pqrs::osx::input_source::properties properties;
    properties.set_first_language("ja");
    properties.set_input_source_id("com.apple.keylayout.Australian");
    properties.set_input_mode_id("com.apple.inputmethod.Japanese.FullWidthRoman");
    manipulator_environment.set_input_source_identifiers(properties);
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }

  // input_source_unless
  {
    pqrs::osx::input_source::properties properties;
    properties.set_first_language("fr");
    properties.set_input_source_id("com.apple.keylayout.US");
    manipulator_environment.set_input_source_identifiers(properties);
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);
  }
}

TEST_CASE("conditions.device") {
  krbn::manipulator_environment manipulator_environment;

  auto device_id_8888_9999 = krbn::device_id(88889999);
  auto device_properties_8888_9999 = krbn::device_properties()
                                         .set(device_id_8888_9999)
                                         .set(krbn::vendor_id(8888))
                                         .set(krbn::product_id(9999))
                                         .set_is_keyboard(true)
                                         .set_is_pointing_device(false);
  manipulator_environment.insert_device_properties(device_id_8888_9999,
                                                   device_properties_8888_9999);

  auto device_id_1000_2000 = krbn::device_id(10002000);
  auto device_properties_1000_2000 = krbn::device_properties()
                                         .set(device_id_1000_2000)
                                         .set(krbn::vendor_id(1000))
                                         .set(krbn::product_id(2000))
                                         .set_is_keyboard(true)
                                         .set_is_pointing_device(false);
  manipulator_environment.insert_device_properties(device_id_1000_2000,
                                                   device_properties_1000_2000);

  auto device_id_1000_2001 = krbn::device_id(10002001);
  auto device_properties_1000_2001 = krbn::device_properties()
                                         .set(device_id_1000_2001)
                                         .set(krbn::vendor_id(1000))
                                         .set(krbn::product_id(2001))
                                         .set_is_keyboard(true)
                                         .set_is_pointing_device(false);
  manipulator_environment.insert_device_properties(device_id_1000_2001,
                                                   device_properties_1000_2001);

  auto device_id_1001_2000 = krbn::device_id(10012000);
  auto device_properties_1001_2000 = krbn::device_properties()
                                         .set(device_id_1001_2000)
                                         .set(krbn::vendor_id(1001))
                                         .set(krbn::product_id(2000))
                                         .set_is_keyboard(true)
                                         .set_is_pointing_device(false);
  manipulator_environment.insert_device_properties(device_id_1001_2000,
                                                   device_properties_1001_2000);

  auto device_id_1001_2001 = krbn::device_id(10012001);
  auto device_properties_1001_2001 = krbn::device_properties()
                                         .set(device_id_1001_2001)
                                         .set(krbn::vendor_id(1001))
                                         .set(krbn::product_id(2001))
                                         .set_is_keyboard(true)
                                         .set_is_pointing_device(false);
  manipulator_environment.insert_device_properties(device_id_1001_2001,
                                                   device_properties_1001_2001);

  auto device_id_1099_9999 = krbn::device_id(10999999);
  auto device_properties_1099_9999 = krbn::device_properties()
                                         .set(device_id_1099_9999)
                                         .set(krbn::vendor_id(1099))
                                         .set(krbn::product_id(9999))
                                         .set_is_keyboard(true)
                                         .set_is_pointing_device(false);
  manipulator_environment.insert_device_properties(device_id_1099_9999,
                                                   device_properties_1099_9999);

#define ENTRY(DEVICE_ID)                                                                      \
  krbn::event_queue::entry(DEVICE_ID,                                                         \
                           krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)), \
                           krbn::event_queue::event(krbn::key_code::a),                       \
                           krbn::event_type::key_down,                                        \
                           krbn::event_queue::event(krbn::key_code::a))

  {
    actual_examples_helper helper("device_if.json");
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_8888_9999),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1000_2000),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1000_2001),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1001_2000),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1001_2001),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1099_9999),
                                                        manipulator_environment) == true);
  }
  {
    actual_examples_helper helper("device_unless.json");
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_8888_9999),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1000_2000),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1000_2001),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1001_2000),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1001_2001),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(ENTRY(device_id_1099_9999),
                                                        manipulator_environment) == false);
  }

  {
    nlohmann::json json;
    json["type"] = "device_if";
    json["identifiers"] = nlohmann::json::array();
    json["identifiers"].push_back(nlohmann::json::object());
    json["identifiers"].back()["vendor_id"] = 1000;
    json["identifiers"].back()["product_id"] = 2000;
    json["identifiers"].back()["is_keyboard"] = true;
    json["identifiers"].back()["is_pointing_device"] = false;
    krbn::manipulator::details::conditions::device condition(json);

    REQUIRE(condition.is_fulfilled(ENTRY(device_id_1000_2000),
                                   manipulator_environment) == true);
    REQUIRE(condition.is_fulfilled(ENTRY(device_id_1000_2001),
                                   manipulator_environment) == false);

    {
      auto dp = krbn::device_properties()
                    .set(krbn::device_id(0))
                    .set(krbn::vendor_id(1000))
                    .set(krbn::product_id(2000))
                    .set_is_keyboard(true)
                    .set_is_pointing_device(false);
      manipulator_environment.insert_device_properties(krbn::device_id(0),
                                                       dp);
      REQUIRE(condition.is_fulfilled(ENTRY(krbn::device_id(0)),
                                     manipulator_environment) == true);
    }
    {
      auto dp = krbn::device_properties()
                    .set(krbn::device_id(0))
                    .set(krbn::vendor_id(1000))
                    .set(krbn::product_id(2000))
                    .set_is_keyboard(false)
                    .set_is_pointing_device(false);
      manipulator_environment.insert_device_properties(krbn::device_id(0),
                                                       dp);
      REQUIRE(condition.is_fulfilled(ENTRY(krbn::device_id(0)),
                                     manipulator_environment) == false);
    }
    {
      auto dp = krbn::device_properties()
                    .set(krbn::device_id(0))
                    .set(krbn::vendor_id(1000))
                    .set(krbn::product_id(2000))
                    .set_is_keyboard(true)
                    .set_is_pointing_device(true);
      manipulator_environment.insert_device_properties(krbn::device_id(0),
                                                       dp);
      REQUIRE(condition.is_fulfilled(ENTRY(krbn::device_id(0)),
                                     manipulator_environment) == false);
    }
    {
      auto dp = krbn::device_properties()
                    .set(krbn::device_id(0))
                    .set(krbn::vendor_id(1000))
                    .set(krbn::product_id(2000))
                    .set_is_keyboard(false)
                    .set_is_pointing_device(true);
      manipulator_environment.insert_device_properties(krbn::device_id(0),
                                                       dp);
      REQUIRE(condition.is_fulfilled(ENTRY(krbn::device_id(0)),
                                     manipulator_environment) == false);
    }
  }

  {
    nlohmann::json json;
    json["type"] = "device_if";
    json["identifiers"] = nlohmann::json::array();
    json["identifiers"].push_back(nlohmann::json::object());
    json["identifiers"].back()["vendor_id"] = 1000;
    json["identifiers"].back()["product_id"] = 2000;
    json["identifiers"].back()["location_id"] = 3000;
    krbn::manipulator::details::conditions::device condition(json);

    {
      auto dp = krbn::device_properties()
                    .set(krbn::device_id(0))
                    .set(krbn::vendor_id(1000))
                    .set(krbn::product_id(2000))
                    .set(krbn::location_id(3000));
      manipulator_environment.insert_device_properties(krbn::device_id(0),
                                                       dp);
      REQUIRE(condition.is_fulfilled(ENTRY(krbn::device_id(0)),
                                     manipulator_environment) == true);
    }
    {
      auto dp = krbn::device_properties()
                    .set(krbn::device_id(0))
                    .set(krbn::vendor_id(1000))
                    .set(krbn::product_id(2000));
      manipulator_environment.insert_device_properties(krbn::device_id(0),
                                                       dp);
      REQUIRE(condition.is_fulfilled(ENTRY(krbn::device_id(0)),
                                     manipulator_environment) == false);
    }
    {
      auto dp = krbn::device_properties()
                    .set(krbn::device_id(0))
                    .set(krbn::vendor_id(1000))
                    .set(krbn::product_id(2000))
                    .set(krbn::location_id(4000));
      manipulator_environment.insert_device_properties(krbn::device_id(0),
                                                       dp);
      REQUIRE(condition.is_fulfilled(ENTRY(krbn::device_id(0)),
                                     manipulator_environment) == false);
    }
  }

#undef ENTRY
}

TEST_CASE("conditions.keyboard_type") {
  krbn::manipulator_environment manipulator_environment;
  krbn::event_queue::entry entry(krbn::device_id(1),
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::a));

  {
    actual_examples_helper helper("keyboard_type_if.json");

    manipulator_environment.set_keyboard_type("iso");
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);

    manipulator_environment.set_keyboard_type("ansi");
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);
  }
  {
    actual_examples_helper helper("keyboard_type_unless.json");

    manipulator_environment.set_keyboard_type("iso");
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);

    manipulator_environment.set_keyboard_type("ansi");
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
