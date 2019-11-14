[![Build Status](https://travis-ci.org/pqrs-org/Karabiner-VirtualHIDDevice.svg?branch=master)](https://travis-ci.org/pqrs-org/Karabiner-VirtualHIDDevice)
[![License](https://img.shields.io/badge/license-Public%20Domain-blue.svg)](https://github.com/pqrs-org/Karabiner-VirtualHIDDevice/blob/master/LICENSE.md)

# Karabiner-VirtualHIDDevice

Karabiner-VirtualHIDDevice is a virtual HID device driver for macOS.

-   A virtual keyboard device implementation for macOS.
    -   Keyboard/Keypad Usage Page is supported.
    -   Consumer Usage Page is supported.
    -   AppleVendor Usage Page (fn key, Mission Control key, Launchpad key, etc.) is supported.
-   A virtual pointing device (mouse) implementation for macOS.

## Supported systems

-   macOS Sierra (10.12)
-   macOS High Sierra (10.13)
-   macOS Mojave (10.14)
-   macOS Catalina (10.15)

## How to build

System requirements:

-   macOS 10.13
-   Xcode 8.3.3 (required for macOS 10.12 support)
-   Command Line Tools for Xcode

### Step 1: Getting source code

Clone the source from github.

```shell
git clone --depth 1 https://github.com/pqrs-org/Karabiner-VirtualHIDDevice.git
```

### Step 2: Building a package

```shell
cd Karabiner-VirtualHIDDevice
make
```

The `make` script will create a redistributable kext into `dist` directory.

## Example

-   `example/virtual_keyboard_example`
-   `example/virtual_pointing_example`

### Usage

Execute the following instructions in Terminal.

1.  Install VirtualHIDDevice.kext by `make install` in the top directory.
2.  `cd example/virtual_pointing_example`
3.  `make`
4.  `make run`

## Acknowledgments

-   Karabiner-VirtualHIDDevice uses the Vendor ID and Product ID from Objective Development.
    <https://github.com/obdev/v-usb/blob/master/usbdrv/USB-IDs-for-free.txt>
