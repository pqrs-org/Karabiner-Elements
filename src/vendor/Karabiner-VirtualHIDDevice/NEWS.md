# Changelog

## Version 6.9.0

- LED has been supported on VirtualHIDKeyboard. <br>
  A HID report {kHIDPage_LEDs, kHIDUsage_LED_CapsLock} will be posted to user space when the VirtualHIDKeyboard LED state is changed.

## Version 6.8.0

- The kext binary has been built on macOS 10.12 in order to avoid kextload issue on macOS 10.12.

## Version 6.7.0

- Report Count of virtual keyboard has been increased. (6 -> 32)

## Version 6.6.0

- `pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization::caps_lock_delay_milliseconds` has been removed.

## Version 6.5.0

- `pqrs::karabiner_virtual_hid_device::hid_report::*::empty` has been added.

## Version 6.4.0

- Add const qualifier to `pqrs::karabiner_virtual_hid_device::hid_report::*::exists`.

## Version 6.3.0

- `pqrs::karabiner_virtual_hid_device::hid_report::modifiers::operator==` has been added.
- `pqrs::karabiner_virtual_hid_device::hid_report::buttons::operator==` has been added.

## Version 6.2.0

- `pqrs::karabiner_virtual_hid_device::hid_report::modifiers::get_raw_value` has been added.
- `pqrs::karabiner_virtual_hid_device::hid_report::keys::get_raw_value` has been added.

## Version 6.1.0

- `pqrs::karabiner_virtual_hid_device::hid_report` interfaces have been improved.

## Version 6.0.0

- macOS 10.11 support has been dropped.
- `kHIDUsage_AppleVendor_TopCase` and `kHIDUsage_AppleVendor_Keyboard` have been supported in `post_keyboard_input_report`.
- `pqrs::karabiner_virtual_hid_device::properties::country_code` has been added.
- `pqrs::karabiner_virtual_hid_device::properties::keyboard_type` has been removed.
- `pqrs::karabiner_virtual_hid_device_methods::dispatch_keyboard_event` has been removed.
- `pqrs::karabiner_virtual_hid_device_methods::clear_keyboard_modifier_flags` has been removed.

## Version 5.0.0

- The Vendor ID and Product ID of virtual devices has been changed. (0x0,0x0 -> 0x16c0,0x27db and 0x16c0,0x27da)

## Version 4.11.0

- Uninstaller removes `/Library/Application Support/org.pqrs` directory if it is empty.

## Version 4.10.0

- `pqrs::karabiner_virtual_hid_device::get_kernel_extension_name()` method has been added.

## Version 4.9.0

- A version string has been included into virtual device's serial number.

## Version 4.8.0

- `clear_keyboard_modifier_flags` method has been added.

## Version 4.7.0

- `dist/update.sh` has been removed. Install new kext to /Library/Extensions directly.

## Version 4.6.0

- `operator==` and `operator!=` has been added into `pointing_input` and `keyboard_event`.

## Version 4.5.0

- `is_virtual_hid_keyboard_ready` method has been added.

## Version 4.4.0

- `operator==` and `operator!=` has been added into `keyboard_initialization`.

## Version 4.3.0

- kextunload calls in `uninstall.sh` and `update.sh` has been removed in order to avoid a macOS problem.

## Version 4.2.0

- Fixed issue:
  - `update.sh` fails to copy kext when the kext file is not in /Library/Extensions and kext is already loaded.

## Version 4.1.0

- `caps_lock_delay_milliseconds` has been added into `pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization`.

## Version 4.0.0

- Support `keyboard_type` in VirtualHIDKeyboard.
