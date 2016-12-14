# Core Processes

* `karabiner_grabber`
  * Run with root privilege.
  * Seize the input devices and modify events then post events using `Karabiner-VirtualHIDDevice`.
* `karabiner_console_user_server`
  * Run with console user privilege.
  * Monitor system preferences values (key repeat, etc) and notify them to `karabiner_grabber`.
  * Monitor a karabiner configuration file and notify changes to `karabiner_grabber`.
  * `karabiner_grabber` seizes devices only when `karabiner_console_user_server` is running.

## program sequence

### start up

`karabiner_grabber`

1. Run `karabiner_grabber`.
3. `karabiner_grabber` opens grabber server unix domain socket.
4. `karabiner_grabber` start polling the session state.
5. When session state is changed, `karabiner_grabber` changes the unix domain socket owner to console user.

### device grabbing

1. Run `karabiner_console_user_server`.
2. Try to open console_user_server unix domain socket.
3. grabber seizes input devices.

## Other notes

IOHIDSystem requires the process is running with the console user privilege.
Thus, `karabiner_grabber` cannot send events to IOHIDSystem directly.

--------------------------------------------------------------------------------

# The difference of event grabbing methods

## IOKit

IOKit allows you to read raw HID input events from kernel.<br />
The highest layer is IOHIDQueue which provides us the HID values.

`karabiner_grabber` uses this method.

## CGEventTapCreate

`CGEventTapCreate` is a limited approach.<br />
It does not work with Secure Keyboard Entry even if we use `kCGHIDEventTap` and root privillege.<br />
Thus, it does not work in Terminal.<br />
You can confirm this behavior in `appendix/eventtap`.

However, we should modify mouse event's modifier flags manually.
Thus, `karabiner_grabber` uses this method to modify mouse events.

--------------------------------------------------------------------------------

# The difference of event posting methods

## IOKit call IOHIKeyboard::dispatchKeyboardEvent in kext

It requires posting HID usage page and usage.
`karabiner_grabber` uses this method by using `Karabiner-VirtualHIDDevice`.

## IOKit device report in kext

It requires posting HID events.<br />
The IOHIKeyboard processes the reports by passing reports to `handleReport`.

## IOHIDPostEvent

It requires posting coregraphics events.<br />

`IOHIDPostEvent` will be failed if the process is not running in the current session user.
(The root user is also forbidden.)

## CGEventPost

It requires posting coregraphics events.<br />

`CGEventPost` does not support some key events in OS X 10.12.

* Mission Control key
* Launchpad key
* Option-Command-Escape

Thus, `karabiner_grabber` does not use `CGEventPost`.

## IOKit device value in kext

It requires posting HID events.<br />
We have to make a complete set of virtual devices to post the IOHIDValue.

--------------------------------------------------------------------------------

# Modifier flags handling in kernel

The modifier flag events are handled in the following sequence in macOS 10.12.

1. Receive HID reports from device.
2. Treat reports in the keyboard device driver.
3. Treat flags in accessibility functions. (eg. sticky keys, zoom)
4. Treat flags in mouse events.
5. Treat flags in IOHIDSystem.
6. Treat flags in Coregraphics.

Thus, `IOHIDPostEvent` will be ignored in accessibility functions and mouse events.

--------------------------------------------------------------------------------

# About hid reports

We can get hid reports from devices via `IOHIDDeviceRegisterInputReportCallback`.<br />
The hid report contains a list of pressed keys, so it seems suitable information to observe.

But `karabiner_grabber` does not use it in order to reduce the device dependancy.

## The limitation of device reports

### Apple devices reports

Apple keyboards does not use generic HID keyboard report descriptor.<br />
Thus, we have to handle them by separate way.

#### Generic HID keyboard report descriptor

```
uint8_t modifiers;
uint8_t reserved;
uint8_t keys[6];
```

##### modifiers bit:

```
0x1 << 0 : left control
0x1 << 1 : left shift
0x1 << 2 : left option
0x1 << 3 : left command
0x1 << 4 : right control
0x1 << 5 : right shift
0x1 << 6 : right option
0x1 << 7 : right command
```

#### Apple HID keyboard report descriptor

```
uint8_t record_id;
uint8_t modifiers;
uint8_t reserved;
uint8_t keys[6];
uint8_t extra_modifiers; // fn
```
