#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "core_configuration.hpp"
#include "thread_utility.hpp"
#include <iostream>
#include <spdlog/spdlog.h>

class logger final {
public:
  static spdlog::logger& get_logger(void) {
    static std::shared_ptr<spdlog::logger> logger;
    if (!logger) {
      logger = spdlog::stdout_logger_mt("core_configuration", true);
    }
    return *logger;
  }
};

TEST_CASE("valid") {
  core_configuration configuration(logger::get_logger(), "json/example.json");

  {
    std::vector<std::pair<krbn::key_code, krbn::key_code>> expected{
        std::make_pair(krbn::key_code(57), krbn::key_code(42)),
        std::make_pair(krbn::key_code(41), krbn::key_code(44)),
    };
    REQUIRE(configuration.get_current_profile_simple_modifications() == expected);
  }
  {
    std::vector<std::pair<krbn::key_code, krbn::key_code>> expected{
        std::make_pair(krbn::key_code::f1, krbn::key_code::display_brightness_decrement),
        std::make_pair(krbn::key_code::f10, krbn::key_code::mute),
        std::make_pair(krbn::key_code::f11, krbn::key_code::volume_decrement),
        std::make_pair(krbn::key_code::f12, krbn::key_code::volume_increment),
        std::make_pair(krbn::key_code::f2, krbn::key_code::display_brightness_increment),
        std::make_pair(krbn::key_code::f3, krbn::key_code::mission_control),
        std::make_pair(krbn::key_code::f4, krbn::key_code::launchpad),
        std::make_pair(krbn::key_code::f5, krbn::key_code::illumination_decrement),
        std::make_pair(krbn::key_code::f6, krbn::key_code::illumination_increment),
        std::make_pair(krbn::key_code::f7, krbn::key_code::rewind),
        std::make_pair(krbn::key_code::f8, krbn::key_code::play_or_pause),
        std::make_pair(krbn::key_code::f9, krbn::key_code::fastforward),
    };
    REQUIRE(configuration.get_current_profile_fn_function_keys() == expected);
  }
  {
    auto actual = configuration.get_current_profile_virtual_hid_keyboard();
    REQUIRE(actual.keyboard_type == krbn::keyboard_type::iso);
  }
  {
    auto actual = configuration.get_current_profile_devices();
    REQUIRE(actual.size() == 2);

    REQUIRE(actual[0].first.vendor_id == krbn::vendor_id(1133));
    REQUIRE(actual[0].first.product_id == krbn::product_id(50475));
    REQUIRE(actual[0].first.is_keyboard == true);
    REQUIRE(actual[0].first.is_pointing_device == false);
    REQUIRE(actual[0].second.ignore == false);
    REQUIRE(actual[0].second.disable_built_in_keyboard_if_exists == false);

    REQUIRE(actual[1].first.vendor_id == krbn::vendor_id(1452));
    REQUIRE(actual[1].first.product_id == krbn::product_id(610));
    REQUIRE(actual[1].first.is_keyboard == true);
    REQUIRE(actual[1].first.is_pointing_device == false);
    REQUIRE(actual[1].second.ignore == true);
    REQUIRE(actual[1].second.disable_built_in_keyboard_if_exists == true);
  }

  REQUIRE(configuration.get_global_check_for_updates_on_startup() == false);
  REQUIRE(configuration.get_global_show_in_menu_bar() == false);

  REQUIRE(configuration.is_loaded() == true);
}

TEST_CASE("broken.json") {
  core_configuration configuration(logger::get_logger(), "json/broken.json");

  std::vector<std::pair<krbn::key_code, krbn::key_code>> expected;
  REQUIRE(configuration.get_current_profile_simple_modifications() == expected);
  REQUIRE(configuration.is_loaded() == false);
}

TEST_CASE("invalid_key_code_name.json") {
  core_configuration configuration(logger::get_logger(), "json/invalid_key_code_name.json");

  std::vector<std::pair<krbn::key_code, krbn::key_code>> expected{
      std::make_pair(krbn::key_code(41), krbn::key_code(44)),
  };
  REQUIRE(configuration.get_current_profile_simple_modifications() == expected);
  REQUIRE(configuration.is_loaded() == true);
}

TEST_CASE("global_configuration") {
  // empty json
  {
    nlohmann::json json;
    core_configuration::global_configuration global_configuration(json);
    REQUIRE(global_configuration.get_check_for_updates_on_startup() == true);
    REQUIRE(global_configuration.get_show_in_menu_bar() == true);
    REQUIRE(global_configuration.get_show_profile_name_in_menu_bar() == false);
  }

  // load values from json
  {
    nlohmann::json json = {
        {"check_for_updates_on_startup", false},
        {"show_in_menu_bar", false},
        {"show_profile_name_in_menu_bar", true},
    };
    core_configuration::global_configuration global_configuration(json);
    REQUIRE(global_configuration.get_check_for_updates_on_startup() == false);
    REQUIRE(global_configuration.get_show_in_menu_bar() == false);
    REQUIRE(global_configuration.get_show_profile_name_in_menu_bar() == true);
  }

  // invalid values in json
  {
    nlohmann::json json = {
        {"check_for_updates_on_startup", nlohmann::json::array()},
        {"show_in_menu_bar", 0},
        {"show_profile_name_in_menu_bar", nlohmann::json::object()},
    };
    core_configuration::global_configuration global_configuration(json);
    REQUIRE(global_configuration.get_check_for_updates_on_startup() == true);
    REQUIRE(global_configuration.get_show_in_menu_bar() == true);
    REQUIRE(global_configuration.get_show_profile_name_in_menu_bar() == false);
  }
}

TEST_CASE("global_configuration.to_json") {
  {
    nlohmann::json json;
    core_configuration::global_configuration global_configuration(json);
    nlohmann::json expected({
        {"check_for_updates_on_startup", true},
        {"show_in_menu_bar", true},
        {"show_profile_name_in_menu_bar", false},
    });
    REQUIRE(global_configuration.to_json() == expected);
  }
  {
    nlohmann::json json({
        {"dummy", {{"keep_me", true}}},
    });
    core_configuration::global_configuration global_configuration(json);
    global_configuration.set_check_for_updates_on_startup(false);
    global_configuration.set_show_in_menu_bar(false);
    global_configuration.set_show_profile_name_in_menu_bar(true);
    nlohmann::json expected({
        {"check_for_updates_on_startup", false},
        {"dummy", {{"keep_me", true}}},
        {"show_in_menu_bar", false},
        {"show_profile_name_in_menu_bar", true},
    });
    REQUIRE(global_configuration.to_json() == expected);
  }
}

int main(int argc, char* const argv[]) {
  thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
