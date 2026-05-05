[![Build Status](https://github.com/pqrs-org/Karabiner-Elements/workflows/Karabiner-Elements%20CI/badge.svg)](https://github.com/pqrs-org/Karabiner-Elements/actions)
[![License](https://img.shields.io/badge/license-Public%20Domain-blue.svg)](https://github.com/pqrs-org/Karabiner-Elements/blob/main/LICENSE.md)

# Karabiner-Elements

Karabiner-Elements is a powerful key remapper for macOS.

## Download

You can download Karabiner-Elements from the [official site](https://karabiner-elements.pqrs.org/).

Alternatively, if you use [Homebrew](https://brew.sh/), you can install Karabiner-Elements with `brew install --cask karabiner-elements`.

### Old releases

You can download previous versions of Karabiner-Elements from the [release notes](https://karabiner-elements.pqrs.org/docs/releasenotes/).

## Supported systems

- macOS 26 Tahoe
    - Both Intel-based Macs and Apple Silicon Macs
- macOS 15 Sequoia
    - Both Intel-based Macs and Apple Silicon Macs
- macOS 14 Sonoma
    - Both Intel-based Macs and Apple Silicon Macs
- macOS 13 Ventura
    - Both Intel-based Macs and Apple Silicon Macs

## Usage

Documentation can be found here: <https://karabiner-elements.pqrs.org/docs/>

## Donations

If you would like to support Karabiner-Elements development financially, donations can be made at <https://karabiner-elements.pqrs.org/docs/pricing/>.

---

## For developers

### System requirements to build Karabiner-Elements

- macOS 15+
- Xcode 26+
- Command Line Tools for Xcode (`xcode-select --install`)
- xz (`brew install xz`)
- [XcodeGen](https://github.com/yonaskolb/XcodeGen) (`brew install xcodegen`)
- CMake (`brew install cmake`)

### How to build the package

#### Step 1: Get the source code

Get the source code by running the following commands in Terminal.app.

```shell
git clone --depth 1 https://github.com/pqrs-org/Karabiner-Elements.git
cd Karabiner-Elements
git submodule update --init --recursive --depth 1
```

#### Step 2: Prepare code signing identities

Code signing is required for the Karabiner-Elements background services to run.
Prepare the appropriate code signing identities for your Apple Developer Program status.

- If you are not enrolled in the Apple Developer Program:
    - Use a development code signing identity.
    - In Xcode Settings, add your account under Apple Accounts, then create an Apple Development certificate from Manage Certificates.
- If you are enrolled in the Apple Developer Program:
    - Create Developer ID Application and Developer ID Installer certificates if you do not already have them.

After preparing the code signing identities, run the following command in Terminal.app to get the identity hashes.

```shell
security find-identity -v
```

The output will look like this.
The strings such as `C3107C61DB3605DA2D4549054B225DAFB1D6FA2D` and `BD3B995B69EBA8FC153B167F063079D19CCC2834` are identity hashes.

```text
  1) C3107C61DB3605DA2D4549054B225DAFB1D6FA2D "Developer ID Installer: Fumihiko Takayama (G43BCU2T37)"
  2) BD3B995B69EBA8FC153B167F063079D19CCC2834 "Developer ID Application: Fumihiko Takayama (G43BCU2T37)"
  3) C0D6EBFEAD3C0EB6DB91C3514FF647917A0B5112 "Apple Development: Fumihiko Takayama (YVB3SM6ECS)"
```

Set these values in environment variables as follows.
To avoid forgetting these settings, you can add them to your shell configuration file, such as `.zshrc`.

```shell
# Specify the identity hash for Developer ID Application or Apple Development.
export PQRS_ORG_CODE_SIGN_IDENTITY=BD3B995B69EBA8FC153B167F063079D19CCC2834
# Specify the identity hash for Developer ID Installer or Apple Development.
export PQRS_ORG_INSTALLER_CODE_SIGN_IDENTITY=C3107C61DB3605DA2D4549054B225DAFB1D6FA2D
```

#### Step 3: Build a package

Build a package by running the following command in Terminal.app.

```shell
make package
```

The `make` script will create a **Karabiner-Elements-VERSION.dmg** in the current directory.
The package is included in the dmg file.

#### Step 4: Install the package you built

Open the dmg file and install Karabiner-Elements from the pkg file inside it.

The permissions you grant to Karabiner-Elements, such as background service startup and Accessibility access, are based on the code signing identity.
Therefore, if you switch from the officially distributed package to your own build, these permissions become invalid and must be granted again.

macOS System Settings may not update its UI when permissions are invalidated by a signer change.
In that case, reset the permissions by following these steps.

1.  Install your package.
2.  In System Settings, disable or remove the following permissions.
    - Disable both of the following background services:
        - Karabiner-Elements Non-Privileged Agents v2
        - Karabiner-Elements Privileged Daemons v2
    - Remove the Accessibility permission.
3.  Restart macOS.
4.  Open Karabiner-Elements and grant the permissions again.

After these steps, the Karabiner-Elements package you built should work.

### Note about pre-built binaries and Swift packages

Karabiner-Elements uses some pre-built binaries and Swift packages during the build.

- Pre-built binaries in the source tree:
    - `vendor/Karabiner-DriverKit-VirtualHIDDevice/dist/Karabiner-DriverKit-VirtualHIDDevice-*.pkg` (the latest one)
- Swift packages resolved by Xcode/SwiftPM during the build include:
    - `Sparkle` for `Karabiner-Updater`
    - `AsyncAlgorithms`, `CodeEditor`, and other packages used by individual apps

The `make package` command does not rebuild the pre-built binaries listed above.<br/>
These binaries are copied into the distributed package.

Swift packages are resolved by Xcode/SwiftPM from their package repositories during the build.
If you want to rebuild or modify them, please follow the instructions for each upstream project.
