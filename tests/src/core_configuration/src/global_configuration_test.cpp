#include <catch2/catch.hpp>

#include "core_configuration/core_configuration.hpp"

TEST_CASE("global_configuration") {
  // empty json
  {
    nlohmann::json json;
    krbn::core_configuration::details::global_configuration global_configuration(json);
    REQUIRE(global_configuration.get_check_for_updates_on_startup() == true);
    REQUIRE(global_configuration.get_show_in_menu_bar() == true);
    REQUIRE(global_configuration.get_show_profile_name_in_menu_bar() == false);
  }

  // load values from json
  {
    nlohmann::json json{
        {"check_for_updates_on_startup", false},
        {"show_in_menu_bar", false},
        {"show_profile_name_in_menu_bar", true},
    };
    krbn::core_configuration::details::global_configuration global_configuration(json);
    REQUIRE(global_configuration.get_check_for_updates_on_startup() == false);
    REQUIRE(global_configuration.get_show_in_menu_bar() == false);
    REQUIRE(global_configuration.get_show_profile_name_in_menu_bar() == true);
  }

  // invalid values in json
  {
    nlohmann::json json{
        {"check_for_updates_on_startup", nlohmann::json::array()},
        {"show_in_menu_bar", 0},
        {"show_profile_name_in_menu_bar", nlohmann::json::object()},
    };
    krbn::core_configuration::details::global_configuration global_configuration(json);
    REQUIRE(global_configuration.get_check_for_updates_on_startup() == true);
    REQUIRE(global_configuration.get_show_in_menu_bar() == true);
    REQUIRE(global_configuration.get_show_profile_name_in_menu_bar() == false);
  }
}
