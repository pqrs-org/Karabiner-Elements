#pragma once

#include "../../share/manipulator_conditions_helper.hpp"
#include "actual_examples_helper.hpp"

void run_device_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "conditions.device"_test = [] {
    krbn::unit_testing::manipulator_conditions_helper manipulator_conditions_helper;
    auto& environment = manipulator_conditions_helper.get_manipulator_environment();

    auto device_id_8888_9999 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(8888), pqrs::hid::product_id::value_t(9999), "", std::nullopt, true, false);

    auto device_id_1000_2000 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), "", std::nullopt, true, false);

    auto device_id_1000_2001 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2001), "", std::nullopt, true, false);

    auto device_id_1001_2000 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1001), pqrs::hid::product_id::value_t(2000), "", std::nullopt, true, false);

    auto device_id_1001_2001 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1001), pqrs::hid::product_id::value_t(2001), "", std::nullopt, true, false);

    auto device_id_1099_9999 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1099), pqrs::hid::product_id::value_t(9999), "", std::nullopt, true, false);

    auto device_id_0000_0000 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(0), pqrs::hid::product_id::value_t(0), "aa-bb-cc-dd-ee-ff", std::nullopt, true, false);

    auto device_id_nullopt_nullopt = manipulator_conditions_helper.prepare_device(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt, true, false);

    auto device_id_1000_2000_tt = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), "", std::nullopt, true, true);

    auto device_id_1000_2000_tf = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), "", std::nullopt, true, false);

    auto device_id_1000_2000_ft = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), "", std::nullopt, false, true);

    auto device_id_1000_2000_ff = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), "", std::nullopt, false, false);

    auto device_id_1000_2000_3000 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), "", krbn::location_id(3000), std::nullopt, std::nullopt);

    auto device_id_1000_2000_none = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), std::nullopt, std::nullopt, std::nullopt, std::nullopt);

    auto device_id_1000_2000_4000 = manipulator_conditions_helper.prepare_device(
        pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), "", krbn::location_id(4000), std::nullopt, std::nullopt);

    {
      actual_examples_helper helper("device_if.jsonc");

      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_8888_9999);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == false);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2001);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == false);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1001_2000);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == false);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1001_2001);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1099_9999);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_0000_0000);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == true);
      }
    }
    {
      actual_examples_helper helper("device_unless.jsonc");

      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_8888_9999);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == false);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2001);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1001_2000);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1001_2001);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == false);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1099_9999);
        expect(helper.get_condition_manager().is_fulfilled(e, environment) == false);
      }
    }

    {
      nlohmann::json json;
      json["type"] = "device_if";
      json["identifiers"] = nlohmann::json::array();
      json["identifiers"].push_back(nlohmann::json::object());
      json["identifiers"].back()["vendor_id"] = 1000;
      json["identifiers"].back()["product_id"] = 2000;
      json["identifiers"].back()["is_keyboard"] = true;
      json["identifiers"].back()["is_pointing_device"] = false;
      krbn::manipulator::conditions::device condition(json);

      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000);
        expect(condition.is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2001);
        expect(condition.is_fulfilled(e, environment) == false);
      }

      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000_tf);
        expect(condition.is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000_ff);
        expect(condition.is_fulfilled(e, environment) == false);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000_tt);
        expect(condition.is_fulfilled(e, environment) == false);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000_ft);
        expect(condition.is_fulfilled(e, environment) == false);
      }
    }

    {
      nlohmann::json json;
      json["type"] = "device_if";
      json["identifiers"] = nlohmann::json::array();
      json["identifiers"].push_back(nlohmann::json::object());
      json["identifiers"].back()["vendor_id"] = 1000;
      json["identifiers"].back()["product_id"] = 2000;
      json["identifiers"].back()["location_id"] = 3000;
      krbn::manipulator::conditions::device condition(json);

      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000_3000);
        expect(condition.is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000_none);
        expect(condition.is_fulfilled(e, environment) == false);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000_4000);
        expect(condition.is_fulfilled(e, environment) == false);
      }
    }

    // std::nullopt
    {
      nlohmann::json json;
      json["type"] = "device_if";
      json["identifiers"] = nlohmann::json::array();
      json["identifiers"].push_back(nlohmann::json::object());
      json["identifiers"].back()["vendor_id"] = 0;
      json["identifiers"].back()["product_id"] = 0;
      krbn::manipulator::conditions::device condition(json);

      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_0000_0000);
        expect(condition.is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_nullopt_nullopt);
        expect(condition.is_fulfilled(e, environment) == true);
      }
      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000);
        expect(condition.is_fulfilled(e, environment) == false);
      }
    }

    // is_built_in_keyboard
    {
      nlohmann::json json;
      json["type"] = "device_if";
      json["identifiers"] = nlohmann::json::array();
      json["identifiers"].push_back(nlohmann::json::object());
      json["identifiers"].back()["is_built_in_keyboard"] = true;
      krbn::manipulator::conditions::device condition(json);

      {
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000);
        expect(condition.is_fulfilled(e, environment) == false);
      }
      {
        manipulator_conditions_helper.get_core_configuration()->get_selected_profile().set_device_treat_as_built_in_keyboard(
            krbn::device_identifiers(pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), "", true, false),
            true);
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000);
        expect(condition.is_fulfilled(e, environment) == true);
      }
      {
        manipulator_conditions_helper.get_core_configuration()->get_selected_profile().set_device_treat_as_built_in_keyboard(
            krbn::device_identifiers(pqrs::hid::vendor_id::value_t(1000), pqrs::hid::product_id::value_t(2000), "", true, false),
            false);
        auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1000_2000);
        expect(condition.is_fulfilled(e, environment) == false);
      }
    }
  };
}
