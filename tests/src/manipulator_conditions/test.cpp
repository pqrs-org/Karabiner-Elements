#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "manipulator/condition_manager.hpp"
#include "manipulator/manipulator_factory.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
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
            "bundle_identifiers", {
                                      "^com\\.apple\\.Terminal$", "^com\\.googlecode\\.iterm2$",
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
            "bundle_identifiers", {
                                      "^com\\.apple\\.Terminal$", "^com\\.googlecode\\.iterm2$", "broken([regex",
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
  manipulator_environment.set_input_source_identifiers({std::string("en"),
                                                        std::string("com.apple.keylayout.US"),
                                                        boost::none});
  manipulator_environment.set_variable("value1", 100);
  manipulator_environment.set_variable("value2", 200);
  manipulator_environment.set_keyboard_type("iso");
}

TEST_CASE("conditions.frontmost_application") {
  actual_examples_helper helper("frontmost_application.json");
  krbn::manipulator_environment manipulator_environment;
  krbn::event_queue::queued_event queued_event(krbn::device_id(1),
                                               0,
                                               krbn::event_queue::queued_event::event(krbn::key_code::a),
                                               krbn::event_type::key_down,
                                               krbn::event_queue::queued_event::event(krbn::key_code::a));

  // bundle_identifiers matching
  manipulator_environment.set_frontmost_application({"com.apple.Terminal",
                                                     "/not_found"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == true);
  // use cache
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == true);

  // Test regex escape works properly
  manipulator_environment.set_frontmost_application({"com/apple/Terminal",
                                                     "/not_found"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == false);
  // use cache
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == false);

  // file_path matching
  manipulator_environment.set_frontmost_application({"com/apple/Terminal",
                                                     "/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == true);

  // frontmost_application_unless
  manipulator_environment.set_frontmost_application({"com.googlecode.iterm2",
                                                     "/Applications/iTerm.app"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == true);
  manipulator_environment.set_frontmost_application({"com.googlecode.iterm2",
                                                     "/Users/tekezo/Applications/iTerm.app"});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == false);
}

TEST_CASE("conditions.input_source") {
  actual_examples_helper helper("input_source.json");
  krbn::manipulator_environment manipulator_environment;
  krbn::event_queue::queued_event queued_event(krbn::device_id(1),
                                               0,
                                               krbn::event_queue::queued_event::event(krbn::key_code::a),
                                               krbn::event_type::key_down,
                                               krbn::event_queue::queued_event::event(krbn::key_code::a));

  // language matching
  manipulator_environment.set_input_source_identifiers({std::string("en"),
                                                        std::string("com.apple.keylayout.Australian"),
                                                        boost::none});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == true);
  // use cache
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == true);

  // Test regex escape works properly
  manipulator_environment.set_input_source_identifiers({std::string("ja"),
                                                        std::string("com/apple/keylayout/Australian"),
                                                        boost::none});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == false);
  // use cache
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == false);

  // input_source_id matching
  manipulator_environment.set_input_source_identifiers({std::string("ja"),
                                                        std::string("com.apple.keylayout.US"),
                                                        boost::none});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == true);

  // input_mode_id matching
  manipulator_environment.set_input_source_identifiers({std::string("ja"),
                                                        std::string("com.apple.keylayout.Australian"),
                                                        std::string("com.apple.inputmethod.Japanese.FullWidthRoman")});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == true);

  // input_source_unless
  manipulator_environment.set_input_source_identifiers({std::string("fr"),
                                                        std::string("com.apple.keylayout.US"),
                                                        boost::none});
  REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                      manipulator_environment) == false);
}

TEST_CASE("conditions.device") {
  krbn::manipulator_environment manipulator_environment;
  auto device_id_8888_9999 = krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
      {"vendor_id", 8888},
      {"product_id", 9999},
      {"is_keyboard", true},
      {"is_pointing_device", false},
  })));
  auto device_id_1000_2000 = krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
      {"vendor_id", 1000},
      {"product_id", 2000},
      {"is_keyboard", true},
      {"is_pointing_device", false},
  })));
  auto device_id_1000_2001 = krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
      {"vendor_id", 1000},
      {"product_id", 2001},
      {"is_keyboard", true},
      {"is_pointing_device", false},
  })));
  auto device_id_1001_2000 = krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
      {"vendor_id", 1001},
      {"product_id", 2000},
      {"is_keyboard", true},
      {"is_pointing_device", false},
  })));
  auto device_id_1001_2001 = krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
      {"vendor_id", 1001},
      {"product_id", 2001},
      {"is_keyboard", true},
      {"is_pointing_device", false},
  })));
  auto device_id_1099_9999 = krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
      {"vendor_id", 1099},
      {"product_id", 9999},
      {"is_keyboard", true},
      {"is_pointing_device", false},
  })));

#define QUEUED_EVENT(DEVICE_ID)                                                              \
  krbn::event_queue::queued_event(DEVICE_ID,                                                 \
                                  0,                                                         \
                                  krbn::event_queue::queued_event::event(krbn::key_code::a), \
                                  krbn::event_type::key_down,                                \
                                  krbn::event_queue::queued_event::event(krbn::key_code::a))

  {
    actual_examples_helper helper("device_if.json");
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_8888_9999),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1000_2000),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1000_2001),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1001_2000),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1001_2001),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1099_9999),
                                                        manipulator_environment) == true);
  }
  {
    actual_examples_helper helper("device_unless.json");
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_8888_9999),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1000_2000),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1000_2001),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1001_2000),
                                                        manipulator_environment) == true);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1001_2001),
                                                        manipulator_environment) == false);
    REQUIRE(helper.get_condition_manager().is_fulfilled(QUEUED_EVENT(device_id_1099_9999),
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

    REQUIRE(condition.is_fulfilled(QUEUED_EVENT(device_id_1000_2000),
                                   manipulator_environment) == true);
    REQUIRE(condition.is_fulfilled(QUEUED_EVENT(device_id_1000_2001),
                                   manipulator_environment) == false);

    REQUIRE(condition.is_fulfilled(QUEUED_EVENT(krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
                                       {"vendor_id", 1000},
                                       {"product_id", 2000},
                                       {"is_keyboard", true},
                                       {"is_pointing_device", false},
                                   })))),
                                   manipulator_environment) == true);
    REQUIRE(condition.is_fulfilled(QUEUED_EVENT(krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
                                       {"vendor_id", 1000},
                                       {"product_id", 2000},
                                       {"is_keyboard", false},
                                       {"is_pointing_device", false},
                                   })))),
                                   manipulator_environment) == false);
    REQUIRE(condition.is_fulfilled(QUEUED_EVENT(krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
                                       {"vendor_id", 1000},
                                       {"product_id", 2000},
                                       {"is_keyboard", true},
                                       {"is_pointing_device", true},
                                   })))),
                                   manipulator_environment) == false);
    REQUIRE(condition.is_fulfilled(QUEUED_EVENT(krbn::types::make_new_device_id(std::make_shared<krbn::device_detail>(nlohmann::json({
                                       {"vendor_id", 1000},
                                       {"product_id", 2000},
                                       {"is_keyboard", false},
                                       {"is_pointing_device", true},
                                   })))),
                                   manipulator_environment) == false);
  }

#undef QUEUED_EVENT
}

TEST_CASE("conditions.keyboard_type") {
  krbn::manipulator_environment manipulator_environment;
  krbn::event_queue::queued_event queued_event(krbn::device_id(1),
                                               0,
                                               krbn::event_queue::queued_event::event(krbn::key_code::a),
                                               krbn::event_type::key_down,
                                               krbn::event_queue::queued_event::event(krbn::key_code::a));

  {
    actual_examples_helper helper("keyboard_type_if.json");

    manipulator_environment.set_keyboard_type("iso");
    REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                        manipulator_environment) == true);

    manipulator_environment.set_keyboard_type("ansi");
    REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                        manipulator_environment) == false);
  }
  {
    actual_examples_helper helper("keyboard_type_unless.json");

    manipulator_environment.set_keyboard_type("iso");
    REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                        manipulator_environment) == false);

    manipulator_environment.set_keyboard_type("ansi");
    REQUIRE(helper.get_condition_manager().is_fulfilled(queued_event,
                                                        manipulator_environment) == true);
  }
}
