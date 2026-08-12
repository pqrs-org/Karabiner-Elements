#include "../../../src/apps/CoreService/include/core_service/daemon/device_grabber_details/device_key_code_manipulator_manager.hpp"
#include "../../../src/apps/CoreService/include/core_service/daemon/device_grabber_details/simple_modifications_manipulator_manager.hpp"
#include "event_queue/utility.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include <boost/ut.hpp>

namespace {
krbn::event_queue::not_null_entries_ptr_t make_entries(
    pqrs::not_null_shared_ptr_t<krbn::device_properties> device_properties,
    pqrs::hid::usage::value_t usage,
    int value,
    krbn::absolute_time_point time_stamp) {
  std::vector<pqrs::osx::iokit_hid_value> hid_values{
      pqrs::osx::iokit_hid_value(time_stamp,
                                 value,
                                 pqrs::hid::usage_page::keyboard_or_keypad,
                                 usage,
                                 std::nullopt,
                                 std::nullopt),
  };
  return krbn::event_queue::utility::make_entries(device_properties,
                                                  hid_values,
                                                  {});
}

void manipulate(const krbn::event_queue::not_null_entries_ptr_t& entries,
                const std::shared_ptr<krbn::event_queue::queue>& input_queue,
                krbn::manipulator::manipulator_managers_connector& connector,
                const std::shared_ptr<const krbn::core_configuration::core_configuration>& core_configuration) {
  for (const auto& entry : *entries) {
    input_queue->push_back_entry(*entry);
    connector.manipulate(entry->get_event_time_stamp().get_time_stamp(),
                         core_configuration);
  }
}

std::vector<pqrs::hid::usage::value_t> make_usages(const krbn::event_queue::queue& queue) {
  std::vector<pqrs::hid::usage::value_t> result;
  for (const auto& entry : queue.get_entries()) {
    if (auto event = entry.get_event().get_if<krbn::momentary_switch_event>()) {
      result.push_back(event->get_usage_pair().get_usage());
    }
  }
  return result;
}

struct test_environment final {
  test_environment()
      : core_configuration(std::make_shared<krbn::core_configuration::core_configuration>()),
        device_properties(std::make_shared<krbn::device_properties>(
            krbn::device_properties::initialization_parameters{
                .device_id = krbn::device_id(1),
                .vendor_id = pqrs::hid::vendor_id::value_t(0x1234),
                .product_id = pqrs::hid::product_id::value_t(0x5678),
                .is_keyboard = true,
            })),
        device_key_code_manipulator_manager(std::make_shared<krbn::core_service::daemon::device_grabber_details::device_key_code_manipulator_manager>()),
        simple_modifications_manipulator_manager(std::make_shared<krbn::core_service::daemon::device_grabber_details::simple_modifications_manipulator_manager>()),
        input_queue(std::make_shared<krbn::event_queue::queue>()),
        device_key_code_manipulated_queue(std::make_shared<krbn::event_queue::queue>()),
        output_queue(std::make_shared<krbn::event_queue::queue>()) {
    connector.emplace_back_connection(
        pqrs::make_weak(device_key_code_manipulator_manager->get_manipulator_manager()),
        input_queue,
        device_key_code_manipulated_queue);
    connector.emplace_back_connection(
        pqrs::make_weak(simple_modifications_manipulator_manager->get_manipulator_manager()),
        output_queue);
    connector.set_manipulator_environment_core_configuration(core_configuration);

    // The real event queue learns the device properties from device_grabbed.
    // Insert them directly here so that the device_if condition can be tested
    // without involving device_grabber.
    device_key_code_manipulated_queue->get_manipulator_environment().insert_device_properties(
        device_properties->get_device_id(),
        device_properties);
  }

  std::shared_ptr<krbn::core_configuration::core_configuration> core_configuration;
  std::shared_ptr<krbn::device_properties> device_properties;
  std::shared_ptr<krbn::core_service::daemon::device_grabber_details::device_key_code_manipulator_manager> device_key_code_manipulator_manager;
  std::shared_ptr<krbn::core_service::daemon::device_grabber_details::simple_modifications_manipulator_manager> simple_modifications_manipulator_manager;
  std::shared_ptr<krbn::event_queue::queue> input_queue;
  std::shared_ptr<krbn::event_queue::queue> device_key_code_manipulated_queue;
  std::shared_ptr<krbn::event_queue::queue> output_queue;
  krbn::manipulator::manipulator_managers_connector connector;
};
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  const auto grave = pqrs::hid::usage::keyboard_or_keypad::keyboard_grave_accent_and_tilde;
  const auto non_us_backslash = pqrs::hid::usage::keyboard_or_keypad::keyboard_non_us_backslash;
  const auto backslash = pqrs::hid::usage::keyboard_or_keypad::keyboard_backslash;

  "device key-code manipulations run before Simple Modifications"_test = [&] {
    test_environment environment;
    auto& profile = environment.core_configuration->get_selected_profile();
    auto device = profile.get_device(environment.device_properties->get_device_identifiers());
    device->set_swap_grave_accent_and_non_us_backslash(true);

    profile.get_simple_modifications()->push_back_pair();
    profile.get_simple_modifications()->replace_pair(
        0,
        R"({"key_code":"non_us_backslash"})",
        R"([{"key_code":"a"}])");
    profile.get_simple_modifications()->push_back_pair();
    profile.get_simple_modifications()->replace_pair(
        1,
        R"({"key_code":"grave_accent_and_tilde"})",
        R"([{"key_code":"b"}])");

    environment.device_key_code_manipulator_manager->update(profile);
    environment.simple_modifications_manipulator_manager->update(profile);

    uint64_t time_stamp = 1;
    for (const auto usage : {grave, non_us_backslash, backslash}) {
      for (const auto value : {1, 0}) {
        manipulate(make_entries(environment.device_properties,
                                usage,
                                value,
                                krbn::absolute_time_point(time_stamp++)),
                   environment.input_queue,
                   environment.connector,
                   environment.core_configuration);
      }
    }

    expect(make_usages(*environment.output_queue) == std::vector<pqrs::hid::usage::value_t>{
                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a,
                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_a,
                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_b,
                                                         pqrs::hid::usage::keyboard_or_keypad::keyboard_b,
                                                         backslash,
                                                         backslash,
                                                     });
  };

  "an active manipulation retains its mapping across a configuration update"_test = [&] {
    test_environment environment;
    auto& profile = environment.core_configuration->get_selected_profile();
    auto device = profile.get_device(environment.device_properties->get_device_identifiers());
    device->set_swap_grave_accent_and_non_us_backslash(true);
    environment.device_key_code_manipulator_manager->update(profile);

    manipulate(make_entries(environment.device_properties,
                            grave,
                            1,
                            krbn::absolute_time_point(1)),
               environment.input_queue,
               environment.connector,
               environment.core_configuration);

    device->set_swap_grave_accent_and_non_us_backslash(false);
    environment.device_key_code_manipulator_manager->update(profile);

    manipulate(make_entries(environment.device_properties,
                            grave,
                            0,
                            krbn::absolute_time_point(2)),
               environment.input_queue,
               environment.connector,
               environment.core_configuration);

    manipulate(make_entries(environment.device_properties,
                            grave,
                            1,
                            krbn::absolute_time_point(3)),
               environment.input_queue,
               environment.connector,
               environment.core_configuration);

    device->set_swap_grave_accent_and_non_us_backslash(true);
    environment.device_key_code_manipulator_manager->update(profile);

    manipulate(make_entries(environment.device_properties,
                            grave,
                            0,
                            krbn::absolute_time_point(4)),
               environment.input_queue,
               environment.connector,
               environment.core_configuration);

    uint64_t time_stamp = 5;
    for (const auto value : {1, 0}) {
      manipulate(make_entries(environment.device_properties,
                              grave,
                              value,
                              krbn::absolute_time_point(time_stamp++)),
                 environment.input_queue,
                 environment.connector,
                 environment.core_configuration);
    }

    expect(make_usages(*environment.output_queue) == std::vector<pqrs::hid::usage::value_t>{
                                                         non_us_backslash,
                                                         non_us_backslash,
                                                         grave,
                                                         grave,
                                                         non_us_backslash,
                                                         non_us_backslash,
                                                     });
  };

  return 0;
}
