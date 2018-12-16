#include <catch2/catch.hpp>

#include "core_configuration/core_configuration.hpp"

TEST_CASE("device.identifiers") {
  // empty json
  {
    nlohmann::json json;
    auto identifiers = krbn::device_identifiers::make_from_json(json);
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
    auto identifiers = krbn::device_identifiers::make_from_json(json);
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
    auto identifiers = krbn::device_identifiers::make_from_json(json);
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
    auto identifiers = krbn::device_identifiers::make_from_json(json);
    nlohmann::json expected({
        {"vendor_id", 0},
        {"product_id", 0},
        {"is_keyboard", false},
        {"is_pointing_device", false},
    });
    REQUIRE(identifiers.to_json() == expected);

    nlohmann::json actual = identifiers.to_json();
    REQUIRE(actual == expected);
  }
  {
    nlohmann::json json({
        {"is_pointing_device", true},
        {"dummy", {{"keep_me", true}}},
    });
    auto identifiers = krbn::device_identifiers::make_from_json(json);
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
    krbn::core_configuration::details::device device(json);
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
    krbn::core_configuration::details::device device(json);
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
    krbn::core_configuration::details::device device(json);
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
