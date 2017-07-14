[![Build Status](https://travis-ci.org/tekezo/Karabiner-Elements.svg?branch=master)](https://travis-ci.org/tekezo/Karabiner-Elements)
[![License](https://img.shields.io/badge/license-Public%20Domain-blue.svg)](https://github.com/tekezo/Karabiner-Elements/blob/master/LICENSE.md)

# Karabiner-Elements

Karabiner-Elements provides a subset of the features planned for the next generation [Karabiner](https://pqrs.org/osx/karabiner) for macOS Sierra. The current version of Karabiner does not work with Sierra, so Karabiner-Elements was created to keep Sierra users up and running until a new version of Karabiner is published.

## Project Status

Karabiner-Elements is ready to use today. It provides a useful subset of Karabiner's features that you can begin using immediately.

You can download the latest Karabiner-Elements from https://pqrs.org/latest/karabiner-elements-latest.dmg

## System requirements

* OS X 10.11.*
* OS X 10.12.*

# Usage

Detailed usage instructions are provided in [the Usage README](usage/README.md).

## Features

* Secure Keyboard Entry: Support [secure entry](https://security.stackexchange.com/questions/47749/how-secure-is-secure-keyboard-entry-in-mac-os-xs-terminal) in the Terminal, Password prompt etc.
* Modifier Flag Sync: Synchronize modifier flags across all connected keyboards.
* Simple Modifications: Map normal keys to arbitrary key functions.
* Function Keys: Map function keys to arbitrary key functions.
* Complex Modifications: Map keys by complex rules. eg key to modifiers, modifiers+key to key, send key event if pressed alone, Hyper key etc.
* Devices: Apply modifications to specified keyboards only.
* Virtual Keyboard: Set the virtual keyboard's type (ANSI, ISO, JIS) and its Caps Lock Delay.
* Profiles: Support the creation of multiple profiles that the user can switch between.
* Inverted Function Keys: Invert the functionality of the function keys with regard to the <kbd>fn</kbd> modifier.
* Log Keyboard Events: Render the keyboard events log.
* Log Application Events: Render the Karabiner-Elements event log.
* Misc: Enable automatic updates, delete Karabiner Elements etc.

## Limitations

* Karabiner-Elements cannot modify eject key due to the limitation of macOS API.
* Karabiner-Elements ignores the `System Preferences > Keyboard > Modifier Keys...` configuration.

## How to build

System requirements:

* OS X 10.11+
* Xcode 8+
* Command Line Tools for Xcode
* Boost 1.61.0+ (header-only) http://www.boost.org/

To install the Boost requirement, [download the latest Boost release](http://www.boost.org/), open the `boost` folder inside of it, and move all of the files there into `/opt/local/include/boost/`.

(For example, the version.hpp should be located in `/opt/local/include/boost/version.hpp`)


### Step 1: Getting source code

Clone the source from github.

```
git clone --depth 1 https://github.com/tekezo/Karabiner-Elements.git
```

### Step 2: Building a package

```
cd Karabiner-Elements
make
```

The `make` script will create a redistributable **Karabiner-Elements-VERSION.dmg** in the current directory.

# Donations

If you would like to contribute financially to the development of Karabiner Elements, donations can be made via https://pqrs.org/osx/karabiner/donation.html.en.
