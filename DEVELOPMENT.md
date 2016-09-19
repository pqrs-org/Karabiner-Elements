# Core Processes

* `karabiner_grabber`
  * Run with root privilege.
  * Seize the input devices and modify events then post events to `karabiner_event_dispatcher`.
* `karabiner_event_dispatcher`
  * Launch with root privilege. And then change uid to console user.
  * Receive input events from `karabiner_grabber` and post them to IOHIDSystem.
* `karabiner_console_user_server`
  * Run with console user privilege.
  * Monitor system preferences values (key repeat, etc) and notify them to `karabiner_grabber`.
  * Monitor a karabiner configuration file and notify changes to `karabiner_grabber`.
  * `karabiner_grabber` seizes devices only when `karabiner_console_user_server` is running.

## About security

`karabiner_grabber` and `karabiner_event_dispatcher` treat very sensitive data (input events from device).
And they seizes devices even in Secure Keyboard Entry.

They are communicating by using unix domain sockets.
To avoid the data leaks, the unix domain socket of `karabiner_event_dispatcher` is owned by root user and forbid access from normal privilege users.

## program sequence

### start up

`karabiner_grabber`

1. Run `karabiner_grabber`.
2. `karabiner_grabber` launches `karabiner_event_dispatcher`.
3. `karabiner_grabber` opens grabber server unix domain socket.
4. `karabiner_grabber` start polling the session state.
5. When session state is changed, `karabiner_grabber` changes the unix domain socket owner to console user.

`karabiner_event_dispatcher`

1. `karabiner_event_dispatcher` provides a unix domain socket for `karabiner_grabber`.
2. `karabiner_event_dispatcher` makes a connection to `karabiner_grabber`.
3. `karabiner_event_dispatcher` changes their uid to the console user.

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

karabiner_grabber uses this method.

## CGEventTapCreate

`CGEventTapCreate` is a limited approach.<br />
It does not work with Secure Keyboard Entry even if we use `kCGHIDEventTap` and root privillege.<br />
Thus, it does not work in Terminal.<br />
You can confirm this behavior in `appendix/eventtap`.

--------------------------------------------------------------------------------

# The difference of event posting methods

## IOHIDPostEvent

It requires posting mac events.<br />

`IOHIDPostEvent` will be failed if the process is not running in the current session user.
(The root user is also forbidden.)

karabiner_console_user_server uses this method.

However, there is a limitation that the mouse events will ignore modifier flags by IOHIDPostEvent.
For example, even if we pressed the command key (and the NX_COMMANDMASK is sent by IOHIDPostEvent),
the mouse click event will be treated as click without any modifier flags. (not command+click)

We have to send modifier event via virtual device driver.


## IOKit device report in kext

It requires posting HID events.<br />
The IOHIKeyboard processes the reports by passing reports to `handleReport`.

However, this method is not enough polite.
Calling the `handleReport` method directly causes a problem that OS X ignores `EnableSecureEventInput`.
So we have to reject this approach for security reason.

Due to the limitation of IOHIDPostEvent, we use the virtual device driver for only modifier flags.


## IOKit device value in kext

It requires posting HID events.<br />
We have to make a complete set of virtual devices to post the IOHIDValue.

## IOKit call IOHIKeyboard::dispatchKeyboardEvent in kext

It requires posting mac events.<br />
We have to make a complete set of virtual devices to post the IOHIDValue.

## CGEventCreate

It requires posting mac events.<br />

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
uint8_t unknown;
uint8_t modifiers;
uint8_t reserved;
uint8_t keys[6];
uint8_t extra_modifiers; // fn
```
