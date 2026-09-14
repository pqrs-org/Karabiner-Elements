#include "../../../src/apps/CoreService/include/core_service/daemon/device_grabber_details/simple_modifications_manipulator_manager.hpp"
#include "event_queue/utility.hpp"
#include "manipulator/manipulator_managers_connector.hpp"
#include <boost/ut.hpp>

namespace {
krbn::event_queue::not_null_entries_ptr_t make_horizontal_wheel_entries(
    pqrs::not_null_shared_ptr_t<krbn::device_properties> device_properties,
    int value,
    krbn::absolute_time_point time_stamp) {
  std::vector<pqrs::osx::iokit_hid_value> hid_values{
      pqrs::osx::iokit_hid_value(time_stamp,
                                 value,
                                 pqrs::hid::usage_page::consumer,
                                 pqrs::hid::usage::consumer::ac_pan,
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

std::vector<std::string> make_button_events(const krbn::event_queue::queue& queue) {
  std::vector<std::string> result;
  for (const auto& entry : queue.get_entries()) {
    if (auto event = entry.get_event().get_if<krbn::momentary_switch_event>()) {
      if (event->pointing_button()) {
        auto name = krbn::momentary_switch_event_details::pointing_button::make_name(event->get_usage_pair().get_usage());
        auto action = entry.get_event_type() == krbn::event_type::key_down ? "key_down" : "key_up";
        result.push_back(name + " " + action);
      }
    }
  }
  return result;
}
} // namespace

void run_reload_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "a held `to_buttons` press is released even when the manipulator is rebuilt mid-hold"_test = [] {
    auto core_configuration = std::make_shared<krbn::core_configuration::core_configuration>();
    auto device_properties = std::make_shared<krbn::device_properties>(
        krbn::device_properties::initialization_parameters{
            .device_id = krbn::device_id(1),
            .vendor_id = pqrs::hid::vendor_id::value_t(0x1234),
            .product_id = pqrs::hid::product_id::value_t(0x5678),
            .is_pointing_device = true,
        });
    auto manager = std::make_shared<krbn::core_service::daemon::device_grabber_details::simple_modifications_manipulator_manager>();
    auto input_queue = std::make_shared<krbn::event_queue::queue>();
    auto output_queue = std::make_shared<krbn::event_queue::queue>();

    krbn::manipulator::manipulator_managers_connector connector;
    connector.emplace_back_connection(pqrs::make_weak(manager->get_manipulator_manager()),
                                      input_queue,
                                      output_queue);
    connector.set_manipulator_environment_core_configuration(core_configuration);

    output_queue->get_manipulator_environment().insert_device_properties(
        device_properties->get_device_id(),
        device_properties);

    auto& profile = core_configuration->get_selected_profile();
    auto device = profile.get_device(device_properties->get_device_identifiers());
    device->set_mouse_horizontal_wheel_to_buttons(true);
    manager->update(profile);

    // Tilt left: button30 goes down.
    manipulate(make_horizontal_wheel_entries(device_properties, -1, krbn::absolute_time_point(1)),
              input_queue, connector, core_configuration);

    // Any settings change rebuilds every manipulator (`invalidate_manipulators` followed by
    // recreating them from the profile), even ones unrelated to this device or setting.
    // The manipulator that pressed button30 above is superseded here, while the switch is
    // still held.
    manager->update(profile);

    // The tilt is released. Without the fix, the superseded manipulator can no longer run
    // (`validity_ == invalid`) and the fresh instance never learns that button30 was down,
    // so button30 would stay stuck pressed forever.
    manipulate(make_horizontal_wheel_entries(device_properties, 0, krbn::absolute_time_point(2)),
              input_queue, connector, core_configuration);

    expect(make_button_events(*output_queue) == std::vector<std::string>{
                                                     "button30 key_down",
                                                     "button30 key_up",
                                                 });
  };
}
