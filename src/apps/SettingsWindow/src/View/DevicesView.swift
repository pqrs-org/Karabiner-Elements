import SwiftUI

struct DevicesView: View {
  @ObservedObject private var settings = Settings.shared
  @ObservedObject private var connectedDevices = ConnectedDevices.shared
  @State private var showEraseNotConnectedDeviceSettingsButton = false

  static let detailedSettingWidth = 400.0

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      Toggle(isOn: $settings.configuration.selectedProfile.modifyMouseEventsByDefault) {
        Text("Modify mouse events by default")
      }
      .switchToggleStyle()
      .padding()

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
            "There are \(connectedDevices.notConnectedConfiguredDevicesCount) other settings for devices that are not currently connected",
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
                  "Remove settings for \(connectedDevices.notConnectedConfiguredDevicesCount) devices",
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

        Text("\(connectedDevice.productName) (\(connectedDevice.manufacturerName))")
          .padding(.leading, 12.0)
          .frame(maxWidth: .infinity, alignment: .leading)

        if connectedDevice.transport != "FIFO" {
          VStack(alignment: .trailing, spacing: 4.0) {
            if connectedDevice.vendorId != 0 {
              Text(
                String(
                  format: "Vendor ID: %5d (0x%04x)",
                  connectedDevice.vendorId,
                  connectedDevice.vendorId)
              )
            }

            if connectedDevice.productId != 0 {
              Text(
                String(
                  format: "Product ID: %5d (0x%04x)",
                  connectedDevice.productId,
                  connectedDevice.productId)
              )
            }

            if !connectedDevice.deviceAddress.isEmpty {
              Text("Device Address: \(connectedDevice.deviceAddress)")
            }
          }
          .font(.callout)
          .monospaced()
        }
      }
    }
  }

  struct ModifyEventsSetting: View {
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
          Text("Apple pointing devices are not supported")
            .foregroundColor(Color(NSColor.placeholderTextColor))
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          VStack(alignment: .leading) {
            Toggle(isOn: $deviceConfiguration.modifyEvents) {
              Text("Modify events")
            }
            .switchToggleStyle()
            .frame(width: 140.0)

            if settings.configuration.globalConfiguration.enableCgeventtapFallback
              && !deviceConfiguration.modifyEvents
              && connectedDevice.isKeyboard
            {
              Label(
                title: {
                  Text(
                    "Key events from this device are handled via the CGEventTap fallback"
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
                Text("Treat as a built-in keyboard")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: detailedSettingWidth)
            }

            if !connectedDevice.isBuiltInKeyboard
              && !deviceConfiguration.treatAsBuiltInKeyboard
            {
              Toggle(isOn: $deviceConfiguration.disableBuiltInKeyboardIfExists) {
                Text("Disable the built-in keyboard while this device is connected")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: detailedSettingWidth)
            }

            if deviceConfiguration.modifyEvents {
              Toggle(isOn: $deviceConfiguration.manipulateCapsLockLed) {
                Text("Manipulate caps lock LED")
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
            Label("Open mouse settings", systemImage: "computermouse")
              .buttonLabelStyle()
          }
        )
        .sheet(isPresented: $showing) {
          DevicesMouseSettingsView(
            connectedDevice: connectedDevice,
            deviceConfiguration: $deviceConfiguration,
            showing: $showing
          )
        }
      } else {
        EmptyView()
      }
    }
  }

  struct GamePadSettings: View {
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
            Label("Open game pad settings", systemImage: "gamecontroller")
              .buttonLabelStyle()
          }
        )
        .sheet(isPresented: $showing) {
          DevicesGamePadSettingsView(
            connectedDevice: connectedDevice,
            deviceConfiguration: $deviceConfiguration,
            showing: $showing
          )
        }
      } else {
        EmptyView()
      }
    }
  }

  struct ExtraSettings: View {
    let connectedDevice: ConnectedDevice
    @Binding var deviceConfiguration: SettingsConfiguration.Device

    var body: some View {
      VStack {
        if deviceConfiguration.modifyEvents {
          if !connectedDevice.isAppleDevice {
            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(isOn: $deviceConfiguration.ignoreVendorEvents) {
                Text(
                  "Ignore vendor events"
                )
                .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
              .frame(width: detailedSettingWidth)

              Label(
                #"It is recommended to enable "Ignore vendor events" for non-Apple devices"#,
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
