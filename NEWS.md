# Changelog

## Karabiner-Elements 15.4.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v15.4.0/Karabiner-Elements-15.4.0.dmg)
-   📅 Release date
    -   Jun 29, 2025
-   🐛 Bug Fixes
    -   Fixed an overflow issue with the mouse key movements.
    -   Fixed an issue that the fn key is unintentionally tapped when use the media keys if "Use all F1, F2, etc. keys as standard function keys" is enabled.
-   ✨ New Features
    -   Supported headset buttons.
    -   Added `Ignore vendor events` option to device settings in Devices tab.
    -   Added `XY movement multiplier` and `Wheels multiplier` to the mouse settings in Devices tab.
    -   Added `frontmost_application_history_index` option into `open_application`.
    -   Added `conditions` option into `to`.
    -   Added `Filter useless events from specific devices` option in Expert tab.
    -   Added `Reorder same timestamp input events to prioritize modifiers` in Expert tab.
    -   Added a toggle to EventViewer for monitoring events.
-   ⚡️ Improvements
    -   Migrated to the SwiftUI life cycle.
    -   Migrated to Swift 6.
    -   Improved `to_delayed_action` behavior to use the modifier flags of the time the key is pressed, when sending events.
    -   Supported the following keys:
        -   consumer::ac_zoom_out
        -   consumer::ac_zoom_in
        -   generic_desktop::system_sleep
    -   Added support for new consumer keys (Thanks to @indygriffiths):
        -   `{ "consumer_key_code": "stop" }`
        -   `{ "consumer_key_code": "bass_boost" }`
        -   `{ "consumer_key_code": "loudness" }`
        -   `{ "consumer_key_code": "bass_increment" }`
        -   `{ "consumer_key_code": "bass_decrement" }`
        -   `{ "consumer_key_code": "ac_search" }`
    -   The implicit conversion of Touch ID to the Lock key has been modified to work even when modifiers are held down.

## Karabiner-Elements 15.3.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v15.3.0/Karabiner-Elements-15.3.0.dmg)
-   📅 Release date
    -   Oct 28, 2024
-   🐛 Bug Fixes
    -   Fixed the behavior when setting the function keys to act as regular f1...f12 keys instead of media controls.
-   ✨ New Features
    -   Added the following variables for `variable_if`, `variable_unless`:
        -   `system.scroll_direction_is_natural`
        -   `system.use_fkeys_as_standard_function_keys`
-   ⚡️ Improvements
    -   The key names in the Function Keys settings became clearer when "Use all F1, F2, etc. keys as standard function keys" is enabled.

## Karabiner-Elements 15.2.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v15.2.0/Karabiner-Elements-15.2.0.dmg)
-   📅 Release date
    -   Oct 7, 2024
-   🐛 Bug Fixes
    -   The method for setting "Use all F1, F2, etc. keys as standard function keys" has been changed.
        Due to internal processing in macOS, with the new virtual keyboard introduced in version 15.1.0, this setting must be changed through System Settings.

## Karabiner-Elements 15.1.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v15.1.0/Karabiner-Elements-15.1.0.dmg)
-   📅 Release date
    -   Oct 6, 2024
-   🔔 Important Notes
    -   After the upgrade, you have to set keyboard type and restart macOS.
        <table><tbody><tr><td><!-- border hack -->
        <img src="https://karabiner-elements.pqrs.org/docs/releasenotes/images/v15.1.0/keyboard-type-selector@2x.png?v=2" alt="keyboard type selector" />
        </td></tr></tbody></table>
        <br/><br/>
        <table><tbody><tr><td><!-- border hack -->
        <img src="https://karabiner-elements.pqrs.org/docs/releasenotes/images/v15.1.0/macos-restart-required@2x.png" alt="macOS restart required" />
        </td></tr></tbody></table>
-   💥 Breaking changes
    -   The implicit conversion feature for fn+arrow keys, fn+return, and fn+delete has been removed.
        For example, fn+up arrow used to be automatically changed to page up, but it will now be treated as fn+up arrow without modification.
    -   To improve the behavior around the fn key, the Vendor ID and Product ID of the virtual keyboard used by Karabiner-Elements have been changed to the same values as those of a real Apple external keyboard.
    -   There were two JSON files that output information about connected devices, but this information has now been consolidated into `karabiner_grabber_devices.json`.
        `karabiner_grabber_device_details.json` is no longer used. Please use `karabiner_grabber_devices.json` from now on.
        -   `karabiner_grabber_devices.json`
        -   `karabiner_grabber_device_details.json` (obsoleted)
-   ✨ New Features
    -   Added [open_application](https://karabiner-elements.pqrs.org/docs/json/complex-modifications-manipulator-definition/to/software_function/open_application/) to complex modifications.
    -   A setting has been added to disable mouse cursor movement and scrolling for both the mouse and gamepad.
    -   Settings of the gamepad stick have been added; deadzone and delta magnitude detection threshold.
    -   Added an option to allow quitting EventViewer with Command+Q and Command+W.
-   ⚡️ Improvements
    -   Fixed an issue where disconnected devices would occasionally remain in the device list.
    -   Updated the embedded screenshots and help messages for macOS 15 Sequoia.
    -   Adjusted the gamepad deadzone default value.

## Karabiner-Elements 15.0.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v15.0.0/Karabiner-Elements-15.0.0.dmg)
-   📅 Release date
    -   Aug 8, 2024
-   💥 Breaking changes
    -   macOS 11 and macOS 12 are no longer supported.
    -   The background service management method has been changed to a new method compatible with macOS 13 and later.
        Therefore, permission needs to be granted for the background service to run. Please follow the steps below.
        -   Open Karabiner-Elements Settings.
        -   Follow the instructions to allow Karabiner-Elements Privileged Daemons to run in the System Settings.
            <br/><br/>
            <table><tbody><tr><td><!-- border hack -->
            <img src="https://karabiner-elements.pqrs.org/docs/releasenotes/images/v15.0.0/settings@2x.png" alt="Karabiner-Elements Setttings" />
            </td></tr></tbody></table>
    -   The method to enable the Multitouch Extension has changed.
        To enable the Multitouch Extension, go to the Misc tab in the settings and turn on `Enable Multitouch Extension`.
        <br/><br/>
        <table><tbody><tr><td><!-- border hack -->
        <img src="https://karabiner-elements.pqrs.org/docs/releasenotes/images/v15.0.0/multitouch-extension@2x.png" alt="Karabiner-Elements Setttings" />
        </td></tr></tbody></table>
-   🔔 Important Notes
    -   A unique random ID called `karabiner_machine_identifier` is now generated during installation.
        This ID is used for machine-specific settings and may be included in the karabiner.json file.
        If you are sharing karabiner.json with others, you might be interested in how this ID is generated and whether it is safe to share.
        Since it is a safe ID to share with others, and there is no need to mask it when you share karabiner.json with others.
        For more details, please refer to the [Privacy](https://karabiner-elements.pqrs.org/docs/privacy/) page.
        <br/><br/>
        <table><tbody><tr><td><!-- border hack -->
        <img src="https://karabiner-elements.pqrs.org/docs/releasenotes/images/v15.0.0/karabiner-machine-identifier@2x.png" alt="karabiner_machine_identifier" />
        </td></tr></tbody></table>
-   ✨ New Features
    -   Added the ability to disable Complex Modifications rules.
    -   Added the ability to duplicate and reorder profiles.
    -   Added the ability to restart Karabiner-Elements from the menu.
    -   Added the ability to reset device settings that are not currently connected.
-   ⚡️ Improvements
    -   Improved "{device} is ignored temporarily until {key_code} is pressed again" message for to make it more understandable. (Thanks to @adamnovak)
    -   Added "Enable Karabiner Notification Window" configuration, which allows you to hide the notification window by turning off this setting. (Thanks to @jwodnicki)
    -   Support `al_consumer_control_configuration` key, used as the Music key. (Thanks to @kambala-decapitator)
    -   Improved behaviour when using the gamepad stick as a pointing device.
    -   Improved `to_if_alone` behavior to use the modifier flags of the time the key is pressed, when sending events.
    -   Added `{"type": "unset"}` option to `set_variable`.
    -   Integrated `karabiner_observer` functions into `karabiner_grabber`, and then `karabiner_observer` has been removed.
        The number of background processes has been reduced, resolving performance issues caused by inter-process communication that were occurring in some environments.
    -   A custom JSON formatter has been introduced to save JSON in a more compact format.
    -   The process for saving the configuration file has been optimized so that settings that have not been changed from their default values are not included in karabiner.json.
    -   Internal changes:
        -   Improved the interface of libkrbn to minimize the use of unsafe pointers in Swift.

## Karabiner-Elements 14.13.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.13.0/Karabiner-Elements-14.13.0.dmg)
-   📅 Release date
    -   Dec 9, 2023
-   🔔 Important Notes
    -   A reboot is required when upgrading from version 14.12.0 or earlier, to update the virtual keyboard driver.
-   ✨ New Features
    -   Palm detection is supported in MultitouchExtension. (Thanks to @quarkw)
    -   Use Bluetooth address (Bluetooth MAC address) is used to identify the device when the Bluetooth device does not have Vendor ID and Product ID. (Thanks to @stackia)
    -   Add the following settings into Devices tab.
        -   `Flip mouse X`
        -   `Flip mouse Y`
        -   `Flip mouse vertical wheel`
        -   `Flip mouse horizontal wheel`
        -   `Swap mouse X and Y`
        -   `Swap mouse wheels`
    -   Gamepad support (DirectInput devices are supported. Xinput is not supported at all)
    -   Added "Add your own rule" button into Complex Modifications settings.
    -   The json content of Complex Modifications can now be changed via the Edit button.
    -   Added "Move item to top" and "Move item to bottom" into the context menu of the Complex Modifications list.
    -   Add the following options to `karabiner_cli`
        -   `--format-json`
        -   `--eval-js`
        -   `--silent`
    -   Changes for users who write their own json.
        -   The `device_address` condition has been added into `device_if` and `device_unless`. (Thanks to @stackia)
        -   Added `key_up_value` to `set_variable`.
        -   Added `mouse_basic` manipulator.
-   ⚡️ Improvements
    -   Added the ability to filter by keywords when adding Complex Modifications rules.
    -   Adjusted the position of the Add rule button in Complex Modifications. (Thanks to @GanZhiXiong)
    -   MultitouchExtension has been rewritten in Swift.

## Karabiner-Elements 14.12.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.12.0/Karabiner-Elements-14.12.0.dmg)
-   📅 Release date
    -   Apr 16, 2023
-   ✨ New Features
    -   Added the ability to switch application icons. This can be changed from the UI tab in Settings.
        -   [Manual](https://karabiner-elements.pqrs.org/docs/manual/misc/change-app-icon/)
    -   Added new application icons. (Thanks to @Zabriskije)
    -   Added the following variables in MultitouchExtension. (Thanks to @codeanpeace)
        -   multitouch_extension_finger_count_upper_quarter_area
        -   multitouch_extension_finger_count_lower_quarter_area
        -   multitouch_extension_finger_count_left_quarter_area
        -   multitouch_extension_finger_count_right_quarter_area
-   ⚡️ Improvements
    -   Prevent moving the mouse from waking up from sleep by ungrab devices during system sleep. (Thanks to @aspacca)
    -   Fixed inconsistent and confusing brightness label in Simple Modifications. (Thanks to @revolter)
    -   Improved MultitouchExtension to synchronize finger count (half and quarter) variables with finger movement.
    -   Moved "Disable the built-in keyboard while this device is connected" settings into Devices tab from Devices > Advanced tab in Settings.
-   🐛 Bug Fixes
    -   Fixed an issue that dynamic keyboard type change (ANSI, ISO and JIS) by changing the country code of the virtual keyboard did not work properly.

## Karabiner-Elements 14.11.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.11.0/Karabiner-Elements-14.11.0.dmg)
-   📅 Release date
    -   Jan 7, 2023
-   💥 Breaking changes
    -   Changed bundle identifier of Karabiner-Elements Settings to `org.pqrs.Karabiner-Elements.Settings` from `org.pqrs.Karabiner-Elements.Preferences`.
-   ✨ New Features
    -   Added `Ask for confirmation when quitting` option into Settings > Misc tab. (Thanks to @basti1302)
-   ⚡️ Improvements
    -   Improved health checks for inter-process communication to better recover from errors.
-   🐛 Bug Fixes
    -   Fixed an issue that uninstaller does not remove Karabiner-Elements.app and Karabiner-EventViewer.app in macOS Ventura.
    -   Fixed an issue that could cause multiple update windows will be shown.

## Karabiner-Elements 14.10.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.10.0/Karabiner-Elements-14.10.0.dmg)
-   📅 Release date
    -   Sep 25, 2022
-   ⚡️ Improvements
    -   Update `set_mouse_cursor_position` to use the current screen if `screen` option is not specified. (Thanks to @gnawf)
    -   Sparkle Framework has been updated.
-   🐛 Bug Fixes
    -   Fixed an issue that the `device_if` and `device_unless` does not work if vendor_id or product_id is zero. (or more precisely, if vendor_id or product_id cannot be retrieved and is set to 0)

## Karabiner-Elements 14.9.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.9.0/Karabiner-Elements-14.9.0.dmg)
-   📅 Release date
    -   Sep 11, 2022
-   ✨ New Features
    -   Supported Remote control buttons. (menu arrows buttons, color buttons)
    -   The `is_built_in_keyboard` has been added into `device_if` and `device_unless`.
    -   The `device_exists_if` and `device_exists_unless` has been added into `conditions`.
-   🐛 Bug Fixes
    -   Fixed an issue that the ignored device's modifier keys are not ignored properly.

## Karabiner-Elements 14.8.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.8.0/Karabiner-Elements-14.8.0.dmg)
-   📅 Release date
    -   Aug 11, 2022
-   ✨ New Features
    -   Add `Enable unsafe configuration` feature into Preferences > Pro tab.
-   🐛 Bug Fixes
    -   Fixed an issue that the built-in keyboard is not be treated as the built-in keyboard if the keyboard is connected via SPI (Serial Peripheral Interface), such as M2 MacBook Air.

## Karabiner-Elements 14.6.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.6.0/Karabiner-Elements-14.6.0.dmg)
-   📅 Release date
    -   Jul 31, 2022
-   ✨ New Features
    -   `Treat as a built-in keyboard` setting has been added into Devices configuration. It works with "Disable the built-in keyboard" feature.
-   🐛 Bug Fixes
    -   Fixed an issue that "Disable the built-in keyboard" feature does not work if the built-in keyboard is not included in the event modification target devices.

## Karabiner-Elements 14.5.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.5.0/Karabiner-Elements-14.5.0.dmg)
-   📅 Release date
    -   Jul 17, 2022
-   💥 Breaking changes
    -   The following modifier flags and manipulators are now canceled even for events from devices not grabbed by Karabiner-Elements, e.g. mouse clicks.
        -   `sticky modifiers`
        -   `to_if_alone`
        -   `to_if_held_down`
        -   `to_delayed_action`
-   ⚡️ Improvements
    -   Improved behavior the sticky modifiers when used together with the built-in trackpad. (Thanks to @quarkw)
    -   Preferences Window is rewritten in SwiftUI.
    -   `set_variable` now supports bool and string value.
    -   Sparkle Framework has been updated.

## Karabiner-Elements 14.4.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.4.0/Karabiner-Elements-14.4.0.dmg)
-   📅 Release date
    -   Mar 1, 2022
-   🐛 Bug Fixes
    -   Fixed an issue that NotificationWindow causes high CPU usage in some environments.

## Karabiner-Elements 14.3.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.3.0/Karabiner-Elements-14.3.0.dmg)
-   📅 Release date
    -   Nov 20, 2021
-   ⚡️ Improvements
    -   Added System Extensions tab to EventViewer.
    -   Added a workaround for macOS's ioreg command issue that consumes high CPU usage on macOS Monterey.

## Karabiner-Elements 14.2.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v14.2.0/Karabiner-Elements-14.2.0.dmg)
-   📅 Release date
    -   Nov 3, 2021
-   💥 Breaking changes
    -   macOS 10.15 support has been dropped.
-   ✨ New Features
    -   Supported Touch ID on Magic Keyboard.
    -   Supported application launcher keys (Mail, Browser, Calculator, etc.) which are belong to HID Consumer Usage Page and Application Launch Buttons Usage.
    -   Add new features for writing your own settings:
        -   Added [`software_function.iokit_power_management_sleep_system`](https://karabiner-elements.pqrs.org/docs/json/complex-modifications-manipulator-definition/to/software_function/iokit_power_management_sleep_system/).
-   🐛 Bug Fixes
    -   Fixed an issue that an alert window appears in an incorrect position on macOS 12 Monterey.
-   ⚡️ Improvements
    -   Update application icons. (Thanks to Kouji TAMURA)
    -   EventViewer is rewritten in SwiftUI.

## Karabiner-Elements 13.7.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v13.7.0/Karabiner-Elements-13.7.0.dmg)
-   📅 Release date
    -   Sep 6, 2021
-   🐛 Bug Fixes
    -   Fix an issue that `shell_command` does not set environment variables (HOME, USER, etc.).

## Karabiner-Elements 13.6.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v13.6.0/Karabiner-Elements-13.6.0.dmg)
-   📅 Release date
    -   Sep 3, 2021
-   ✨ New Features
    -   Add new features for writing your own settings:
        -   Added [`software_function.cg_event_double_click`](https://karabiner-elements.pqrs.org/docs/json/complex-modifications-manipulator-definition/to/software_function/cg_event_double_click/).
        -   Added [`software_function.set_mouse_cursor_position`](https://karabiner-elements.pqrs.org/docs/json/complex-modifications-manipulator-definition/to/software_function/set_mouse_cursor_position/).
        -   Added [`set_notification_message`](https://karabiner-elements.pqrs.org/docs/json/complex-modifications-manipulator-definition/to/set-notification-message/).
-   🐛 Bug Fixes
    -   Fixed an issue that karabiner.json parse error message is not shown in Preferences > Log if the error message contains corrupted characters.
-   ⚡️ Improvements
    -   Changed to log the output of shell_command into console_user_server.log.
    -   The notification window transparency will be increased when the mouse cursor is hovered.

## Karabiner-Elements 13.5.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v13.5.0/Karabiner-Elements-13.5.0.dmg)
-   📅 Release date
    -   Jun 20, 2021
-   ⚡️ Improvements
    -   Improved temporary directory handling to reduce the frequency of mds process accessing it.
    -   Improved `mouse_key.speed_multiplier` handling when it is used with `to.modifiers`, the modifier is retained while speed_multiplier is active.
    -   The caps lock LED manipulation has been enabled by default.
        -   Note: This change applies to newly connected keyboards.
    -   Several Objective-C code around GUI has been rewritten in Swift + SwiftUI.

## Karabiner-Elements 13.4.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v13.4.0/Karabiner-Elements-13.4.0.dmg)
-   📅 Release date
    -   Apr 4, 2021
-   🐛 Bug Fixes
    -   Fixed an issue that Karabiner-Elements stops working with `virtual_hid_keyboard is not ready. Please wait for a while.` error message when you kill `karabiner_console_user_server` process manually.
-   ⚡️ Improvements
    -   Changed to terminate virtual HID devices immediately after quit Karabiner-Elements.

### 🔔 Notes for Mac computers with Apple Silicon (Apple M1) users

macOS Big Sur 11.2 or earlier has an issue within memory management and HID device handling that causes kernel panics on Mac computers with Apple Silicon.
The virtual device that is used internally by Karabiner-Elements triggers this issue, and a kernel panic might be caused at macOS shutdown.

We confirmed that this issue has been fixed in [macOS Big Sur 11.3 Beta](https://beta.apple.com/sp/betaprogram/).
Please use macOS Big Sur 11.3 Beta or later versions if you are facing the kernel panic issue.

## Karabiner-Elements 13.3.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v13.3.0/Karabiner-Elements-13.3.0.dmg)
-   📅 Release date
    -   Jan 29, 2021
-   🐛 Bug Fixes
    -   Fixed an issue that installer requires Rosetta 2 on Apple Silicon Macs.
-   💥 Breaking changes
    -   Changed the default function of f4 key to `spotlight`, f5 key to `dictation`, f6 key to `f6`.
        -   Note: This change will be applied when you create a new profile.
    -   Changed to Karabiner-Elements does not modify f1-f12 keys on touch bar.
    -   The caps lock delay of Karabiner Virtual HID Keyboard has been removed.
        The state of caps lock will be changed immediately after you press the caps lock key.
-   ✨ New Features
    -   Supported [`sticky modifier keys`](https://karabiner-elements.pqrs.org/docs/help/how-to/sticky-modifier-key/)
    -   Added `apple_vendor_keyboard_key_code` and `apple_vendor_top_case_key_code` to event definitions.
        -   [apple_vendor_keyboard_key_code](https://github.com/pqrs-org/Karabiner-Elements/blob/main/src/share/types/momentary_switch_event_details/apple_vendor_keyboard_key_code.hpp)
        -   [apple_vendor_top_case_key_code](https://github.com/pqrs-org/Karabiner-Elements/blob/main/src/share/types/momentary_switch_event_details/apple_vendor_top_case_key_code.hpp)
-   ⚡️ Improvements
    -   Updated in-app screenshots to Big Sur.
    -   Added the close button to notification window.

## Karabiner-Elements 13.1.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v13.1.0/Karabiner-Elements-13.1.0.dmg)
-   📅 Release date
    -   Oct 30, 2020
-   🔔 Important Notes
    -   **Restarting macOS is required** after upgrading from v13.0.0.<br />
        The following alert will be shown after upgrade. Please follow the instructions.<br />
        <img src="https://karabiner-elements.pqrs.org/docs/releasenotes/images/v13.1.0/driver-version-not-matched-alert.png" alt="driver version not matched alert" width="400" />
-   🐛 Bug Fixes
    -   Fixed an issue that Karabiner-DriverKit-VirtualHIDDevice might crash when caps lock key is pressed in some environments.

## Karabiner-Elements 13.0.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v13.0.0/Karabiner-Elements-13.0.0.dmg)
-   📅 Release date
    -   Oct 4, 2020
-   💥 Breaking changes
    -   macOS 10.12 - 10.14 support has been dropped.
-   ✨ New Features
    -   Supported macOS Big Sur (11.0)
    -   Supported both Intel-based Macs and Apple Silicon Macs.
    -   Changed the virtual keyboard and mouse implementation to DriverKit from deprecated kernel extension.
-   ⚡️ Improvements
    -   Improved preferences window messages.
    -   Partial support for comments in karabiner.json configuration file.
        -   Supported reading json file with comments.
        -   Limitation: The json comments will be removed if you change the json from Preferences GUI or command line interface.

## Karabiner-Elements 12.10.0

-   [📦 Download](https://github.com/pqrs-org/Karabiner-Elements/releases/download/v12.10.0/Karabiner-Elements-12.10.0.dmg)
-   📅 Release date
    -   Jun 27, 2020
-   ✨ New Features
    -   `event_changed_if` and `event_changed_unless` has been added to `conditions`.
-   ⚡️ Improvements
    -   Improved sending f1-f12 keys in complex modification (e.g., "change command+e to f2") by ignoring media key mappings for these keys.
    -   Improved caps lock LED handling.
    -   Improved uninstaller adding the kernel extension staging area clean up.
    -   Improved complex modifications json checker.
    -   "Check for updates" has been improved.
        -   Updated Sparkle signing to EdDSA (ed25519) from DSA.
        -   URL of appcast.xml has been updated.

## Karabiner-Elements 12.9.0

-   📅 Release date
    -   Jan 18, 2020
-   ✨ New Features
    -   `Open config folder` button has been added into Preferences.<br>
        The feature provides [an easy way exporting configuration](https://karabiner-elements.pqrs.org/docs/manual/operation/export/).
-   ⚡️ Improvements
    -   Added a workaround for non-dismissible `Device is ignored temporarily` alert
        which is caused by some devices that sends abnormal input events.
-   🐛 Bug Fixes
    -   Fixed `Add {key_code} to Karabiner-Elements` button on EventViewer for unnamed keys (raw number key codes).
    -   Fixed an issue that modifier flag events are not dispatched when changing pointing button while other character keys are pressed.
        (e.g., when changing pointing_button::button3 -> command+pointing_button::button1, command modifier is not sent when button1 is pressed while `t` key is also pressed.)

## Karabiner-Elements 12.8.0

-   📅 Release date
    -   Nov 17, 2019
-   ✨ New Features
    -   Add `--show-current-profile-name` option into `karabiner_cli`.
    -   Add `--list-profile-names` option into `karabiner_cli`.
-   🐛 Bug Fixes
    -   Fixed an issue that Karabiner-Elements stops working after switching user on macOS Catalina.
    -   Fixed an issue that Caps Lock LED is always manipulated even `Manipulate LED` setting is off.
    -   Fixed an issue that MultitouchExtension does not handle ignored area properly when a finger is touched ignored area repeatedly.

## Karabiner-Elements 12.7.0

-   📅 Release date
    -   Sep 12, 2019
-   ✨ New Features
    -   MultitouchExtension app has been added.
        -   Documentation:
            -   [Usage](https://karabiner-elements.pqrs.org/docs/manual/misc/multitouch-extension/)
            -   [How to integrate MultitouchExtension to your complex modifications](https://karabiner-elements.pqrs.org/docs/json/extra/multitouch-extension/)
    -   Dark Mode has been supported.
    -   Added `--set-variables` option into `karabiner_cli`.
-   🐛 Bug Fixes
    -   Fixed an issue that Karabiner-Elements might stop working after sleep on macOS Catalina.

## Karabiner-Elements 12.6.0

-   📅 Release date
    -   Aug 16, 2019
-   ✨ New Features
    -   Support macOS Catalina.
-   🐛 Bug Fixes
    -   Fixed an issue that Karabiner-Elements fails to grab a device in rare cases.
-   ⚡️ Improvements
    -   Introduced karabiner_kextd.
        (kext loading function was separated from karabiner_grabber.)
    -   Improved EventViewer to show modifier flags of key events.
    -   Suppressed unnecessary log messages.

## Karabiner-Elements 12.5.0

-   📅 Release date
    -   Jun 10, 2019
-   ✨ New Features
    -   Add `Delay before open device` configuration into `Karabiner-Elements Preferences > Devices > Advanced` tab.
    -   Added `Show key code in hexadecimal format` option into EventViewer.
-   🐛 Bug Fixes
    -   Fixed a key stuck issue which occurs when the key is released,
        and at the exact same moment Karabiner-Elements opens the device.
    -   Fixed an issue which Karabiner-Elements mistakes a remote user for a current console user
        if another user is logged in from Screen Sharing while console is used.
-   ⚡️ Improvements
    -   Improved `XXX is ignored temporarily until YYY is pressed again` behavior.
    -   Move `Disable the built-in keyboard while one of the following selected devices is connected` configuration
        into `Karabiner-Elements Preferences > Devices > Advanced` tab.

## Karabiner-Elements 12.4.0

-   📅 Release date
    -   May 14, 2019
-   ✨ New Features
    -   Add `Mouse Key XY speed` configuration into `Karabiner-Elements Preferences > Virtual Keyboard` tab.
    -   `Device is ignored temporarily` alert has been introduced.<br/>
        This alert will be shown if you hold keys or buttons down before Karabiner-Elements opens the device.<br/>
        Please press the described key or button again to dismiss the alert.
-   🐛 Bug Fixes
    -   Fixed a key stuck issue which occurs when the key is held down before Karabiner-Elements opens the device.
-   ⚡️ Improvements
    -   Event code format on EventViewer changed to decimal number from hex.

## Karabiner-Elements 12.3.0

-   📅 Release date
    -   Apr 24, 2019
-   💥 Breaking changes
    -   complex modifications json will be checked strictly since this release.<br/>
        Please check error messages if your complex modifications do not work after upgrade.
-   ✨ New Features
    -   Added [Change mouse motion to scroll](https://ke-complex-modifications.pqrs.org/#mouse_motion_to_scroll) feature.<br/>
        -   Note: You have to enable your mice on [Devices tab](https://karabiner-elements.pqrs.org/docs/manual/configuration/configure-devices/) when you want to use this feature.
    -   Added `--lint-complex-modifications` option into `karabiner_cli`.
        It allows you checks a complex-modifications json file.
-   ⚡️ Improvements
    -   Set Karabiner-Elements.app and Karabiner-EventViewer.app immutable
        in order to ensure unremovable them except built-in uninstaller.
        Please use the [uninstaller](https://karabiner-elements.pqrs.org/docs/manual/operation/uninstall/) when you want to remove Karabiner-Elements.
    -   Added a wait before grabbing device in order to avoid an macOS issue that device will be unusable after Karabiner-Elements is quit.
    -   Changes for users who write their own json.
        -   `to` and `to_*` support single object, e.g., `"to": { "key_code": "spacebar" }`.
        -   New modifier aliases are added: `left_alt`, `left_gui`, `right_alt`, `right_gui`.
        -   `key_code`, `consumer_key_code` and `pointing_button` supports a number value, e.g., `"from": {"key_code": 175}`.

## Karabiner-Elements 12.2.0

-   📅 Release date
    -   Feb 1, 2019
-   ✨ New Features
    -   Karabiner-Elements makes a backup file of karabiner.json before updating it if the backup file does not exists.
        (~/.config/karabiner/automatic_backups/karabiner_YYYYMMDD.json)
-   🐛 Bug Fixes
    -   Fixed an issue that Caps Lock LED does not work on macOS Mojave.
-   ⚡️ Improvements
    -   `shell_command` string max length has been expanded. (256 byte -> 32 KB)
    -   A device grabbing process has been improved. (Observing device state by a separated `karabiner_observer` process.)
    -   The event processing has been improved and the latency has been reduced by using [pqrs::dispatcher](https://github.com/pqrs-org/cpp-dispatcher).

## Karabiner-Elements 12.1.0

-   📅 Release date
    -   May 30, 2018
-   💥 Breaking changes
    -   Changed the order of `to_if_alone` and `to_after_key_up` event handling.<br />
        `to_if_alone` will be handled before `to_after_key_up`.
-   ✨ New Features
    -   Added new items into `simultaneous_options`:
        -   `simultaneous_options.detect_key_down_uninterruptedly`
        -   `simultaneous_options.key_up_when`
    -   Added new parameters into `to event definition`:
        -   `hold_down_milliseconds`
        -   `halt`
-   🐛 Bug Fixes
    -   Fixed an issue that random key repeat happen at extremely high system CPU usage.
-   ⚡️ Improvements
    -   Increased rollover limit of virtual keyboard. (6 -&gt; 32)<br />
        This change mainly improves usability when you are using multiple keyboards at the same time.
    -   Improved modifier flags handling in `to_after_key_up` and `to_if_alone`.

## Karabiner-Elements 12.0.0

-   📅 Release date
    -   Apr 12, 2018
-   💥 Breaking changes
    -   macOS 10.11 support has been dropped.<br />
        Karabiner-Elements works on macOS 10.12 (Sierra) or later.
    -   `Keyboard type` in the virtual keyboard preferences has been removed. (Adverse effect of virtual keyboard improvement.)<br />
        Please change the keyboard type from `System Preferences > Keyboard > Change Keyboard Type...`.
    -   `Caps Lock Delay` in the virtual keyboard preferences has been removed. (Adverse effect of virtual keyboard improvement.)
    -   Changed `simultaneous` behaviour to post key_up events when any key is released.
    -   Changed `to_after_key_up` and `to_if_alone` behaviour as mandatory modifiers are removed from these events.
-   ✨ New Features
    -   Added `simultaneous_options.key_down_order`, `simultaneous_options.key_up_order` and `simultaneous_options.to_after_key_up`.
-   🐛 Bug Fixes
    -   Fixed an issue that `to_if_alone`, `to_if_held_down` and `to_delayed_action` does not work properly with `simultaneous`.
-   ⚡️ Improvements
    -   The virtual keyboard compatibility has been improved.
    -   EventViewer has been improved showing the correct key name for PC keyboard keys and international keys.
    -   Improved keyboard repeat handling with `simultaneous`.

## Karabiner-Elements 11.6.0

-   📅 Release date
    -   Feb 20, 2018
-   ✨ New Features
    -   Simultaneous key presses has been supported in complex modifications.
-   ⚡️ Improvements
    -   Improved Mouse key scroll wheel direction referring `System Preferences > Mouse > Scroll direction`.
    -   Improved modifier flags handling around pointing button manipulations.
    -   Mouse keys have been added into Simple Modifications.
    -   The eject key has been added into the from key of Simple Modifications.
    -   The Vendor ID and Product ID of virtual devices has been changed. (0x0,0x0 -> 0x16c0,0x27db and 0x16c0,0x27da)

## Karabiner-Elements 11.5.0

-   📅 Release date
    -   Dec 30, 2017
-   ✨ New Features
    -   `to_if_held_down` has been added.
-   🐛 Bug Fixes
    -   Avoided a VMware Remote Console issue that mouse pointer does not work properly on VMRC when Karabiner-Elements grabs the pointing device.
    -   Fixed an issue that `to_if_alone` does not work properly when `to` is empty.
-   ⚡️ Improvements
    -   Improved modifier flags handling in `to events`.
    -   Improved a way to save karabiner.json.

## Karabiner-Elements 11.4.0

-   📅 Release date
    -   Dec 7, 2017
-   ✨ New Features
    -   `mouse_key` has been added.
        -   Examples:
            -   Mouse keys (simple):
                [json](https://github.com/pqrs-org/KE-complex_modifications/blob/main/public/json/mouse_keys_simple.json)
                [(src)](https://github.com/pqrs-org/KE-complex_modifications/blob/main/src/json/mouse_keys_simple.json.erb)
            -   Mouse keys (full)
                [json](https://github.com/pqrs-org/KE-complex_modifications/blob/main/public/json/mouse_keys_full.json)
                [(src)](https://github.com/pqrs-org/KE-complex_modifications/blob/main/src/json/mouse_keys_full.json.erb)
    -   `location_id` has been added to `device_if` and `device_unless`.
-   🐛 Bug Fixes
    -   Fixed an issue that the checkbox in `Preferences > Devices` is disabled for keyboards which do not have their own vendor id.

## Karabiner-Elements 11.3.0

-   📅 Release date
    -   Nov 12, 2017
-   🐛 Bug Fixes
    -   Fixed an issue that Karabiner-11.2.0 does not work properly on some environments due to a possibility of macOS kernel extension cache problem.

## Karabiner-Elements 11.2.0

-   📅 Release date
    -   Nov 9, 2017
-   ✨ New Features
    -   Mouse button modifications has been added.<br />
        Note:
        -   You have to enable your Mouse manually in Preferences &gt; Devices tab.
        -   Karabiner-Elements cannot modify Apple's pointing devices.
    -   `to_delayed_action` has been added.
    -   `input_source_if` and `input_source_unless` has been added to `conditions`.
    -   `select_input_source` has been added.
    -   `keyboard_type_if` and `keyboard_type_unless` has been added to `conditions`.
    -   The caps lock LED manipulation has been disabled with non Apple keyboards until it is enabled manually.
-   ⚡️ Improvements
    -   The virtual keyboard handling has been improved.

## Karabiner-Elements 11.1.0

-   📅 Release date
    -   Oct 4, 2017
-   🐛 Bug Fixes
    -   Fixed an issue that modifier flags becomes improperly state by mouse events.

## Karabiner-Elements 11.0.0

-   📅 Release date
    -   Sep 18, 2017
-   ✨ New Features
    -   The first stable release of Karabiner-Elements.
        (There is no changes from Karabiner-Elements 0.91.16.)

## Karabiner-Elements 0.91.16

-   Karabiner-Elements waits grabbing device until all modifier keys are released in order to avoid modifier flags stuck issue in mouse events.
-   Support consumer keys (e.g., media key events in Logitech keyboards.)

## Karabiner-Elements 0.91.13

-   Add per device support in `Simple Modifications` and `Fn Function Keys`.
-   The modifier flag event handling has been improved.

## Karabiner-Elements 0.91.12

-   `device_if` and `device_unless` has been added to `conditions`.
    -   An example: <https://github.com/pqrs-org/KE-complex_modifications/blob/main/public/json/example_device.json>

## Karabiner-Elements 0.91.11

-   Fixed an issue that modifier flags might become improperly state in complex_modifications.
    (In complex_modifications rules which changes modifier+modifier to modifier.)

## Karabiner-Elements 0.91.10

-   macOS 10.13 (High Sierra) support has been improved.

## Karabiner-Elements 0.91.9

-   `variable_if` and `variable_unless` has been added to `conditions`.
    You can use `set_variable` to change the variables.
    -   An example: <https://github.com/pqrs-org/KE-complex_modifications/blob/ef8074892e5fff8a4781a898869f8d341b5a815a/docs/json/personal_tekezo.json>
-   `to_after_key_up` has been added to `complex_modifications > basic`.
-   `"from": { "any": "key_code" }` has been added to `complex_modifications > basic`.
    You can use this to disable untargeted keys in your mode. (e.g., disable untargeted keys in Launcher Mode.)
    -   An example: <https://github.com/pqrs-org/KE-complex_modifications/blob/ef8074892e5fff8a4781a898869f8d341b5a815a/docs/json/personal_tekezo.json#L818-L844>
-   `Variables` tab has been added into `EventViewer`.
    You can confirm the `set_variable` result in `Variables` tab.

## Karabiner-Elements 0.91.8

-   Fixed an issue that karabiner_grabber might be crashed when frontmost application is changed.

## Karabiner-Elements 0.91.7

-   Shell command execution has been supported. (e.g., Launch apps in <https://ke-complex-modifications.pqrs.org/> )

## Karabiner-Elements 0.91.6

-   The conditional event manipulation has been supported. (`frontmost_application_if` and `frontmost_application_unless`)

## Karabiner-Elements 0.91.5

-   GUI for complex_modifications has been added.
-   Syntax of `complex_modifications > parameters` has been changed.

## Karabiner-Elements 0.91.4

-   The modifier flag event handling has been improved.
-   Show warning and error logs by colored text in Log tab.

## Karabiner-Elements 0.91.3

-   Add timeout to `to_if_alone`.

## Karabiner-Elements 0.91.2

-   Initial support of `complex_modifications > basic > to_if_alone`.

## Karabiner-Elements 0.91.1

-   Fixed an issue that Karabiner-Elements stops working after user switching.
-   Initial support of `complex_modifications` (No GUI yet).

## Karabiner-Elements 0.91.0

-   event manipulation has been changed to `src/core/grabber/include/manipulator/details/basic.hpp`.
