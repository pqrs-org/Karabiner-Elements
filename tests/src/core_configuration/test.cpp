#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "core_configuration.hpp"
#include "manipulator/details/types.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>
#include <iostream>

using simple_modifications = krbn::core_configuration::profile::simple_modifications;

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("valid") {
  krbn::core_configuration configuration("json/example.json");

  {
    std::vector<std::pair<simple_modifications::definition, simple_modifications::definition>> expected{
        {{"key_code", "caps_lock"}, {"key_code", "delete_or_backspace"}},
        {{"key_code", "escape"}, {"key_code", "spacebar"}},
    };
    REQUIRE(configuration.get_selected_profile().get_simple_modifications().get_pairs() == expected);
  }
  {
    auto manipulator = configuration.get_selected_profile().get_complex_modifications().get_rules()[0].get_manipulators()[0].get_json();
    REQUIRE(manipulator["type"] == "basic");
    REQUIRE(manipulator["from"]["key_code"] == "open_bracket");
  }
  {
    std::vector<std::pair<simple_modifications::definition, simple_modifications::definition>> expected{
        {{"key_code", "f1"}, {"key_code", "escape"}},
        {{"key_code", "f2"}, {"consumer_key_code", "display_brightness_increment"}},
        {{"key_code", "f3"}, {"key_code", "mission_control"}},
        {{"key_code", "f4"}, {"key_code", "launchpad"}},
        {{"key_code", "f5"}, {"key_code", "illumination_decrement"}},
        {{"key_code", "f6"}, {"key_code", "illumination_increment"}},
        {{"key_code", "f7"}, {"consumer_key_code", "rewind"}},
        {{"key_code", "f8"}, {"consumer_key_code", "play_or_pause"}},
        {{"key_code", "f9"}, {"consumer_key_code", "fastforward"}},
        {{"key_code", "f10"}, {"consumer_key_code", "mute"}},
        {{"key_code", "f11"}, {"consumer_key_code", "volume_decrement"}},
        {{"key_code", "f12"}, {"consumer_key_code", "volume_increment"}},
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
    REQUIRE(actual.size() == 3);

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

  std::vector<std::pair<simple_modifications::definition, simple_modifications::definition>> expected{
      {{"key_code", "caps_lock_2"}, {"key_code", "delete_or_backspace"}},
      {{"key_code", "escape"}, {"key_code", "spacebar"}},
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
  std::ifstream input("json/default_fn_function_keys.json");
  return nlohmann::json::parse(input);
}

nlohmann::json get_default_virtual_hid_keyboard_json(void) {
  return nlohmann::json({
      {"caps_lock_delay_milliseconds", 0},
      {"keyboard_type", ""},
  });
}

std::vector<std::pair<simple_modifications::definition, simple_modifications::definition>> make_default_fn_function_keys_pairs(void) {
  return std::vector<std::pair<simple_modifications::definition, simple_modifications::definition>>({
      {{"key_code", "f1"}, {"consumer_key_code", "display_brightness_decrement"}},
      {{"key_code", "f2"}, {"consumer_key_code", "display_brightness_increment"}},
      {{"key_code", "f3"}, {"key_code", "mission_control"}},
      {{"key_code", "f4"}, {"key_code", "launchpad"}},
      {{"key_code", "f5"}, {"key_code", "illumination_decrement"}},
      {{"key_code", "f6"}, {"key_code", "illumination_increment"}},
      {{"key_code", "f7"}, {"consumer_key_code", "rewind"}},
      {{"key_code", "f8"}, {"consumer_key_code", "play_or_pause"}},
      {{"key_code", "f9"}, {"consumer_key_code", "fastforward"}},
      {{"key_code", "f10"}, {"consumer_key_code", "mute"}},
      {{"key_code", "f11"}, {"consumer_key_code", "volume_decrement"}},
      {{"key_code", "f12"}, {"consumer_key_code", "volume_increment"}},
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
    REQUIRE(profile.get_fn_function_keys().get_pairs() == make_default_fn_function_keys_pairs());
    REQUIRE(profile.get_devices().size() == 0);

    REQUIRE(profile.get_device_ignore(krbn::device_identifiers(krbn::vendor_id(0x05ac),
                                                               krbn::product_id(0x8600),
                                                               true,
                                                               false)) == true);
    REQUIRE(profile.get_device_ignore(krbn::device_identifiers(krbn::vendor_id(0x05ac),
                                                               krbn::product_id(0x262),
                                                               true,
                                                               false)) == false);
  }

  // load values from json
  {
    nlohmann::json json({
        {"name", "profile 1"},
        {"selected", true},
        {"simple_modifications", {
                                     {
                                         "from 1",
                                         "to 1",
                                     },
                                     {
                                         "from 3",
                                         "to 3",
                                     },
                                     {
                                         "from 2",
                                         "to 2",
                                     },
                                     {
                                         "from 10",
                                         "to 10",
                                     },
                                 }},
        {"fn_function_keys", {
                                 {
                                     "f3",
                                     "to f3",
                                 },
                                 {
                                     "f4",
                                     "to f4",
                                 },
                                 {
                                     "f13",
                                     "to f13",
                                 },
                             }},
        {"devices", {
                        {
                            {"identifiers", {
                                                {
                                                    "vendor_id",
                                                    1234,
                                                },
                                                {
                                                    "product_id",
                                                    5678,
                                                },
                                                {
                                                    "is_keyboard",
                                                    true,
                                                },
                                                {
                                                    "is_pointing_device",
                                                    true,
                                                },
                                            }},
                            {"ignore", true},
                            {"disable_built_in_keyboard_if_exists", true},
                            {"manipulate_caps_lock_led", false},
                        },
                        // duplicated identifiers
                        {
                            {"identifiers", {
                                                {
                                                    "vendor_id",
                                                    1234,
                                                },
                                                {
                                                    "product_id",
                                                    5678,
                                                },
                                                {
                                                    "is_keyboard",
                                                    true,
                                                },
                                                {
                                                    "is_pointing_device",
                                                    true,
                                                },
                                            }},
                            {"ignore", true},
                            {"disable_built_in_keyboard_if_exists", true},
                            {"manipulate_caps_lock_led", false},
                        },
                        {
                            {"identifiers", {
                                                {
                                                    "vendor_id",
                                                    4321,
                                                },
                                                {
                                                    "product_id",
                                                    8765,
                                                },
                                                {
                                                    "is_keyboard",
                                                    true,
                                                },
                                                {
                                                    "is_pointing_device",
                                                    false,
                                                },
                                            }},
                            {"ignore", false},
                            {"disable_built_in_keyboard_if_exists", true},
                            {"manipulate_caps_lock_led", false},
                        },
                    }},
    });
    krbn::core_configuration::profile profile(json);
    REQUIRE(profile.get_name() == std::string("profile 1"));
    REQUIRE(profile.get_selected() == true);
    {
      std::vector<std::pair<simple_modifications::definition, simple_modifications::definition>> expected({
          {{"key_code", "from 1"}, {"key_code", "to 1"}},
          {{"key_code", "from 2"}, {"key_code", "to 2"}},
          {{"key_code", "from 3"}, {"key_code", "to 3"}},
          {{"key_code", "from 10"}, {"key_code", "to 10"}},
      });
      REQUIRE(profile.get_simple_modifications().get_pairs() == expected);
    }
    {
      auto expected = make_default_fn_function_keys_pairs();
      expected[2].second.set_value("to f3");
      expected[3].second.set_value("to f4");
      expected.emplace_back(simple_modifications::definition("key_code", "f13"),
                            simple_modifications::definition("key_code", "to f13"));
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
              "vendor_id",
              1234,
          },
          {
              "product_id",
              5678,
          },
          {
              "is_keyboard",
              true,
          },
          {
              "is_pointing_device",
              true,
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
                "vendor_id",
                1111,
            },
            {
                "product_id",
                2222,
            },
            {
                "is_keyboard",
                false,
            },
            {
                "is_pointing_device",
                true,
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
        REQUIRE((profile.get_devices())[4].get_ignore() == true);
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
    REQUIRE(profile.get_fn_function_keys().get_pairs() == make_default_fn_function_keys_pairs());
  }
  {
    nlohmann::json json({
        {"simple_modifications", {
                                     {
                                         "number",
                                         0,
                                     },
                                     {
                                         "object",
                                         nlohmann::json::object(),
                                     },
                                     {
                                         "array",
                                         nlohmann::json::array(),
                                     },
                                     {
                                         "key",
                                         "value",
                                     },
                                 }},
    });
    krbn::core_configuration::profile profile(json);
    {
      std::vector<std::pair<simple_modifications::definition, simple_modifications::definition>> expected({
          {{"key_code", "key"}, {"key_code", "value"}},
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
                                      {"parameters", nlohmann::json::object({
                                                         {"basic.to_if_alone_timeout_milliseconds", 1000},
                                                         {"basic.to_delayed_action_delay_milliseconds", 500},
                                                     })},
                                  })},
        {"devices", nlohmann::json::array()},
        {"name", ""},
        {"selected", false},
        {"simple_modifications", nlohmann::json::array()},
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
                                                    "vendor_id",
                                                    1234,
                                                },
                                                {
                                                    "product_id",
                                                    5678,
                                                },
                                                {
                                                    "is_keyboard",
                                                    true,
                                                },
                                                {
                                                    "is_pointing_device",
                                                    true,
                                                },
                                            }},
                            {"ignore", true},
                            {"disable_built_in_keyboard_if_exists", true},
                            {"manipulate_caps_lock_led", false},
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
    profile.get_simple_modifications().replace_pair(1, {"key_code", "from 1"}, {"key_code", "to 1"});
    // {
    //   "": "",
    //   "from 1": "to 1"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(2, {"key_code", "from 3"}, {"key_code", "to 3"});
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(3, {"key_code", "from 4"}, {"key_code", "to 4"});
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(4, {"key_code", "from 2"}, {"key_code", "to 2"});
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4",
    //   "from 2": "to 2"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(5, {"key_code", "from 2"}, {"key_code", "to 2.0"});
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
    profile.get_simple_modifications().replace_pair(5, {"key_code", ""}, {"key_code", "to 0"});
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0",
    //   "": "to 0"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(6, {"key_code", "from 0"}, {"key_code", ""});
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
    profile.get_simple_modifications().replace_pair(7, {"key_code", "from 5"}, {"key_code", "to 5"});
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

    profile.get_fn_function_keys().replace_second({"key_code", "f3"}, {"key_code", "to f3"});
    profile.get_fn_function_keys().replace_second({"key_code", "not found"}, {"key_code", "do nothing"});

    profile.get_virtual_hid_keyboard().set_keyboard_type("iso");

    auto expected_fn_function_keys = get_default_fn_function_keys_json();
    expected_fn_function_keys[2]["to"]["key_code"] = "to f3";
    auto expected_virtual_hid_keyboard = get_default_virtual_hid_keyboard_json();
    expected_virtual_hid_keyboard["keyboard_type"] = "iso";
    nlohmann::json expected({
        {"complex_modifications", nlohmann::json::object({
                                      {"rules", nlohmann::json::array()},
                                      {"parameters", nlohmann::json::object({
                                                         {"basic.to_if_alone_timeout_milliseconds", 1000},
                                                         {"basic.to_delayed_action_delay_milliseconds", 500},
                                                     })},
                                  })},
        {"devices", {
                        {
                            {"identifiers", {
                                                {
                                                    "vendor_id",
                                                    1234,
                                                },
                                                {
                                                    "product_id",
                                                    5678,
                                                },
                                                {
                                                    "is_keyboard",
                                                    true,
                                                },
                                                {
                                                    "is_pointing_device",
                                                    true,
                                                },
                                            }},
                            {"ignore", true},
                            {"disable_built_in_keyboard_if_exists", true},
                            {"fn_function_keys", nlohmann::json::array()},
                            {"manipulate_caps_lock_led", false},
                            {"simple_modifications", nlohmann::json::array()},
                        },
                    }},
        {"dummy", {{"keep_me", true}}},
        {"name", "profile 1"},
        {"selected", true},
        {"simple_modifications", nlohmann::json::array()},
        {"fn_function_keys", expected_fn_function_keys},
        {"virtual_hid_keyboard", expected_virtual_hid_keyboard},
    });

    expected["simple_modifications"].push_back(nlohmann::json::object());
    expected["simple_modifications"].back()["from"]["key_code"] = "from 1";
    expected["simple_modifications"].back()["to"]["key_code"] = "to 1";
    expected["simple_modifications"].push_back(nlohmann::json::object());
    expected["simple_modifications"].back()["from"]["key_code"] = "from 4";
    expected["simple_modifications"].back()["to"]["key_code"] = "to 4";
    expected["simple_modifications"].push_back(nlohmann::json::object());
    expected["simple_modifications"].back()["from"]["key_code"] = "from 2";
    expected["simple_modifications"].back()["to"]["key_code"] = "to 2";
    expected["simple_modifications"].push_back(nlohmann::json::object());
    expected["simple_modifications"].back()["from"]["key_code"] = "from 5";
    expected["simple_modifications"].back()["to"]["key_code"] = "to 5";

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

  // load values from json (v2)
  {
    auto json = nlohmann::json::array();
    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "a";
    json.back()["to"]["key_code"] = "f1";
    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "b";
    json.back()["to"]["key_code"] = "f2";
    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "dummy";
    json.back()["to"]["key_code"] = "f3";
    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f4";
    json.back()["to"]["key_code"] = "dummy";
    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "f5";
    json.back()["to"]["key_code"] = nlohmann::json();
    json.push_back(nlohmann::json::object());
    json.back()["dummy"]["key_code"] = nlohmann::json();

    krbn::core_configuration::profile::simple_modifications simple_modifications(json);
    REQUIRE(simple_modifications.get_pairs().size() == 5);

    {
      std::vector<std::pair<simple_modifications::definition, simple_modifications::definition>> expected({
          {{"key_code", "a"}, {"key_code", "f1"}},
          {{"key_code", "b"}, {"key_code", "f2"}},
          {{"key_code", "dummy"}, {"key_code", "f3"}},
          {{"key_code", "f4"}, {"key_code", "dummy"}},
          {{"key_code", "f5"}, {"", ""}},
      });
      REQUIRE(simple_modifications.get_pairs() == expected);
    }
  }

  // load values from json (v1)
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
      std::vector<std::pair<simple_modifications::definition, simple_modifications::definition>> expected({
          {{"key_code", "a"}, {"key_code", "f1"}},
          {{"key_code", "b"}, {"key_code", "f2"}},
          {{"key_code", "dummy"}, {"key_code", "f3"}},
          {{"key_code", "f4"}, {"key_code", "dummy"}},
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
    REQUIRE(simple_modifications.to_json() == nlohmann::json::array());
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
    simple_modifications.replace_pair(4, {"key_code", "a"}, {"key_code", "f5"}); // will be ignored since "a" already exists.
    simple_modifications.push_back_pair();
    simple_modifications.replace_pair(5, {"key_code", "c"}, {"key_code", "f6"});

    auto expected = nlohmann::json::array();
    expected.push_back(nlohmann::json::object());
    expected.back()["from"]["key_code"] = "a";
    expected.back()["to"]["key_code"] = "f1";
    expected.push_back(nlohmann::json::object());
    expected.back()["from"]["key_code"] = "b";
    expected.back()["to"]["key_code"] = "f2";
    expected.push_back(nlohmann::json::object());
    expected.back()["from"]["key_code"] = "dummy";
    expected.back()["to"]["key_code"] = "f3";
    expected.push_back(nlohmann::json::object());
    expected.back()["from"]["key_code"] = "f4";
    expected.back()["to"]["key_code"] = "dummy";
    expected.push_back(nlohmann::json::object());
    expected.back()["from"]["key_code"] = "c";
    expected.back()["to"]["key_code"] = "f6";

    REQUIRE(simple_modifications.to_json() == expected);
  }
  {
    // simple_modifications.to_json have to be compatible with manipulator::details::event_definition

    auto json = nlohmann::json::array();
    json.push_back(nlohmann::json::object());
    json.back()["from"]["consumer_key_code"] = "mute";
    json.back()["to"]["pointing_button"] = "button3";
    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "a";
    json.back()["to"]["key_code"] = "f1";

    krbn::core_configuration::profile::simple_modifications simple_modifications(json);
    {
      krbn::manipulator::details::from_event_definition from_event_definition(simple_modifications.get_pairs()[0].first.to_json());
      REQUIRE(from_event_definition.get_consumer_key_code() == krbn::consumer_key_code::mute);
    }
    {
      krbn::manipulator::details::to_event_definition to_event_definition(simple_modifications.get_pairs()[0].second.to_json());
      REQUIRE(to_event_definition.get_pointing_button() == krbn::pointing_button::button3);
    }
    {
      krbn::manipulator::details::from_event_definition from_event_definition(simple_modifications.get_pairs()[1].first.to_json());
      REQUIRE(from_event_definition.get_key_code() == krbn::key_code::a);
    }
    {
      krbn::manipulator::details::to_event_definition to_event_definition(simple_modifications.get_pairs()[1].second.to_json());
      REQUIRE(to_event_definition.get_key_code() == krbn::key_code::f1);
    }
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
            "rules",
            {
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
    {
      nlohmann::json json({
          {
              "rules",
              "rule 1",
          },
      });
      krbn::core_configuration::profile::complex_modifications complex_modifications(json);
      REQUIRE(complex_modifications.get_rules().empty());
    }
    {
      krbn::core_configuration::profile::complex_modifications complex_modifications(nlohmann::json::array());
      REQUIRE(complex_modifications.get_rules().empty());
    }
  }
}

TEST_CASE("complex_modifications.push_back_rule") {
  {
    nlohmann::json json({
        {
            "rules",
            {
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
            "rules",
            {
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
            "rules",
            {
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
    REQUIRE(virtual_hid_keyboard.get_keyboard_type() == std::string(""));
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
    REQUIRE(virtual_hid_keyboard.get_keyboard_type() == std::string(""));
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
                                "vendor_id",
                                1234,
                            },
                            {
                                "product_id",
                                5678,
                            },
                            {
                                "is_keyboard",
                                true,
                            },
                            {
                                "is_pointing_device",
                                true,
                            },
                        }},
        {"disable_built_in_keyboard_if_exists", true},
        {"ignore", true},
        {"manipulate_caps_lock_led", false},
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
        {"disable_built_in_keyboard_if_exists", nlohmann::json::array()},
        {"identifiers", nullptr},
        {"ignore", 1},
        {"manipulate_caps_lock_led", false},
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
        {"disable_built_in_keyboard_if_exists", false},
        {"identifiers", {
                            {
                                "vendor_id",
                                0,
                            },
                            {
                                "product_id",
                                0,
                            },
                            {
                                "is_keyboard",
                                false,
                            },
                            {
                                "is_pointing_device",
                                false,
                            },
                        }},
        {"ignore", false},
        {"fn_function_keys", nlohmann::json::array()},
        {"manipulate_caps_lock_led", false},
        {"simple_modifications", nlohmann::json::array()},
    });
    REQUIRE(device.to_json() == expected);

    nlohmann::json actual = device;
    REQUIRE(actual == expected);
  }
  {
    nlohmann::json json({
        {"dummy", {{"keep_me", true}}},
        {"identifiers", {
                            {
                                "is_keyboard",
                                true,
                            },
                            {
                                "dummy",
                                {{"keep_me", true}},
                            },
                        }},
        {"ignore", true},
        {"manipulate_caps_lock_led", true},
    });
    krbn::core_configuration::profile::device device(json);
    nlohmann::json expected({
        {"disable_built_in_keyboard_if_exists", false},
        {"dummy", {{"keep_me", true}}},
        {"fn_function_keys", nlohmann::json::array()},
        {"identifiers", {
                            {
                                "dummy",
                                {{"keep_me", true}},
                            },
                            {
                                "vendor_id",
                                0,
                            },
                            {
                                "product_id",
                                0,
                            },
                            {
                                "is_keyboard",
                                true,
                            },
                            {
                                "is_pointing_device",
                                false,
                            },
                        }},
        {"ignore", true},
        {"manipulate_caps_lock_led", true},
        {"simple_modifications", nlohmann::json::array()},
    });
    REQUIRE(device.to_json() == expected);
  }
}
