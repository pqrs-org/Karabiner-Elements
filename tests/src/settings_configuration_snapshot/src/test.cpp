#include "../../../../src/apps/SettingsWindow/src/cpp/settings_configuration_snapshot.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;

  "snapshot_of_newly_connected_devices_preserves_serialized_configuration"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    auto& profile = configuration.get_selected_profile();
    const krbn::device_identifiers configured_identifiers({
        .vendor_id = pqrs::hid::vendor_id::value_t(123),
        .product_id = pqrs::hid::product_id::value_t(456),
        .is_keyboard = true,
    });
    profile.get_device(configured_identifiers)->set_ignore(true);
    const auto configuration_before = configuration.to_json();

    // Newly connected devices appear in the snapshot, but their implicit defaults
    // must not change the saved configuration or the configured device count.
    krbn::connected_devices connected_devices;
    const auto properties = krbn::device_properties::make_device_properties(nlohmann::json::parse(R"({
      "device_id": 54321,
      "device_identifiers": {"vendor_id": 789, "product_id": 123, "is_keyboard": true}
    })"));
    connected_devices.push_back_device(properties);
    settings_remembered_device_properties::get_instance().remember_connected_devices(connected_devices);

    const auto snapshot = settings_configuration_snapshot(configuration).to_json();
    const auto key = properties->get_device_identifiers().to_normalized_json().dump();
    expect(snapshot.at("selected_profile").at("devices").contains(key));
    expect(configuration.to_json() == configuration_before);
    expect(profile.not_connected_configured_devices_count(connected_devices) == size_t(1));
  };

  "snapshot_preserves_new_device_defaults"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    auto& profile = configuration.get_selected_profile();
    profile.set_ignore_pointing_device_events_by_default(false);
    krbn::connected_devices connected_devices;
    const auto mouse = krbn::device_properties::make_device_properties(nlohmann::json::parse(R"({
      "device_id": 54322,
      "device_identifiers": {"vendor_id": 789, "product_id": 124, "is_pointing_device": true}
    })"));
    const auto apple_mouse = krbn::device_properties::make_device_properties(nlohmann::json::parse(R"({
      "device_id": 54323,
      "device_identifiers": {"vendor_id": 1452, "product_id": 125, "is_pointing_device": true}
    })"));
    connected_devices.push_back_device(mouse);
    connected_devices.push_back_device(apple_mouse);
    settings_remembered_device_properties::get_instance().remember_connected_devices(connected_devices);

    const auto mouse_key = mouse->get_device_identifiers().to_normalized_json().dump();
    const auto apple_key = apple_mouse->get_device_identifiers().to_normalized_json().dump();
    auto snapshot = settings_configuration_snapshot(configuration).to_json();
    const auto& devices = snapshot.at("selected_profile").at("devices");
    expect(devices.at(mouse_key).at("ignore") == false);
    // Apple pointing devices are ignored unless explicitly configured otherwise.
    expect(devices.at(apple_key).at("ignore") == true);

    profile.get_device(apple_mouse->get_device_identifiers())->set_ignore(false);
    snapshot = settings_configuration_snapshot(configuration).to_json();
    expect(snapshot.at("selected_profile").at("devices").at(apple_key).at("ignore") == false);
  };

  return 0;
}
