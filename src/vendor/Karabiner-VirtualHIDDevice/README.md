[![Build Status](https://travis-ci.org/tekezo/Karabiner-VirtualHIDDevice.svg?branch=master)](https://travis-ci.org/tekezo/Karabiner-VirtualHIDDevice)
[![License](https://img.shields.io/badge/license-Public%20Domain-blue.svg)](https://github.com/tekezo/Karabiner-VirtualHIDDevice/blob/master/LICENSE.md)

# Karabiner-VirtualHIDDevice

Karabiner-VirtualHIDDevice is a virtual HID device driver for macOS.

## How to build

System requirements:

* OS X 10.11+
* Xcode 8+
* Command Line Tools for Xcode

### Step 1: Getting source code

Clone the source from github.

```
git clone --depth 1 https://github.com/tekezo/Karabiner-VirtualHIDDevice.git
```

### Step 2: Building a package

```
cd Karabiner-VirtualHIDDevice
make
```

The `make` script will create a redistributable kext into `dist` directory.

## Example

An example is in `example/virtual_pointing_example`.

### Usage

Execute the following instructions in Terminal.

1. Install VirtualHIDManager.kext by `make install` in the top directory.
2. `cd example/virtual_pointing_example`
3. `make`
4. `make run`
