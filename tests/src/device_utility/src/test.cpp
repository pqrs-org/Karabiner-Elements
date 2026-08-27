#include "../../share/ut_helper.hpp"
#include "device_utility.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  // Verifies the effective ignore value when pointing devices are enabled by
  // default: ordinary pointing devices remain enabled, Apple pointing devices
  // (including the built-in FIFO device) are ignored as a safeguard, and an
  // explicit per-device setting takes precedence over that safeguard.
  "device_utility.determine_should_ignore_device"_test = [] {
    krbn::core_configuration::core_configuration core_configuration;
    auto& profile = core_configuration.get_selected_profile();
    profile.set_ignore_pointing_device_events_by_default(false);

    auto pointing_device = krbn::device_properties(
        krbn::device_properties::initialization_parameters{
            .vendor_id = pqrs::hid::vendor_id::value_t(1234),
            .product_id = pqrs::hid::product_id::value_t(5678),
            .is_pointing_device = true,
        });
    expect(!krbn::device_utility::determine_should_ignore_device(core_configuration,
                                                                 pointing_device));

    auto apple_pointing_device = krbn::device_properties(
        krbn::device_properties::initialization_parameters{
            .vendor_id = pqrs::hid::vendor_id::value_t(0x05ac),
            .product_id = pqrs::hid::product_id::value_t(1234),
            .is_pointing_device = true,
        });
    expect(krbn::device_utility::determine_should_ignore_device(core_configuration,
                                                                apple_pointing_device));

    // No actual Apple device is currently known to report both keyboard and
    // pointing-device capabilities. This hypothetical case verifies that its
    // pointing-device functionality would still be protected.
    auto apple_composite_device = krbn::device_properties(
        krbn::device_properties::initialization_parameters{
            .vendor_id = pqrs::hid::vendor_id::value_t(0x05ac),
            .product_id = pqrs::hid::product_id::value_t(4321),
            .is_keyboard = true,
            .is_pointing_device = true,
        });
    expect(krbn::device_utility::determine_should_ignore_device(core_configuration,
                                                                apple_composite_device));

    auto fifo_pointing_device = krbn::device_properties(
        krbn::device_properties::initialization_parameters{
            .transport = "FIFO",
            .is_pointing_device = true,
        });
    expect(krbn::device_utility::determine_should_ignore_device(core_configuration,
                                                                fifo_pointing_device));

    // An explicit per-device setting takes precedence over the Apple pointing
    // device safeguard.
    profile.get_device(apple_pointing_device.get_device_identifiers())->set_ignore(false);
    expect(!krbn::device_utility::determine_should_ignore_device(core_configuration,
                                                                 apple_pointing_device));

    profile.get_device(apple_pointing_device.get_device_identifiers())->set_ignore(true);
    expect(krbn::device_utility::determine_should_ignore_device(core_configuration,
                                                                apple_pointing_device));
  };

  return 0;
}
