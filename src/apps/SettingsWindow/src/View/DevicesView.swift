import SwiftUI

struct DevicesView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var connectedDevices = ConnectedDevices.shared
  @State private var showEraseNotConnectedDeviceSettingsButton = false

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      // Build all device rows eagerly so offscreen grids are laid out before scrolling.
      ScrollView {
        VStack(alignment: .leading, spacing: 4.0) {
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

                    Grid(alignment: .leading, horizontalSpacing: 8.0, verticalSpacing: 6.0) {
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

                    if deviceConfiguration.wrappedValue.modifyEvents
                      && !connectedDevice.isAppleDevice
                    {
                      AppLocalizedLabel(
                        "settings.devices.vendor_events_hint",
                        systemImage: "lightbulb"
                      )
                      .foregroundColor(Color(NSColor.textColor))
                      .font(.caption)
                      .frame(maxWidth: .infinity, alignment: .leading)
                      .padding(.leading, 20.0)
                      .padding(.top, 4.0)
                    }
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
        .padding(8.0)
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
            .fixedSize(horizontal: true, vertical: false)

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
      if connectedDevice.isKeyboard {
        if !connectedDevice.isBuiltInKeyboard
          && !deviceConfiguration.disableBuiltInKeyboardIfExists
        {
          DetailedSetting(
            title: "settings.devices.treat_as_built_in",
            isOn: $deviceConfiguration.treatAsBuiltInKeyboard)
        }

        if !connectedDevice.isBuiltInKeyboard
          && !deviceConfiguration.treatAsBuiltInKeyboard
        {
          DetailedSetting(
            title: "settings.devices.disable_built_in",
            isOn: $deviceConfiguration.disableBuiltInKeyboardIfExists)
        }

        if deviceConfiguration.modifyEvents {
          DetailedSetting(
            title: "settings.devices.caps_lock_led",
            isOn: $deviceConfiguration.manipulateCapsLockLed)

          DetailedSetting(
            title: "settings.devices.swap_iso_keys",
            isOn: $deviceConfiguration.swapGraveAccentAndNonUsBackslash)
        }
      }
    }
  }

  struct DetailedSetting: View {
    let title: String
    @Binding var isOn: Bool

    var body: some View {
      GridRow {
        AppLocalizedText(title)
          .font(.callout)

        Toggle(isOn: $isOn) {
          AppLocalizedText(title)
        }
        .switchToggleStyle(controlSize: .mini, font: .callout)
        .labelsHidden()
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
            AppLocalizedConstrainedLabel(
              "settings.devices.open_mouse_settings", systemImage: "computermouse"
            )
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
            AppLocalizedConstrainedLabel(
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
      if deviceConfiguration.modifyEvents && !connectedDevice.isAppleDevice {
        DetailedSetting(
          title: "settings.devices.ignore_vendor_events",
          isOn: $deviceConfiguration.ignoreVendorEvents)
      }
    }
  }
}
