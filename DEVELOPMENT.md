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
0x0007003a,
0x00ff0005,
0x0007003b,
0x00ff0004,
0x0007003c,
0xff010010,
0x0007003d,
0xff010004,
0x0007003e,
0x00ff0009,
0x0007003f,
0x00ff0008,
0x00070040,
0x000C00B4,
0x00070041,
0x000C00CD,
0x00070042,
0x000C00B3,
0x00070043,
0x000C00E2,
0x00070044,
0x000C00EA,
0x00070045,
0x000C00E9
```
