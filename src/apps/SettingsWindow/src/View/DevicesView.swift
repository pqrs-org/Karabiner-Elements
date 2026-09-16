import SwiftUI

struct DevicesView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var connectedDevices = ConnectedDevices.shared
  @State private var showEraseNotConnectedDeviceSettingsButton = false

  static let detailedSettingWidth = 400.0

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      List {
        ForEach(connectedDevices.connectedDevices) { connectedDevice in
          if let deviceConfiguration = settings.deviceConfigurationBinding(connectedDevice) {
            VStack(alignment: .leading, spacing: 0.0) {
              DeviceName(connectedDevice: connectedDevice)
                .if(connectedDevice.isVirtualDevice) {
                  $0.foregroundColor(Color(NSColor.placeholderTextColor))
                }

              if !connectedDevice.isVirtualDevice {
                VStack(alignment: .leading, spacing: 0.0) {
                  ModifyEventsSetting(
                    connectedDevice: connectedDevice,
                    deviceConfiguration: deviceConfiguration)

                  VStack(alignment: .leading, spacing: 6.0) {
                    KeyboardSettings(
                      connectedDevice: connectedDevice,
                      deviceConfiguration: deviceConfiguration)

                    MouseSettings(
                      connectedDevice: connectedDevice,
                      deviceConfiguration: deviceConfiguration)

                    GamePadSettings(
                      connectedDevice: connectedDevice,
                      deviceConfiguration: deviceConfiguration)

                    ExtraSettings(
                      connectedDevice: connectedDevice,
                      deviceConfiguration: deviceConfiguration)
                  }
                  .padding(.leading, 20.0)
                  .padding(.top, 8.0)
                }
                .padding(.leading, 62.0)
                .padding(.top, 20.0)
              }
            }
            .padding(.vertical, 12.0)
            .padding(.trailing, 12.0)
            .frame(maxWidth: .infinity, alignment: .leading)
            .overlay(
              RoundedRectangle(cornerRadius: 8)
                .stroke(
                  Color(NSColor.selectedControlColor),
                  lineWidth: deviceConfiguration.wrappedValue.modifyEvents
                    && !connectedDevice.isVirtualDevice
                    ? 3 : 0
                )
                .padding(2)
            )
          }
        }
      }
      .background(Color(NSColor.textBackgroundColor))

      if connectedDevices.notConnectedConfiguredDevicesCount > 0 {
        HStack {
          Label(
            localized(
              "settings.devices.disconnected_count",
              arguments: ["count": String(connectedDevices.notConnectedConfiguredDevicesCount)]),
            systemImage: InfoBorder.icon
          )
          .frame(maxWidth: .infinity, alignment: .leading)

          if !showEraseNotConnectedDeviceSettingsButton {
            Button(
              action: {
                showEraseNotConnectedDeviceSettingsButton = true
              },
              label: {
                Image(systemName: "trash")
                  .buttonLabelStyle()
              }
            )
          } else {
            Button(
              role: .destructive,
              action: {
                settings.eraseNotConnectedDeviceSettings()
              },
              label: {
                Label(
                  localized(
                    "settings.devices.remove_disconnected",
                    arguments: [
                      "count": String(connectedDevices.notConnectedConfiguredDevicesCount)
                    ]),
                  systemImage: "trash"
                )
                .buttonLabelStyle()
              }
            )
            .deleteButtonStyle()
          }
        }
        .modifier(InfoBorder())
        .padding()
      }
    }
  }

  struct DeviceName: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice

    var body: some View {
      HStack(alignment: .center, spacing: 0) {
        HStack(spacing: 4.0) {
          if connectedDevice.isKeyboard {
            Image(systemName: "keyboard")
          }
          if connectedDevice.isPointingDevice {
            Image(systemName: "computermouse")
          }
          if connectedDevice.isGamePad {
            Image(systemName: "gamecontroller")
          }
          if connectedDevice.isConsumer {
            Image(systemName: "headphones")
          }
        }
        .frame(width: 50.0, alignment: .trailing)

        Text(
          "\(connectedDevice.localizedProductName(localized)) (\(connectedDevice.localizedManufacturerName(localized)))"
        )
        .padding(.leading, 12.0)
        .frame(maxWidth: .infinity, alignment: .leading)

        if connectedDevice.transport != "FIFO" {
          VStack(alignment: .trailing, spacing: 4.0) {
            if connectedDevice.vendorId != 0 {
              AppLocalizedText(
                "settings.devices.vendor_id",
                arguments: [
                  "decimal": String(format: "%5d", connectedDevice.vendorId),
                  "hex": String(format: "0x%04x", connectedDevice.vendorId),
                ])
            }

            if connectedDevice.productId != 0 {
              AppLocalizedText(
                "settings.devices.product_id",
                arguments: [
                  "decimal": String(format: "%5d", connectedDevice.productId),
                  "hex": String(format: "0x%04x", connectedDevice.productId),
                ])
            }

            if !connectedDevice.deviceAddress.isEmpty {
              AppLocalizedText(
                "settings.devices.address", arguments: ["address": connectedDevice.deviceAddress])
            }
          }
          .font(.callout)
          .monospaced()
        }
      }
    }
  }

  struct ModifyEventsSetting: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    @ObservedObject private var settings = Settings.shared

    var body: some View {
      HStack(alignment: .top) {
        if connectedDevice.isAppleDevice,
          !connectedDevice.isKeyboard,
          connectedDevice.isPointingDevice,
          !settings.configuration.globalConfiguration.unsafeUi
        {
          AppLocalizedText("settings.devices.apple_pointing_unsupported")
            .foregroundColor(Color(NSColor.placeholderTextColor))
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          VStack(alignment: .leading) {
            Toggle(isOn: $deviceConfiguration.modifyEvents) {
              AppLocalizedText("settings.devices.modify_events")
            }
            .switchToggleStyle()
            .frame(width: 140.0)

            if settings.configuration.globalConfiguration.enableCgeventtapFallback
              && !deviceConfiguration.modifyEvents
              && connectedDevice.isKeyboard
            {
              Label(
                title: {
                  AppLocalizedText(
                    "settings.devices.fallback_hint"
                  )
                  .textSelection(.enabled)
                },
                icon: {
                  Image(systemName: InfoBorder.icon)
                }
              )
              .modifier(InfoBorder())
            }
          }
        }
      }
    }
  }

  struct KeyboardSettings: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    @ObservedObject private var settings = Settings.shared

    var body: some View {
      VStack {
        if connectedDevice.isKeyboard {
          VStack(alignment: .leading, spacing: 6.0) {
            if !connectedDevice.isBuiltInKeyboard
              && !deviceConfiguration.disableBuiltInKeyboardIfExists
            {
              Toggle(isOn: $deviceConfiguration.treatAsBuiltInKeyboard) {
                AppLocalizedText("settings.devices.treat_as_built_in")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: detailedSettingWidth)
            }

            if !connectedDevice.isBuiltInKeyboard
              && !deviceConfiguration.treatAsBuiltInKeyboard
            {
              Toggle(isOn: $deviceConfiguration.disableBuiltInKeyboardIfExists) {
                AppLocalizedText("settings.devices.disable_built_in")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: detailedSettingWidth)
            }

            if deviceConfiguration.modifyEvents {
              Toggle(isOn: $deviceConfiguration.manipulateCapsLockLed) {
                AppLocalizedText("settings.devices.caps_lock_led")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: detailedSettingWidth)

              Toggle(isOn: $deviceConfiguration.swapGraveAccentAndNonUsBackslash) {
                AppLocalizedText("settings.devices.swap_iso_keys")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: detailedSettingWidth)
            }
          }
        }
      }
    }
  }

  struct MouseSettings: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device
    @State var showing = false

    var body: some View {
      if deviceConfiguration.modifyEvents
        && connectedDevice.isPointingDevice
      {
        Button(
          action: {
            showing = true
          },
          label: {
            AppLocalizedLabel("settings.devices.open_mouse_settings", systemImage: "computermouse")
              .buttonLabelStyle()
          }
        )
        .sheet(isPresented: $showing) {
          DevicesMouseSettingsView(
            connectedDevice: connectedDevice,
            deviceConfiguration: $deviceConfiguration,
            showing: $showing
          )
          .modifier(SettingsLanguage())
        }
      } else {
        EmptyView()
      }
    }
  }

  struct GamePadSettings: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device
    @State var showing = false

    var body: some View {
      if deviceConfiguration.modifyEvents && connectedDevice.isGamePad {
        Button(
          action: {
            showing = true
          },
          label: {
            AppLocalizedLabel(
              "settings.devices.open_gamepad_settings", systemImage: "gamecontroller"
            )
            .buttonLabelStyle()
          }
        )
        .sheet(isPresented: $showing) {
          DevicesGamePadSettingsView(
            connectedDevice: connectedDevice,
            deviceConfiguration: $deviceConfiguration,
            showing: $showing
          )
          .modifier(SettingsLanguage())
        }
      } else {
        EmptyView()
      }
    }
  }

  struct ExtraSettings: View {
    @AppLocalizationContext private var localized
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    var body: some View {
      VStack {
        if deviceConfiguration.modifyEvents {
          if !connectedDevice.isAppleDevice {
            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(isOn: $deviceConfiguration.ignoreVendorEvents) {
                AppLocalizedText(
                  "settings.devices.ignore_vendor_events"
                )
                .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: detailedSettingWidth)

              AppLocalizedLabel(
                "settings.devices.vendor_events_hint",
                systemImage: "lightbulb"
              )
              .foregroundColor(Color(NSColor.textColor))
              .font(.caption)
            }
          }
        }
      }
    }
  }
}
