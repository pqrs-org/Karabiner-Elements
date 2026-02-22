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

    auto device_id_8888_9999 = manipulator_conditions_helper.prepare_device(krbn::device_properties::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(8888),
        .product_id = pqrs::hid::product_id::value_t(9999),
        .is_keyboard = true,
    });
    krbn::manipulator::conditions::condition_context condition_context{
        .device_id = device_id_8888_9999,
        .state = krbn::event_queue::state::original,
    };

    expect(helper.get_condition_manager().is_fulfilled(condition_context,
                                                       environment) == false);

    manipulator_conditions_helper.prepare_device(krbn::device_properties::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(1000),
        .product_id = pqrs::hid::product_id::value_t(2000),
        .is_keyboard = true,
    });

    expect(helper.get_condition_manager().is_fulfilled(condition_context,
                                                       environment) == true);
  };

  "device_exists_unless"_test = [] {
    // true if {vid:2000, pid:1000} does not exist.
    actual_examples_helper helper("device_exists_unless.jsonc");

    krbn::unit_testing::manipulator_conditions_helper manipulator_conditions_helper;
    auto& environment = manipulator_conditions_helper.get_manipulator_environment();

    auto device_id_8888_9999 = manipulator_conditions_helper.prepare_device(krbn::device_properties::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(8888),
        .product_id = pqrs::hid::product_id::value_t(9999),
        .is_keyboard = true,
    });
    krbn::manipulator::conditions::condition_context condition_context{
        .device_id = device_id_8888_9999,
        .state = krbn::event_queue::state::original,
    };

    expect(helper.get_condition_manager().is_fulfilled(condition_context,
                                                       environment) == true);

    manipulator_conditions_helper.prepare_device(krbn::device_properties::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(1000),
        .product_id = pqrs::hid::product_id::value_t(2000),
        .is_keyboard = true,
    });

    expect(helper.get_condition_manager().is_fulfilled(condition_context,
                                                       environment) == false);
  };
}
