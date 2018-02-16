# Version 5.0.0

* The Vendor ID and Product ID of virtual devices has been changed. (0x0,0x0 -> 0x16c0,0x27db and 0x16c0,0x27da)


# Version 4.11.0

* Uninstaller removes `/Library/Application Support/org.pqrs` directory if it is empty.


# Version 4.10.0

* `pqrs::karabiner_virtual_hid_device::get_kernel_extension_name()` method has been added.


# Version 4.9.0

* A version string has been included into virtual device's serial number.


# Version 4.8.0

* `clear_keyboard_modifier_flags` method has been added.


# Version 4.7.0

* `dist/update.sh` has been removed. Install new kext to /Library/Extensions directly.


# Version 4.6.0

* `operator==` and `operator!=` has been added into `pointing_input` and `keyboard_event`.


# Version 4.5.0

* `is_virtual_hid_keyboard_ready` method has been added.


# Version 4.4.0

* `operator==` and `operator!=` has been added into `keyboard_initialization`.


# Version 4.3.0

* kextunload calls in `uninstall.sh` and `update.sh` has been removed in order to avoid a macOS problem.


# Version 4.2.0

* Fixed issue:
  * `update.sh` fails to copy kext when the kext file is not in /Library/Extensions and kext is already loaded.


# Version 4.1.0

* `caps_lock_delay_milliseconds` has been added into `pqrs::karabiner_virtual_hid_device::properties::keyboard_initialization`.


# Version 4.0.0

* Support `keyboard_type` in VirtualHIDKeyboard.
