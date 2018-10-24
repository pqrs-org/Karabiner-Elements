[![Build Status](https://travis-ci.org/tekezo/Karabiner-VirtualHIDDevice.svg?branch=master)](https://travis-ci.org/tekezo/Karabiner-VirtualHIDDevice)
[![License](https://img.shields.io/badge/license-Public%20Domain-blue.svg)](https://github.com/tekezo/Karabiner-VirtualHIDDevice/blob/master/LICENSE.md)

# Karabiner-VirtualHIDDevice

Karabiner-VirtualHIDDevice is a virtual HID device driver for macOS.

## System requirements

- macOS Sierra (10.12)
- macOS High Sierra (10.13)
- macOS Mojave (10.14)

## How to build

System requirements:

- macOS 10.13+
- Xcode 8.3.3 (required for macOS 10.12 support)
- Command Line Tools for Xcode

### Step 1: Getting source code

Clone the source from github.

```shell
git clone --depth 1 https://github.com/tekezo/Karabiner-VirtualHIDDevice.git
```

### Step 2: Building a package

```shell
cd Karabiner-VirtualHIDDevice
make
```

The `make` script will create a redistributable kext into `dist` directory.

## Example

- `example/virtual_keyboard_example`
- `example/virtual_pointing_example`

### Usage

Execute the following instructions in Terminal.

1. Install VirtualHIDDevice.kext by `make install` in the top directory.
2. `cd example/virtual_pointing_example`
3. `make`
4. `make run`

## Acknowledgments

- Karabiner-VirtualHIDDevice uses the Vendor ID and Product ID from Objective Development.
  <https://github.com/obdev/v-usb/blob/master/usbdrv/USB-IDs-for-free.txt>
