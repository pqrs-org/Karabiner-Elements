#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "core_configuration.hpp"
#include "thread_utility.hpp"
#include <iostream>

TEST_CASE("valid") {
  krbn::core_configuration configuration("json/example.json");

  {
    std::vector<std::pair<std::string, std::string>> expected{
        {"caps_lock", "delete_or_backspace"},
        {"escape", "spacebar"},
    };
    REQUIRE(configuration.get_selected_profile().get_simple_modifications().get_pairs() == expected);
  }
  {
    auto manipulator = configuration.get_selected_profile().get_complex_modifications().get_rules()[0].get_manipulators()[0].get_json();
    REQUIRE(manipulator["type"] == "basic");
    REQUIRE(manipulator["from"]["key_code"] == "open_bracket");
  }
  {
    std::vector<std::pair<std::string, std::string>> expected{
        {"f1", "display_brightness_decrement"},
        {"f2", "display_brightness_increment"},
        {"f3", "mission_control"},
        {"f4", "launchpad"},
        {"f5", "illumination_decrement"},
        {"f6", "illumination_increment"},
        {"f7", "rewind"},
        {"f8", "play_or_pause"},
        {"f9", "fastforward"},
        {"f10", "mute"},
        {"f11", "volume_decrement"},
        {"f12", "volume_increment"},
    };
    REQUIRE(configuration.get_selected_profile().get_fn_function_keys().get_pairs() == expected);
  }
  {
    auto& complex_modifications = configuration.get_selected_profile().get_complex_modifications();
    auto& rules = complex_modifications.get_rules();
    REQUIRE(complex_modifications.get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 800);
    REQUIRE(rules[0].get_manipulators()[0].get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 800);
    REQUIRE(rules[0].get_manipulators()[2].get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 400);
    REQUIRE(rules[0].get_description() == "Change control+[ to escape.");
    REQUIRE(rules[1].get_description() == "description test");
    REQUIRE(rules[2].get_description() == "");
  }
  {
    REQUIRE(configuration.get_selected_profile().get_virtual_hid_keyboard().get_keyboard_type() == "iso");
    REQUIRE(configuration.get_selected_profile().get_virtual_hid_keyboard().get_caps_lock_delay_milliseconds() == 100);
  }
  {
    auto& actual = configuration.get_selected_profile().get_devices();
    REQUIRE(actual.size() == 2);

    REQUIRE(actual[0].get_identifiers().get_vendor_id() == krbn::vendor_id(1133));
    REQUIRE(actual[0].get_identifiers().get_product_id() == krbn::product_id(50475));
    REQUIRE(actual[0].get_identifiers().get_is_keyboard() == true);
    REQUIRE(actual[0].get_identifiers().get_is_pointing_device() == false);
    REQUIRE(actual[0].get_ignore() == false);
    REQUIRE(actual[0].get_disable_built_in_keyboard_if_exists() == false);

    REQUIRE(actual[1].get_identifiers().get_vendor_id() == krbn::vendor_id(1452));
    REQUIRE(actual[1].get_identifiers().get_product_id() == krbn::product_id(610));
    REQUIRE(actual[1].get_identifiers().get_is_keyboard() == true);
    REQUIRE(actual[1].get_identifiers().get_is_pointing_device() == false);
    REQUIRE(actual[1].get_ignore() == true);
    REQUIRE(actual[1].get_disable_built_in_keyboard_if_exists() == true);
  }

  REQUIRE(configuration.get_global_configuration().get_check_for_updates_on_startup() == false);
  REQUIRE(configuration.get_global_configuration().get_show_in_menu_bar() == false);

  REQUIRE(configuration.is_loaded() == true);

  {
    std::ifstream input("json/to_json_example.json");
    auto expected = nlohmann::json::parse(input);
    REQUIRE(configuration.to_json() == expected);
  }
}

TEST_CASE("broken.json") {
  {
    krbn::core_configuration configuration("json/broken.json");

    REQUIRE(configuration.get_selected_profile().get_simple_modifications().get_pairs().empty());
    REQUIRE(configuration.is_loaded() == false);

    REQUIRE(configuration.get_global_configuration().get_check_for_updates_on_startup() == true);
    REQUIRE(configuration.get_global_configuration().get_show_in_menu_bar() == true);
    REQUIRE(configuration.get_global_configuration().get_show_profile_name_in_menu_bar() == false);
    REQUIRE(configuration.get_profiles().size() == 1);
    REQUIRE((configuration.get_profiles())[0].get_name() == "Default profile");
    REQUIRE((configuration.get_profiles())[0].get_selected() == true);
    REQUIRE((configuration.get_profiles())[0].get_fn_function_keys().get_pairs().size() == 12);

    {
      // to_json result is default json if is_loaded == false
      std::ifstream input("json/to_json_default.json");
      auto expected = nlohmann::json::parse(input);
      REQUIRE(configuration.to_json() == expected);
    }
  }
  {
    krbn::core_configuration configuration("a.out");

    REQUIRE(configuration.get_selected_profile().get_simple_modifications().get_pairs().empty());
    REQUIRE(configuration.is_loaded() == false);
  }
}

TEST_CASE("invalid_key_code_name.json") {
  krbn::core_configuration configuration("json/invalid_key_code_name.json");

  std::vector<std::pair<std::string, std::string>> expected{
      {"caps_lock_2", "delete_or_backspace"},
      {"escape", "spacebar"},
  };
  REQUIRE(configuration.get_selected_profile().get_simple_modifications().get_pairs() == expected);
  REQUIRE(configuration.is_loaded() == true);
}

TEST_CASE("global_configuration") {
  // empty json
  {
    nlohmann::json json;
    krbn::core_configuration::global_configuration global_configuration(json);
    REQUIRE(global_configuration.get_check_for_updates_on_startup() == true);
    REQUIRE(global_configuration.get_show_in_menu_bar() == true);
    REQUIRE(global_configuration.get_show_profile_name_in_menu_bar() == false);
  }

  // load values from json
  {
    nlohmann::json json({
        {"check_for_updates_on_startup", false},
        {"show_in_menu_bar", false},
        {"show_profile_name_in_menu_bar", true},
    });
    krbn::core_configuration::global_configuration global_configuration(json);
    REQUIRE(global_configuration.get_check_for_updates_on_startup() == false);
    REQUIRE(global_configuration.get_show_in_menu_bar() == false);
    REQUIRE(global_configuration.get_show_profile_name_in_menu_bar() == true);
  }

  // invalid values in json
  {
    nlohmann::json json({
        {"check_for_updates_on_startup", nlohmann::json::array()},
        {"show_in_menu_bar", 0},
        {"show_profile_name_in_menu_bar", nlohmann::json::object()},
    });
    krbn::core_configuration::global_configuration global_configuration(json);
    REQUIRE(global_configuration.get_check_for_updates_on_startup() == true);
    REQUIRE(global_configuration.get_show_in_menu_bar() == true);
    REQUIRE(global_configuration.get_show_profile_name_in_menu_bar() == false);
  }
}

TEST_CASE("global_configuration.to_json") {
  {
    nlohmann::json json;
    krbn::core_configuration::global_configuration global_configuration(json);
    nlohmann::json expected({
        {"check_for_updates_on_startup", true},
        {"show_in_menu_bar", true},
        {"show_profile_name_in_menu_bar", false},
    });
    REQUIRE(global_configuration.to_json() == expected);

    nlohmann::json actual = global_configuration;
    REQUIRE(actual == expected);
  }
  {
    nlohmann::json json({
        {"dummy", {{"keep_me", true}}},
    });
    krbn::core_configuration::global_configuration global_configuration(json);
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

namespace {
nlohmann::json get_default_fn_function_keys_json(void) {
  return nlohmann::json({
      {"f1", "display_brightness_decrement"},
      {"f10", "mute"},
      {"f11", "volume_decrement"},
      {"f12", "volume_increment"},
      {"f2", "display_brightness_increment"},
      {"f3", "mission_control"},
      {"f4", "launchpad"},
      {"f5", "illumination_decrement"},
      {"f6", "illumination_increment"},
      {"f7", "rewind"},
      {"f8", "play_or_pause"},
      {"f9", "fastforward"},
  });
}

nlohmann::json get_default_virtual_hid_keyboard_json(void) {
  return nlohmann::json({
      {"caps_lock_delay_milliseconds", 0},
      {"keyboard_type", "ansi"},
  });
}

std::vector<std::pair<std::string, std::string>> get_default_fn_function_keys_pairs(void) {
  return std::vector<std::pair<std::string, std::string>>({
      {"f1", "display_brightness_decrement"},
      {"f2", "display_brightness_increment"},
      {"f3", "mission_control"},
      {"f4", "launchpad"},
      {"f5", "illumination_decrement"},
      {"f6", "illumination_increment"},
      {"f7", "rewind"},
      {"f8", "play_or_pause"},
      {"f9", "fastforward"},
      {"f10", "mute"},
      {"f11", "volume_decrement"},
      {"f12", "volume_increment"},
  });
}
} // namespace

TEST_CASE("profile") {
  // empty json
  {
    nlohmann::json json;
    krbn::core_configuration::profile profile(json);
    REQUIRE(profile.get_name() == std::string(""));
    REQUIRE(profile.get_selected() == false);
    REQUIRE(profile.get_simple_modifications().get_pairs().size() == 0);
    REQUIRE(profile.get_fn_function_keys().get_pairs() == get_default_fn_function_keys_pairs());
    REQUIRE(profile.get_devices().size() == 0);
  }

  // load values from json
  {
    nlohmann::json json({
        {"name", "profile 1"},
        {"selected", true},
        {"simple_modifications", {
                                     {
                                         "from 1", "to 1",
                                     },
                                     {
                                         "from 3", "to 3",
                                     },
                                     {
                                         "from 2", "to 2",
                                     },
                                     {
                                         "from 10", "to 10",
                                     },
                                 }},
        {"fn_function_keys", {
                                 {
                                     "f3", "to f3",
                                 },
                                 {
                                     "f4", "to f4",
                                 },
                                 {
                                     "f13", "to f13",
                                 },
                             }},
        {"devices", {
                        {
                            {"identifiers", {
                                                {
                                                    "vendor_id", 1234,
                                                },
                                                {
                                                    "product_id", 5678,
                                                },
                                                {
                                                    "is_keyboard", true,
                                                },
                                                {
                                                    "is_pointing_device", true,
                                                },
                                            }},
                            {"ignore", true},
                            {"disable_built_in_keyboard_if_exists", true},
                        },
                        // duplicated identifiers
                        {
                            {"identifiers", {
                                                {
                                                    "vendor_id", 1234,
                                                },
                                                {
                                                    "product_id", 5678,
                                                },
                                                {
                                                    "is_keyboard", true,
                                                },
                                                {
                                                    "is_pointing_device", true,
                                                },
                                            }},
                            {"ignore", true},
                            {"disable_built_in_keyboard_if_exists", true},
                        },
                        {
                            {"identifiers", {
                                                {
                                                    "vendor_id", 4321,
                                                },
                                                {
                                                    "product_id", 8765,
                                                },
                                                {
                                                    "is_keyboard", true,
                                                },
                                                {
                                                    "is_pointing_device", false,
                                                },
                                            }},
                            {"ignore", false},
                            {"disable_built_in_keyboard_if_exists", true},
                        },
                    }},
    });
    krbn::core_configuration::profile profile(json);
    REQUIRE(profile.get_name() == std::string("profile 1"));
    REQUIRE(profile.get_selected() == true);
    {
      std::vector<std::pair<std::string, std::string>> expected({
          {"from 1", "to 1"},
          {"from 2", "to 2"},
          {"from 3", "to 3"},
          {"from 10", "to 10"},
      });
      REQUIRE(profile.get_simple_modifications().get_pairs() == expected);
    }
    {
      auto expected = get_default_fn_function_keys_pairs();
      expected[2].second = "to f3";
      expected[3].second = "to f4";
      REQUIRE(profile.get_fn_function_keys().get_pairs() == expected);
    }
    {
      REQUIRE(profile.get_devices().size() == 3);
      REQUIRE((profile.get_devices())[0].get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
      REQUIRE((profile.get_devices())[0].get_identifiers().get_product_id() == krbn::product_id(5678));
      REQUIRE((profile.get_devices())[0].get_ignore() == true);
      REQUIRE((profile.get_devices())[0].get_disable_built_in_keyboard_if_exists() == true);
      REQUIRE((profile.get_devices())[1].get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
      REQUIRE((profile.get_devices())[1].get_identifiers().get_product_id() == krbn::product_id(5678));
      REQUIRE((profile.get_devices())[1].get_ignore() == true);
      REQUIRE((profile.get_devices())[1].get_disable_built_in_keyboard_if_exists() == true);
      REQUIRE((profile.get_devices())[2].get_identifiers().get_vendor_id() == krbn::vendor_id(4321));
      REQUIRE((profile.get_devices())[2].get_identifiers().get_product_id() == krbn::product_id(8765));
      REQUIRE((profile.get_devices())[2].get_ignore() == false);
      REQUIRE((profile.get_devices())[2].get_disable_built_in_keyboard_if_exists() == true);
    }

    // set_device (existing identifiers)
    {
      auto identifiers = krbn::device_identifiers(nlohmann::json({
          {
              "vendor_id", 1234,
          },
          {
              "product_id", 5678,
          },
          {
              "is_keyboard", true,
          },
          {
              "is_pointing_device", true,
          },
      }));
      profile.set_device_ignore(identifiers, false);
      REQUIRE(profile.get_devices().size() == 3);
      // devices[0] is changed.
      REQUIRE((profile.get_devices())[0].get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
      REQUIRE((profile.get_devices())[0].get_identifiers().get_product_id() == krbn::product_id(5678));
      REQUIRE((profile.get_devices())[0].get_ignore() == false);
      REQUIRE((profile.get_devices())[0].get_disable_built_in_keyboard_if_exists() == true);
      // devices[1] is not changed.
      REQUIRE((profile.get_devices())[1].get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
      REQUIRE((profile.get_devices())[1].get_identifiers().get_product_id() == krbn::product_id(5678));
      REQUIRE((profile.get_devices())[1].get_ignore() == true);
      REQUIRE((profile.get_devices())[1].get_disable_built_in_keyboard_if_exists() == true);
      REQUIRE((profile.get_devices())[2].get_identifiers().get_vendor_id() == krbn::vendor_id(4321));
      REQUIRE((profile.get_devices())[2].get_identifiers().get_product_id() == krbn::product_id(8765));
      REQUIRE((profile.get_devices())[2].get_ignore() == false);
      REQUIRE((profile.get_devices())[2].get_disable_built_in_keyboard_if_exists() == true);

      profile.set_device_disable_built_in_keyboard_if_exists(identifiers, false);
      REQUIRE(profile.get_devices().size() == 3);
      // devices[0] is changed.
      REQUIRE((profile.get_devices())[0].get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
      REQUIRE((profile.get_devices())[0].get_identifiers().get_product_id() == krbn::product_id(5678));
      REQUIRE((profile.get_devices())[0].get_ignore() == false);
      REQUIRE((profile.get_devices())[0].get_disable_built_in_keyboard_if_exists() == false);
      // devices[1] is not changed.
      REQUIRE((profile.get_devices())[1].get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
      REQUIRE((profile.get_devices())[1].get_identifiers().get_product_id() == krbn::product_id(5678));
      REQUIRE((profile.get_devices())[1].get_ignore() == true);
      REQUIRE((profile.get_devices())[1].get_disable_built_in_keyboard_if_exists() == true);
      REQUIRE((profile.get_devices())[2].get_identifiers().get_vendor_id() == krbn::vendor_id(4321));
      REQUIRE((profile.get_devices())[2].get_identifiers().get_product_id() == krbn::product_id(8765));
      REQUIRE((profile.get_devices())[2].get_ignore() == false);
      REQUIRE((profile.get_devices())[2].get_disable_built_in_keyboard_if_exists() == true);
    }
    // set_device (new identifiers)
    {
      {
        auto identifiers = krbn::device_identifiers(nlohmann::json({
            {
                "vendor_id", 1111,
            },
            {
                "product_id", 2222,
            },
            {
                "is_keyboard", false,
            },
            {
                "is_pointing_device", true,
            },
        }));
        profile.set_device_ignore(identifiers, true);
        REQUIRE(profile.get_devices().size() == 4);
        REQUIRE((profile.get_devices())[3].get_identifiers().get_vendor_id() == krbn::vendor_id(1111));
        REQUIRE((profile.get_devices())[3].get_identifiers().get_product_id() == krbn::product_id(2222));
        REQUIRE((profile.get_devices())[3].get_identifiers().get_is_keyboard() == false);
        REQUIRE((profile.get_devices())[3].get_identifiers().get_is_pointing_device() == true);
        REQUIRE((profile.get_devices())[3].get_ignore() == true);
        REQUIRE((profile.get_devices())[3].get_disable_built_in_keyboard_if_exists() == false);
      }

      {
        krbn::device_identifiers identifiers(krbn::vendor_id(1112),
                                             krbn::product_id(2222),
                                             false,
                                             true);
        profile.set_device_disable_built_in_keyboard_if_exists(identifiers, true);
        REQUIRE(profile.get_devices().size() == 5);
        REQUIRE((profile.get_devices())[4].get_identifiers().get_vendor_id() == krbn::vendor_id(1112));
        REQUIRE((profile.get_devices())[4].get_identifiers().get_product_id() == krbn::product_id(2222));
        REQUIRE((profile.get_devices())[4].get_identifiers().get_is_keyboard() == false);
        REQUIRE((profile.get_devices())[4].get_identifiers().get_is_pointing_device() == true);
        REQUIRE((profile.get_devices())[4].get_ignore() == false);
        REQUIRE((profile.get_devices())[4].get_disable_built_in_keyboard_if_exists() == true);
      }
    }
  }

  // invalid values in json
  {
    nlohmann::json json({
        {"name", nlohmann::json::array()},
        {"selected", 0},
        {"simple_modifications", ""},
        {"fn_function_keys", nlohmann::json::array()},
        {"devices", ""},
    });
    krbn::core_configuration::profile profile(json);
    REQUIRE(profile.get_name() == std::string(""));
    REQUIRE(profile.get_selected() == false);
    REQUIRE(profile.get_simple_modifications().get_pairs().size() == 0);
    REQUIRE(profile.get_fn_function_keys().get_pairs() == get_default_fn_function_keys_pairs());
  }
  {
    nlohmann::json json({
        {"simple_modifications", {
                                     {
                                         "number", 0,
                                     },
                                     {
                                         "object", nlohmann::json::object(),
                                     },
                                     {
                                         "array", nlohmann::json::array(),
                                     },
                                     {
                                         "key", "value",
                                     },
                                 }},
    });
    krbn::core_configuration::profile profile(json);
    {
      std::vector<std::pair<std::string, std::string>> expected({
          {"key", "value"},
      });
      REQUIRE(profile.get_simple_modifications().get_pairs() == expected);
    }
  }
}

TEST_CASE("profile.to_json") {
  {
    nlohmann::json json;
    krbn::core_configuration::profile profile(json);
    nlohmann::json expected({
        {"complex_modifications", nlohmann::json::object({
                                      {"rules", nlohmann::json::array()},
                                      {"parameters", nlohmann::json::object({{"basic.to_if_alone_timeout_milliseconds", 1000}})},
                                  })},
        {"devices", nlohmann::json::array()},
        {"name", ""},
        {"selected", false},
        {"simple_modifications", nlohmann::json::object()},
        {"fn_function_keys", get_default_fn_function_keys_json()},
        {"virtual_hid_keyboard", get_default_virtual_hid_keyboard_json()},
    });
    REQUIRE(profile.to_json() == expected);
  }
  {
    nlohmann::json json({
        {"dummy", {{"keep_me", true}}},
        {"devices", {
                        {
                            {"identifiers", {
                                                {
                                                    "vendor_id", 1234,
                                                },
                                                {
                                                    "product_id", 5678,
                                                },
                                                {
                                                    "is_keyboard", true,
                                                },
                                                {
                                                    "is_pointing_device", true,
                                                },
                                            }},
                            {"ignore", true},
                            {"disable_built_in_keyboard_if_exists", true},
                        },
                    }},
    });
    krbn::core_configuration::profile profile(json);
    profile.set_name("profile 1");
    profile.set_selected(true);

    profile.get_simple_modifications().push_back_pair();
    // {
    //   "": ""
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(1, "from 1", "to 1");
    // {
    //   "": "",
    //   "from 1": "to 1"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(2, "from 3", "to 3");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(3, "from 4", "to 4");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(4, "from 2", "to 2");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4",
    //   "from 2": "to 2"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(5, "from 2", "to 2.0");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0"
    // }

    profile.get_simple_modifications().erase_pair(2);
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(5, "", "to 0");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0",
    //   "": "to 0"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(6, "from 0", "");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0",
    //   "": "to 0",
    //   "from 0": ""
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(7, "from 5", "to 5");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0",
    //   "": "to 0",
    //   "from 0": "",
    //   "from 5": "to 5"
    // }

    profile.get_fn_function_keys().replace_second("f3", "to f3");
    profile.get_fn_function_keys().replace_second("not found", "do nothing");

    profile.get_virtual_hid_keyboard().set_keyboard_type("iso");

    auto expected_fn_function_keys = get_default_fn_function_keys_json();
    expected_fn_function_keys["f3"] = "to f3";
    auto expected_virtual_hid_keyboard = get_default_virtual_hid_keyboard_json();
    expected_virtual_hid_keyboard["keyboard_type"] = "iso";
    nlohmann::json expected({
        {"complex_modifications", nlohmann::json::object({
                                      {"rules", nlohmann::json::array()},
                                      {"parameters", nlohmann::json::object({{"basic.to_if_alone_timeout_milliseconds", 1000}})},
                                  })},
        {"devices", {
                        {
                            {"identifiers", {
                                                {
                                                    "vendor_id", 1234,
                                                },
                                                {
                                                    "product_id", 5678,
                                                },
                                                {
                                                    "is_keyboard", true,
                                                },
                                                {
                                                    "is_pointing_device", true,
                                                },
                                            }},
                            {"ignore", true},
                            {"disable_built_in_keyboard_if_exists", true},
                            {"simple_modifications", nlohmann::json::object()},
                            {"fn_function_keys", nlohmann::json::object()},
                        },
                    }},
        {"dummy", {{"keep_me", true}}},
        {"name", "profile 1"},
        {"selected", true},
        {"simple_modifications", {
                                     {
                                         "from 1", "to 1",
                                     },
                                     {
                                         "from 2", "to 2",
                                     },
                                     {
                                         "from 4", "to 4",
                                     },
                                     {
                                         "from 5", "to 5",
                                     },
                                 }},
        {"fn_function_keys", expected_fn_function_keys},
        {"virtual_hid_keyboard", expected_virtual_hid_keyboard},
    });
    REQUIRE(profile.to_json() == expected);
  }
}

TEST_CASE("simple_modifications") {
  // empty json
  {
    nlohmann::json json;
    krbn::core_configuration::profile::simple_modifications simple_modifications(json);
    REQUIRE(simple_modifications.get_pairs().size() == 0);
  }

  // load values from json
  {
    nlohmann::json json({
        {"a", "f1"},
        {"b", "f2"},
        {"dummy", "f3"},
        {"f4", "dummy"},
    });
    krbn::core_configuration::profile::simple_modifications simple_modifications(json);
    REQUIRE(simple_modifications.get_pairs().size() == 4);

    {
      std::vector<std::pair<std::string, std::string>> expected({
          {"a", "f1"},
          {"b", "f2"},
          {"dummy", "f3"},
          {"f4", "dummy"},
      });
      REQUIRE(simple_modifications.get_pairs() == expected);
    }
  }

  // invalid values in json
  {
    krbn::core_configuration::profile::simple_modifications simple_modifications(10);
    REQUIRE(simple_modifications.get_pairs().size() == 0);
  }
}

TEST_CASE("simple_modifications.to_json") {
  {
    nlohmann::json json;
    krbn::core_configuration::profile::simple_modifications simple_modifications(json);
    REQUIRE(simple_modifications.to_json() == nlohmann::json::object());
  }
  {
    nlohmann::json json({
        {"a", "f1"},
        {"b", "f2"},
        {"dummy", "f3"},
        {"f4", "dummy"},
    });
    krbn::core_configuration::profile::simple_modifications simple_modifications(json);
    simple_modifications.push_back_pair();
    simple_modifications.replace_pair(4, "a", "f5"); // will be ignored since "a" already exists.
    simple_modifications.push_back_pair();
    simple_modifications.replace_pair(5, "c", "f6");
    REQUIRE(simple_modifications.to_json() == nlohmann::json({
                                                  {"a", "f1"},
                                                  {"b", "f2"},
                                                  {"c", "f6"},
                                                  {"dummy", "f3"},
                                                  {"f4", "dummy"},
                                              }));
  }
}

TEST_CASE("complex_modifications") {
  // empty json
  {
    nlohmann::json json;
    krbn::core_configuration::profile::complex_modifications complex_modifications(json);
    REQUIRE(complex_modifications.get_rules().empty());
    REQUIRE(complex_modifications.get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 1000);
  }

  // load values from json
  {
    nlohmann::json json({
        {
            "rules", {
                         {
                             {"description", "rule 1"},
                         },
                         {
                             {"description", "rule 2"},
                         },
                         {
                             {"description", "rule 3"},
                         },
                     },
        },
    });
    krbn::core_configuration::profile::complex_modifications complex_modifications(json);
    REQUIRE(complex_modifications.get_rules().size() == 3);
  }

  // invalid values in json
  {
    nlohmann::json json({
        {
            "rules", "rule 1",
        },
    });
    krbn::core_configuration::profile::complex_modifications complex_modifications(json);
    REQUIRE(complex_modifications.get_rules().empty());
  }
}

TEST_CASE("complex_modifications.push_back_rule") {
  {
    nlohmann::json json({
        {
            "rules", {
                         {
                             {"description", "rule 1"},
                         },
                         {
                             {"description", "rule 2"},
                         },
                         {
                             {"description", "rule 3"},
                         },
                     },
        },
    });
    krbn::core_configuration::profile::complex_modifications complex_modifications(json);
    auto& rules = complex_modifications.get_rules();
    REQUIRE(rules.size() == 3);
    REQUIRE(rules[0].get_description() == "rule 1");
    REQUIRE(rules[1].get_description() == "rule 2");
    REQUIRE(rules[2].get_description() == "rule 3");

    krbn::core_configuration::profile::complex_modifications::parameters parameters;
    nlohmann::json rule_json;
    rule_json["description"] = "rule 4";
    krbn::core_configuration::profile::complex_modifications::rule rule(rule_json, parameters);

    complex_modifications.push_back_rule(rule);
    REQUIRE(rules.size() == 4);
    REQUIRE(rules[0].get_description() == "rule 1");
    REQUIRE(rules[1].get_description() == "rule 2");
    REQUIRE(rules[2].get_description() == "rule 3");
    REQUIRE(rules[3].get_description() == "rule 4");
  }
}

TEST_CASE("complex_modifications.erase_rule") {
  {
    nlohmann::json json({
        {
            "rules", {
                         {
                             {"description", "rule 1"},
                         },
                         {
                             {"description", "rule 2"},
                         },
                         {
                             {"description", "rule 3"},
                         },
                     },
        },
    });
    krbn::core_configuration::profile::complex_modifications complex_modifications(json);
    auto& rules = complex_modifications.get_rules();
    REQUIRE(rules.size() == 3);
    REQUIRE(rules[0].get_description() == "rule 1");
    REQUIRE(rules[1].get_description() == "rule 2");
    REQUIRE(rules[2].get_description() == "rule 3");

    complex_modifications.erase_rule(0);
    REQUIRE(rules.size() == 2);
    REQUIRE(rules[0].get_description() == "rule 2");
    REQUIRE(rules[1].get_description() == "rule 3");

    complex_modifications.erase_rule(1);
    REQUIRE(rules.size() == 1);
    REQUIRE(rules[0].get_description() == "rule 2");

    complex_modifications.erase_rule(1);
    REQUIRE(rules.size() == 1);
  }
}

TEST_CASE("complex_modifications.swap_rules") {
  {
    nlohmann::json json({
        {
            "rules", {
                         {
                             {"description", "rule 1"},
                         },
                         {
                             {"description", "rule 2"},
                         },
                         {
                             {"description", "rule 3"},
                         },
                     },
        },
    });
    krbn::core_configuration::profile::complex_modifications complex_modifications(json);
    auto& rules = complex_modifications.get_rules();
    REQUIRE(rules.size() == 3);
    REQUIRE(rules[0].get_description() == "rule 1");
    REQUIRE(rules[1].get_description() == "rule 2");
    REQUIRE(rules[2].get_description() == "rule 3");

    complex_modifications.swap_rules(1, 2);
    REQUIRE(rules.size() == 3);
    REQUIRE(rules[0].get_description() == "rule 1");
    REQUIRE(rules[1].get_description() == "rule 3");
    REQUIRE(rules[2].get_description() == "rule 2");
  }
}

TEST_CASE("complex_modifications.parameters") {
  // empty json
  {
    nlohmann::json json;
    krbn::core_configuration::profile::complex_modifications::parameters parameters(json);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1000);
  }

  // load values from json
  {
    nlohmann::json json;
    json["basic.to_if_alone_timeout_milliseconds"] = 1234;
    krbn::core_configuration::profile::complex_modifications::parameters parameters(json);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1234);
  }

  // invalid values in json
  {
    nlohmann::json json;
    json["basic.to_if_alone_timeout_milliseconds"] = "1234";
    krbn::core_configuration::profile::complex_modifications::parameters parameters(json);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1000);
  }
}

TEST_CASE("virtual_hid_keyboard") {
  // empty json
  {
    nlohmann::json json;
    krbn::core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.get_keyboard_type() == std::string("ansi"));
    REQUIRE(virtual_hid_keyboard.get_caps_lock_delay_milliseconds() == 0);
  }

  // load values from json
  {
    nlohmann::json json({
        {"keyboard_type", "iso"},
        {"caps_lock_delay_milliseconds", 300},
    });
    krbn::core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.get_keyboard_type() == std::string("iso"));
    REQUIRE(virtual_hid_keyboard.get_caps_lock_delay_milliseconds() == 300);
  }

  // invalid values in json
  {
    nlohmann::json json({
        {"keyboard_type", nlohmann::json::array()},
        {"caps_lock_delay_milliseconds", nlohmann::json::object()},
    });
    krbn::core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.get_keyboard_type() == std::string("ansi"));
    REQUIRE(virtual_hid_keyboard.get_caps_lock_delay_milliseconds() == 0);
  }
}

TEST_CASE("virtual_hid_keyboard.to_json") {
  {
    nlohmann::json json;
    krbn::core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.to_json() == get_default_virtual_hid_keyboard_json());

    nlohmann::json actual = virtual_hid_keyboard;
    REQUIRE(actual == get_default_virtual_hid_keyboard_json());
  }
  {
    nlohmann::json json({
        {"dummy", {{"keep_me", true}}},
    });
    krbn::core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
    virtual_hid_keyboard.set_caps_lock_delay_milliseconds(200);
    virtual_hid_keyboard.set_keyboard_type("iso");

    nlohmann::json expected({
        {"caps_lock_delay_milliseconds", 200},
        {"dummy", {{"keep_me", true}}},
        {"keyboard_type", "iso"},
    });
    REQUIRE(virtual_hid_keyboard.to_json() == expected);
  }
}

TEST_CASE("device.identifiers") {
  // empty json
  {
    nlohmann::json json;
    krbn::device_identifiers identifiers(json);
    REQUIRE(identifiers.get_vendor_id() == krbn::vendor_id(0));
    REQUIRE(identifiers.get_product_id() == krbn::product_id(0));
    REQUIRE(identifiers.get_is_keyboard() == false);
    REQUIRE(identifiers.get_is_pointing_device() == false);
  }

  // load values from json
  {
    nlohmann::json json({
        {"vendor_id", 1234},
        {"product_id", 5678},
        {"is_keyboard", true},
        {"is_pointing_device", true},
    });
    krbn::device_identifiers identifiers(json);
    REQUIRE(identifiers.get_vendor_id() == krbn::vendor_id(1234));
    REQUIRE(identifiers.get_product_id() == krbn::product_id(5678));
    REQUIRE(identifiers.get_is_keyboard() == true);
    REQUIRE(identifiers.get_is_pointing_device() == true);
  }

  // invalid values in json
  {
    nlohmann::json json({
        {"vendor_id", nlohmann::json::array()},
        {"product_id", true},
        {"is_keyboard", 1},
        {"is_pointing_device", nlohmann::json::array()},
    });
    krbn::device_identifiers identifiers(json);
    REQUIRE(identifiers.get_vendor_id() == krbn::vendor_id(0));
    REQUIRE(identifiers.get_product_id() == krbn::product_id(0));
    REQUIRE(identifiers.get_is_keyboard() == false);
    REQUIRE(identifiers.get_is_pointing_device() == false);
  }

  // construct with vendor_id, product_id, ...
  {
    krbn::device_identifiers identifiers(krbn::vendor_id(1234), krbn::product_id(5678), true, false);
    REQUIRE(identifiers.get_vendor_id() == krbn::vendor_id(1234));
    REQUIRE(identifiers.get_product_id() == krbn::product_id(5678));
    REQUIRE(identifiers.get_is_keyboard() == true);
    REQUIRE(identifiers.get_is_pointing_device() == false);
  }
  {
    krbn::device_identifiers identifiers(krbn::vendor_id(4321), krbn::product_id(8765), false, true);
    REQUIRE(identifiers.get_vendor_id() == krbn::vendor_id(4321));
    REQUIRE(identifiers.get_product_id() == krbn::product_id(8765));
    REQUIRE(identifiers.get_is_keyboard() == false);
    REQUIRE(identifiers.get_is_pointing_device() == true);
  }
}

TEST_CASE("device.identifiers.to_json") {
  {
    nlohmann::json json;
    krbn::device_identifiers identifiers(json);
    nlohmann::json expected({
        {"vendor_id", 0},
        {"product_id", 0},
        {"is_keyboard", false},
        {"is_pointing_device", false},
    });
    REQUIRE(identifiers.to_json() == expected);

    nlohmann::json actual = identifiers;
    REQUIRE(actual == expected);
  }
  {
    nlohmann::json json({
        {"is_pointing_device", true},
        {"dummy", {{"keep_me", true}}},
    });
    krbn::device_identifiers identifiers(json);
    nlohmann::json expected({
        {"dummy", {{"keep_me", true}}},
        {"vendor_id", 0},
        {"product_id", 0},
        {"is_keyboard", false},
        {"is_pointing_device", true},
    });
    REQUIRE(identifiers.to_json() == expected);
  }
}

TEST_CASE("device") {
  // empty json
  {
    nlohmann::json json;
    krbn::core_configuration::profile::device device(json);
    REQUIRE(device.get_identifiers().get_vendor_id() == krbn::vendor_id(0));
    REQUIRE(device.get_identifiers().get_product_id() == krbn::product_id(0));
    REQUIRE(device.get_identifiers().get_is_keyboard() == false);
    REQUIRE(device.get_identifiers().get_is_pointing_device() == false);
    REQUIRE(device.get_ignore() == false);
    REQUIRE(device.get_disable_built_in_keyboard_if_exists() == false);
  }

  // load values from json
  {
    nlohmann::json json({
        {"identifiers", {
                            {
                                "vendor_id", 1234,
                            },
                            {
                                "product_id", 5678,
                            },
                            {
                                "is_keyboard", true,
                            },
                            {
                                "is_pointing_device", true,
                            },
                        }},
        {"ignore", true},
        {"disable_built_in_keyboard_if_exists", true},
    });
    krbn::core_configuration::profile::device device(json);
    REQUIRE(device.get_identifiers().get_vendor_id() == krbn::vendor_id(1234));
    REQUIRE(device.get_identifiers().get_product_id() == krbn::product_id(5678));
    REQUIRE(device.get_identifiers().get_is_keyboard() == true);
    REQUIRE(device.get_identifiers().get_is_pointing_device() == true);
    REQUIRE(device.get_ignore() == true);
    REQUIRE(device.get_disable_built_in_keyboard_if_exists() == true);
  }

  // invalid values in json
  {
    nlohmann::json json({
        {"identifiers", nullptr},
        {"ignore", 1},
        {"disable_built_in_keyboard_if_exists", nlohmann::json::array()},
    });
    krbn::core_configuration::profile::device device(json);
    REQUIRE(device.get_identifiers().get_vendor_id() == krbn::vendor_id(0));
    REQUIRE(device.get_identifiers().get_product_id() == krbn::product_id(0));
    REQUIRE(device.get_identifiers().get_is_keyboard() == false);
    REQUIRE(device.get_identifiers().get_is_pointing_device() == false);
    REQUIRE(device.get_ignore() == false);
    REQUIRE(device.get_disable_built_in_keyboard_if_exists() == false);
  }
}

TEST_CASE("device.to_json") {
  {
    nlohmann::json json;
    krbn::core_configuration::profile::device device(json);
    nlohmann::json expected({
        {"identifiers", {
                            {
                                "vendor_id", 0,
                            },
                            {
                                "product_id", 0,
                            },
                            {
                                "is_keyboard", false,
                            },
                            {
                                "is_pointing_device", false,
                            },
                        }},
        {"ignore", false},
        {"disable_built_in_keyboard_if_exists", false},
        {"simple_modifications", nlohmann::json::object()},
        {"fn_function_keys", nlohmann::json::object()},
    });
    REQUIRE(device.to_json() == expected);

    nlohmann::json actual = device;
    REQUIRE(actual == expected);
  }
  {
    nlohmann::json json({
        {"identifiers", {
                            {
                                "is_keyboard", true,
                            },
                            {
                                "dummy", {{"keep_me", true}},
                            },
                        }},
        {"ignore", true},
        {"dummy", {{"keep_me", true}}},
    });
    krbn::core_configuration::profile::device device(json);
    nlohmann::json expected({
        {"identifiers", {
                            {
                                "dummy", {{"keep_me", true}},
                            },
                            {
                                "vendor_id", 0,
                            },
                            {
                                "product_id", 0,
                            },
                            {
                                "is_keyboard", true,
                            },
                            {
                                "is_pointing_device", false,
                            },
                        }},
        {"ignore", true},
        {"disable_built_in_keyboard_if_exists", false},
        {"simple_modifications", nlohmann::json::object()},
        {"fn_function_keys", nlohmann::json::object()},
        {"dummy", {{"keep_me", true}}},
    });
    REQUIRE(device.to_json() == expected);
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
