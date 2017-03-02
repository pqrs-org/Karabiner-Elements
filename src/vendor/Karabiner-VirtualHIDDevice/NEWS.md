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
