#include <catch2/catch.hpp>

#include "core_configuration/core_configuration.hpp"

TEST_CASE("device.identifiers") {
  // empty json
  {
    auto json = nlohmann::json::object();
    auto identifiers = json.get<krbn::device_identifiers>();
    REQUIRE(identifiers.get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
    REQUIRE(identifiers.get_product_id() == pqrs::hid::product_id::value_t(0));
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
    auto identifiers = json.get<krbn::device_identifiers>();
    REQUIRE(identifiers.get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
    REQUIRE(identifiers.get_product_id() == pqrs::hid::product_id::value_t(5678));
    REQUIRE(identifiers.get_is_keyboard() == true);
    REQUIRE(identifiers.get_is_pointing_device() == true);
  }

  // construct with vendor_id, product_id, ...
  {
    krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1234), pqrs::hid::product_id::value_t(5678), true, false);
    REQUIRE(identifiers.get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
    REQUIRE(identifiers.get_product_id() == pqrs::hid::product_id::value_t(5678));
    REQUIRE(identifiers.get_is_keyboard() == true);
    REQUIRE(identifiers.get_is_pointing_device() == false);
  }
  {
    krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(4321), pqrs::hid::product_id::value_t(8765), false, true);
    REQUIRE(identifiers.get_vendor_id() == pqrs::hid::vendor_id::value_t(4321));
    REQUIRE(identifiers.get_product_id() == pqrs::hid::product_id::value_t(8765));
    REQUIRE(identifiers.get_is_keyboard() == false);
    REQUIRE(identifiers.get_is_pointing_device() == true);
  }
}

TEST_CASE("device.identifiers.to_json") {
  {
    auto json = nlohmann::json::object();
    auto identifiers = json.get<krbn::device_identifiers>();
    nlohmann::json expected({
        {"vendor_id", 0},
        {"product_id", 0},
        {"is_keyboard", false},
        {"is_pointing_device", false},
    });
    REQUIRE(nlohmann::json(identifiers) == expected);
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
    });
    REQUIRE(nlohmann::json(identifiers) == expected);
  }
}

TEST_CASE("device") {
  // empty json
  {
    auto json = nlohmann::json::object();
    krbn::core_configuration::details::device device(json);
    REQUIRE(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
    REQUIRE(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(0));
    REQUIRE(device.get_identifiers().get_is_keyboard() == false);
    REQUIRE(device.get_identifiers().get_is_pointing_device() == false);
    REQUIRE(device.get_ignore() == false);
    REQUIRE(device.get_manipulate_caps_lock_led() == false);
    REQUIRE(device.get_disable_built_in_keyboard_if_exists() == false);
  }

  // load values from json
  {
    nlohmann::json json({
        {"identifiers", {
                            {"vendor_id", 1234},
                            {"product_id", 5678},
                            {"is_keyboard", true},
                            {"is_pointing_device", true},
                        }},
        {"disable_built_in_keyboard_if_exists", true},
        {"ignore", true},
        {"manipulate_caps_lock_led", true},
    });
    krbn::core_configuration::details::device device(json);
    REQUIRE(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
    REQUIRE(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
    REQUIRE(device.get_identifiers().get_is_keyboard() == true);
    REQUIRE(device.get_identifiers().get_is_pointing_device() == true);
    REQUIRE(device.get_ignore() == true);
    REQUIRE(device.get_manipulate_caps_lock_led() == true);
    REQUIRE(device.get_disable_built_in_keyboard_if_exists() == true);
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
      krbn::core_configuration::details::device device(json);
      REQUIRE(device.get_ignore() == true);
    }
    {
      json["ignore"] = false;
      krbn::core_configuration::details::device device(json);
      REQUIRE(device.get_ignore() == false);
    }
  }
  {
    // ignore_ == true for specific devices

    nlohmann::json json({
        {"identifiers", {
                            {"vendor_id", 0x05ac},
                            {"product_id", 0x8600},
                            {"is_keyboard", true},
                            {"is_pointing_device", false},
                        }},
    });
    {
      krbn::core_configuration::details::device device(json);
      REQUIRE(device.get_ignore() == true);
    }
    {
      json["ignore"] = false;
      krbn::core_configuration::details::device device(json);
      REQUIRE(device.get_ignore() == false);
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
      krbn::core_configuration::details::device device(json);
      REQUIRE(device.get_ignore() == true);
    }
    {
      json["ignore"] = false;
      krbn::core_configuration::details::device device(json);
      REQUIRE(device.get_ignore() == false);
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
      krbn::core_configuration::details::device device(json);
      REQUIRE(device.get_manipulate_caps_lock_led() == true);
    }
    {
      json["manipulate_caps_lock_led"] = false;
      krbn::core_configuration::details::device device(json);
      REQUIRE(device.get_manipulate_caps_lock_led() == false);
    }
  }
}

TEST_CASE("device.to_json") {
  {
    auto json = nlohmann::json::object();
    krbn::core_configuration::details::device device(json);
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

    nlohmann::json actual = device.to_json();
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
    krbn::core_configuration::details::device device(json);
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
