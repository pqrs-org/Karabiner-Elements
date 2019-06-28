[![Build Status](https://travis-ci.com/tekezo/Karabiner-Elements.svg?branch=master)](https://travis-ci.com/tekezo/Karabiner-Elements)
[![License](https://img.shields.io/badge/license-Public%20Domain-blue.svg)](https://github.com/tekezo/Karabiner-Elements/blob/master/LICENSE.md)

# Karabiner-Elements

Karabiner-Elements is a powerful utility for keyboard customization on macOS Sierra or later.

[Karabiner](https://pqrs.org/osx/karabiner) stopped working because of the keyboard driver architecture changes at macOS Sierra.
Thus, Karabiner-Elements was made from scratch for new macOS.

## Project Status

Karabiner-Elements is ready to use today. It provides a useful subset of Karabiner's features that you can begin using immediately.

You can download the latest Karabiner-Elements from <https://pqrs.org/osx/karabiner/>

### Old releases

You can download previous versions of Karabiner-Elements from here:
<https://github.com/tekezo/pqrs.org/tree/master/webroot/osx/karabiner/files>

## System requirements

- macOS Sierra (10.12)
- macOS High Sierra (10.13)
- macOS Mojave (10.14)

# Usage

<https://pqrs.org/osx/karabiner/document.html>

## How to build

System requirements to build Karabiner-Elements:

- macOS 10.14+
- Xcode 10+
- Command Line Tools for Xcode
- CMake (`brew install cmake`)

### Step 1: Getting source code

Clone the source from github.

```shell
git clone --depth 1 https://github.com/tekezo/Karabiner-Elements.git
```

### Step 2: Building a package

```shell
cd Karabiner-Elements
make package
```

The `make` script will create a redistributable **Karabiner-Elements-VERSION.dmg** in the current directory.

### Note: About pre-built binaries in the source tree

Karabiner-Elements uses some pre-built binaries in the source tree.

- `src/vendor/Karabiner-VirtualHIDDevice/dist/*.kext`
- `src/vendor/Sparkle/Sparkle.framework`

Above `make package` command does not rebuild these binaries.<br/>
(These binaries will be copied in the distributed package.)

If you want to rebuild these binaries, you have to build them manually.<br/>
Please follow the instruction of these projects.

#### About rebuilding kext in Karabiner-VirtualHIDDevice

If you want to build kext in Karabiner-VirtualHIDDevice, macOS requires a valid certificate which be able to sign the built kext.<br/>
Unless such certificate, macOS refuses to load the built kext.<br/>
Please read a documentation about [System Integrity Protection Guide](https://developer.apple.com/library/archive/documentation/Security/Conceptual/System_Integrity_Protection_Guide/KernelExtensions/KernelExtensions.html) for more details.

(We are including the pre-built kext binary to avoid the restriction that macOS requires a uncommon certificate.)

# Donations

If you would like to contribute financially to the development of Karabiner Elements, donations can be made via <https://pqrs.org/osx/karabiner/pricing.html>
