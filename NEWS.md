# Changelog

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
                [json](https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/mouse_keys_simple.json)
                [(src)](https://github.com/pqrs-org/KE-complex_modifications/blob/master/src/json/mouse_keys_simple.json.erb)
            -   Mouse keys (full)
                [json](https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/mouse_keys_full.json)
                [(src)](https://github.com/pqrs-org/KE-complex_modifications/blob/master/src/json/mouse_keys_full.json.erb)
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
    -   An example: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/example_device.json>

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

-   GUI for complex_modifications has been added. <https://github.com/pqrs-org/Karabiner-Elements/tree/master/usage#how-to-use-complex-modifications>
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
