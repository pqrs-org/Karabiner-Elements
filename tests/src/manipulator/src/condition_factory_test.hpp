#include "../../share/manipulator_conditions_helper.hpp"
#include "../../share/manipulator_helper.hpp"
#include <boost/ut.hpp>

namespace modifier_definition = krbn::manipulator::modifier_definition;
using krbn::manipulator::event_definition;
using krbn::manipulator::to_event_definition;

void run_condition_factory_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "condition_factory::make_device_if_condition"_test = [] {
    krbn::unit_testing::manipulator_conditions_helper manipulator_conditions_helper;

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
                                false,
                            },
                        }},
    });
    krbn::core_configuration::details::device device(json,
                                                     krbn::core_configuration::error_handling::loose);

    auto device_id_1234_5678_keyboard = manipulator_conditions_helper.prepare_device(krbn::device_properties::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(1234),
        .product_id = pqrs::hid::product_id::value_t(5678),
        .is_keyboard = true, // is_keyboard,
    });

    auto device_id_1234_5678_mouse = manipulator_conditions_helper.prepare_device(krbn::device_properties::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(1234),
        .product_id = pqrs::hid::product_id::value_t(5678),
        .is_pointing_device = true,
    });

    auto device_id_1234_5000_keyboard = manipulator_conditions_helper.prepare_device(krbn::device_properties::initialization_parameters{
        .vendor_id = pqrs::hid::vendor_id::value_t(1234),
        .product_id = pqrs::hid::product_id::value_t(5000),
        .is_keyboard = true,
    });

    auto c = krbn::manipulator::condition_factory::make_device_if_condition(device);

    {
      auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1234_5678_keyboard);
      expect(c->is_fulfilled(e, manipulator_conditions_helper.get_manipulator_environment()) == true);
    }
    {
      auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1234_5678_mouse);
      expect(c->is_fulfilled(e, manipulator_conditions_helper.get_manipulator_environment()) == false);
    }
    {
      auto e = manipulator_conditions_helper.make_event_queue_entry(device_id_1234_5000_keyboard);
      expect(c->is_fulfilled(e, manipulator_conditions_helper.get_manipulator_environment()) == false);
    }
  };

  "condition_factory::make_frontmost_application_unless_condition"_test = [] {
    krbn::unit_testing::manipulator_conditions_helper manipulator_conditions_helper;

    auto c = krbn::manipulator::condition_factory::make_frontmost_application_unless_condition({
        "^com\\.apple\\.loginwindow$",
    });

    {
      pqrs::osx::frontmost_application_monitor::application application;
      application.set_bundle_identifier("com.apple.Terminal");
      application.set_file_path("/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal");
      manipulator_conditions_helper.get_manipulator_environment().set_frontmost_application(application);

      auto e = manipulator_conditions_helper.make_event_queue_entry(krbn::device_id(0));
      expect(c->is_fulfilled(e, manipulator_conditions_helper.get_manipulator_environment()) == true);
    }
    {
      pqrs::osx::frontmost_application_monitor::application application;
      application.set_bundle_identifier("com.apple.loginwindow");
      application.set_file_path("/System/Library/CoreServices/loginwindow.app/Contents/MacOS/loginwindow");
      manipulator_conditions_helper.get_manipulator_environment().set_frontmost_application(application);

      auto e = manipulator_conditions_helper.make_event_queue_entry(krbn::device_id(0));
      expect(c->is_fulfilled(e, manipulator_conditions_helper.get_manipulator_environment()) == false);
    }
  };
}
