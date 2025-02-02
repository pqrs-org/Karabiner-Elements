import SwiftUI

struct DevicesView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var connectedDevices = LibKrbn.ConnectedDevices.shared
  @State private var showEraseNotConnectedDeviceSettingsButton = false

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      List {
        VStack(alignment: .leading, spacing: 0.0) {
          ForEach(connectedDevices.connectedDevices) { connectedDevice in
            if let connectedDeviceSetting = settings.findConnectedDeviceSetting(connectedDevice) {
              VStack(alignment: .leading, spacing: 0.0) {
                DeviceName(connectedDevice: connectedDevice)
                  .if(connectedDevice.isVirtualDevice) {
                    $0.foregroundColor(Color(NSColor.placeholderTextColor))
                  }

                if !connectedDevice.isVirtualDevice {
                  VStack(alignment: .leading, spacing: 0.0) {
                    ModifyEventsSetting(connectedDeviceSetting: connectedDeviceSetting)

                    VStack(alignment: .leading, spacing: 6.0) {
                      KeyboardSettings(connectedDeviceSetting: connectedDeviceSetting)

                      MouseSettings(connectedDeviceSetting: connectedDeviceSetting)

                      GamePadSettings(connectedDeviceSetting: connectedDeviceSetting)

                      ExtraSettings(connectedDeviceSetting: connectedDeviceSetting)
                    }
                    .padding(.leading, 20.0)
                    .padding(.top, 8.0)
                    .frame(width: 400.0)
                  }
                  .padding(.leading, 62.0)
                  .padding(.top, 20.0)
                }
              }
              .padding(.vertical, 12.0)
              .padding(.trailing, 12.0)
              .overlay(
                RoundedRectangle(cornerRadius: 8)
                  .stroke(
                    Color(NSColor.selectedControlColor),
                    lineWidth: settings.findConnectedDeviceSetting(connectedDevice)?.modifyEvents
                      ?? false && !connectedDevice.isVirtualDevice
                      ? 3 : 0
                  )
                  .padding(2)
              )

              Divider()
            }
          }

          Spacer()
        }
      }
      .background(Color(NSColor.textBackgroundColor))

      if settings.notConnectedDeviceSettingsCount > 0 {
        HStack {
          Text(
            "There are \(settings.notConnectedDeviceSettingsCount) other settings for devices that are not currently connected"
          )

          Spacer()

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
                  "Remove settings for \(settings.notConnectedDeviceSettingsCount) devices",
                  systemImage: "trash"
                )
                .buttonLabelStyle()
              }
            )
            .deleteButtonStyle()
          }
        }
        .padding()
        .foregroundColor(Color.infoForeground)
        .background(Color.infoBackground)
      }
    }
    .padding()
  }

  struct DeviceName: View {
    let connectedDevice: LibKrbn.ConnectedDevice

    var body: some View {
      HStack(alignment: .center, spacing: 0) {
        HStack(spacing: 4.0) {
          Spacer()
          if connectedDevice.isKeyboard {
            Image(systemName: "keyboard")
          }
          if connectedDevice.isPointingDevice {
            Image(systemName: "computermouse")
          }
          if connectedDevice.isGamePad {
            Image(systemName: "gamecontroller")
          }
        }
        .frame(width: 50.0)

        Text("\(connectedDevice.productName) (\(connectedDevice.manufacturerName))")
          .padding(.leading, 12.0)
          .frame(maxWidth: .infinity, alignment: .leading)

        if connectedDevice.transport != "FIFO" {
          VStack(alignment: .trailing, spacing: 4.0) {
            if connectedDevice.vendorId != 0 {
              HStack(alignment: .firstTextBaseline, spacing: 0) {
                Spacer()

                Text(
                  String(
                    format: "Vendor ID: %5d (0x%04x)",
                    connectedDevice.vendorId,
                    connectedDevice.vendorId)
                )
              }
            }

            if connectedDevice.productId != 0 {
              HStack(alignment: .center, spacing: 0) {
                Spacer()

                Text(
                  String(
                    format: "Product ID: %5d (0x%04x)",
                    connectedDevice.productId,
                    connectedDevice.productId)
                )
              }
            }

            if !connectedDevice.deviceAddress.isEmpty {
              HStack(alignment: .center, spacing: 0) {
                Spacer()

                Text("Device Address: ")

                Text(connectedDevice.deviceAddress)
              }
            }
          }
          .font(.custom("Menlo", size: 12.0))
          .frame(width: 220.0)
        }
      }
    }
  }

  struct ModifyEventsSetting: View {
    @ObservedObject var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting

    @ObservedObject private var settings = LibKrbn.Settings.shared

    var body: some View {
      HStack(alignment: .top) {
        if connectedDeviceSetting.connectedDevice.isAppleDevice,
          !connectedDeviceSetting.connectedDevice.isKeyboard,
          connectedDeviceSetting.connectedDevice.isPointingDevice,
          !settings.unsafeUI
        {
          Text("Apple pointing devices are not supported")
            .foregroundColor(Color(NSColor.placeholderTextColor))
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          Toggle(isOn: $connectedDeviceSetting.modifyEvents) {
            Text("Modify events")
          }
          .switchToggleStyle()
          .frame(width: 140.0)
        }

        Spacer()
      }
    }
  }

  struct KeyboardSettings: View {
    @ObservedObject var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting

    @ObservedObject private var settings = LibKrbn.Settings.shared

    var body: some View {
      VStack {
        if connectedDeviceSetting.connectedDevice.isKeyboard {
          VStack(alignment: .leading, spacing: 6.0) {
            if !connectedDeviceSetting.connectedDevice.isBuiltInKeyboard
              && !connectedDeviceSetting.disableBuiltInKeyboardIfExists
            {
              Toggle(isOn: $connectedDeviceSetting.treatAsBuiltInKeyboard) {
                Text("Treat as a built-in keyboard")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
            }

            if !connectedDeviceSetting.connectedDevice.isBuiltInKeyboard
              && !connectedDeviceSetting.treatAsBuiltInKeyboard
            {
              Toggle(isOn: $connectedDeviceSetting.disableBuiltInKeyboardIfExists) {
                Text("Disable the built-in keyboard while this device is connected")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
            }

            if connectedDeviceSetting.modifyEvents {
              Toggle(isOn: $connectedDeviceSetting.manipulateCapsLockLed) {
                Text("Manipulate caps lock LED")
                  .frame(maxWidth: .infinity, alignment: .leading)
              }
              .switchToggleStyle(controlSize: .mini, font: .callout)
            }
          }
        }
      }
    }
  }

  struct MouseSettings: View {
    @ObservedObject var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting
    @State var showing = false

    var body: some View {
      VStack {
        if connectedDeviceSetting.modifyEvents
          && connectedDeviceSetting.connectedDevice.isPointingDevice
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
              connectedDeviceSetting: connectedDeviceSetting,
              showing: $showing
            )
          }
        }
      }
    }
  }

  struct GamePadSettings: View {
    @ObservedObject var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting
    @State var showing = false

    var body: some View {
      VStack {
        if connectedDeviceSetting.modifyEvents && connectedDeviceSetting.connectedDevice.isGamePad {
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
              connectedDeviceSetting: connectedDeviceSetting,
              showing: $showing
            )
          }
        }
      }
    }
  }

  struct ExtraSettings: View {
    @ObservedObject var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting

    var body: some View {
      VStack {
        if connectedDeviceSetting.modifyEvents {
          Toggle(isOn: $connectedDeviceSetting.ignoreVendorEvents) {
            Text(
              "Ignore vendor events\n(It is recommended to enable this setting for non-Apple devices)"
            )
            .frame(maxWidth: .infinity, alignment: .leading)
          }
          .switchToggleStyle(controlSize: .mini, font: .callout)
        }
      }
    }
  }
}

struct DevicesView_Previews: PreviewProvider {
  static var previews: some View {
    DevicesView()
  }
}
