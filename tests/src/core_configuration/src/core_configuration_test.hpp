#include "core_configuration/core_configuration.hpp"
#include "json_utility.hpp"
#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>
#include <iostream>

namespace {
nlohmann::json get_default_fn_function_keys_json(void) {
  std::ifstream input("json/default_fn_function_keys.json");
  return krbn::json_utility::parse_jsonc(input);
}

nlohmann::json get_default_virtual_hid_keyboard_json(void) {
  return nlohmann::json{
      {"country_code", 0},
      {"indicate_sticky_modifier_keys_state", true},
      {"mouse_key_xy_scale", 100},
  };
}

std::vector<std::pair<std::string, std::string>> make_default_fn_function_keys_pairs(void) {
  std::vector<std::pair<std::string, std::string>> pairs;

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f1"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "display_brightness_decrement"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f2"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "display_brightness_increment"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f3"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"apple_vendor_keyboard_key_code", "mission_control"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f4"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"apple_vendor_keyboard_key_code", "spotlight"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f5"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "dictation"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f6"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"key_code", "f6"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f7"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "rewind"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f8"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "play_or_pause"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f9"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "fast_forward"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f10"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "mute"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f11"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "volume_decrement"}})}).dump());

  pairs.emplace_back(nlohmann::json::object({{"key_code", "f12"}}).dump(),
                     nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "volume_increment"}})}).dump());

  return pairs;
}
} // namespace

void run_core_configuration_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "valid"_test = [] {
    krbn::core_configuration::core_configuration configuration("json/example.jsonc", geteuid());

    {
      std::vector<std::pair<std::string, std::string>> expected;

      expected.emplace_back(nlohmann::json::object({{"key_code", "caps_lock"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"key_code", "delete_or_backspace"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "escape"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"key_code", "spacebar"}})}).dump());

      expect(configuration.get_selected_profile().get_simple_modifications().get_pairs() == expected);
    }
    {
      auto manipulator = configuration.get_selected_profile().get_complex_modifications().get_rules()[0].get_manipulators()[0].get_json();
      expect(manipulator["type"] == "basic");
      expect(manipulator["from"]["key_code"] == "open_bracket");
    }
    {
      std::vector<std::pair<std::string, std::string>> expected;

      expected.emplace_back(nlohmann::json::object({{"key_code", "f1"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"key_code", "escape"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f2"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "display_brightness_increment"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f3"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"apple_vendor_keyboard_key_code", "mission_control"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f4"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"apple_vendor_keyboard_key_code", "spotlight"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f5"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "dictation"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f6"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"key_code", "f6"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f7"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "rewind"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f8"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "play_or_pause"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f9"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "fast_forward"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f10"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "mute"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f11"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "volume_decrement"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "f12"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "volume_increment"}})}).dump());

      expect(configuration.get_selected_profile().get_fn_function_keys().get_pairs() == expected);
    }
    {
      auto& complex_modifications = configuration.get_selected_profile().get_complex_modifications();
      auto& rules = complex_modifications.get_rules();
      expect(complex_modifications.get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 800);
      expect(rules[0].get_manipulators()[0].get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 800);
      expect(rules[0].get_manipulators()[2].get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 400);
      expect(rules[0].get_description() == "Emacs bindings, etc.");
      expect(rules[1].get_description() == "description test");
      expect(rules[2].get_description() == "");
    }
    {
      expect(configuration
                 .get_selected_profile()
                 .get_virtual_hid_keyboard()
                 .get_country_code() == pqrs::hid::country_code::value_t(99));
    }
    {
      auto& actual = configuration.get_selected_profile().get_devices();
      expect(actual.size() == 3);

      expect(actual[0]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1133));
      expect(actual[0]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(50475));
      expect(actual[0]->get_identifiers().get_is_keyboard() == true);
      expect(actual[0]->get_identifiers().get_is_pointing_device() == false);
      expect(actual[0]->get_ignore() == false);
      expect(actual[0]->get_disable_built_in_keyboard_if_exists() == false);

      expect(actual[1]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1452));
      expect(actual[1]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(610));
      expect(actual[1]->get_identifiers().get_is_keyboard() == true);
      expect(actual[1]->get_identifiers().get_is_pointing_device() == false);
      expect(actual[1]->get_ignore() == true);
      expect(actual[1]->get_disable_built_in_keyboard_if_exists() == true);
    }

    expect(configuration.get_global_configuration().get_check_for_updates_on_startup() == false);
    expect(configuration.get_global_configuration().get_show_in_menu_bar() == false);
    expect(configuration.get_global_configuration().get_show_profile_name_in_menu_bar() == false);
    expect(configuration.get_global_configuration().get_ask_for_confirmation_before_quitting() == false);
    expect(configuration.get_global_configuration().get_unsafe_ui() == true);

    expect(configuration.is_loaded() == true);

    {
      std::ifstream input("json/to_json_example.json");
      auto expected = krbn::json_utility::parse_jsonc(input);
      expect(configuration.to_json() == expected) << "json/to_json_example.json is not match";
    }
  };

  "not found"_test = [] {
    {
      krbn::core_configuration::core_configuration configuration("json/not_found.json", geteuid());
      expect(configuration.get_selected_profile().get_name() == "Default profile");
      expect(configuration.is_loaded() == false);
    }
  };

  "broken.json"_test = [] {
    {
      krbn::core_configuration::core_configuration configuration("json/broken.json", geteuid());

      expect(configuration.get_selected_profile().get_simple_modifications().get_pairs().empty());
      expect(configuration.is_loaded() == false);

      expect(configuration.get_global_configuration().get_check_for_updates_on_startup() == true);
      expect(configuration.get_global_configuration().get_show_in_menu_bar() == true);
      expect(configuration.get_global_configuration().get_show_profile_name_in_menu_bar() == false);
      expect(configuration.get_global_configuration().get_ask_for_confirmation_before_quitting() == true);
      expect(configuration.get_global_configuration().get_unsafe_ui() == false);
      expect(configuration.get_profiles().size() == 1);
      expect((configuration.get_profiles())[0]->get_name() == "Default profile");
      expect((configuration.get_profiles())[0]->get_selected() == true);
      expect((configuration.get_profiles())[0]->get_fn_function_keys().get_pairs().size() == 12);

      {
        // to_json result is default json if is_loaded == false
        std::ifstream input("json/to_json_default.json");
        auto expected = krbn::json_utility::parse_jsonc(input);
        expect(configuration.to_json() == expected);
      }
    }
    {
      krbn::core_configuration::core_configuration configuration("/bin/ls", geteuid());

      expect(configuration.get_selected_profile().get_simple_modifications().get_pairs().empty());
      expect(configuration.is_loaded() == false);
    }
  };

  "invalid_key_code_name.json"_test = [] {
    krbn::core_configuration::core_configuration configuration("json/invalid_key_code_name.json", geteuid());

    std::vector<std::pair<std::string, std::string>> expected;

    expected.emplace_back(nlohmann::json::object({{"key_code", "caps_lock_2"}}).dump(),
                          nlohmann::json::array({nlohmann::json::object({{"key_code", "delete_or_backspace"}})}).dump());

    expected.emplace_back(nlohmann::json::object({{"key_code", "escape"}}).dump(),
                          nlohmann::json::array({nlohmann::json::object({{"key_code", "spacebar"}})}).dump());

    expect(configuration.get_selected_profile().get_simple_modifications().get_pairs() == expected);
    expect(configuration.is_loaded() == true);
  };

  "global_configuration.to_json"_test = [] {
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::global_configuration global_configuration(json);
      nlohmann::json expected({});
      expect(global_configuration.to_json() == expected);

      auto actual = global_configuration.to_json();
      expect(actual == expected);
    }
    {
      nlohmann::json json{
          {"dummy", {{"keep_me", true}}},
      };
      krbn::core_configuration::details::global_configuration global_configuration(json);
      global_configuration.set_check_for_updates_on_startup(false);
      global_configuration.set_show_in_menu_bar(false);
      global_configuration.set_show_profile_name_in_menu_bar(true);
      global_configuration.set_ask_for_confirmation_before_quitting(false);
      global_configuration.set_unsafe_ui(true);
      nlohmann::json expected({
          {"check_for_updates_on_startup", false},
          {"dummy", {{"keep_me", true}}},
          {"show_in_menu_bar", false},
          {"show_profile_name_in_menu_bar", true},
          {"ask_for_confirmation_before_quitting", false},
          {"unsafe_ui", true},
      });
      expect(global_configuration.to_json() == expected);
    }
  };

  "machine_specific.to_json"_test = [] {
    {
      krbn::core_configuration::core_configuration configuration("json/machine_specific.jsonc", geteuid());

      std::ifstream input("json/machine_specific.jsonc");
      auto expected = krbn::json_utility::parse_jsonc(input);

      {
        auto& e = configuration.get_machine_specific().get_entry(krbn::karabiner_machine_identifier("krbn-identifier1"));
        expect(true == e.get_enable_multitouch_extension());
      }

      auto json = configuration.to_json();
      expect(expected["machine_specific"] == json["machine_specific"]);
    }

    // from emtpy json
    {
      krbn::core_configuration::core_configuration configuration("", geteuid());

      auto json = configuration.to_json();
      expect(!json.contains("machine_specific"));

      // set value
      {
        auto& e = configuration.get_machine_specific().get_entry(krbn::karabiner_machine_identifier("krbn-identifier1"));
        e.set_enable_multitouch_extension(true);
      }

      json = configuration.to_json();
      expect(nlohmann::json::object({
                 {"krbn-identifier1", nlohmann::json::object({
                                          {"enable_multitouch_extension", true},
                                      })},
             }) == json["machine_specific"]);

      // set default value (omitted in to_json)
      {
        auto& e = configuration.get_machine_specific().get_entry(krbn::karabiner_machine_identifier("krbn-identifier1"));
        e.set_enable_multitouch_extension(false);
      }

      json = configuration.to_json();
      expect(!json.contains("machine_specific"));
    }
  };

  "profile"_test = [] {
    // empty json
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::profile profile(json);
      expect(profile.get_name() == std::string(""));
      expect(profile.get_selected() == false);
      expect(profile.get_simple_modifications().get_pairs().size() == 0);
      expect(profile.get_fn_function_keys().get_pairs() == make_default_fn_function_keys_pairs());
      expect(profile.get_devices().size() == 0);

      expect(profile.get_device_ignore(krbn::device_identifiers(pqrs::hid::vendor_id::value_t(4176),
                                                                pqrs::hid::product_id::value_t(1031),
                                                                true,  // is_keyboard
                                                                false, // is_pointing_device
                                                                false, // is_game_pad
                                                                ""     // device_address
                                                                )) == true);
      expect(profile.get_device_ignore(krbn::device_identifiers(pqrs::hid::vendor_id::value_t(0x05ac),
                                                                pqrs::hid::product_id::value_t(0x262),
                                                                true,  // is_keyboard
                                                                false, // is_pointing_device
                                                                false, // is_game_pad
                                                                ""     // device_address
                                                                )) == false);
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
                              {"treat_as_built_in_keyboard", false},
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
                              {"treat_as_built_in_keyboard", false},
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
                              {"treat_as_built_in_keyboard", false},
                          },
                      }},
      });
      krbn::core_configuration::details::profile profile(json);
      expect(profile.get_name() == std::string("profile 1"));
      expect(profile.get_selected() == true);
      {
        std::vector<std::pair<std::string, std::string>> expected;

        expected.emplace_back(nlohmann::json::object({{"key_code", "from 1"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "to 1"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "from 2"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "to 2"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "from 3"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "to 3"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "from 10"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "to 10"}})}).dump());

        expect(profile.get_simple_modifications().get_pairs() == expected);
      }
      {
        auto expected = make_default_fn_function_keys_pairs();
        expected[2].second = nlohmann::json::array({nlohmann::json::object({{"key_code", "to f3"}})}).dump();
        expected[3].second = nlohmann::json::array({nlohmann::json::object({{"key_code", "to f4"}})}).dump();
        expected.emplace_back(nlohmann::json::object({{"key_code", "f13"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "to f13"}})}).dump());
        expect(profile.get_fn_function_keys().get_pairs() == expected);
      }
      {
        expect(profile.get_devices().size() == 3);
        expect((profile.get_devices())[0]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
        expect((profile.get_devices())[0]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
        expect((profile.get_devices())[0]->get_ignore() == true);
        expect((profile.get_devices())[0]->get_disable_built_in_keyboard_if_exists() == true);
        expect((profile.get_devices())[1]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
        expect((profile.get_devices())[1]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
        expect((profile.get_devices())[1]->get_ignore() == true);
        expect((profile.get_devices())[1]->get_disable_built_in_keyboard_if_exists() == true);
        expect((profile.get_devices())[2]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(4321));
        expect((profile.get_devices())[2]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(8765));
        expect((profile.get_devices())[2]->get_ignore() == false);
        expect((profile.get_devices())[2]->get_disable_built_in_keyboard_if_exists() == true);
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
        expect(profile.get_devices().size() == 3);
        // devices[0] is changed.
        expect((profile.get_devices())[0]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
        expect((profile.get_devices())[0]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
        expect((profile.get_devices())[0]->get_ignore() == false);
        expect((profile.get_devices())[0]->get_disable_built_in_keyboard_if_exists() == true);
        // devices[1] is not changed.
        expect((profile.get_devices())[1]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
        expect((profile.get_devices())[1]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
        expect((profile.get_devices())[1]->get_ignore() == true);
        expect((profile.get_devices())[1]->get_disable_built_in_keyboard_if_exists() == true);
        expect((profile.get_devices())[2]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(4321));
        expect((profile.get_devices())[2]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(8765));
        expect((profile.get_devices())[2]->get_ignore() == false);
        expect((profile.get_devices())[2]->get_disable_built_in_keyboard_if_exists() == true);

        profile.set_device_disable_built_in_keyboard_if_exists(identifiers, false);
        expect(profile.get_devices().size() == 3);
        // devices[0] is changed.
        expect((profile.get_devices())[0]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
        expect((profile.get_devices())[0]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
        expect((profile.get_devices())[0]->get_ignore() == false);
        expect((profile.get_devices())[0]->get_disable_built_in_keyboard_if_exists() == false);
        // devices[1] is not changed.
        expect((profile.get_devices())[1]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
        expect((profile.get_devices())[1]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
        expect((profile.get_devices())[1]->get_ignore() == true);
        expect((profile.get_devices())[1]->get_disable_built_in_keyboard_if_exists() == true);
        expect((profile.get_devices())[2]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(4321));
        expect((profile.get_devices())[2]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(8765));
        expect((profile.get_devices())[2]->get_ignore() == false);
        expect((profile.get_devices())[2]->get_disable_built_in_keyboard_if_exists() == true);
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
          expect(profile.get_devices().size() == 4);
          expect((profile.get_devices())[3]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1111));
          expect((profile.get_devices())[3]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(2222));
          expect((profile.get_devices())[3]->get_identifiers().get_is_keyboard() == false);
          expect((profile.get_devices())[3]->get_identifiers().get_is_pointing_device() == true);
          expect((profile.get_devices())[3]->get_ignore() == true);
          expect((profile.get_devices())[3]->get_disable_built_in_keyboard_if_exists() == false);
        }

        {
          krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1112),
                                               pqrs::hid::product_id::value_t(2222),
                                               false,              // is_keyboard
                                               true,               // is_pointing_device
                                               false,              // is_game_pad
                                               "ec-ba-73-21-e6-f5" // device_address (ignored)
          );
          profile.set_device_disable_built_in_keyboard_if_exists(identifiers, true);
          expect(profile.get_devices().size() == 5);
          expect((profile.get_devices())[4]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1112));
          expect((profile.get_devices())[4]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(2222));
          expect((profile.get_devices())[4]->get_identifiers().get_device_address() == "");
          expect((profile.get_devices())[4]->get_identifiers().get_is_keyboard() == false);
          expect((profile.get_devices())[4]->get_identifiers().get_is_pointing_device() == true);
          expect((profile.get_devices())[4]->get_ignore() == true);
          expect((profile.get_devices())[4]->get_disable_built_in_keyboard_if_exists() == true);
        }

        {
          krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(0),
                                               pqrs::hid::product_id::value_t(0),
                                               false,              // is_keyboard
                                               true,               // is_pointing_device
                                               false,              // is_game_pad
                                               "ec-ba-73-21-e6-f5" // device_address
          );
          profile.set_device_disable_built_in_keyboard_if_exists(identifiers, true);
          expect(profile.get_devices().size() == 6);
          expect((profile.get_devices())[5]->get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
          expect((profile.get_devices())[5]->get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(0));
          expect((profile.get_devices())[5]->get_identifiers().get_device_address() == "ec-ba-73-21-e6-f5");
          expect((profile.get_devices())[5]->get_identifiers().get_is_keyboard() == false);
          expect((profile.get_devices())[5]->get_identifiers().get_is_pointing_device() == true);
          expect((profile.get_devices())[5]->get_ignore() == true);
          expect((profile.get_devices())[5]->get_disable_built_in_keyboard_if_exists() == true);
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

        expected.emplace_back(nlohmann::json::object({{"key_code", "key"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "value"}})}).dump());

        expect(profile.get_simple_modifications().get_pairs() == expected);
      }
    }
  };

  "profile.to_json"_test = [] {
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
          {"name", ""},
          {"selected", false},
          {"parameters", nlohmann::json::object({
                             {"delay_milliseconds_before_open_device", 1000},
                         })},
          {"simple_modifications", nlohmann::json::array()},
          {"fn_function_keys", get_default_fn_function_keys_json()},
          {"virtual_hid_keyboard", get_default_virtual_hid_keyboard_json()},
      });
      expect(profile.to_json() == expected);
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
                                                  {"device_address", "ec-ba-73-21-e6-f5"},
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
                              {"mouse_discard_horizontal_wheel", false},
                              {"mouse_discard_vertical_wheel", false},
                              {"mouse_discard_x", false},
                              {"mouse_discard_y", false},
                              {"mouse_flip_horizontal_wheel", false},
                              {"mouse_flip_vertical_wheel", false},
                              {"mouse_flip_x", false},
                              {"mouse_flip_y", false},
                              {"mouse_swap_xy", false},
                              {"mouse_swap_wheels", false},
                              {"game_pad_swap_sticks", false},
                              {"treat_as_built_in_keyboard", false},
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

      profile.get_virtual_hid_keyboard().set_country_code(pqrs::hid::country_code::value_t(20));
      profile.get_virtual_hid_keyboard().set_mouse_key_xy_scale(250);

      auto expected_fn_function_keys = get_default_fn_function_keys_json();
      expected_fn_function_keys[2]["to"][0]["key_code"] = "to f3";
      expected_fn_function_keys[2]["to"][0].erase("apple_vendor_keyboard_key_code"); // Remove expose_all
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
                                                  {"device_address", "ec-ba-73-21-e6-f5"},
                                                  {
                                                      "is_keyboard",
                                                      true,
                                                  },
                                                  {
                                                      "is_pointing_device",
                                                      true,
                                                  },
                                                  {
                                                      "is_game_pad",
                                                      false,
                                                  },
                                              }},
                              {"ignore", true},
                              {"disable_built_in_keyboard_if_exists", true},
                              {"fn_function_keys", nlohmann::json::array()},
                              {"game_pad_swap_sticks", false},
                              {"manipulate_caps_lock_led", false},
                              {"mouse_discard_horizontal_wheel", false},
                              {"mouse_discard_vertical_wheel", false},
                              {"mouse_discard_x", false},
                              {"mouse_discard_y", false},
                              {"mouse_flip_horizontal_wheel", false},
                              {"mouse_flip_vertical_wheel", false},
                              {"mouse_flip_x", false},
                              {"mouse_flip_y", false},
                              {"mouse_swap_wheels", false},
                              {"mouse_swap_xy", false},
                              {"simple_modifications", nlohmann::json::array()},
                              {"treat_as_built_in_keyboard", false},
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
      expected["simple_modifications"].back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "to 1"}})});
      expected["simple_modifications"].push_back(nlohmann::json::object());
      expected["simple_modifications"].back()["from"]["key_code"] = "from 4";
      expected["simple_modifications"].back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "to 4"}})});
      expected["simple_modifications"].push_back(nlohmann::json::object());
      expected["simple_modifications"].back()["from"]["key_code"] = "from 2";
      expected["simple_modifications"].back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "to 2"}})});
      expected["simple_modifications"].push_back(nlohmann::json::object());
      expected["simple_modifications"].back()["from"]["key_code"] = "from 5";
      expected["simple_modifications"].back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "to 5"}})});

      expect(profile.to_json() == expected);
    }
  };

  "simple_modifications"_test = [] {
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
      expect(simple_modifications.get_pairs().size() == 5);

      {
        std::vector<std::pair<std::string, std::string>> expected;

        expected.emplace_back(nlohmann::json::object({{"key_code", "a"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "f1"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "b"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "f2"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "dummy"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "f3"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "f4"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "dummy"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "f5"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", nlohmann::json()}})}).dump());

        expect(simple_modifications.get_pairs() == expected);
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
      expect(simple_modifications.get_pairs().size() == 4);

      {
        std::vector<std::pair<std::string, std::string>> expected;

        expected.emplace_back(nlohmann::json::object({{"key_code", "a"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "f1"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "b"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "f2"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "dummy"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "f3"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "f4"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "dummy"}})}).dump());

        expect(simple_modifications.get_pairs() == expected);
      }
    }

    // replace_pair
    {
      // v3 style
      krbn::core_configuration::details::simple_modifications simple_modifications;
      simple_modifications.push_back_pair();
      simple_modifications.replace_pair(0,
                                        "{ \"key_code\" : \"a\" }",
                                        "[{\"key_code\":\"a\"}]");
      expect(simple_modifications.get_pairs()[0].first == nlohmann::json::object({{"key_code", "a"}}).dump());
      expect(simple_modifications.get_pairs()[0].second == nlohmann::json::array({nlohmann::json::object({{"key_code", "a"}})}).dump());

      // v2 style
      simple_modifications.replace_pair(0,
                                        "{ \"key_code\" : \"a\" }",
                                        "{\"key_code\":\"b\"}");
      expect(simple_modifications.get_pairs()[0].first == nlohmann::json::object({{"key_code", "a"}}).dump());
      expect(simple_modifications.get_pairs()[0].second == nlohmann::json::array({nlohmann::json::object({{"key_code", "b"}})}).dump());

      // Invalid from json
      simple_modifications.replace_pair(0,
                                        "{ \"key_code\" : \"a\" }",
                                        "{\"}");
      expect(simple_modifications.get_pairs()[0].first == nlohmann::json::object({{"key_code", "a"}}).dump());
      expect(simple_modifications.get_pairs()[0].second == nlohmann::json::array({nlohmann::json::object({{"key_code", "b"}})}).dump());

      // Invalid to json
      simple_modifications.replace_pair(0,
                                        "{\"}",
                                        "{ \"key_code\" : \"c\" }");
      expect(simple_modifications.get_pairs()[0].first == nlohmann::json::object({{"key_code", "a"}}).dump());
      expect(simple_modifications.get_pairs()[0].second == nlohmann::json::array({nlohmann::json::object({{"key_code", "b"}})}).dump());
    }

    // replace_second
    {
      // v3 style
      krbn::core_configuration::details::simple_modifications simple_modifications;
      simple_modifications.push_back_pair();
      simple_modifications.replace_pair(0,
                                        "{ \"key_code\" : \"a\" }",
                                        "[{\"key_code\":\"a\"}]");
      expect(simple_modifications.get_pairs()[0].first == nlohmann::json::object({{"key_code", "a"}}).dump());
      expect(simple_modifications.get_pairs()[0].second == nlohmann::json::array({nlohmann::json::object({{"key_code", "a"}})}).dump());

      // v2 style
      simple_modifications.replace_pair(0,
                                        "{ \"key_code\" : \"a\" }",
                                        "{\"key_code\":\"b\"}");
      expect(simple_modifications.get_pairs()[0].first == nlohmann::json::object({{"key_code", "a"}}).dump());
      expect(simple_modifications.get_pairs()[0].second == nlohmann::json::array({nlohmann::json::object({{"key_code", "b"}})}).dump());

      simple_modifications.replace_second("{ \"key_code\" : \"a\" }",
                                          "[{\"key_code\":\"c\"}]");
      expect(simple_modifications.get_pairs()[0].first == nlohmann::json::object({{"key_code", "a"}}).dump());
      expect(simple_modifications.get_pairs()[0].second == nlohmann::json::array({nlohmann::json::object({{"key_code", "c"}})}).dump());

      simple_modifications.replace_second("{\"key_code\":\"a\"}",
                                          "[{\"key_code\":\"d\"}]");
      expect(simple_modifications.get_pairs()[0].first == nlohmann::json::object({{"key_code", "a"}}).dump());
      expect(simple_modifications.get_pairs()[0].second == nlohmann::json::array({nlohmann::json::object({{"key_code", "d"}})}).dump());

      // Invalid from json
      simple_modifications.replace_second("{\"}",
                                          "[{ \"key_code\" : \"a\" }]");

      // Invalid to json
      simple_modifications.replace_second("{ \"key_code\" : \"a\" }",
                                          "[{\"}]");
    }
  };

  "simple_modifications.to_json"_test = [] {
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
      expected.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "f1"}})});
      expected.push_back(nlohmann::json::object());
      expected.back()["from"]["key_code"] = "b";
      expected.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "f2"}})});
      expected.push_back(nlohmann::json::object());
      expected.back()["from"]["key_code"] = "dummy";
      expected.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "f3"}})});
      expected.push_back(nlohmann::json::object());
      expected.back()["from"]["key_code"] = "f4";
      expected.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "dummy"}})});
      expected.push_back(nlohmann::json::object());
      expected.back()["from"]["key_code"] = "c";
      expected.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "f6"}})});

      expect(simple_modifications.to_json() == expected);
    }
    {
      // simple_modifications.to_json have to be compatible with manipulator::event_definition

      auto json = nlohmann::json::array();
      json.push_back(nlohmann::json::object());
      json.back()["from"]["consumer_key_code"] = "mute";
      json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"pointing_button", "button3"}})});
      json.push_back(nlohmann::json::object());
      json.back()["from"]["key_code"] = "a";
      json.back()["to"] = nlohmann::json::array({nlohmann::json::object({{"key_code", "f1"}})});

      krbn::core_configuration::details::simple_modifications simple_modifications;
      simple_modifications.update(json);
      {
        krbn::manipulator::manipulators::basic::from_event_definition from_event_definition(krbn::json_utility::parse_jsonc(simple_modifications.get_pairs()[0].first));
        expect(from_event_definition.get_event_definitions().size() == 1);
        expect(from_event_definition.get_event_definitions().front().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::consumer,
                                     pqrs::hid::usage::consumer::mute));
      }
      {
        krbn::manipulator::to_event_definition to_event_definition(krbn::json_utility::parse_jsonc(simple_modifications.get_pairs()[0].second)[0]);
        expect(to_event_definition.get_event_definition().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::button,
                                     pqrs::hid::usage::button::button_3));
      }
      {
        krbn::manipulator::manipulators::basic::from_event_definition from_event_definition(krbn::json_utility::parse_jsonc(simple_modifications.get_pairs()[1].first));
        expect(from_event_definition.get_event_definitions().size() == 1);
        expect(from_event_definition.get_event_definitions().front().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_a));
      }
      {
        krbn::manipulator::to_event_definition to_event_definition(krbn::json_utility::parse_jsonc(simple_modifications.get_pairs()[1].second)[0]);
        expect(to_event_definition.get_event_definition().get_if<krbn::momentary_switch_event>()->get_usage_pair() ==
               pqrs::hid::usage_pair(pqrs::hid::usage_page::keyboard_or_keypad,
                                     pqrs::hid::usage::keyboard_or_keypad::keyboard_f1));
      }
    }
  };

  "complex_modifications"_test = [] {
    // empty json
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::complex_modifications complex_modifications(json);
      expect(complex_modifications.get_rules().empty());
      expect(complex_modifications.get_parameters().get_basic_to_if_alone_timeout_milliseconds() == 1000);
    }

    // load values from json
    {
      auto manipulators = nlohmann::json::array({
          nlohmann::json::object({
              {"from", nlohmann::json::object({{"key_code", "spacebar"}})},
              {"type", "basic"},
          }),
      });

      nlohmann::json json({
          {
              "rules",
              {
                  {
                      {"description", "rule 1"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 2"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 3"},
                      {"manipulators", manipulators},
                  },
              },
          },
      });
      krbn::core_configuration::details::complex_modifications complex_modifications(json);
      expect(complex_modifications.get_rules().size() == 3);
    }
  };

  "complex_modifications.push_back_rule"_test = [] {
    {
      auto manipulators = nlohmann::json::array({
          nlohmann::json::object({
              {"from", nlohmann::json::object({{"key_code", "spacebar"}})},
              {"type", "basic"},
          }),
      });

      nlohmann::json json({
          {
              "rules",
              {
                  {
                      {"description", "rule 1"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 2"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 3"},
                      {"manipulators", manipulators},
                  },
              },
          },
      });
      krbn::core_configuration::details::complex_modifications complex_modifications(json);
      auto& rules = complex_modifications.get_rules();
      expect(rules.size() == 3);
      expect(rules[0].get_description() == "rule 1");
      expect(rules[1].get_description() == "rule 2");
      expect(rules[2].get_description() == "rule 3");

      krbn::core_configuration::details::complex_modifications_parameters parameters;
      nlohmann::json rule_json;
      rule_json["description"] = "rule 4";
      rule_json["manipulators"] = manipulators;
      krbn::core_configuration::details::complex_modifications_rule rule(rule_json, parameters);

      complex_modifications.push_back_rule(rule);
      expect(rules.size() == 4);
      expect(rules[0].get_description() == "rule 1");
      expect(rules[1].get_description() == "rule 2");
      expect(rules[2].get_description() == "rule 3");
      expect(rules[3].get_description() == "rule 4");
    }
  };

  "complex_modifications.replace_rule"_test = [] {
    {
      auto manipulators = nlohmann::json::array({
          nlohmann::json::object({
              {"from", nlohmann::json::object({{"key_code", "spacebar"}})},
              {"type", "basic"},
          }),
      });

      nlohmann::json json({
          {
              "rules",
              {
                  {
                      {"description", "rule 1"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 2"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 3"},
                      {"manipulators", manipulators},
                  },
              },
          },
      });
      krbn::core_configuration::details::complex_modifications complex_modifications(json);
      auto& rules = complex_modifications.get_rules();
      expect(3 == rules.size());
      expect("rule 1"sv == rules[0].get_description());
      expect("rule 2"sv == rules[1].get_description());
      expect("rule 3"sv == rules[2].get_description());

      complex_modifications.replace_rule(1,
                                         krbn::core_configuration::details::complex_modifications_rule(
                                             nlohmann::json({
                                                 {"description", "replaced 2"},
                                                 {"manipulators", manipulators},
                                             }),
                                             krbn::core_configuration::details::complex_modifications_parameters()));
      expect(3 == rules.size());
      expect("rule 1"sv == rules[0].get_description());
      expect("replaced 2"sv == rules[1].get_description());
      expect("rule 3"sv == rules[2].get_description());

      complex_modifications.replace_rule(3,
                                         krbn::core_configuration::details::complex_modifications_rule(
                                             nlohmann::json({
                                                 {"description", "ignored"},
                                                 {"manipulators", manipulators},
                                             }),
                                             krbn::core_configuration::details::complex_modifications_parameters()));
      expect(3 == rules.size());
      expect("rule 1"sv == rules[0].get_description());
      expect("replaced 2"sv == rules[1].get_description());
      expect("rule 3"sv == rules[2].get_description());
    }
  };

  "complex_modifications.erase_rule"_test = [] {
    {
      auto manipulators = nlohmann::json::array({
          nlohmann::json::object({
              {"from", nlohmann::json::object({{"key_code", "spacebar"}})},
              {"type", "basic"},
          }),
      });

      nlohmann::json json({
          {
              "rules",
              {
                  {
                      {"description", "rule 1"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 2"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 3"},
                      {"manipulators", manipulators},
                  },
              },
          },
      });
      krbn::core_configuration::details::complex_modifications complex_modifications(json);
      auto& rules = complex_modifications.get_rules();
      expect(rules.size() == 3);
      expect(rules[0].get_description() == "rule 1");
      expect(rules[1].get_description() == "rule 2");
      expect(rules[2].get_description() == "rule 3");

      complex_modifications.erase_rule(0);
      expect(rules.size() == 2);
      expect(rules[0].get_description() == "rule 2");
      expect(rules[1].get_description() == "rule 3");

      complex_modifications.erase_rule(1);
      expect(rules.size() == 1);
      expect(rules[0].get_description() == "rule 2");

      complex_modifications.erase_rule(1);
      expect(rules.size() == 1);
    }
  };

  "complex_modifications.move_rule"_test = [] {
    {
      auto manipulators = nlohmann::json::array({
          nlohmann::json::object({
              {"from", nlohmann::json::object({{"key_code", "spacebar"}})},
              {"type", "basic"},
          }),
      });

      nlohmann::json json({
          {
              "rules",
              {
                  {
                      {"description", "rule 1"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 2"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 3"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 4"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 5"},
                      {"manipulators", manipulators},
                  },
              },
          },
      });

      {
        krbn::core_configuration::details::complex_modifications complex_modifications(json);
        auto& rules = complex_modifications.get_rules();
        expect(5_ul == rules.size());
        expect("rule 1" == rules[0].get_description());
        expect("rule 2" == rules[1].get_description());
        expect("rule 3" == rules[2].get_description());
        expect("rule 4" == rules[3].get_description());
        expect("rule 5" == rules[4].get_description());
      }

      // 0 -> 2
      {
        krbn::core_configuration::details::complex_modifications complex_modifications(json);
        auto& rules = complex_modifications.get_rules();
        complex_modifications.move_rule(0, 2);
        expect(5_ul == rules.size());
        expect("rule 2"sv == rules[0].get_description());
        expect("rule 1"sv == rules[1].get_description());
        expect("rule 3"sv == rules[2].get_description());
        expect("rule 4"sv == rules[3].get_description());
        expect("rule 5"sv == rules[4].get_description());
      }

      // 2 -> 0
      {
        krbn::core_configuration::details::complex_modifications complex_modifications(json);
        auto& rules = complex_modifications.get_rules();
        complex_modifications.move_rule(2, 0);
        expect(5_ul == rules.size());
        expect("rule 3" == rules[0].get_description());
        expect("rule 1" == rules[1].get_description());
        expect("rule 2" == rules[2].get_description());
        expect("rule 4" == rules[3].get_description());
        expect("rule 5" == rules[4].get_description());
      }
    }
  };

  "complex_modifications.parameters"_test = [] {
    // empty json
    {
      nlohmann::json json;
      krbn::core_configuration::details::complex_modifications_parameters parameters(json);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1000);
    }

    // load values from json
    {
      nlohmann::json json;
      json["basic.to_if_alone_timeout_milliseconds"] = 1234;
      krbn::core_configuration::details::complex_modifications_parameters parameters(json);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1234);
    }

    // invalid values in json
    {
      nlohmann::json json;
      json["basic.to_if_alone_timeout_milliseconds"] = "1234";
      krbn::core_configuration::details::complex_modifications_parameters parameters(json);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1000);
    }

    // normalize
    {
      krbn::core_configuration::details::complex_modifications_parameters parameters(nlohmann::json::object({
          {"basic.simultaneous_threshold_milliseconds", -1000},
          {"basic.to_if_alone_timeout_milliseconds", -1000},
          {"basic.to_if_held_down_threshold_milliseconds", -1000},
          {"basic.to_delayed_action_delay_milliseconds", -1000},
      }));

      expect(parameters.get_basic_simultaneous_threshold_milliseconds() == 0);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 0);
      expect(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 0);
      expect(parameters.get_basic_to_delayed_action_delay_milliseconds() == 0);
    }
    {
      krbn::core_configuration::details::complex_modifications_parameters parameters(nlohmann::json::object({
          {"basic.simultaneous_threshold_milliseconds", 100000},
          {"basic.to_if_alone_timeout_milliseconds", 100000},
          {"basic.to_if_held_down_threshold_milliseconds", 100000},
          {"basic.to_delayed_action_delay_milliseconds", 100000},
      }));

      expect(parameters.get_basic_simultaneous_threshold_milliseconds() == 1000);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 100000);
      expect(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 100000);
      expect(parameters.get_basic_to_delayed_action_delay_milliseconds() == 100000);
    }
    {
      krbn::core_configuration::details::complex_modifications_parameters parameters(nlohmann::json::object({}));

      parameters.set_value("basic.simultaneous_threshold_milliseconds", -1000);
      parameters.set_value("basic.to_if_alone_timeout_milliseconds", -1000);
      parameters.set_value("basic.to_if_held_down_threshold_milliseconds", -1000);
      parameters.set_value("basic.to_delayed_action_delay_milliseconds", -1000);

      expect(parameters.get_basic_simultaneous_threshold_milliseconds() == 0);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 0);
      expect(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 0);
      expect(parameters.get_basic_to_delayed_action_delay_milliseconds() == 0);

      parameters.set_value("basic.simultaneous_threshold_milliseconds", 100000);
      parameters.set_value("basic.to_if_alone_timeout_milliseconds", 100000);
      parameters.set_value("basic.to_if_held_down_threshold_milliseconds", 100000);
      parameters.set_value("basic.to_delayed_action_delay_milliseconds", 100000);

      expect(parameters.get_basic_simultaneous_threshold_milliseconds() == 1000);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 100000);
      expect(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 100000);
      expect(parameters.get_basic_to_delayed_action_delay_milliseconds() == 100000);
    }
  };

  "complex_modifications.minmax_parameter_value"_test = [] {
    {
      krbn::core_configuration::core_configuration configuration("json/minmax_parameter_value_test1.json", geteuid());
      auto actual = configuration.get_selected_profile().get_complex_modifications().minmax_parameter_value("basic.simultaneous_threshold_milliseconds");
      expect(actual->first == 101);
      expect(actual->second == 401);
    }
    {
      krbn::core_configuration::core_configuration configuration("json/minmax_parameter_value_test2.json", geteuid());
      auto actual = configuration.get_selected_profile().get_complex_modifications().minmax_parameter_value("basic.simultaneous_threshold_milliseconds");
      expect(actual->first == 102);
      expect(actual->second == 402);
    }
    {
      krbn::core_configuration::core_configuration configuration("json/minmax_parameter_value_test3.json", geteuid());
      auto actual = configuration.get_selected_profile().get_complex_modifications().minmax_parameter_value("basic.simultaneous_threshold_milliseconds");
      expect(actual->first == 103);
      expect(actual->second == 403);
    }

    {
      krbn::core_configuration::core_configuration configuration("json/minmax_parameter_value_test1.json", geteuid());
      expect(!configuration.get_selected_profile().get_complex_modifications().minmax_parameter_value("unknown"));
    }
  };

  "virtual_hid_keyboard"_test = [] {
    // empty json
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json);
      expect(virtual_hid_keyboard.get_country_code() == pqrs::hid::country_code::value_t(0));
    }

    // load values from json
    {
      nlohmann::json json({
          {"country_code", 10},
      });
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json);
      expect(virtual_hid_keyboard.get_country_code() == pqrs::hid::country_code::value_t(10));
    }

    // invalid values in json
    try {
      nlohmann::json json({
          {"country_code", nlohmann::json::object()},
      });
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json);
      expect(false);
    } catch (pqrs::json::unmarshal_error& ex) {
      expect(std::string_view("json must be number, but is `{}`") == ex.what());
    } catch (...) {
      expect(false);
    }
  };

  "virtual_hid_keyboard.to_json"_test = [] {
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json);
      expect(nlohmann::json(virtual_hid_keyboard) == get_default_virtual_hid_keyboard_json());
    }
    {
      nlohmann::json json({
          {"dummy", {{"keep_me", true}}},
      });
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json);
      virtual_hid_keyboard.set_country_code(pqrs::hid::country_code::value_t(10));
      virtual_hid_keyboard.set_mouse_key_xy_scale(50);

      nlohmann::json expected({
          {"country_code", 10},
          {"indicate_sticky_modifier_keys_state", true},
          {"mouse_key_xy_scale", 50},
          {"dummy", {{"keep_me", true}}},
      });
      expect(nlohmann::json(virtual_hid_keyboard) == expected);
    }
  };
}
