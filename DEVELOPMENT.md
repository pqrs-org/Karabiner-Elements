# IOKit drivers in Karabiner

* VirtualHIDManager
  * IOService which provides UserClient.
* VirtualHIDKeyboard
  * Virtual keyboard which sends generic keyboard events.
* VirtualHIDConsumer
  * Virtual keyboard which sends media control events.


# About security

Virtual HID devices must not allow to post input events from remote session.
It causes the current session will be controled by another user.


# The difference of event grabbing methods

## CGEventTapCreate

`CGEventTapCreate` is a limited approach.<br />
It does not work with Secure Keyboard Entry even if we use `kCGHIDEventTap` and root privillege.<br />
Thus, it does not work in Terminal.<br />
You can confirm this behavior in `appendix/eventtap`.

## IOKit

IOKit allows you to read raw HID input events from kernel.<br />
The highest layer is IOHIDQueue which provides us the HID values.

# The difference of event posting methods

## IOKit device report

It requires posting HID events.<br />
The IOHIKeyboard processes the reports by passing reports to `handleReport`.

However, this method is not enough polite.
Calling the `handleReport` method directly causes a problem that OS X ignores `EnableSecureEventInput`.
So we have to reject this approach.

(Is there another way to push input report to virtual device...?)

## IOKit device value

It requires posting HID events.<br />

## IOKit call IOHIKeyboard::dispatchKeyboardEvent

It requires posting mac events.<br />

## IOHIDPostEvent

It requires posting mac events.<br />

`IOHIDPostEvent` will be failed if the process is not running in the current session user.
(The root user is also forbidden.)

## CGEventCreate

It requires posting mac events.<br />


# The limitation of `device reports`

## Apple devices reports

Apple keyboards does not use generic HID keyboard report descriptor.

### Generic HID keyboard report descriptor

```
uint8_t modifiers;
uint8_t reserved;
uint8_t keys[6];
```

#### modifiers bit:

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

### Apple HID keyboard report descriptor

```
uint8_t unknown;
uint8_t modifiers;
uint8_t reserved;
uint8_t keys[6];
uint8_t extra_modifiers; // fn
```

## The f1-f12 keys handling

We have to convert the f1 key report to the brightness control report manually.

### The actual result of f1

```
  report[0]:0x1
  report[1]:0x0
  report[2]:0x0
  report[3]:0x3a
  report[4]:0x0
  report[5]:0x0
  report[6]:0x0
  report[7]:0x0
  report[8]:0x0
  report[9]:0x0
```

### The actual result of fn-f1

```
  report[0]:0x1
  report[1]:0x0
  report[2]:0x0
  report[3]:0x3a
  report[4]:0x0
  report[5]:0x0
  report[6]:0x0
  report[7]:0x0
  report[8]:0x0
  report[9]:0x2
```

# About FnFunctionUsageMap

```
0x0007003a, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF1)
0x00ff0005, // ??, ??

0x0007003b, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF2)
0x00ff0004,

0x0007003c, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF3)
0xff010010,

0x0007003d, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF4)
0xff010004,

0x0007003e, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF5)
0x00ff0009,

0x0007003f, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF6)
0x00ff0008,

0x00070040, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF7)
0x000C00B4,

0x00070041, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF8)
0x000C00CD,

0x00070042, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF9)
0x000C00B3,

0x00070043, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF10)
0x000C00E2,

0x00070044, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF11)
0x000C00EA,

0x00070045, // (kHIDPage_KeyboardOrKeypad << 32 | kHIDUsage_KeyboardF12)
0x000C00E9
```

# program sequence

1. Run grabber.
2. Open grabber server unix domain socket.
3. Polling session state in grabber.
4. When session state is changed, grabber change the unix domain socket owner to console user.
5. Run console_user_server.
6. Try to open console_user_server unix domain socket.
7. Send `KRBN_OPERATION_TYPE_CONNECT` to grabber from console_user_server.
8. grabber connects to console_user_server.
9. grabber seizes input devices.
