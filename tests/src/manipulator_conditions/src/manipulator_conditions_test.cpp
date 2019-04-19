#include <catch2/catch.hpp>

#include "../../share/json_helper.hpp"
#include "manipulator/condition_manager.hpp"
#include "manipulator/manipulator_factory.hpp"

TEST_CASE("manipulator.manipulator_factory") {
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
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::frontmost_application*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "input_source_if"},
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::input_source*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "input_source_unless"},
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::input_source*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json;
    json["type"] = "input_source_if";
    json["input_sources"] = nlohmann::json::array();
    {
      nlohmann::json j;
      j["language"] = "^en$";
      j["input_source_id"] = "^com\\.apple\\.keylayout\\.US$";
      j["input_mode_id"] = "^com\\.apple\\.inputmethod\\.SCIM\\.ITABC$";
      json["input_sources"].push_back(j);
    }
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::input_source*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "variable_if"},
        {"name", "variable_name"},
        {"value", 1},
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::variable*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json({
        {"type", "variable_unless"},
        {"name", "variable_name"},
        {"value", 1},
    });
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::variable*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json;
    json["type"] = "keyboard_type_if";
    json["keyboard_types"] = nlohmann::json::array({"iso"});
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::keyboard_type*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::nop*>(condition.get()) == nullptr);
  }
  {
    nlohmann::json json;
    json["type"] = "keyboard_type_unless";
    json["keyboard_types"] = nlohmann::json::array({"iso"});
    auto condition = krbn::manipulator::manipulator_factory::make_condition(json);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::keyboard_type*>(condition.get()) != nullptr);
    REQUIRE(dynamic_cast<krbn::manipulator::conditions::nop*>(condition.get()) == nullptr);
  }
}

namespace {
class actual_examples_helper final {
public:
  actual_examples_helper(const std::string& file_name) {
    std::ifstream input(std::string("json/") + file_name);
    auto json = nlohmann::json::parse(input);
    for (const auto& j : json) {
      try {
        condition_manager_.push_back_condition(krbn::manipulator::manipulator_factory::make_condition(j));
      } catch (pqrs::json::unmarshal_error& e) {
        error_messages_.push_back(e.what());
      } catch (std::exception& e) {
        REQUIRE(false);
      }
    }
  }

  const krbn::manipulator::condition_manager& get_condition_manager(void) const {
    return condition_manager_;
  }

  const std::vector<std::string> get_error_messages(void) const {
    return error_messages_;
  }

private:
  krbn::manipulator::condition_manager condition_manager_;
  std::vector<std::string> error_messages_;
};
} // namespace

TEST_CASE("manipulator_environment.save_to_file") {
  krbn::manipulator::manipulator_environment manipulator_environment;
  manipulator_environment.enable_json_output("tmp/manipulator_environment.json");

  pqrs::osx::frontmost_application_monitor::application application;
  application.set_bundle_identifier("com.apple.Terminal");
  application.set_file_path("/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal");
  manipulator_environment.set_frontmost_application(application);

  pqrs::osx::input_source::properties properties;
  properties.set_first_language("en");
  properties.set_input_source_id("com.apple.keylayout.US");
  manipulator_environment.set_input_source_properties(properties);

  manipulator_environment.set_variable("value1", 100);
  manipulator_environment.set_variable("value2", 200);

  pqrs::osx::system_preferences::properties system_preferences_properties;
  system_preferences_properties.set_use_fkeys_as_standard_function_keys(true);
  system_preferences_properties.set_scroll_direction_is_natural(false);
  system_preferences_properties.set_keyboard_types(
      std::map<pqrs::osx::system_preferences::keyboard_type_key,
               pqrs::osx::iokit_keyboard_type>({
          {
              pqrs::osx::system_preferences::keyboard_type_key(krbn::vendor_id_karabiner_virtual_hid_device,
                                                               krbn::product_id_karabiner_virtual_hid_keyboard,
                                                               krbn::hid_country_code(0)),
              pqrs::osx::iokit_keyboard_type(41),
          },
      }));
  manipulator_environment.set_system_preferences_properties(system_preferences_properties);

  manipulator_environment.set_virtual_hid_keyboard_country_code(krbn::hid_country_code(0));

  krbn::async_file_writer::wait();

  REQUIRE(krbn::unit_testing::json_helper::compare_files("expected/manipulator_environment.json",
                                                         "tmp/manipulator_environment.json"));
}

TEST_CASE("conditions.frontmost_application") {
  actual_examples_helper helper("frontmost_application.json");
  krbn::manipulator::manipulator_environment manipulator_environment;
  krbn::event_queue::entry entry(krbn::device_id(1),
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::a));

  // bundle_identifiers matching
  {
    pqrs::osx::frontmost_application_monitor::application application;
    application.set_bundle_identifier("com.apple.Terminal");
    application.set_file_path("/not_found");
    manipulator_environment.set_frontmost_application(application);
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
    // use cache
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }

  // Test regex escape works properly
  {
    pqrs::osx::frontmost_application_monitor::application application;
    application.set_bundle_identifier("com/apple/Terminal");
    application.set_file_path("/not_found");
    manipulator_environment.set_frontmost_application(application);

    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);
    // use cache
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);
  }

  // file_path matching
  {
    pqrs::osx::frontmost_application_monitor::application application;
    application.set_bundle_identifier("com/apple/Terminal");
    application.set_file_path("/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal");
    manipulator_environment.set_frontmost_application(application);

    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }

  // frontmost_application_unless
  {
    pqrs::osx::frontmost_application_monitor::application application;
    application.set_bundle_identifier("com.googlecode.iterm2");
    application.set_file_path("/Applications/iTerm.app");
    manipulator_environment.set_frontmost_application(application);

    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }

  {
    pqrs::osx::frontmost_application_monitor::application application;
    application.set_bundle_identifier("com.googlecode.iterm2");
    application.set_file_path("/Users/tekezo/Applications/iTerm.app");
    manipulator_environment.set_frontmost_application(application);

    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);
  }
}

TEST_CASE("conditions.input_source") {
  actual_examples_helper helper("input_source.json");
  krbn::manipulator::manipulator_environment manipulator_environment;
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
    manipulator_environment.set_input_source_properties(properties);
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
    manipulator_environment.set_input_source_properties(properties);
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
    manipulator_environment.set_input_source_properties(properties);
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }

  // input_mode_id matching
  {
    pqrs::osx::input_source::properties properties;
    properties.set_first_language("ja");
    properties.set_input_source_id("com.apple.keylayout.Australian");
    properties.set_input_mode_id("com.apple.inputmethod.Japanese.FullWidthRoman");
    manipulator_environment.set_input_source_properties(properties);
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }

  // input_source_unless
  {
    pqrs::osx::input_source::properties properties;
    properties.set_first_language("fr");
    properties.set_input_source_id("com.apple.keylayout.US");
    manipulator_environment.set_input_source_properties(properties);
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);
  }
}

TEST_CASE("conditions.device") {
  krbn::manipulator::manipulator_environment manipulator_environment;

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

    REQUIRE(helper.get_error_messages() ==
            std::vector<std::string>({
                "`vendor_id` must be specified: `{\"description\":\"condition is ignored if error\"}`",
            }));

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

    REQUIRE(helper.get_error_messages() ==
            std::vector<std::string>({
                "`vendor_id` must be specified: `{\"description\":\"condition is ignored if error\"}`",
            }));

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
    krbn::manipulator::conditions::device condition(json);

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
    krbn::manipulator::conditions::device condition(json);

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
  krbn::manipulator::manipulator_environment manipulator_environment;
  krbn::event_queue::entry entry(krbn::device_id(1),
                                 krbn::event_queue::event_time_stamp(krbn::absolute_time_point(0)),
                                 krbn::event_queue::event(krbn::key_code::a),
                                 krbn::event_type::key_down,
                                 krbn::event_queue::event(krbn::key_code::a));

  pqrs::osx::system_preferences::properties system_preferences_properties;
  system_preferences_properties.set_use_fkeys_as_standard_function_keys(true);
  system_preferences_properties.set_scroll_direction_is_natural(false);
  system_preferences_properties.set_keyboard_types(
      std::map<pqrs::osx::system_preferences::keyboard_type_key,
               pqrs::osx::iokit_keyboard_type>({
          {
              pqrs::osx::system_preferences::keyboard_type_key(krbn::vendor_id_karabiner_virtual_hid_device,
                                                               krbn::product_id_karabiner_virtual_hid_keyboard,
                                                               krbn::hid_country_code(0)),
              pqrs::osx::iokit_keyboard_type(40), // ansi
          },
          {
              pqrs::osx::system_preferences::keyboard_type_key(krbn::vendor_id_karabiner_virtual_hid_device,
                                                               krbn::product_id_karabiner_virtual_hid_keyboard,
                                                               krbn::hid_country_code(1)),
              pqrs::osx::iokit_keyboard_type(41), // iso
          },
      }));
  manipulator_environment.set_system_preferences_properties(system_preferences_properties);

  {
    actual_examples_helper helper("keyboard_type_if.json");

    // iso
    manipulator_environment.set_virtual_hid_keyboard_country_code(krbn::hid_country_code(1));
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);

    // ansi
    manipulator_environment.set_virtual_hid_keyboard_country_code(krbn::hid_country_code(0));
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);
  }
  {
    actual_examples_helper helper("keyboard_type_unless.json");

    // iso
    manipulator_environment.set_virtual_hid_keyboard_country_code(krbn::hid_country_code(1));
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == false);

    // ansi
    manipulator_environment.set_virtual_hid_keyboard_country_code(krbn::hid_country_code(0));
    REQUIRE(helper.get_condition_manager().is_fulfilled(entry,
                                                        manipulator_environment) == true);
  }
}
