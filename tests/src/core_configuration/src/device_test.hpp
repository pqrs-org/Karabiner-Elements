#include "core_configuration/core_configuration.hpp"
#include <boost/ut.hpp>

void run_device_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "device.identifiers"_test = [] {
    // empty json
    {
      auto json = nlohmann::json::object();
      auto identifiers = json.get<krbn::device_identifiers>();
      expect(identifiers.get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
      expect(identifiers.get_product_id() == pqrs::hid::product_id::value_t(0));
      expect(identifiers.get_is_keyboard() == false);
      expect(identifiers.get_is_pointing_device() == false);
    }

    // load values from json
    {
      nlohmann::json json({
          {"vendor_id", 1234},
          {"product_id", 5678},
          {"device_address", "ec-ba-73-21-e6-f5"},
          {"is_keyboard", true},
          {"is_pointing_device", true},
      });
      auto identifiers = json.get<krbn::device_identifiers>();
      expect(identifiers.get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(identifiers.get_product_id() == pqrs::hid::product_id::value_t(5678));
      expect(identifiers.get_device_address() == "ec-ba-73-21-e6-f5");
      expect(identifiers.get_is_keyboard() == true);
      expect(identifiers.get_is_pointing_device() == true);
    }

    // construct with vendor_id, product_id, ...
    {
      krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1234),
                                           pqrs::hid::product_id::value_t(5678),
                                           true,               // is_keyboard
                                           false,              // is_pointing_device
                                           false,              // is_game_pad
                                           "ec-ba-73-21-e6-f5" // device_address (ignored)
      );
      expect(identifiers.get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(identifiers.get_product_id() == pqrs::hid::product_id::value_t(5678));
      expect(identifiers.get_device_address() == "");
      expect(identifiers.get_is_keyboard() == true);
      expect(identifiers.get_is_pointing_device() == false);
    }
    {
      krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(0),
                                           pqrs::hid::product_id::value_t(0),
                                           true,               // is_keyboard
                                           false,              // is_pointing_device
                                           false,              // is_game_pad
                                           "ec-ba-73-21-e6-f5" // device_address
      );
      expect(identifiers.get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
      expect(identifiers.get_product_id() == pqrs::hid::product_id::value_t(0));
      expect(identifiers.get_device_address() == "ec-ba-73-21-e6-f5");
      expect(identifiers.get_is_keyboard() == true);
      expect(identifiers.get_is_pointing_device() == false);
    }

    {
      krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(4321),
                                           pqrs::hid::product_id::value_t(8765),
                                           false, // is_keyboard
                                           true,  // is_pointing_device
                                           false, // is_game_pad
                                           ""     // device_address
      );
      expect(identifiers.get_vendor_id() == pqrs::hid::vendor_id::value_t(4321));
      expect(identifiers.get_product_id() == pqrs::hid::product_id::value_t(8765));
      expect(identifiers.get_device_address() == "");
      expect(identifiers.get_is_keyboard() == false);
      expect(identifiers.get_is_pointing_device() == true);
    }
  };

  "device.identifiers.to_json"_test = [] {
    {
      auto json = nlohmann::json::object();
      auto identifiers = json.get<krbn::device_identifiers>();
      nlohmann::json expected({
          {"vendor_id", 0},
          {"product_id", 0},
          {"is_keyboard", false},
          {"is_pointing_device", false},
          {"is_game_pad", false},
      });
      expect(nlohmann::json(identifiers) == expected);
    }
    {
      nlohmann::json json({
          {"is_pointing_device", true},
          {"dummy", {{"keep_me", true}}},
      });
      auto identifiers = json.get<krbn::device_identifiers>();
      nlohmann::json expected({
          {"dummy", {{"keep_me", true}}},
          {"vendor_id", 0},
          {"product_id", 0},
          {"is_keyboard", false},
          {"is_pointing_device", true},
          {"is_game_pad", false},
      });
      expect(nlohmann::json(identifiers) == expected);
    }
  };

  "device.find_default_value"_test = [] {
    auto json = nlohmann::json::object();
    krbn::core_configuration::details::device device(json,
                                                     krbn::core_configuration::error_handling::strict);
    expect(1.0_d == device.find_default_value(
                        device.get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold()));
    expect(20_i == device.find_default_value(
                       device.get_game_pad_xy_stick_continued_movement_interval_milliseconds()));
    expect(50_i == device.find_default_value(
                       device.get_game_pad_xy_stick_flicking_input_window_milliseconds()));
    expect(1.0_d == device.find_default_value(
                        device.get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold()));
    expect(10_i == device.find_default_value(
                       device.get_game_pad_wheels_stick_continued_movement_interval_milliseconds()));
    expect(50_i == device.find_default_value(
                       device.get_game_pad_wheels_stick_flicking_input_window_milliseconds()));
  };

  "device"_test = [] {
    // empty json
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::device device(json,
                                                       krbn::core_configuration::error_handling::strict);
      expect(pqrs::hid::vendor_id::value_t(0) == device.get_identifiers().get_vendor_id());
      expect(pqrs::hid::product_id::value_t(0) == device.get_identifiers().get_product_id());
      expect("" == device.get_identifiers().get_device_address());
      expect(false == device.get_identifiers().get_is_keyboard());
      expect(false == device.get_identifiers().get_is_pointing_device());
      expect(false == device.get_ignore());
      expect(false == device.get_manipulate_caps_lock_led());
      expect(false == device.get_treat_as_built_in_keyboard());
      expect(false == device.get_disable_built_in_keyboard_if_exists());
      {
        auto& v = device.get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold();
        expect(device.find_default_value(v) == v);
      }
      {
        auto& v = device.get_game_pad_xy_stick_continued_movement_interval_milliseconds();
        expect(device.find_default_value(v) == v);
      }
      {
        auto& v = device.get_game_pad_xy_stick_flicking_input_window_milliseconds();
        expect(device.find_default_value(v) == v);
      }
      {
        auto& v = device.get_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold();
        expect(device.find_default_value(v) == v);
      }
      {
        auto& v = device.get_game_pad_wheels_stick_continued_movement_interval_milliseconds();
        expect(device.find_default_value(v) == v);
      }
      {
        auto& v = device.get_game_pad_wheels_stick_flicking_input_window_milliseconds();
        expect(device.find_default_value(v) == v);
      }
      expect(device.get_game_pad_stick_x_formula() == std::nullopt);
      expect(device.get_game_pad_stick_y_formula() == std::nullopt);
      expect(device.get_game_pad_stick_vertical_wheel_formula() == std::nullopt);
      expect(device.get_game_pad_stick_horizontal_wheel_formula() == std::nullopt);
    }

    // load values from json
    {
      nlohmann::json json({
          {"identifiers", {
                              {"vendor_id", 1234},
                              {"product_id", 5678},
                              {"device_address", "ec-ba-73-21-e6-f5"},
                              {"is_keyboard", true},
                              {"is_pointing_device", true},
                          }},
          {"disable_built_in_keyboard_if_exists", true},
          {"ignore", true},
          {"manipulate_caps_lock_led", true},
          {"treat_as_built_in_keyboard", false},
          {"game_pad_xy_stick_continued_movement_absolute_magnitude_threshold", 0.5},
          {"game_pad_xy_stick_continued_movement_interval_milliseconds", 15},
          {"game_pad_xy_stick_flicking_input_window_milliseconds", 123},
          // string
          {"game_pad_stick_x_formula", "cos(radian) * acceleration * 127"},
          // array of string
          {"game_pad_stick_y_formula", nlohmann::json::array({
                                           "if (acceleration < 0.5,",
                                           "  cos(radian) * acceleration * 127 * 0.5,",
                                           "  cos(radian) * acceleration * 127",
                                           ")",
                                       })},
      });
      krbn::core_configuration::details::device device(json,
                                                       krbn::core_configuration::error_handling::strict);
      expect(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      expect(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
      expect(device.get_identifiers().get_device_address() == "ec-ba-73-21-e6-f5");
      expect(device.get_identifiers().get_is_keyboard() == true);
      expect(device.get_identifiers().get_is_pointing_device() == true);
      expect(device.get_ignore() == true);
      expect(device.get_manipulate_caps_lock_led() == true);
      expect(device.get_treat_as_built_in_keyboard() == false);
      expect(device.get_disable_built_in_keyboard_if_exists() == true);
      expect(device.get_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold() == 0.5);
      expect(device.get_game_pad_xy_stick_continued_movement_interval_milliseconds() == 15);
      expect(device.get_game_pad_xy_stick_flicking_input_window_milliseconds() == 123);
      expect(device.get_game_pad_stick_x_formula() == "cos(radian) * acceleration * 127");
      expect(device.get_game_pad_stick_y_formula() == "if (acceleration < 0.5,\n"
                                                      "  cos(radian) * acceleration * 127 * 0.5,\n"
                                                      "  cos(radian) * acceleration * 127\n"
                                                      ")");
    }

    // Special default value for specific devices
    {
      // ignore_ == true if is_pointing_device

      nlohmann::json json({
          {"identifiers", {
                              {"vendor_id", 1234},
                              {"product_id", 5678},
                              {"is_keyboard", false},
                              {"is_pointing_device", true},
                          }},
      });
      {
        krbn::core_configuration::details::device device(json,
                                                         krbn::core_configuration::error_handling::strict);
        expect(device.get_ignore() == true);
      }
      {
        json["ignore"] = false;
        krbn::core_configuration::details::device device(json,
                                                         krbn::core_configuration::error_handling::strict);
        expect(device.get_ignore() == false);
      }
    }
    {
      // ignore_ == true for specific devices

      nlohmann::json json({
          {"identifiers", {
                              {"vendor_id", 0x1050},
                              {"product_id", 0x407},
                              {"is_keyboard", true},
                              {"is_pointing_device", false},
                          }},
      });
      {
        krbn::core_configuration::details::device device(json,
                                                         krbn::core_configuration::error_handling::strict);
        expect(device.get_ignore() == true);
      }
      {
        json["ignore"] = false;
        krbn::core_configuration::details::device device(json,
                                                         krbn::core_configuration::error_handling::strict);
        expect(device.get_ignore() == false);
      }
    }
    {
      // manipulate_caps_lock_led_ == true for specific devices

      nlohmann::json json({
          {"identifiers", {
                              {"vendor_id", 0x5ac},
                              {"product_id", 0x262},
                              {"is_keyboard", true},
                              {"is_pointing_device", false},
                          }},
      });
      {
        krbn::core_configuration::details::device device(json,
                                                         krbn::core_configuration::error_handling::strict);
        expect(device.get_manipulate_caps_lock_led() == true);
      }
      {
        json["manipulate_caps_lock_led"] = false;
        krbn::core_configuration::details::device device(json,
                                                         krbn::core_configuration::error_handling::strict);
        expect(device.get_manipulate_caps_lock_led() == false);
      }
    }
    // Coordinate between settings
    {
      nlohmann::json json({
          {"identifiers", {
                              {"vendor_id", 1234},
                              {"product_id", 5678},
                              {"is_keyboard", false},
                              {"is_pointing_device", true},
                          }},
          {"treat_as_built_in_keyboard", true},
          {"disable_built_in_keyboard_if_exists", true},
      });
      {
        // disable_built_in_keyboard_if_exists will be false when treat_as_built_in_keyboard is true.

        krbn::core_configuration::details::device device(json,
                                                         krbn::core_configuration::error_handling::strict);
        expect(device.get_treat_as_built_in_keyboard() == true);
        expect(device.get_disable_built_in_keyboard_if_exists() == false);

        // disable_built_in_keyboard_if_exists will be false even set true.

        device.set_disable_built_in_keyboard_if_exists(true); // ignored
        expect(device.get_disable_built_in_keyboard_if_exists() == false);

        // If treat_as_built_in_keyboard is false, disable_built_in_keyboard_if_exists can be true.

        device.set_treat_as_built_in_keyboard(false);
        expect(device.get_treat_as_built_in_keyboard() == false);
        expect(device.get_disable_built_in_keyboard_if_exists() == false);

        device.set_disable_built_in_keyboard_if_exists(true);
        expect(device.get_disable_built_in_keyboard_if_exists() == true);

        // disable_built_in_keyboard_if_exists will be false if treat_as_built_in_keyboard is set true

        device.set_treat_as_built_in_keyboard(true);
        expect(device.get_treat_as_built_in_keyboard() == true);
        expect(device.get_disable_built_in_keyboard_if_exists() == false);
      }
    }
  };

  "device.to_json"_test = [] {
    {
      auto json = nlohmann::json::object();
      krbn::core_configuration::details::device empty_device(json,
                                                             krbn::core_configuration::error_handling::strict);
      nlohmann::json expected({
          {"disable_built_in_keyboard_if_exists", false},
          {"fn_function_keys", nlohmann::json::array()},
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
                              {
                                  "is_game_pad",
                                  false,
                              },
                          }},
          {"ignore", false},
          {"manipulate_caps_lock_led", false},
          {"simple_modifications", nlohmann::json::array()},
          {"treat_as_built_in_keyboard", false},
      });
      expect(empty_device.to_json() == expected) << "empty_device.to_json() == expected";
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
          {"treat_as_built_in_keyboard", true},
          {"mouse_discard_horizontal_wheel", true},
          {"mouse_discard_vertical_wheel", true},
          {"mouse_discard_x", true},
          {"mouse_discard_y", true},
          {"mouse_flip_horizontal_wheel", true},
          {"mouse_flip_vertical_wheel", true},
          {"mouse_flip_x", true},
          {"mouse_flip_y", true},
          {"mouse_swap_wheels", true},
          {"mouse_swap_xy", true},
          {"game_pad_swap_sticks", true},
          {"game_pad_xy_stick_continued_movement_absolute_magnitude_threshold", 0.5},
          {"game_pad_xy_stick_continued_movement_interval_milliseconds", 10},
          {"game_pad_xy_stick_flicking_input_window_milliseconds", 123},
          {"game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold", 0.08},
          {"game_pad_wheels_stick_continued_movement_interval_milliseconds", 40},
          {"game_pad_wheels_stick_flicking_input_window_milliseconds", 124},
          {"game_pad_stick_x_formula", "cos(radian) * acceleration * 127"},
          {"game_pad_stick_y_formula", nlohmann::json::array({
                                           "var a := acceleration ^ 2;",
                                           "sin(radian) * a * 127",
                                       })},
          {"game_pad_stick_vertical_wheel_formula", "sgn(sin(radian))"},
          {"game_pad_stick_horizontal_wheel_formula", "sgn(cos(radian))"},
      });
      krbn::core_configuration::details::device device(json,
                                                       krbn::core_configuration::error_handling::strict);
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
                              {
                                  "is_game_pad",
                                  false,
                              },
                          }},
          {"ignore", true},
          {"game_pad_swap_sticks", true},
          {"game_pad_xy_stick_continued_movement_absolute_magnitude_threshold", 0.5},
          {"game_pad_xy_stick_continued_movement_interval_milliseconds", 10},
          {"game_pad_xy_stick_flicking_input_window_milliseconds", 123},
          {"game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold", 0.08},
          {"game_pad_wheels_stick_continued_movement_interval_milliseconds", 40},
          {"game_pad_wheels_stick_flicking_input_window_milliseconds", 124},
          {"game_pad_stick_x_formula", "cos(radian) * acceleration * 127"},
          {"game_pad_stick_y_formula", nlohmann::json::array({
                                           "var a := acceleration ^ 2;",
                                           "sin(radian) * a * 127",
                                       })},
          {"game_pad_stick_vertical_wheel_formula", "sgn(sin(radian))"},
          {"game_pad_stick_horizontal_wheel_formula", "sgn(cos(radian))"},
          {"manipulate_caps_lock_led", true},
          {"mouse_discard_horizontal_wheel", true},
          {"mouse_discard_vertical_wheel", true},
          {"mouse_discard_x", true},
          {"mouse_discard_y", true},
          {"mouse_flip_horizontal_wheel", true},
          {"mouse_flip_vertical_wheel", true},
          {"mouse_flip_x", true},
          {"mouse_flip_y", true},
          {"mouse_swap_wheels", true},
          {"mouse_swap_xy", true},
          {"simple_modifications", nlohmann::json::array()},
          {"treat_as_built_in_keyboard", true},
      });
      expect(device.to_json() == expected);
    }
  };

  "device.validate_stick_formula"_test = [] {
    expect(krbn::core_configuration::details::device::validate_stick_formula("cos(radian) * delta_magnitude"));
    expect(!krbn::core_configuration::details::device::validate_stick_formula("cos(unknown) * delta_magnitude"));
  };
}
