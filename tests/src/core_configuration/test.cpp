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
  {
    core_configuration configuration(logger::get_logger(), "json/broken.json");

    std::vector<std::pair<krbn::key_code, krbn::key_code>> expected;
    REQUIRE(configuration.get_current_profile_simple_modifications() == expected);
    REQUIRE(configuration.is_loaded() == false);
  }
  {
    core_configuration configuration(logger::get_logger(), "a.out");

    std::vector<std::pair<krbn::key_code, krbn::key_code>> expected;
    REQUIRE(configuration.get_current_profile_simple_modifications() == expected);
    REQUIRE(configuration.is_loaded() == false);
  }
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
    nlohmann::json json({
        {"check_for_updates_on_startup", false},
        {"show_in_menu_bar", false},
        {"show_profile_name_in_menu_bar", true},
    });
    core_configuration::global_configuration global_configuration(json);
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

    nlohmann::json actual = global_configuration;
    REQUIRE(actual == expected);
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
}

TEST_CASE("profile") {
  // empty json
  {
    nlohmann::json json;
    core_configuration::profile profile(json);
    REQUIRE(profile.get_name() == std::string(""));
    REQUIRE(profile.get_selected() == false);
    REQUIRE(profile.get_simple_modifications().size() == 0);
    REQUIRE(profile.get_fn_function_keys() == get_default_fn_function_keys_pairs());
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
    core_configuration::profile profile(json);
    REQUIRE(profile.get_name() == std::string("profile 1"));
    REQUIRE(profile.get_selected() == true);
    {
      std::vector<std::pair<std::string, std::string>> expected({
          {"from 1", "to 1"},
          {"from 2", "to 2"},
          {"from 3", "to 3"},
          {"from 10", "to 10"},
      });
      REQUIRE(profile.get_simple_modifications() == expected);
    }
    {
      auto expected = get_default_fn_function_keys_pairs();
      expected[2].second = "to f3";
      expected[3].second = "to f4";
      REQUIRE(profile.get_fn_function_keys() == expected);
    }
    {
      REQUIRE(profile.get_devices().size() == 2);
      REQUIRE((profile.get_devices())[0].get_identifiers().get_vendor_id() == 1234);
      REQUIRE((profile.get_devices())[0].get_identifiers().get_product_id() == 5678);
      REQUIRE((profile.get_devices())[0].get_ignore() == true);
      REQUIRE((profile.get_devices())[0].get_disable_built_in_keyboard_if_exists() == true);
      REQUIRE((profile.get_devices())[1].get_identifiers().get_vendor_id() == 4321);
      REQUIRE((profile.get_devices())[1].get_identifiers().get_product_id() == 8765);
      REQUIRE((profile.get_devices())[1].get_ignore() == false);
      REQUIRE((profile.get_devices())[1].get_disable_built_in_keyboard_if_exists() == true);
    }

    // set_device (existing identifiers)
    {
      auto identifiers = core_configuration::profile::device::identifiers(nlohmann::json({
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
      profile.set_device(identifiers, false, false);
      REQUIRE(profile.get_devices().size() == 2);
      REQUIRE((profile.get_devices())[0].get_ignore() == false);
      REQUIRE((profile.get_devices())[0].get_disable_built_in_keyboard_if_exists() == false);
    }
    // set_device (new identifiers)
    {
      auto identifiers = core_configuration::profile::device::identifiers(nlohmann::json({
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
      profile.set_device(identifiers, true, false);
      REQUIRE(profile.get_devices().size() == 3);
      REQUIRE((profile.get_devices())[2].get_identifiers().get_vendor_id() == 1111);
      REQUIRE((profile.get_devices())[2].get_identifiers().get_product_id() == 2222);
      REQUIRE((profile.get_devices())[2].get_identifiers().get_is_keyboard() == false);
      REQUIRE((profile.get_devices())[2].get_identifiers().get_is_pointing_device() == true);
      REQUIRE((profile.get_devices())[2].get_ignore() == true);
      REQUIRE((profile.get_devices())[2].get_disable_built_in_keyboard_if_exists() == false);
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
    core_configuration::profile profile(json);
    REQUIRE(profile.get_name() == std::string(""));
    REQUIRE(profile.get_selected() == false);
    REQUIRE(profile.get_simple_modifications().size() == 0);
    REQUIRE(profile.get_fn_function_keys() == get_default_fn_function_keys_pairs());
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
    core_configuration::profile profile(json);
    {
      std::vector<std::pair<std::string, std::string>> expected({
          {"key", "value"},
      });
      REQUIRE(profile.get_simple_modifications() == expected);
    }
  }
}

TEST_CASE("profile.to_json") {
  {
    nlohmann::json json;
    core_configuration::profile profile(json);
    nlohmann::json expected({
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
    core_configuration::profile profile(json);
    profile.set_name("profile 1");
    profile.set_selected(true);

    profile.push_back_simple_modification();
    // {
    //   "": ""
    // }

    profile.push_back_simple_modification();
    profile.replace_simple_modification(1, "from 1", "to 1");
    // {
    //   "": "",
    //   "from 1": "to 1"
    // }

    profile.push_back_simple_modification();
    profile.replace_simple_modification(2, "from 3", "to 3");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3"
    // }

    profile.push_back_simple_modification();
    profile.replace_simple_modification(3, "from 4", "to 4");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4"
    // }

    profile.push_back_simple_modification();
    profile.replace_simple_modification(4, "from 2", "to 2");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4",
    //   "from 2": "to 2"
    // }

    profile.push_back_simple_modification();
    profile.replace_simple_modification(5, "from 2", "to 2.0");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 3": "to 3",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0"
    // }

    profile.erase_simple_modification(2);
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0"
    // }

    profile.push_back_simple_modification();
    profile.replace_simple_modification(5, "", "to 0");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0",
    //   "": "to 0"
    // }

    profile.push_back_simple_modification();
    profile.replace_simple_modification(6, "from 0", "");
    // {
    //   "": "",
    //   "from 1": "to 1",
    //   "from 4": "to 4",
    //   "from 2": "to 2",
    //   "from 2": "to 2.0",
    //   "": "to 0",
    //   "from 0": ""
    // }

    profile.push_back_simple_modification();
    profile.replace_simple_modification(7, "from 5", "to 5");
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

    profile.replace_fn_function_keys("f3", "to f3");

    profile.get_virtual_hid_keyboard().set_keyboard_type("iso");

    auto expected_fn_function_keys = get_default_fn_function_keys_json();
    expected_fn_function_keys["f3"] = "to f3";
    auto expected_virtual_hid_keyboard = get_default_virtual_hid_keyboard_json();
    expected_virtual_hid_keyboard["keyboard_type"] = "iso";
    nlohmann::json expected({
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

TEST_CASE("virtual_hid_keyboard") {
  // empty json
  {
    nlohmann::json json;
    core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.get_keyboard_type() == std::string("ansi"));
    REQUIRE(virtual_hid_keyboard.get_caps_lock_delay_milliseconds() == 0);
  }

  // load values from json
  {
    nlohmann::json json({
        {"keyboard_type", "iso"},
        {"caps_lock_delay_milliseconds", 300},
    });
    core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.get_keyboard_type() == std::string("iso"));
    REQUIRE(virtual_hid_keyboard.get_caps_lock_delay_milliseconds() == 300);
  }

  // invalid values in json
  {
    nlohmann::json json({
        {"keyboard_type", nlohmann::json::array()},
        {"caps_lock_delay_milliseconds", nlohmann::json::object()},
    });
    core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.get_keyboard_type() == std::string("ansi"));
    REQUIRE(virtual_hid_keyboard.get_caps_lock_delay_milliseconds() == 0);
  }
}

TEST_CASE("virtual_hid_keyboard.to_json") {
  {
    nlohmann::json json;
    core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
    REQUIRE(virtual_hid_keyboard.to_json() == get_default_virtual_hid_keyboard_json());

    nlohmann::json actual = virtual_hid_keyboard;
    REQUIRE(actual == get_default_virtual_hid_keyboard_json());
  }
  {
    nlohmann::json json({
        {"dummy", {{"keep_me", true}}},
    });
    core_configuration::profile::virtual_hid_keyboard virtual_hid_keyboard(json);
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
    core_configuration::profile::device::identifiers identifiers(json);
    REQUIRE(identifiers.get_vendor_id() == 0);
    REQUIRE(identifiers.get_product_id() == 0);
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
    core_configuration::profile::device::identifiers identifiers(json);
    REQUIRE(identifiers.get_vendor_id() == 1234);
    REQUIRE(identifiers.get_product_id() == 5678);
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
    core_configuration::profile::device::identifiers identifiers(json);
    REQUIRE(identifiers.get_vendor_id() == 0);
    REQUIRE(identifiers.get_product_id() == 0);
    REQUIRE(identifiers.get_is_keyboard() == false);
    REQUIRE(identifiers.get_is_pointing_device() == false);
  }
}

TEST_CASE("device.identifiers.to_json") {
  {
    nlohmann::json json;
    core_configuration::profile::device::identifiers identifiers(json);
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
    core_configuration::profile::device::identifiers identifiers(json);
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
    core_configuration::profile::device device(json);
    REQUIRE(device.get_identifiers().get_vendor_id() == 0);
    REQUIRE(device.get_identifiers().get_product_id() == 0);
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
    core_configuration::profile::device device(json);
    REQUIRE(device.get_identifiers().get_vendor_id() == 1234);
    REQUIRE(device.get_identifiers().get_product_id() == 5678);
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
    core_configuration::profile::device device(json);
    REQUIRE(device.get_identifiers().get_vendor_id() == 0);
    REQUIRE(device.get_identifiers().get_product_id() == 0);
    REQUIRE(device.get_identifiers().get_is_keyboard() == false);
    REQUIRE(device.get_identifiers().get_is_pointing_device() == false);
    REQUIRE(device.get_ignore() == false);
    REQUIRE(device.get_disable_built_in_keyboard_if_exists() == false);
  }
}

TEST_CASE("device.to_json") {
  {
    nlohmann::json json;
    core_configuration::profile::device device(json);
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
    core_configuration::profile::device device(json);
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
        {"dummy", {{"keep_me", true}}},
    });
    REQUIRE(device.to_json() == expected);
  }
}

int main(int argc, char* const argv[]) {
  thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
