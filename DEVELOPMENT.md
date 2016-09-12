# Core Processes

* `karabiner_grabber`
  * Run with root privilege.
  * Seize the input devices and post events to `karabiner_console_user_server`.
* `karabiner_console_user_server`
  * Run with console user privilege.
  * Receive input events from `karabiner_grabber` and then modify events and post them to IOHIDSystem.

## About security

`karabiner_grabber` and `karabiner_console_user_server` are connected by unix domain socket.

If a cracked `karabiner_console_user_server` is connected, the input events will be leaked.
To avoid this problem, `karabiner_grabber` checks the codesign certificate of `karabiner_console_user_server`.
If the both process are not signed with the same certificate, `karabiner_grabber` does not send events to `karabiner_console_user_server`.


## program sequence

1. Run `karabiner_grabber`.
2. Open grabber server unix domain socket.
3. Polling session state in grabber.
4. When session state is changed, grabber change the unix domain socket owner to console user.
5. Run `karabiner_console_user_server`.
6. Try to open console_user_server unix domain socket.
7. Send `KRBN_OPERATION_TYPE_CONNECT` to grabber from console_user_server.
8. grabber connects to console_user_server.
9. grabber seizes input devices.

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


## IOKit device report in kext

It requires posting HID events.<br />
The IOHIKeyboard processes the reports by passing reports to `handleReport`.

However, this method is not enough polite.
Calling the `handleReport` method directly causes a problem that OS X ignores `EnableSecureEventInput`.
So we have to reject this approach for security reason.

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

But `karabiner_grabber` does not use it to reduce device dependancy.

## The limitation of `device reports`

### Apple devices reports

Apple keyboards does not use generic HID keyboard report descriptor.

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
