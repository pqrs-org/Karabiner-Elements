#include "../../share/ut_helper.hpp"
#include "core_configuration/core_configuration.hpp"
#include "json_utility.hpp"
#include "manipulator/manipulators/basic/basic.hpp"
#include "manipulator/types.hpp"
#include <boost/ut.hpp>
#include <iostream>

void run_core_configuration_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "valid"_test = [] {
    krbn::core_configuration::core_configuration configuration("json/example.jsonc",
                                                               geteuid(),
                                                               krbn::core_configuration::error_handling::strict);

    {
      std::vector<std::pair<std::string, std::string>> expected;

      expected.emplace_back(nlohmann::json::object({{"key_code", "caps_lock"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"key_code", "delete_or_backspace"}})}).dump());

      expected.emplace_back(nlohmann::json::object({{"key_code", "escape"}}).dump(),
                            nlohmann::json::array({nlohmann::json::object({{"key_code", "spacebar"}})}).dump());

      expect(expected == configuration.get_selected_profile().get_simple_modifications()->get_pairs()) << UT_SHOW_LINE;
    }
    {
      auto manipulator = configuration.get_selected_profile().get_complex_modifications()->get_rules()[0]->get_manipulators()[0]->to_json();
      expect("basic" == manipulator["type"]);
      expect("open_bracket" == manipulator["from"]["key_code"]);
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
                            nlohmann::json::array({nlohmann::json::object({{"generic_desktop", "do_not_disturb"}})}).dump());

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

      expect(expected == configuration.get_selected_profile().get_fn_function_keys()->get_pairs()) << UT_SHOW_LINE;
    }
    {
      auto complex_modifications = configuration.get_selected_profile().get_complex_modifications();
      auto& rules = complex_modifications->get_rules();
      expect(complex_modifications->get_parameters()->get_basic_to_if_alone_timeout_milliseconds() == 800);
      expect(rules[0]->get_manipulators()[0]->get_parameters()->get_basic_to_if_alone_timeout_milliseconds() == 800);
      expect(rules[0]->get_manipulators()[2]->get_parameters()->get_basic_to_if_alone_timeout_milliseconds() == 400);
      expect(rules[0]->get_description() == "Emacs bindings, etc.");
      expect(rules[1]->get_description() == "description test");
      expect(rules[2]->get_description() == "");
    }
    {
      expect("jis"s == configuration.get_selected_profile().get_virtual_hid_keyboard()->get_keyboard_type_v2());
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
    expect(configuration.get_global_configuration().get_show_additional_menu_items() == false);
    expect(configuration.get_global_configuration().get_ask_for_confirmation_before_quitting() == false);
    expect(configuration.get_global_configuration().get_unsafe_ui() == true);
    expect(configuration.get_global_configuration().get_filter_useless_events_from_specific_devices() == false);
    expect(configuration.get_global_configuration().get_reorder_same_timestamp_input_events_to_prioritize_modifiers() == false);
    expect(configuration.get_global_configuration().get_cgevent_double_click_interval_milliseconds() == 500);
    expect(configuration.get_global_configuration().get_cgevent_double_click_distance() == 4);

    expect(configuration.is_loaded() == true);

    {
      std::ifstream input("json/to_json_example.json");
      auto expected = krbn::json_utility::parse_jsonc(input);
      expect(configuration.to_json() == expected) << UT_SHOW_LINE;
    }
  };

  "not found"_test = [] {
    {
      krbn::core_configuration::core_configuration configuration("json/not_found.json",
                                                                 geteuid(),
                                                                 krbn::core_configuration::error_handling::strict);
      expect(configuration.get_selected_profile().get_name() == "Default profile");
      expect(configuration.is_loaded() == false);
    }
  };

  "broken.json"_test = [] {
    {
      krbn::core_configuration::core_configuration configuration("json/broken.json",
                                                                 geteuid(),
                                                                 krbn::core_configuration::error_handling::strict);

      expect(configuration.get_selected_profile().get_simple_modifications()->get_pairs().empty());
      expect(configuration.is_loaded() == false);
      expect(configuration.get_parse_error_message() == "[json.exception.parse_error.101] parse error at line 7, column 1: syntax error while parsing object key - unexpected end of input; expected string literal");

      expect(configuration.get_global_configuration().get_check_for_updates_on_startup() == true);
      expect(configuration.get_global_configuration().get_show_in_menu_bar() == true);
      expect(configuration.get_global_configuration().get_show_profile_name_in_menu_bar() == false);
      expect(configuration.get_global_configuration().get_show_additional_menu_items() == false);
      expect(configuration.get_global_configuration().get_ask_for_confirmation_before_quitting() == true);
      expect(configuration.get_global_configuration().get_unsafe_ui() == false);
      expect(configuration.get_global_configuration().get_filter_useless_events_from_specific_devices() == true);
      expect(configuration.get_global_configuration().get_reorder_same_timestamp_input_events_to_prioritize_modifiers() == true);
      expect(configuration.get_global_configuration().get_cgevent_double_click_interval_milliseconds() == 500);
      expect(configuration.get_global_configuration().get_cgevent_double_click_distance() == 4);
      expect(configuration.get_profiles().size() == 1);
      expect((configuration.get_profiles())[0]->get_name() == "Default profile");
      expect((configuration.get_profiles())[0]->get_selected() == true);
      expect((configuration.get_profiles())[0]->get_fn_function_keys()->get_pairs().size() == 12);

      {
        // to_json result is default json if is_loaded == false
        std::ifstream input("json/to_json_default.json");
        auto expected = krbn::json_utility::parse_jsonc(input);
        expect(configuration.to_json() == expected) << UT_SHOW_LINE;
      }
    }
    {
      krbn::core_configuration::core_configuration configuration("/bin/ls",
                                                                 geteuid(),
                                                                 krbn::core_configuration::error_handling::strict);

      expect(configuration.get_selected_profile().get_simple_modifications()->get_pairs().empty());
      expect(configuration.is_loaded() == false);
    }
  };

  "invalid_key_code_name.json"_test = [] {
    krbn::core_configuration::core_configuration configuration("json/invalid_key_code_name.json",
                                                               geteuid(),
                                                               krbn::core_configuration::error_handling::strict);

    std::vector<std::pair<std::string, std::string>> expected;

    expected.emplace_back(nlohmann::json::object({{"key_code", "caps_lock_2"}}).dump(),
                          nlohmann::json::array({nlohmann::json::object({{"key_code", "delete_or_backspace"}})}).dump());

    expected.emplace_back(nlohmann::json::object({{"key_code", "escape"}}).dump(),
                          nlohmann::json::array({nlohmann::json::object({{"key_code", "spacebar"}})}).dump());

    expect(configuration.get_selected_profile().get_simple_modifications()->get_pairs() == expected);
    expect(configuration.is_loaded() == true);
  };

  "global_configuration.to_json"_test = [] {
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::global_configuration global_configuration(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      nlohmann::json expected({});
      expect(global_configuration.to_json() == expected);

      auto actual = global_configuration.to_json();
      expect(actual == expected);
    }
    {
      nlohmann::json json{
          {"dummy", {{"keep_me", true}}},
      };
      krbn::core_configuration::details::global_configuration global_configuration(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      global_configuration.set_check_for_updates_on_startup(false);
      global_configuration.set_show_in_menu_bar(false);
      global_configuration.set_show_profile_name_in_menu_bar(true);
      global_configuration.set_show_additional_menu_items(true);
      global_configuration.set_ask_for_confirmation_before_quitting(false);
      global_configuration.set_unsafe_ui(true);
      global_configuration.set_filter_useless_events_from_specific_devices(false);
      global_configuration.set_reorder_same_timestamp_input_events_to_prioritize_modifiers(false);
      global_configuration.set_cgevent_double_click_interval_milliseconds(250);
      global_configuration.set_cgevent_double_click_distance(8);
      nlohmann::json expected({
          {"check_for_updates_on_startup", false},
          {"dummy", {{"keep_me", true}}},
          {"show_in_menu_bar", false},
          {"show_profile_name_in_menu_bar", true},
          {"show_additional_menu_items", true},
          {"ask_for_confirmation_before_quitting", false},
          {"unsafe_ui", true},
          {"filter_useless_events_from_specific_devices", false},
          {"reorder_same_timestamp_input_events_to_prioritize_modifiers", false},
          {"cgevent_double_click_interval_milliseconds", 250},
          {"cgevent_double_click_distance", 8},
      });
      expect(global_configuration.to_json() == expected);
    }
  };

  "machine_specific.to_json"_test = [] {
    {
      krbn::core_configuration::core_configuration configuration("json/machine_specific.jsonc",
                                                                 geteuid(),
                                                                 krbn::core_configuration::error_handling::strict);

      std::ifstream input("json/machine_specific.jsonc");
      auto expected = krbn::json_utility::parse_jsonc(input);

      {
        auto& e = configuration.get_machine_specific().get_entry(krbn::karabiner_machine_identifier("krbn-identifier1"));
        expect(true == e.get_enable_multitouch_extension());
        expect(std::string("/Applications/Visual Studio Code.app") ==
               e.get_external_editor_path());
      }

      auto json = configuration.to_json();
      expect(expected["machine_specific"] == json["machine_specific"]);
    }

    // from emtpy json
    {
      krbn::core_configuration::core_configuration configuration("",
                                                                 geteuid(),
                                                                 krbn::core_configuration::error_handling::strict);

      auto json = configuration.to_json();
      expect(!json.contains("machine_specific"));

      // set value
      {
        auto& e = configuration.get_machine_specific().get_entry(krbn::karabiner_machine_identifier("krbn-identifier1"));
        e.set_enable_multitouch_extension(true);
        e.set_external_editor_path("/Applications/CotEditor.app");
      }

      json = configuration.to_json();
      expect(nlohmann::json::object({
                 {"krbn-identifier1", nlohmann::json::object({
                                          {"enable_multitouch_extension", true},
                                          {"external_editor_path", "/Applications/CotEditor.app"},
                                      })},
             }) == json["machine_specific"]);

      // set default value (omitted in to_json)
      {
        auto& e = configuration.get_machine_specific().get_entry(krbn::karabiner_machine_identifier("krbn-identifier1"));
        e.set_enable_multitouch_extension(false);
        e.set_external_editor_path("/System/Applications/TextEdit.app");
      }

      json = configuration.to_json();
      expect(!json.contains("machine_specific"));
    }
  };

  "profile"_test = [] {
    // empty json
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::profile profile(json,
                                                         krbn::core_configuration::error_handling::strict);
      expect(profile.get_name() == std::string(""));
      expect(profile.get_selected() == false);
      expect(profile.get_simple_modifications()->get_pairs().empty());
      expect(profile.get_fn_function_keys()->get_pairs().size() == 12);
      expect(profile.get_devices().size() == 0);

      expect(profile.get_device(krbn::device_identifiers(pqrs::hid::vendor_id::value_t(4176),
                                                         pqrs::hid::product_id::value_t(1031),
                                                         true,  // is_keyboard
                                                         false, // is_pointing_device
                                                         false, // is_game_pad
                                                         false, // is_consumer
                                                         false, // is_virtual_device
                                                         ""     // device_address
                                                         ))
                 ->get_ignore() == true);
      expect(profile.get_device(krbn::device_identifiers(pqrs::hid::vendor_id::value_t(0x05ac),
                                                         pqrs::hid::product_id::value_t(0x262),
                                                         true,  // is_keyboard
                                                         false, // is_pointing_device
                                                         false, // is_game_pad
                                                         false, // is_consumer
                                                         false, // is_virtual_device
                                                         ""     // device_address
                                                         ))
                 ->get_ignore() == false);
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
      krbn::core_configuration::details::profile profile(json,
                                                         krbn::core_configuration::error_handling::strict);
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

        expect(profile.get_simple_modifications()->get_pairs() == expected) << UT_SHOW_LINE;
      }
      {
        std::vector<std::pair<std::string, std::string>> expected;
        expected.emplace_back(nlohmann::json::object({{"key_code", "f1"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "display_brightness_decrement"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "f2"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "display_brightness_increment"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "f3"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "to f3"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "f4"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "to f4"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "f5"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"consumer_key_code", "dictation"}})}).dump());

        expected.emplace_back(nlohmann::json::object({{"key_code", "f6"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"generic_desktop", "do_not_disturb"}})}).dump());

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

        expected.emplace_back(nlohmann::json::object({{"key_code", "f13"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "to f13"}})}).dump());

        expect(expected == profile.get_fn_function_keys()->get_pairs()) << UT_SHOW_LINE;
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
        profile.get_device(identifiers)->set_ignore(false);
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

        profile.get_device(identifiers)->set_disable_built_in_keyboard_if_exists(false);
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
          profile.get_device(identifiers)->set_ignore(true);
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
                                               false,              // is_consumer
                                               false,              // is_virtual_device
                                               "ec-ba-73-21-e6-f5" // device_address (ignored)
          );
          profile.get_device(identifiers)->set_disable_built_in_keyboard_if_exists(true);
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
                                               false,              // is_consumer
                                               false,              // is_virtual_device
                                               "ec-ba-73-21-e6-f5" // device_address
          );
          profile.get_device(identifiers)->set_disable_built_in_keyboard_if_exists(true);
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
      krbn::core_configuration::details::profile profile(json,
                                                         krbn::core_configuration::error_handling::strict);
      {
        std::vector<std::pair<std::string, std::string>> expected;

        expected.emplace_back(nlohmann::json::object({{"key_code", "key"}}).dump(),
                              nlohmann::json::array({nlohmann::json::object({{"key_code", "value"}})}).dump());

        expect(expected == profile.get_simple_modifications()->get_pairs()) << UT_SHOW_LINE;
      }
    }
  };

  "profile.to_json"_test = [] {
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::profile empty_profile(json,
                                                               krbn::core_configuration::error_handling::strict);
      expect(json == empty_profile.to_json()) << UT_SHOW_LINE;
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
                              {"treat_as_built_in_keyboard", false},
                          },
                      }},
      });
      krbn::core_configuration::details::profile profile(json,
                                                         krbn::core_configuration::error_handling::strict);
      profile.set_name("profile 1");
      profile.set_selected(true);

      profile.get_parameters()->set_delay_milliseconds_before_open_device(std::chrono::milliseconds(500));

      profile.get_simple_modifications()->push_back_pair();
      // {
      //   "": ""
      // }

      profile.get_simple_modifications()->push_back_pair();
      profile.get_simple_modifications()->replace_pair(1,
                                                       nlohmann::json{{"key_code", "from 1"}}.dump(),
                                                       nlohmann::json{{"key_code", "to 1"}}.dump());
      // {
      //   "": "",
      //   "from 1": "to 1"
      // }

      profile.get_simple_modifications()->push_back_pair();
      profile.get_simple_modifications()->replace_pair(2,
                                                       nlohmann::json{{"key_code", "from 3"}}.dump(),
                                                       nlohmann::json{{"key_code", "to 3"}}.dump());
      // {
      //   "": "",
      //   "from 1": "to 1",
      //   "from 3": "to 3"
      // }

      profile.get_simple_modifications()->push_back_pair();
      profile.get_simple_modifications()->replace_pair(3,
                                                       nlohmann::json{{"key_code", "from 4"}}.dump(),
                                                       nlohmann::json{{"key_code", "to 4"}}.dump());
      // {
      //   "": "",
      //   "from 1": "to 1",
      //   "from 3": "to 3",
      //   "from 4": "to 4"
      // }

      profile.get_simple_modifications()->push_back_pair();
      profile.get_simple_modifications()->replace_pair(4,
                                                       nlohmann::json{{"key_code", "from 2"}}.dump(),
                                                       nlohmann::json{{"key_code", "to 2"}}.dump());
      // {
      //   "": "",
      //   "from 1": "to 1",
      //   "from 3": "to 3",
      //   "from 4": "to 4",
      //   "from 2": "to 2"
      // }

      profile.get_simple_modifications()->push_back_pair();
      profile.get_simple_modifications()->replace_pair(5,
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

      profile.get_simple_modifications()->erase_pair(2);
      // {
      //   "": "",
      //   "from 1": "to 1",
      //   "from 4": "to 4",
      //   "from 2": "to 2",
      //   "from 2": "to 2.0"
      // }

      profile.get_simple_modifications()->push_back_pair();
      profile.get_simple_modifications()->replace_pair(5,
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

      profile.get_simple_modifications()->push_back_pair();
      profile.get_simple_modifications()->replace_pair(6,
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

      profile.get_simple_modifications()->push_back_pair();
      profile.get_simple_modifications()->replace_pair(7,
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

      profile.get_fn_function_keys()->replace_second(nlohmann::json{{"key_code", "f3"}}.dump(),
                                                     nlohmann::json{{"key_code", "to f3"}}.dump());
      profile.get_fn_function_keys()->replace_second(nlohmann::json{{"key_code", "not found"}}.dump(),
                                                     nlohmann::json{{"key_code", "do nothing"}}.dump());

      profile.get_virtual_hid_keyboard()->set_keyboard_type_v2("ansi");
      profile.get_virtual_hid_keyboard()->set_mouse_key_xy_scale(250);

      auto expected = R"(

{
  "devices": [
    {
      "identifiers": {
        "vendor_id": 1234,
        "product_id": 5678,
        "device_address": "ec-ba-73-21-e6-f5",
        "is_keyboard": true,
        "is_pointing_device": true
      },
      "disable_built_in_keyboard_if_exists": true,
      "manipulate_caps_lock_led": false
    }
  ],
  "dummy": {
    "keep_me": true
  },
  "name": "profile 1",
  "selected": true,
  "parameters": {
    "delay_milliseconds_before_open_device": 500
  },
  "simple_modifications": [],
  "fn_function_keys": [
    {
      "from": {"key_code": "f3"},
      "to": [
        {"key_code": "to f3"}
      ]
    }
  ],
  "virtual_hid_keyboard": {
    "keyboard_type_v2": "ansi",
    "mouse_key_xy_scale": 250
  }
}

)"_json;

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

      expect(expected == profile.to_json()) << UT_SHOW_LINE;
    }
  };

  "profile.erase_not_connected_configured_devices"_test = [] {
    auto json = R"(

{
  "devices": [
    {
      "identifiers": {
        "vendor_id": 1234,
        "product_id": 1000,
        "is_keyboard": true
      },
      "ignore": true
    },
    {
      "identifiers": {
        "vendor_id": 1234,
        "product_id": 1001,
        "is_keyboard": true
      },
      "ignore": true
    },
    {
      "identifiers": {
        "vendor_id": 1234,
        "product_id": 1002,
        "is_keyboard": true
      },
      "ignore": true
    },
    {
      "identifiers": {
        "vendor_id": 1234,
        "product_id": 1003,
        "is_keyboard": true
      }
    }
  ]
}

)"_json;

    krbn::core_configuration::details::profile profile(json,
                                                       krbn::core_configuration::error_handling::strict);

    expect(4 == profile.get_devices().size());

    krbn::connected_devices connected_devices;

    connected_devices.push_back_device(std::make_shared<krbn::device_properties>(krbn::device_properties::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(1234),
        .product_id = pqrs::hid::product_id::value_t(1001),
        .is_keyboard = true,
    }));

    // Note: product_id:1003 is not counted because it remains in its default settings.
    expect(2 == profile.not_connected_configured_devices_count(connected_devices));

    profile.erase_not_connected_configured_devices(connected_devices);

    expect(2 == profile.get_devices().size());
    expect(pqrs::hid::product_id::value_t(1001) == profile.get_devices()[0]->get_identifiers().get_product_id());
    expect(pqrs::hid::product_id::value_t(1003) == profile.get_devices()[1]->get_identifiers().get_product_id());
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

      expect(expected == simple_modifications.to_json(nlohmann::json::array()));
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
      krbn::core_configuration::details::complex_modifications complex_modifications(json,
                                                                                     krbn::core_configuration::error_handling::strict);
      expect(complex_modifications.get_rules().empty());
      expect(complex_modifications.get_parameters()->get_basic_to_if_alone_timeout_milliseconds() == 1000);

      expect(json == complex_modifications.to_json()) << UT_SHOW_LINE;
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
      krbn::core_configuration::details::complex_modifications complex_modifications(json,
                                                                                     krbn::core_configuration::error_handling::strict);
      expect(complex_modifications.get_rules().size() == 3);

      expect(json == complex_modifications.to_json()) << UT_SHOW_LINE;
    }
  };

  "complex_modifications.push_front_rule"_test = [] {
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
      krbn::core_configuration::details::complex_modifications complex_modifications(json,
                                                                                     krbn::core_configuration::error_handling::strict);
      auto& rules = complex_modifications.get_rules();
      expect(rules.size() == 3);
      expect(rules[0]->get_description() == "rule 1");
      expect(rules[1]->get_description() == "rule 2");
      expect(rules[2]->get_description() == "rule 3");

      nlohmann::json rule_json;
      rule_json["description"] = "rule 4";
      rule_json["manipulators"] = manipulators;
      auto rule = std::make_shared<krbn::core_configuration::details::complex_modifications_rule>(
          rule_json,
          complex_modifications.get_parameters(),
          krbn::core_configuration::error_handling::strict);

      complex_modifications.push_front_rule(rule);
      expect(rules.size() == 4);
      expect(rules[0]->get_description() == "rule 4");
      expect(rules[1]->get_description() == "rule 1");
      expect(rules[2]->get_description() == "rule 2");
      expect(rules[3]->get_description() == "rule 3");

      nlohmann::json expected_json({
          {
              "rules",
              {
                  {
                      {"description", "rule 4"},
                      {"manipulators", manipulators},
                  },
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

      expect(expected_json == complex_modifications.to_json()) << UT_SHOW_LINE;
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
      krbn::core_configuration::details::complex_modifications complex_modifications(json,
                                                                                     krbn::core_configuration::error_handling::strict);
      auto& rules = complex_modifications.get_rules();
      expect(3 == rules.size());
      expect("rule 1"sv == rules[0]->get_description());
      expect("rule 2"sv == rules[1]->get_description());
      expect("rule 3"sv == rules[2]->get_description());

      {
        auto rule = std::make_shared<krbn::core_configuration::details::complex_modifications_rule>(
            nlohmann::json({
                {"description", "replaced 2"},
                {"manipulators", manipulators},
            }),
            complex_modifications.get_parameters(),
            krbn::core_configuration::error_handling::strict);
        complex_modifications.replace_rule(1, rule);
        expect(3 == rules.size());
        expect("rule 1"sv == rules[0]->get_description());
        expect("replaced 2"sv == rules[1]->get_description());
        expect("rule 3"sv == rules[2]->get_description());
      }

      {
        auto rule = std::make_shared<krbn::core_configuration::details::complex_modifications_rule>(
            nlohmann::json({
                {"description", "ignored"},
                {"manipulators", manipulators},
            }),
            complex_modifications.get_parameters(),
            krbn::core_configuration::error_handling::strict);
        complex_modifications.replace_rule(3, rule);
        expect(3 == rules.size());
        expect("rule 1"sv == rules[0]->get_description());
        expect("replaced 2"sv == rules[1]->get_description());
        expect("rule 3"sv == rules[2]->get_description());
      }

      nlohmann::json expected_json({
          {
              "rules",
              {
                  {
                      {"description", "rule 1"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "replaced 2"},
                      {"manipulators", manipulators},
                  },
                  {
                      {"description", "rule 3"},
                      {"manipulators", manipulators},
                  },
              },
          },
      });

      expect(expected_json == complex_modifications.to_json()) << UT_SHOW_LINE;
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
      krbn::core_configuration::details::complex_modifications complex_modifications(json,
                                                                                     krbn::core_configuration::error_handling::strict);
      auto& rules = complex_modifications.get_rules();
      expect(rules.size() == 3);
      expect(rules[0]->get_description() == "rule 1");
      expect(rules[1]->get_description() == "rule 2");
      expect(rules[2]->get_description() == "rule 3");

      complex_modifications.erase_rule(0);
      expect(rules.size() == 2);
      expect(rules[0]->get_description() == "rule 2");
      expect(rules[1]->get_description() == "rule 3");

      complex_modifications.erase_rule(1);
      expect(rules.size() == 1);
      expect(rules[0]->get_description() == "rule 2");

      complex_modifications.erase_rule(1);
      expect(rules.size() == 1);

      {
        nlohmann::json expected_json({
            {
                "rules",
                {
                    {
                        {"description", "rule 2"},
                        {"manipulators", manipulators},
                    },
                },
            },
        });
        expect(expected_json == complex_modifications.to_json()) << UT_SHOW_LINE;
      }

      complex_modifications.erase_rule(0);
      expect(rules.size() == 0);

      {
        auto expected_json = "{}"_json;
        expect(expected_json == complex_modifications.to_json()) << UT_SHOW_LINE;
      }
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
        krbn::core_configuration::details::complex_modifications complex_modifications(json,
                                                                                       krbn::core_configuration::error_handling::strict);
        auto& rules = complex_modifications.get_rules();
        expect(5_ul == rules.size());
        expect("rule 1" == rules[0]->get_description());
        expect("rule 2" == rules[1]->get_description());
        expect("rule 3" == rules[2]->get_description());
        expect("rule 4" == rules[3]->get_description());
        expect("rule 5" == rules[4]->get_description());
      }

      // 0 -> 2
      {
        krbn::core_configuration::details::complex_modifications complex_modifications(json,
                                                                                       krbn::core_configuration::error_handling::strict);
        auto& rules = complex_modifications.get_rules();
        complex_modifications.move_rule(0, 2);
        expect(5_ul == rules.size());
        expect("rule 2"sv == rules[0]->get_description());
        expect("rule 1"sv == rules[1]->get_description());
        expect("rule 3"sv == rules[2]->get_description());
        expect("rule 4"sv == rules[3]->get_description());
        expect("rule 5"sv == rules[4]->get_description());
      }

      // 2 -> 0
      {
        krbn::core_configuration::details::complex_modifications complex_modifications(json,
                                                                                       krbn::core_configuration::error_handling::strict);
        auto& rules = complex_modifications.get_rules();
        complex_modifications.move_rule(2, 0);
        expect(5_ul == rules.size());
        expect("rule 3" == rules[0]->get_description());
        expect("rule 1" == rules[1]->get_description());
        expect("rule 2" == rules[2]->get_description());
        expect("rule 4" == rules[3]->get_description());
        expect("rule 5" == rules[4]->get_description());
      }
    }
  };

  "complex_modifications.parameters"_test = [] {
    // empty json
    {
      auto json = "{}"_json;
      krbn::core_configuration::details::complex_modifications_parameters parameters(json,
                                                                                     krbn::core_configuration::error_handling::strict);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1000);
    }

    // load values from json
    {
      nlohmann::json json;
      json["basic.to_if_alone_timeout_milliseconds"] = 1234;
      krbn::core_configuration::details::complex_modifications_parameters parameters(json,
                                                                                     krbn::core_configuration::error_handling::strict);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1234);
    }

    // invalid values in json
    {
      nlohmann::json json;
      json["basic.to_if_alone_timeout_milliseconds"] = "1234";
      krbn::core_configuration::details::complex_modifications_parameters parameters(json,
                                                                                     krbn::core_configuration::error_handling::loose);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 1000);
    }

    // normalize
    {
      krbn::core_configuration::details::complex_modifications_parameters parameters(
          nlohmann::json::object({
              {"basic.simultaneous_threshold_milliseconds", -1000},
              {"basic.to_if_alone_timeout_milliseconds", -1000},
              {"basic.to_if_held_down_threshold_milliseconds", -1000},
              {"basic.to_delayed_action_delay_milliseconds", -1000},
          }),
          krbn::core_configuration::error_handling::strict);

      expect(parameters.get_basic_simultaneous_threshold_milliseconds() == 0);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 0);
      expect(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 0);
      expect(parameters.get_basic_to_delayed_action_delay_milliseconds() == 0);
    }
    {
      krbn::core_configuration::details::complex_modifications_parameters parameters(
          nlohmann::json::object({
              {"basic.simultaneous_threshold_milliseconds", 100000},
              {"basic.to_if_alone_timeout_milliseconds", 100000},
              {"basic.to_if_held_down_threshold_milliseconds", 100000},
              {"basic.to_delayed_action_delay_milliseconds", 100000},
          }),
          krbn::core_configuration::error_handling::strict);

      expect(parameters.get_basic_simultaneous_threshold_milliseconds() == 1000);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 100000);
      expect(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 100000);
      expect(parameters.get_basic_to_delayed_action_delay_milliseconds() == 100000);
    }
    {
      krbn::core_configuration::details::complex_modifications_parameters parameters(
          nlohmann::json::object({}),
          krbn::core_configuration::error_handling::strict);

      parameters.set_basic_simultaneous_threshold_milliseconds(-1000);
      parameters.set_basic_to_if_alone_timeout_milliseconds(-1000);
      parameters.set_basic_to_if_held_down_threshold_milliseconds(-1000);
      parameters.set_basic_to_delayed_action_delay_milliseconds(-1000);

      expect(parameters.get_basic_simultaneous_threshold_milliseconds() == 0);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 0);
      expect(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 0);
      expect(parameters.get_basic_to_delayed_action_delay_milliseconds() == 0);

      parameters.set_basic_simultaneous_threshold_milliseconds(100000);
      parameters.set_basic_to_if_alone_timeout_milliseconds(100000);
      parameters.set_basic_to_if_held_down_threshold_milliseconds(100000);
      parameters.set_basic_to_delayed_action_delay_milliseconds(100000);

      expect(parameters.get_basic_simultaneous_threshold_milliseconds() == 1000);
      expect(parameters.get_basic_to_if_alone_timeout_milliseconds() == 100000);
      expect(parameters.get_basic_to_if_held_down_threshold_milliseconds() == 100000);
      expect(parameters.get_basic_to_delayed_action_delay_milliseconds() == 100000);
    }
  };

  "virtual_hid_keyboard"_test = [] {
    // empty json
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect(""s == virtual_hid_keyboard.get_keyboard_type_v2());
    }

    // load values from json
    {
      nlohmann::json json({
          {"keyboard_type_v2", "ansi"},
      });
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect("ansi"s == virtual_hid_keyboard.get_keyboard_type_v2());
    }

    // invalid values in json
    try {
      nlohmann::json json({
          {"keyboard_type_v2", nlohmann::json::object()},
      });
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect(false);
    } catch (pqrs::json::unmarshal_error& ex) {
      expect(std::string_view("`keyboard_type_v2` must be array of string, or string, but is `{}`") == ex.what());
    } catch (...) {
      expect(false);
    }
  };

  "virtual_hid_keyboard.to_json"_test = [] {
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      expect(json == virtual_hid_keyboard.to_json()) << UT_SHOW_LINE;
    }
    {
      nlohmann::json json({
          {"dummy", {{"keep_me", true}}},
      });
      krbn::core_configuration::details::virtual_hid_keyboard virtual_hid_keyboard(json,
                                                                                   krbn::core_configuration::error_handling::strict);
      virtual_hid_keyboard.set_keyboard_type_v2("iso");
      virtual_hid_keyboard.set_mouse_key_xy_scale(50);

      auto expected = R"(

{
  "dummy": {
    "keep_me": true
  },
  "keyboard_type_v2": "iso",
  "mouse_key_xy_scale": 50
}

      )"_json;
      expect(expected == virtual_hid_keyboard.to_json()) << UT_SHOW_LINE;
    }
  };
}
