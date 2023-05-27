#pragma once

#include "../../share/manipulator_conditions_helper.hpp"
#include "actual_examples_helper.hpp"

void run_device_exists_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "device_exists_if"_test = [] {
    // true if {vid:2000, pid:1000} exists.
    actual_examples_helper helper("device_exists_if.jsonc");

    krbn::unit_testing::manipulator_conditions_helper manipulator_conditions_helper;
    auto& environment = manipulator_conditions_helper.get_manipulator_environment();

    auto device_id_8888_9999 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(8888),  // vendor_id
        pqrs::hid::product_id::value_t(9999), // product_id
        std::nullopt,                         // location_id
        true,                                 // is_keyboard,
        false,                                // is_pointing_device
        ""                                    // device_address
    );

    {
      auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_8888_9999);
      expect(helper.get_condition_manager().is_fulfilled(e, environment) == false);
    }

    manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000),  // vendor_id
        pqrs::hid::product_id::value_t(2000), // product_id
        std::nullopt,                         // location_id
        true,                                 // is_keyboard,
        false,                                // is_pointing_device
        ""                                    // device_address
    );

    {
      auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_8888_9999);
      expect(helper.get_condition_manager().is_fulfilled(e, environment) == true);
    }
  };

  "device_exists_unless"_test = [] {
    // true if {vid:2000, pid:1000} does not exist.
    actual_examples_helper helper("device_exists_unless.jsonc");

    krbn::unit_testing::manipulator_conditions_helper manipulator_conditions_helper;
    auto& environment = manipulator_conditions_helper.get_manipulator_environment();

    auto device_id_8888_9999 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(8888),  // vendor_id
        pqrs::hid::product_id::value_t(9999), // product_id
        std::nullopt,                         // location_id
        true,                                 // is_keyboard,
        false,                                // is_pointing_device
        ""                                    // device_address
    );

    {
      auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_8888_9999);
      expect(helper.get_condition_manager().is_fulfilled(e, environment) == true);
    }

    manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000),  // vendor_id
        pqrs::hid::product_id::value_t(2000), // product_id
        std::nullopt,                         // location_id
        true,                                 // is_keyboard,
        false,                                // is_pointing_device
        ""                                    // device_address
    );

    {
      auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_8888_9999);
      expect(helper.get_condition_manager().is_fulfilled(e, environment) == false);
    }
  };
}
