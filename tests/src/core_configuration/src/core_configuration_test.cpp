#include <catch2/catch.hpp>

#include "core_configuration/core_configuration.hpp"
#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"
#include <iostream>

using simple_modifications = krbn::core_configuration::details::simple_modifications;

TEST_CASE("valid") {
  krbn::core_configuration::core_configuration configuration("json/example.json");

  {
    std::vector<std::pair<std::string, std::string>> expected;

    expected.emplace_back(nlohmann::json{{"key_code", "caps_lock"}}.dump(),
                          nlohmann::json{{"key_code", "delete_or_backspace"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "escape"}}.dump(),
                          nlohmann::json{{"key_code", "spacebar"}}.dump());

    REQUIRE(configuration.get_selected_profile().get_simple_modifications().get_pairs() == expected);
  }
  {
    auto manipulator = configuration.get_selected_profile().get_complex_modifications().get_rules()[0].get_manipulators()[0].get_json();
    REQUIRE(manipulator["type"] == "basic");
    REQUIRE(manipulator["from"]["key_code"] == "open_bracket");
  }
  {
    std::vector<std::pair<std::string, std::string>> expected;

    expected.emplace_back(nlohmann::json{{"key_code", "f1"}}.dump(),
                          nlohmann::json{{"key_code", "escape"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f2"}}.dump(),
                          nlohmann::json{{"consumer_key_code", "display_brightness_increment"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f3"}}.dump(),
                          nlohmann::json{{"key_code", "mission_control"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f4"}}.dump(),
                          nlohmann::json{{"key_code", "launchpad"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f5"}}.dump(),
                          nlohmann::json{{"key_code", "illumination_decrement"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f6"}}.dump(),
                          nlohmann::json{{"key_code", "illumination_increment"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f7"}}.dump(),
                          nlohmann::json{{"consumer_key_code", "rewind"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f8"}}.dump(),
                          nlohmann::json{{"consumer_key_code", "play_or_pause"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f9"}}.dump(),
                          nlohmann::json{{"consumer_key_code", "fastforward"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f10"}}.dump(),
                          nlohmann::json{{"consumer_key_code", "mute"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f11"}}.dump(),
                          nlohmann::json{{"consumer_key_code", "volume_decrement"}}.dump());

    expected.emplace_back(nlohmann::json{{"key_code", "f12"}}.dump(),
                          nlohmann::json{{"consumer_key_code", "volume_increment"}}.dump());

    REQUIRE(configuration.get_selected_profile().get_fn_function_keys().get_pairs() == expected);
  }
  {
    auto& complex_modifications = configuration.get_selected_profile().get_complex_modifications();
    auto& rules = complex_modifications.get_rules();
    REQUIRE(complex_modifications.get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 800);
    REQUIRE(rules[0].get_manipulators()[0].get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 800);
    REQUIRE(rules[0].get_manipulators()[2].get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 400);
    REQUIRE(rules[0].get_description() == "Emacs bindings, etc.");
    REQUIRE(rules[1].get_description() == "description test");
    REQUIRE(rules[2].get_description() == "");
  }
  {
    REQUIRE(configuration
                .get_selected_profile()
                .get_virtual_hid_keyboard()
                .get_country_code() == krbn::hid_country_code(99));
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

TEST_CASE("not found") {
  {
    krbn::core_configuration::core_configuration configuration("json/not_found.json");
    REQUIRE(configuration.get_selected_profile().get_name() == "Default profile");
    REQUIRE(configuration.is_loaded() == false);
  }
}

TEST_CASE("broken.json") {
  {
    krbn::core_configuration::core_configuration configuration("json/broken.json");

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
    krbn::core_configuration::core_configuration configuration("/bin/ls");

    REQUIRE(configuration.get_selected_profile().get_simple_modifications().get_pairs().empty());
    REQUIRE(configuration.is_loaded() == false);
  }
}

TEST_CASE("invalid_key_code_name.json") {
  krbn::core_configuration::core_configuration configuration("json/invalid_key_code_name.json");

  std::vector<std::pair<std::string, std::string>> expected;

  expected.emplace_back(nlohmann::json{{"key_code", "caps_lock_2"}}.dump(),
                        nlohmann::json{{"key_code", "delete_or_backspace"}}.dump());

  expected.emplace_back(nlohmann::json{{"key_code", "escape"}}.dump(),
                        nlohmann::json{{"key_code", "spacebar"}}.dump());

  REQUIRE(configuration.get_selected_profile().get_simple_modifications().get_pairs() == expected);
  REQUIRE(configuration.is_loaded() == true);
}

TEST_CASE("global_configuration.to_json") {
  {
    nlohmann::json json;
    krbn::core_configuration::details::global_configuration global_configuration(json);
    nlohmann::json expected({
        {"check_for_updates_on_startup", true},
        {"show_in_menu_bar", true},
        {"show_profile_name_in_menu_bar", false},
    });
    REQUIRE(global_configuration.to_json() == expected);

    auto actual = global_configuration.to_json();
    REQUIRE(actual == expected);
  }
  {
    nlohmann::json json{
        {"dummy", {{"keep_me", true}}},
    };
    krbn::core_configuration::details::global_configuration global_configuration(json);
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
  return nlohmann::json{
      {"country_code", 0},
      {"mouse_key_xy_scale", 100},
  };
}

std::vector<std::pair<std::string, std::string>> make_default_fn_function_keys_pairs(void) {
  std::vector<std::pair<std::string, std::string>> pairs;

  pairs.emplace_back(nlohmann::json{{"key_code", "f1"}}.dump(),
                     nlohmann::json{{"consumer_key_code", "display_brightness_decrement"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f2"}}.dump(),
                     nlohmann::json{{"consumer_key_code", "display_brightness_increment"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f3"}}.dump(),
                     nlohmann::json{{"key_code", "mission_control"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f4"}}.dump(),
                     nlohmann::json{{"key_code", "launchpad"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f5"}}.dump(),
                     nlohmann::json{{"key_code", "illumination_decrement"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f6"}}.dump(),
                     nlohmann::json{{"key_code", "illumination_increment"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f7"}}.dump(),
                     nlohmann::json{{"consumer_key_code", "rewind"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f8"}}.dump(),
                     nlohmann::json{{"consumer_key_code", "play_or_pause"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f9"}}.dump(),
                     nlohmann::json{{"consumer_key_code", "fastforward"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f10"}}.dump(),
                     nlohmann::json{{"consumer_key_code", "mute"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f11"}}.dump(),
                     nlohmann::json{{"consumer_key_code", "volume_decrement"}}.dump());

  pairs.emplace_back(nlohmann::json{{"key_code", "f12"}}.dump(),
                     nlohmann::json{{"consumer_key_code", "volume_increment"}}.dump());

  return pairs;
}
} // namespace

TEST_CASE("profile") {
  // empty json
  {
    auto json = nlohmann::json::object();
    krbn::core_configuration::details::profile profile(json);
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
    krbn::core_configuration::details::profile profile(json);
    REQUIRE(profile.get_name() == std::string("profile 1"));
    REQUIRE(profile.get_selected() == true);
    {
      std::vector<std::pair<std::string, std::string>> expected;

      expected.emplace_back(nlohmann::json{{"key_code", "from 1"}}.dump(),
                            nlohmann::json{{"key_code", "to 1"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "from 2"}}.dump(),
                            nlohmann::json{{"key_code", "to 2"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "from 3"}}.dump(),
                            nlohmann::json{{"key_code", "to 3"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "from 10"}}.dump(),
                            nlohmann::json{{"key_code", "to 10"}}.dump());

      REQUIRE(profile.get_simple_modifications().get_pairs() == expected);
    }
    {
      auto expected = make_default_fn_function_keys_pairs();
      expected[2].second = nlohmann::json{{"key_code", "to f3"}}.dump();
      expected[3].second = nlohmann::json{{"key_code", "to f4"}}.dump();
      expected.emplace_back(nlohmann::json{{"key_code", "f13"}}.dump(),
                            nlohmann::json{{"key_code", "to f13"}}.dump());
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
      auto identifiers = nlohmann::json::object(
                             {
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
                             })
                             .get<krbn::device_identifiers>();
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
        auto identifiers = nlohmann::json::object(
                               {
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
                               })
                               .get<krbn::device_identifiers>();
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
    krbn::core_configuration::details::profile profile(json);
    {
      std::vector<std::pair<std::string, std::string>> expected;

      expected.emplace_back(nlohmann::json{{"key_code", "key"}}.dump(),
                            nlohmann::json{{"key_code", "value"}}.dump());

      REQUIRE(profile.get_simple_modifications().get_pairs() == expected);
    }
  }
}

TEST_CASE("profile.to_json") {
  {
    auto json = nlohmann::json::object();
    krbn::core_configuration::details::profile profile(json);
    nlohmann::json expected({
        {"complex_modifications", nlohmann::json::object({
                                      {"rules", nlohmann::json::array()},
                                      {"parameters", nlohmann::json::object({
                                                         {"basic.simultaneous_threshold_milliseconds", 50},
                                                         {"basic.to_if_alone_timeout_milliseconds", 1000},
                                                         {"basic.to_if_held_down_threshold_milliseconds", 500},
                                                         {"basic.to_delayed_action_delay_milliseconds", 500},
                                                         {"mouse_motion_to_scroll.speed", 100},
                                                     })},
                                  })},
        {"devices", nlohmann::json::array()},
        {"name", ""},
        {"selected", false},
        {"parameters", nlohmann::json::object({
                           {"delay_milliseconds_before_open_device", 1000},
                       })},
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
    krbn::core_configuration::details::profile profile(json);
    profile.set_name("profile 1");
    profile.set_selected(true);

    profile.get_parameters().set_delay_milliseconds_before_open_device(std::chrono::milliseconds(500));

    profile.get_simple_modifications().push_back_pair();
    // {
    //   "": ""
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(1,
                                                    nlohmann::json{{"key_code", "from 1"}}.dump(),
                                                    nlohmann::json{{"key_code", "to 1"}}.dump());
    // {
    //   "": "",
    //   "from 1": "to 1"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(2,
                                                    nlohmann::json{{"key_code", "from 3"}}.dump(),
                                                    nlohmann::json{{"key_code", "to 3"}}.dump());
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(3,
                                                    nlohmann::json{{"key_code", "from 4"}}.dump(),
                                                    nlohmann::json{{"key_code", "to 4"}}.dump());
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(4,
                                                    nlohmann::json{{"key_code", "from 2"}}.dump(),
                                                    nlohmann::json{{"key_code", "to 2"}}.dump());
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4",
    //   "from 2": "to 2"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(5,
                                                    nlohmann::json{{"key_code", "from 2"}}.dump(),
                                                    nlohmann::json{{"key_code", "to 2.0"}}.dump());
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
    profile.get_simple_modifications().replace_pair(5,
                                                    nlohmann::json::object().dump(),
                                                    nlohmann::json{{"key_code", "to 0"}}.dump());
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0",
    //   "": "to 0"
    // }

    profile.get_simple_modifications().push_back_pair();
    profile.get_simple_modifications().replace_pair(6,
                                                    nlohmann::json{{"key_code", "from 0"}}.dump(),
                                                    nlohmann::json::object().dump());
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
    profile.get_simple_modifications().replace_pair(7,
                                                    nlohmann::json{{"key_code", "from 5"}}.dump(),
                                                    nlohmann::json{{"key_code", "to 5"}}.dump());
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

    profile.get_fn_function_keys().replace_second(nlohmann::json{{"key_code", "f3"}}.dump(),
                                                  nlohmann::json{{"key_code", "to f3"}}.dump());
    profile.get_fn_function_keys().replace_second(nlohmann::json{{"key_code", "not found"}}.dump(),
                                                  nlohmann::json{{"key_code", "do nothing"}}.dump());

    profile.get_virtual_hid_keyboard().set_country_code(krbn::hid_country_code(20));
    profile.get_virtual_hid_keyboard().set_mouse_key_xy_scale(250);

    auto expected_fn_function_keys = get_default_fn_function_keys_json();
    expected_fn_function_keys[2]["to"]["key_code"] = "to f3";
    auto expected_virtual_hid_keyboard = get_default_virtual_hid_keyboard_json();
    expected_virtual_hid_keyboard["country_code"] = 20;
    expected_virtual_hid_keyboard["mouse_key_xy_scale"] = 250;
    nlohmann::json expected({
        {"complex_modifications", nlohmann::json::object({
                                      {"rules", nlohmann::json::array()},
                                      {"parameters", nlohmann::json::object({
                                                         {"basic.simultaneous_threshold_milliseconds", 50},
                                                         {"basic.to_if_alone_timeout_milliseconds", 1000},
                                                         {"basic.to_if_held_down_threshold_milliseconds", 500},
                                                         {"basic.to_delayed_action_delay_milliseconds", 500},
                                                         {"mouse_motion_to_scroll.speed", 100},
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
        {"parameters", nlohmann::json::object({
                           {"delay_milliseconds_before_open_device", 500},
                       })},
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

    krbn::core_configuration::details::simple_modifications simple_modifications;
    simple_modifications.update(json);
    REQUIRE(simple_modifications.get_pairs().size() == 5);

    {
      std::vector<std::pair<std::string, std::string>> expected;

      expected.emplace_back(nlohmann::json{{"key_code", "a"}}.dump(),
                            nlohmann::json{{"key_code", "f1"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "b"}}.dump(),
                            nlohmann::json{{"key_code", "f2"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "dummy"}}.dump(),
                            nlohmann::json{{"key_code", "f3"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "f4"}}.dump(),
                            nlohmann::json{{"key_code", "dummy"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "f5"}}.dump(),
                            nlohmann::json{{"key_code", nlohmann::json()}}.dump());

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
    krbn::core_configuration::details::simple_modifications simple_modifications;
    simple_modifications.update(json);
    REQUIRE(simple_modifications.get_pairs().size() == 4);

    {
      std::vector<std::pair<std::string, std::string>> expected;

      expected.emplace_back(nlohmann::json{{"key_code", "a"}}.dump(),
                            nlohmann::json{{"key_code", "f1"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "b"}}.dump(),
                            nlohmann::json{{"key_code", "f2"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "dummy"}}.dump(),
                            nlohmann::json{{"key_code", "f3"}}.dump());

      expected.emplace_back(nlohmann::json{{"key_code", "f4"}}.dump(),
                            nlohmann::json{{"key_code", "dummy"}}.dump());

      REQUIRE(simple_modifications.get_pairs() == expected);
    }
  }

  // replace_pair
  {
    krbn::core_configuration::details::simple_modifications simple_modifications;
    simple_modifications.push_back_pair();
    simple_modifications.replace_pair(0,
                                      "{ \"key_code\" : \"a\" }",
                                      "{\"key_code\":\"a\"}");
    REQUIRE(simple_modifications.get_pairs()[0].first == nlohmann::json{{"key_code", "a"}}.dump());
    REQUIRE(simple_modifications.get_pairs()[0].second == nlohmann::json{{"key_code", "a"}}.dump());

    // Invalid from json
    simple_modifications.replace_pair(0,
                                      "{ \"key_code\" : \"a\" }",
                                      "{\"}");
    REQUIRE(simple_modifications.get_pairs()[0].first == nlohmann::json{{"key_code", "a"}}.dump());
    REQUIRE(simple_modifications.get_pairs()[0].second == nlohmann::json{{"key_code", "a"}}.dump());

    // Invalid to json
    simple_modifications.replace_pair(0,
                                      "{\"}",
                                      "{ \"key_code\" : \"a\" }");
    REQUIRE(simple_modifications.get_pairs()[0].first == nlohmann::json{{"key_code", "a"}}.dump());
    REQUIRE(simple_modifications.get_pairs()[0].second == nlohmann::json{{"key_code", "a"}}.dump());
  }

  // replace_second
  {
    krbn::core_configuration::details::simple_modifications simple_modifications;
    simple_modifications.push_back_pair();
    simple_modifications.replace_pair(0,
                                      "{ \"key_code\" : \"a\" }",
                                      "{\"key_code\":\"a\"}");
    REQUIRE(simple_modifications.get_pairs()[0].first == nlohmann::json{{"key_code", "a"}}.dump());
    REQUIRE(simple_modifications.get_pairs()[0].second == nlohmann::json{{"key_code", "a"}}.dump());

    simple_modifications.replace_second("{ \"key_code\" : \"a\" }",
                                        "{\"key_code\":\"b\"}");
    REQUIRE(simple_modifications.get_pairs()[0].first == nlohmann::json{{"key_code", "a"}}.dump());
    REQUIRE(simple_modifications.get_pairs()[0].second == nlohmann::json{{"key_code", "b"}}.dump());

    simple_modifications.replace_second("{\"key_code\":\"a\"}",
                                        "{\"key_code\":\"c\"}");
    REQUIRE(simple_modifications.get_pairs()[0].first == nlohmann::json{{"key_code", "a"}}.dump());
    REQUIRE(simple_modifications.get_pairs()[0].second == nlohmann::json{{"key_code", "c"}}.dump());

    // Invalid from json
    simple_modifications.replace_second("{\"}",
                                        "{ \"key_code\" : \"a\" }");

    // Invalid to json
    simple_modifications.replace_second("{ \"key_code\" : \"a\" }",
                                        "{\"}");
  }
}

TEST_CASE("simple_modifications.to_json") {
  {
    nlohmann::json json({
        {"a", "f1"},
        {"b", "f2"},
        {"dummy", "f3"},
        {"f4", "dummy"},
    });
    krbn::core_configuration::details::simple_modifications simple_modifications;
    simple_modifications.update(json);
    simple_modifications.push_back_pair();
    // will be ignored since "a" already exists.
    simple_modifications.replace_pair(4,
                                      "{\"key_code\":\"a\"}",
                                      nlohmann::json{{"key_code", "f5"}}.dump());
    simple_modifications.push_back_pair();
    simple_modifications.replace_pair(5,
                                      "{ \"key_code\" : \"a\" }",
                                      nlohmann::json{{"key_code", "f5"}}.dump());
    simple_modifications.push_back_pair();
    simple_modifications.replace_pair(6,
                                      nlohmann::json{{"key_code", "c"}}.dump(),
                                      nlohmann::json{{"key_code", "f6"}}.dump());

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
    // simple_modifications.to_json have to be compatible with manipulator::event_definition

    auto json = nlohmann::json::array();
    json.push_back(nlohmann::json::object());
    json.back()["from"]["consumer_key_code"] = "mute";
    json.back()["to"]["pointing_button"] = "button3";
    json.push_back(nlohmann::json::object());
    json.back()["from"]["key_code"] = "a";
    json.back()["to"]["key_code"] = "f1";

    krbn::core_configuration::details::simple_modifications simple_modifications;
    simple_modifications.update(json);
    {
      krbn::manipulator::manipulators::basic::from_event_definition from_event_definition(nlohmann::json::parse(simple_modifications.get_pairs()[0].first));
      REQUIRE(from_event_definition.get_event_definitions().size() == 1);
      REQUIRE(from_event_definition.get_event_definitions().front().get_consumer_key_code() == krbn::consumer_key_code::mute);
    }
    {
      krbn::manipulator::to_event_definition to_event_definition(nlohmann::json::parse(simple_modifications.get_pairs()[0].second));
      REQUIRE(to_event_definition.get_event_definition().get_pointing_button() == krbn::pointing_button::button3);
    }
    {
      krbn::manipulator::manipulators::basic::from_event_definition from_event_definition(nlohmann::json::parse(simple_modifications.get_pairs()[1].first));
      REQUIRE(from_event_definition.get_event_definitions().size() == 1);
      REQUIRE(from_event_definition.get_event_definitions().front().get_key_code() == krbn::key_code::a);
    }
    {
      krbn::manipulator::to_event_definition to_event_definition(nlohmann::json::parse(simple_modifications.get_pairs()[1].second));
      REQUIRE(to_event_definition.get_event_definition().get_key_code() == krbn::key_code::f1);
    }
  }
}

TEST_CASE("complex_modifications") {
  // empty json
  {
    auto json = nlohmann::json::object();
    krbn::core_configuration::details::complex_modifications complex_modifications(json);
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
    krbn::core_configuration::details::complex_modifications complex_modifications(json);
    REQUIRE(complex_modifications.get_rules().size() == 3);
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
    krbn::core_configuration::details::complex_modifications complex_modifications(json);
    auto& rules = complex_modifications.get_rules();
    REQUIRE(rules.size() == 3);
    REQUIRE(rules[0].get_description() == "rule 1");
    REQUIRE(rules[1].get_description() == "rule 2");
    REQUIRE(rules[2].get_description() == "rule 3");

    krbn::core_configuration::details::complex_modifications_parameters parameters;
    nlohmann::json rule_json;
    rule_json["description"] = "rule 4";
    krbn::core_configuration::details::complex_modifications_rule rule(rule_json, parameters);

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
    krbn::core_configuration::details::complex_modifications complex_modifications(json);
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
    krbn::core_configuration::details::complex_modifications complex_modifications(json);
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
    krbn::core_configuration::details::complex_modifications_parameters parameters(json);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1000);
  }

  // load values from json
  {
    nlohmann::json json;
    json["basic.to_if_alone_timeout_milliseconds"] = 1234;
    krbn::core_configuration::details::complex_modifications_parameters parameters(json);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1234);
  }

  // invalid values in json
  {
    nlohmann::json json;
    json["basic.to_if_alone_timeout_milliseconds"] = "1234";
    krbn::core_configuration::details::complex_modifications_parameters parameters(json);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1000);
  }

  // normalize
  {
    krbn::core_configuration::details::complex_modifications_parameters parameters(nlohmann::json::object({
        {"basic.simultaneous_threshold_milliseconds", -1000},
        {"basic.to_if_alone_timeout_milliseconds", -1000},
        {"basic.to_if_held_down_threshold_milliseconds", -1000},
        {"basic.to_delayed_action_delay_milliseconds", -1000},
    }));

    REQUIRE(parameters.get_basic_simultaneous_threshold_milliseconds() == 0);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 0);
    REQUIRE(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 0);
    REQUIRE(parameters.get_basic_to_delayed_action_delay_milliseconds() == 0);
  }
  {
    krbn::core_configuration::details::complex_modifications_parameters parameters(nlohmann::json::object({
        {"basic.simultaneous_threshold_milliseconds", 100000},
        {"basic.to_if_alone_timeout_milliseconds", 100000},
        {"basic.to_if_held_down_threshold_milliseconds", 100000},
        {"basic.to_delayed_action_delay_milliseconds", 100000},
    }));

    REQUIRE(parameters.get_basic_simultaneous_threshold_milliseconds() == 1000);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 100000);
    REQUIRE(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 100000);
    REQUIRE(parameters.get_basic_to_delayed_action_delay_milliseconds() == 100000);
  }
  {
    krbn::core_configuration::details::complex_modifications_parameters parameters(nlohmann::json::object({}));

    parameters.set_value("basic.simultaneous_threshold_milliseconds", -1000);
    parameters.set_value("basic.to_if_alone_timeout_milliseconds", -1000);
    parameters.set_value("basic.to_if_held_down_threshold_milliseconds", -1000);
    parameters.set_value("basic.to_delayed_action_delay_milliseconds", -1000);

    REQUIRE(parameters.get_basic_simultaneous_threshold_milliseconds() == 0);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 0);
    REQUIRE(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 0);
    REQUIRE(parameters.get_basic_to_delayed_action_delay_milliseconds() == 0);

    parameters.set_value("basic.simultaneous_threshold_milliseconds", 100000);
    parameters.set_value("basic.to_if_alone_timeout_milliseconds", 100000);
    parameters.set_value("basic.to_if_held_down_threshold_milliseconds", 100000);
    parameters.set_value("basic.to_delayed_action_delay_milliseconds", 100000);

    REQUIRE(parameters.get_basic_simultaneous_threshold_milliseconds() == 1000);
    REQUIRE(parameters.get_basic_to_if_alone_timeout_milliseconds() == 100000);
    REQUIRE(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 100000);
    REQUIRE(parameters.get_basic_to_delayed_action_delay_milliseconds() == 100000);
  }
}

TEST_CASE("complex_modifications.minmax_parameter_value") {
  {
    krbn::core_configuration::core_configuration configuration("json/minmax_parameter_value_test1.json");
    auto actual = configuration.get_selected_profile().get_complex_modifications().minmax_parameter_value("basic.simultaneous_threshold_milliseconds");
    REQUIRE(actual->first == 101);
    REQUIRE(actual->second == 401);
  }
  {
    krbn::core_configuration::core_configuration configuration("json/minmax_parameter_value_test2.json");
    auto actual = configuration.get_selected_profile().get_complex_modifications().minmax_parameter_value("basic.simultaneous_threshold_milliseconds");
    REQUIRE(actual->first == 102);
    REQUIRE(actual->second == 402);
  }
  {
    krbn::core_configuration::core_configuration configuration("json/minmax_parameter_value_test3.json");
    auto actual = configuration.get_selected_profile().get_complex_modifications().minmax_parameter_value("basic.simultaneous_threshold_milliseconds");
    REQUIRE(actual->first == 103);
    REQUIRE(actual->second == 403);
  }

  {
    krbn::core_configuration::core_configuration configuration("json/minmax_parameter_value_test1.json");
    REQUIRE(!configuration.get_selected_profile().get_complex_modifications().minmax_parameter_value("unknown"));
  }
}

TEST_CASE("virtual_hid_keyboard") {
  // empty json
  {
    auto json = nlohmann::json::object();
    krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.get_country_code() == krbn::hid_country_code(0));
  }

  // load values from json
  {
    nlohmann::json json({
        {"country_code", 10},
    });
    krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.get_country_code() == krbn::hid_country_code(10));
  }

  // invalid values in json
  {
    nlohmann::json json({
        {"country_code", nlohmann::json::object()},
    });
    REQUIRE_THROWS_AS(
        krbn::core_configuration::details::virtual_hid_keyboard(json),
        pqrs::json::unmarshal_error);
    REQUIRE_THROWS_WITH(
        krbn::core_configuration::details::virtual_hid_keyboard(json),
        "json must be number, but is `{}`");
  }
}

TEST_CASE("virtual_hid_keyboard.to_json") {
  {
    auto json = nlohmann::json::object();
    krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(nlohmann::json(virtual_hid_keyboard) == get_default_virtual_hid_keyboard_json());
  }
  {
    nlohmann::json json({
        {"dummy", {{"keep_me", true}}},
    });
    krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json);
    virtual_hid_keyboard.set_country_code(krbn::hid_country_code(10));
    virtual_hid_keyboard.set_mouse_key_xy_scale(50);

    nlohmann::json expected({
        {"country_code", 10},
        {"mouse_key_xy_scale", 50},
        {"dummy", {{"keep_me", true}}},
    });
    REQUIRE(nlohmann::json(virtual_hid_keyboard) == expected);
  }
}
