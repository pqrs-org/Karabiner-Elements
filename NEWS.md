# Changelog

## Beta

- Fixed a key stuck issue when the key is pressed immediately after the keyboard is connected.
- Add `Mouse Key XY speed` configuration into `Karabiner-Elements Preferences > Virtual Keyboard` tab.
- Event code format on EventViewer changed to decimal number from hex.

## Version 12.3.0

- Added `Change mouse motion to scroll` feature.<br/>
  <https://pqrs.org/osx/karabiner/complex_modifications/#mouse_motion_to_scroll> <br/>
  Note:<br/>
  You have to enable your mice on Devices tab when you want to use this feature.<br/>
  <https://pqrs.org/osx/karabiner/document.html#configuration-devices>
- complex modifications json will be checked strictly since this release.
  Please check error messages if your complex modifications do not work after upgrade.
  <https://pqrs.org/osx/karabiner/json.html#location>
- Set Karabiner-Elements.app and Karabiner-EventViewer.app immutable
  in order to ensure unremovable them except built-in uninstaller.
  Please use the uninstaller when you want to remove Karabiner-Elements.
  <https://pqrs.org/osx/karabiner/document.html#uninstall>
- Added `--lint-complex-modifications` option into `karabiner_cli`.
  It allows you checks a complex-modifications json file.
- Added a wait before grabbing device in order to avoid an macOS issue that device will be unusable after Karabiner-Elements is quit.
- Changes for users who write their own json.
  - `to` and `to_*` support single object, e.g., `"to": { "key_code": "spacebar" }`.
  - New modifier aliases are added: `left_alt`, `left_gui`, `right_alt`, `right_gui`.
  - `key_code`, `consumer_key_code` and `pointing_button` supports a number value, e.g., `"from": {"key_code": 175}`.

## Version 12.2.0

- Fixed an issue that Caps Lock LED does not work on macOS Mojave.
- `shell_command` string max length has been expanded. (256 byte -> 32 KB)
- Karabiner-Elements makes a backup file of karabiner.json before updating it if the backup file does not exists.
  (~/.config/karabiner/automatic_backups/karabiner_YYYYMMDD.json)
- A device grabbing process has been improved. (Observing device state by a separated `karabiner_observer` process.)
- The event processing has been improved and the latency has been reduced by using [pqrs::dispatcher](https://github.com/pqrs-org/dispatcher).

## Version 12.1.0

- Added new items into `simultaneous_options`:
  - `simultaneous_options.detect_key_down_uninterruptedly`
  - `simultaneous_options.key_up_when`
- Added new parameters into `to event definition`:
  - `hold_down_milliseconds`
  - `halt`
- Increased rollover limit of virtual keyboard. (6 -&gt; 32)<br />
  This change mainly improves usability when you are using multiple keyboards at the same time.
- Improved modifier flags handling in `to_after_key_up` and `to_if_alone`.
- Fixed an issue that random key repeat happen at extremely high system CPU usage.
- Incompatible changes
  - Changed the order of `to_if_alone` and `to_after_key_up` event handling.<br />
    `to_if_alone` will be handled before `to_after_key_up`.

## Version 12.0.0

- macOS 10.11 support has been dropped.<br />
  Karabiner-Elements works on macOS 10.12 (Sierra) or later.
- The virtual keyboard compatibility has been improved.
- `Keyboard type` in the virtual keyboard preferences has been removed. (Adverse effect of virtual keyboard improvement.)<br />
  Please change the keyboard type from `System Preferences > Keyboard > Change Keyboard Type...`.
  ![Keyboard Preferences](https://pqrs.org/osx/karabiner/img/news/v12.0.0_0.png)
- `Caps Lock Delay` in the virtual keyboard preferences has been removed. (Adverse effect of virtual keyboard improvement.)
- EventViewer has been improved showing the correct key name for PC keyboard keys and international keys.
- Added `simultaneous_options.key_down_order`, `simultaneous_options.key_up_order` and `simultaneous_options.to_after_key_up`.
- Fixed an issue that `to_if_alone`, `to_if_held_down` and `to_delayed_action` does not work properly with `simultaneous`.
- Improved keyboard repeat handling with `simultaneous`.
- Incompatible changes
  - Changed `simultaneous` behaviour to post key_up events when any key is released.
  - Changed `to_after_key_up` and `to_if_alone` behaviour as mandatory modifiers are removed from these events.

## Version 11.6.0

- Simultaneous key presses has been supported in complex modifications.
  - Example: <https://pqrs.org/osx/karabiner/json.html#typical-complex_modifications-examples-simultaneous-equal-delete-to-forward-delete>
  - Detail: <https://pqrs.org/osx/karabiner/json.html#simultaneous>
- Improved Mouse key scroll wheel direction referring `System Preferences > Mouse > Scroll direction`.
- Improved modifier flags handling around pointing button manipulations.
- Mouse keys have been added into Simple Modifications.
  ![mouse keys in Simple Modifications](https://pqrs.org/osx/karabiner/img/news/v11.6.0_0.png)
- The eject key has been added into the from key of Simple Modifications.
- The Vendor ID and Product ID of virtual devices has been changed. (0x0,0x0 -> 0x16c0,0x27db and 0x16c0,0x27da)

## Version 11.5.0

- `to_if_held_down` has been added.
  - Examples:
    - Open Alfred 3 if escape is held down.
      - <https://pqrs.org/osx/karabiner/json.html#typical-complex_modifications-examples-open-alfred-when-escape-is-held-down>
    - Quit application by holding command-q.
      - <https://pqrs.org/osx/karabiner/complex_modifications/#command_q>
- Avoided a VMware Remote Console issue that mouse pointer does not work properly on VMRC when Karabiner-Elements grabs the pointing device.
- Improved a way to save karabiner.json.
- Improved modifier flags handling in `to events`.
- Fixed an issue that `to_if_alone` does not work properly when `to` is empty.

## Version 11.4.0

- Fixed an issue that the checkbox in `Preferences > Devices` is disabled for keyboards which do not have their own vendor id.
- `mouse_key` has been added.
  - Examples:
    - Mouse keys (simple)
      - src: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/src/json/mouse_keys_simple.json.erb>
      - json: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/mouse_keys_simple.json>
    - Mouse keys (full)
      - src: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/src/json/mouse_keys_full.json.erb>
      - json: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/mouse_keys_full.json>
- `location_id` has been added to `device_if` and `device_unless`.

## Version 11.3.0

- Fixed an issue that Karabiner-11.2.0 does not work properly on some environments due to a possibility of macOS kernel extension cache problem.

## Version 11.2.0

- The caps lock LED manipulation has been disabled with non Apple keyboards until it is enabled manually.
  ![has caps lock led](https://pqrs.org/osx/karabiner/img/news/v11.1.16_0.png)
- Mouse button modifications has been added.<br />
  Note:
  - You have to enable your Mouse manually in Preferences &gt; Devices tab.
  - Karabiner-Elements cannot modify Apple's pointing devices.
- `to_delayed_action` has been added.
  - Examples
    - Quit application by pressing command-q twice
      - src: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/src/json/command_q.json.erb>
      - json: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/command_q.json>
    - Emacs key bindings [C-x key strokes]
      - src: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/src/json/emacs_key_bindings.json.erb>
      - json: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/emacs_key_bindings.json>
- `input_source_if` and `input_source_unless` has been added to `conditions`.
  - Examples
    - <https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/example_input_source.json>
    - <https://github.com/tekezo/Karabiner-Elements/blob/master/tests/src/manipulator_conditions/json/input_source.json>
- `select_input_source` has been added.
  - Example
    - <https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/example_select_input_source.json>
- `keyboard_type_if` and `keyboard_type_unless` has been added to `conditions`.
  - Example
    - Change control-[ to escape
      - src: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/src/json/example_keyboard_type.json.erb>
      - json: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/example_keyboard_type.json>
- The virtual keyboard handling has been improved.

## Version 11.1.0

- Fixed an issue that modifier flags becomes improperly state by mouse events.

## Version 11.0.0

- The first stable release of Karabiner-Elements.
  (There is no changes from Karabiner-Elements 0.91.16.)

## Version 0.91.16

- Karabiner-Elements waits grabbing device until all modifier keys are released in order to avoid modifier flags stuck issue in mouse events.
- Support consumer keys (e.g., media key events in Logitech keyboards.)

## Version 0.91.13

- Add per device support in `Simple Modifications` and `Fn Function Keys`.
  ![Simple Modifications](https://pqrs.org/osx/karabiner/img/news/v0.91.13_0.png)
- The modifier flag event handling has been improved.

## Version 0.91.12

- `device_if` and `device_unless` has been added to `conditions`.
  - An example: <https://github.com/pqrs-org/KE-complex_modifications/blob/master/docs/json/example_device.json>

## Version 0.91.11

- Fixed an issue that modifier flags might become improperly state in complex_modifications.
  (In complex_modifications rules which changes modifier+modifier to modifier.)

## Version 0.91.10

- macOS 10.13 (High Sierra) support has been improved.

## Version 0.91.9

- `variable_if` and `variable_unless` has been added to `conditions`.
  You can use `set_variable` to change the variables.
  - An example: <https://github.com/pqrs-org/KE-complex_modifications/blob/ef8074892e5fff8a4781a898869f8d341b5a815a/docs/json/personal_tekezo.json>
- `to_after_key_up` has been added to `complex_modifications > basic`.
- `"from": { "any": "key_code" }` has been added to `complex_modifications > basic`.
  You can use this to disable untargeted keys in your mode. (e.g., disable untargeted keys in Launcher Mode.)
  - An example: <https://github.com/pqrs-org/KE-complex_modifications/blob/ef8074892e5fff8a4781a898869f8d341b5a815a/docs/json/personal_tekezo.json#L818-L844>
- `Variables` tab has been added into `EventViewer`.
  You can confirm the `set_variable` result in `Variables` tab.

## Version 0.91.8

- Fixed an issue that karabiner_grabber might be crashed when frontmost application is changed.

## Version 0.91.7

- Shell command execution has been supported. (e.g., Launch apps in <https://pqrs.org/osx/karabiner/complex_modifications/> )

## Version 0.91.6

- The conditional event manipulation has been supported. (`frontmost_application_if` and `frontmost_application_unless`)

## Version 0.91.5

- GUI for complex_modifications has been added. <https://github.com/tekezo/Karabiner-Elements/tree/master/usage#how-to-use-complex-modifications>
- Syntax of `complex_modifications > parameters` has been changed.

## Version 0.91.4

- The modifier flag event handling has been improved.
- Show warning and error logs by colored text in Log tab.

## Version 0.91.3

- Add timeout to `to_if_alone`.

## Version 0.91.2

- Initial support of `complex_modifications > basic > to_if_alone`.

## Version 0.91.1

- Fixed an issue that Karabiner-Elements stops working after user switching.
- Initial support of `complex_modifications` (No GUI yet).

## Version 0.91.0

- event manipulation has been changed to `src/core/grabber/include/manipulator/details/basic.hpp`.
