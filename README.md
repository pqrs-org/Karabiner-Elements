[![Build Status](https://github.com/pqrs-org/Karabiner-Elements/workflows/Karabiner-Elements%20CI/badge.svg)](https://github.com/pqrs-org/Karabiner-Elements/actions)
[![License](https://img.shields.io/badge/license-Public%20Domain-blue.svg)](https://github.com/pqrs-org/Karabiner-Elements/blob/main/LICENSE.md)

# Karabiner-Elements

Karabiner-Elements is a powerful utility for keyboard customization on macOS Sierra or later.

## Download

You can download Karabiner-Elements from [official site](https://karabiner-elements.pqrs.org/).

### Old releases

You can download previous versions of Karabiner-Elements from [here](https://karabiner-elements.pqrs.org/docs/releasenotes/).

## Supported systems

-   macOS 11 Big Sur
-   macOS 12 Monterey

## Usage

<https://karabiner-elements.pqrs.org/docs/>

## Donations

If you would like to contribute financially to the development of Karabiner Elements, donations can be made via <https://karabiner-elements.pqrs.org/docs/pricing/>

---

## For developers

### How to build

System requirements to build Karabiner-Elements:

-   macOS 11+
-   Xcode 13+
    -   Xcode for macOS Monterey is required.
    -   Xcode 13 RC and Xcode 13.0 are not satisfy this requirements. As of Sep 2021, use Xcode 13 beta 5 at Sep 2021.
-   Command Line Tools for Xcode (`xcode-select --install`)
-   xz (`brew install xz`)
-   [XcodeGen](https://github.com/yonaskolb/XcodeGen) (`brew install xcodegen`)
-   CMake (`brew install cmake`)

#### Steps

1.  Get source code by executing a following command in Terminal.app.

    ```shell
    git clone --depth 1 https://github.com/pqrs-org/Karabiner-Elements.git
    cd Karabiner-Elements
    git submodule update --init --recursive --depth 1
    ```

2.  (Optional) If you have a codesign identity:

    1.  Find your codesign identity.

        ```shell
        security find-identity -p codesigning -v | grep 'Developer ID Application'
        ```

        The result is as follows.

        ```text
        1) 8D660191481C98F5C56630847A6C39D95C166F22 "Developer ID Application: Fumihiko Takayama (G43BCU2T37)"
        ```

        Your codesign identity is `8D660191481C98F5C56630847A6C39D95C166F22` in the above case.

    2.  Set environment variable to use your codesign identity.

        ```shell
        export PQRS_ORG_CODE_SIGN_IDENTITY=8D660191481C98F5C56630847A6C39D95C166F22
        ```

    3.  Find your codesign identity for installer signing.

        ```shell
        security find-identity -p basic -v | grep 'Developer ID Installer'
        ```

        The result is as follows.

        ```text
        1) C86BB5F7830071C7B0B07D168A9A9375CC2D02C5 "Developer ID Installer: Fumihiko Takayama (G43BCU2T37)"
        ```

        Your codesign identity is `C86BB5F7830071C7B0B07D168A9A9375CC2D02C5` in the above case.

    4.  Set environment variable to use your codesign identity for installer signing.

        ```shell
        export PQRS_ORG_INSTALLER_CODE_SIGN_IDENTITY=C86BB5F7830071C7B0B07D168A9A9375CC2D02C5
        ```

3.  Build a package by executing a following command in Terminal.app.

    ```shell
    make package
    ```

    The `make` script will create a redistributable **Karabiner-Elements-VERSION.dmg** in the current directory.

#### Note: About pre-built binaries in the source tree

Karabiner-Elements uses some pre-built binaries in the source tree.

-   `src/vendor/Karabiner-DriverKit-VirtualHIDDevice/dist/Karabiner-DriverKit-VirtualHIDDevice-*.dmg`
-   `Sparkle.framework` in `src/apps/PreferencesWindow/`

Above `make package` command does not rebuild these binaries.<br/>
(These binaries will be copied in the distributed package.)

If you want to rebuild these binaries, you have to build them manually.<br/>
Please follow the instruction of these projects.
