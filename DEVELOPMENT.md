# The difference of event grabbing methods

## CGEventTapCreate

`CGEventTapCreate` is a limited approach.<br />
It does not work with Secure Keyboard Entry even if we use `kCGHIDEventTap` and root privillege.<br />
Thus, it does not work in Terminal.<br />
You can confirm this behavior in `appendix/eventtap`.

## IOKit

IOKit allows you to read raw HID input events from kernel.

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
