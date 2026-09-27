#include "../../../../src/apps/SettingsWindow/src/cpp/settings_configuration_snapshot.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;

  "snapshot_of_newly_connected_devices_does_not_mutate_profile"_test = [] {
    krbn::core_configuration::core_configuration configuration;
    auto& profile = configuration.get_selected_profile();
    const krbn::device_identifiers configured_identifiers({
        .vendor_id = pqrs::hid::vendor_id::value_t(123),
        .product_id = pqrs::hid::product_id::value_t(456),
        .is_keyboard = true,
    });
    profile.get_device(configured_identifiers)->set_ignore(true);
    const auto configuration_before = configuration.to_json();
    const auto devices_before = profile.get_devices();

    // A device notification triggers snapshot generation on the dispatcher while
    // the main thread may be counting configured devices. A newly seen device
    // must appear in the snapshot without being appended to the shared profile.
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
    // Comparing serialized JSON alone misses implicit devices whose defaults
    // are omitted from to_json(). Compare the actual device list as well.
    expect(profile.get_devices() == devices_before);
    expect(configuration.to_json() == configuration_before);
    expect(profile.not_connected_configured_devices_count(connected_devices) == size_t(1));
  };

  return 0;
}
