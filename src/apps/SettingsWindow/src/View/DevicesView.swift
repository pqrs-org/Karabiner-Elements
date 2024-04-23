import SwiftUI

struct DevicesView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var connectedDevices = LibKrbn.ConnectedDevices.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      List {
        VStack(alignment: .leading, spacing: 0.0) {
          ForEach(connectedDevices.connectedDevices) { connectedDevice in
            VStack(alignment: .leading, spacing: 0.0) {
              DeviceName(connectedDevice: connectedDevice)

              if !connectedDevice.isKarabinerVirtualHidDevice {
                VStack(alignment: .leading, spacing: 0.0) {
                  ModifyEventsSetting(connectedDevice: connectedDevice)

                  VStack(alignment: .leading, spacing: 12.0) {
                    KeyboardSettings(connectedDevice: connectedDevice)

                    MouseSettings(connectedDevice: connectedDevice)

                    GamePadSettings(connectedDevice: connectedDevice)
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
            .overlay(
              RoundedRectangle(cornerRadius: 8)
                .stroke(
                  Color(NSColor.selectedControlColor),
                  lineWidth: settings.findConnectedDeviceSetting(connectedDevice)?.modifyEvents
                    ?? false
                    ? 3 : 0
                )
                .padding(2)
            )

            Divider()
          }

          Spacer()
        }
      }
      .background(Color(NSColor.textBackgroundColor))
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
            Image(systemName: "capsule.portrait")
          }
          if connectedDevice.isGamePad {
            Image(systemName: "gamecontroller")
          }
        }
        .frame(width: 50.0)

        Text("\(connectedDevice.productName) (\(connectedDevice.manufacturerName))")
          .padding(.leading, 12.0)

        Spacer()
      }
    }
  }

  struct ModifyEventsSetting: View {
    let connectedDevice: LibKrbn.ConnectedDevice
    @ObservedObject private var settings = LibKrbn.Settings.shared
    @State var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting?

    var body: some View {
      HStack(alignment: .top) {
        if connectedDevice.isAppleDevice,
          !connectedDevice.isKeyboard,
          connectedDevice.isPointingDevice,
          !settings.unsafeUI
        {
          Text("Apple pointing devices are not supported")
            .foregroundColor(Color(NSColor.placeholderTextColor))
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          if let s = connectedDeviceSetting {
            let binding = Binding {
              s
            } set: {
              connectedDeviceSetting = $0
            }
            Toggle(isOn: binding.modifyEvents) {
              Text("Modify events")
            }
            .switchToggleStyle()
            .frame(width: 140.0)
          }
        }

        Spacer()

        if connectedDevice.transport != "FIFO" {
          VStack(alignment: .trailing, spacing: 4.0) {
            if connectedDevice.vendorId != 0 {
              HStack(alignment: .firstTextBaseline, spacing: 0) {
                Spacer()

                Text("Vendor ID: ")

                Text(
                  String(
                    format: "%5d (0x%04x)",
                    connectedDevice.vendorId,
                    connectedDevice.vendorId)
                )
              }
            }

            if connectedDevice.productId != 0 {
              HStack(alignment: .center, spacing: 0) {
                Spacer()

                Text("Product ID: ")

                Text(
                  String(
                    format: "%5d (0x%04x)",
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
        }
      }
      .onAppear {
        setConnectedDeviceSetting()
      }
      .onChange(of: settings.connectedDeviceSettings) { _ in
        setConnectedDeviceSetting()
      }
    }

    private func setConnectedDeviceSetting() {
      connectedDeviceSetting = settings.findConnectedDeviceSetting(connectedDevice)
    }
  }

  struct KeyboardSettings: View {
    let connectedDevice: LibKrbn.ConnectedDevice
    @ObservedObject private var settings = LibKrbn.Settings.shared
    @State var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting?

    var body: some View {
      VStack {
        if connectedDevice.isKeyboard {
          VStack(alignment: .leading, spacing: 2.0) {
            if let s = connectedDeviceSetting {
              let binding = Binding {
                s
              } set: {
                connectedDeviceSetting = $0
              }

              if !connectedDevice.isBuiltInKeyboard
                && !s.disableBuiltInKeyboardIfExists
              {
                Toggle(isOn: binding.treatAsBuiltInKeyboard) {
                  Text("Treat as a built-in keyboard")
                    .frame(maxWidth: .infinity, alignment: .leading)
                }
                .switchToggleStyle(controlSize: .mini, font: .callout)
              }

              if !connectedDevice.isBuiltInKeyboard
                && !s.treatAsBuiltInKeyboard
              {
                Toggle(isOn: binding.disableBuiltInKeyboardIfExists) {
                  Text("Disable the built-in keyboard while this device is connected")
                    .frame(maxWidth: .infinity, alignment: .leading)
                }
                .switchToggleStyle(controlSize: .mini, font: .callout)
              }

              if s.modifyEvents {
                Toggle(isOn: binding.manipulateCapsLockLed) {
                  Text("Manipulate caps lock LED")
                    .frame(maxWidth: .infinity, alignment: .leading)
                }
                .switchToggleStyle(controlSize: .mini, font: .callout)
              }
            }
          }
          .frame(width: 400.0)
        }
      }
      .onAppear {
        setConnectedDeviceSetting()
      }
      .onChange(of: settings.connectedDeviceSettings) { _ in
        setConnectedDeviceSetting()
      }
    }

    private func setConnectedDeviceSetting() {
      connectedDeviceSetting = settings.findConnectedDeviceSetting(connectedDevice)
    }
  }

  struct MouseSettings: View {
    let connectedDevice: LibKrbn.ConnectedDevice
    @ObservedObject private var settings = LibKrbn.Settings.shared
    @State var connectedDeviceSetting: LibKrbn.ConnectedDeviceSetting?

    var body: some View {
      VStack {
        if connectedDevice.isPointingDevice
          || connectedDevice.isGamePad
        {
          if let s = connectedDeviceSetting {
            let binding = Binding {
              s
            } set: {
              connectedDeviceSetting = $0
            }
            if s.modifyEvents {
              HStack(alignment: .top, spacing: 100.0) {
                VStack(alignment: .leading, spacing: 2.0) {
                  Toggle(isOn: binding.mouseFlipX) {
                    Text("Flip mouse X")
                      .frame(maxWidth: .infinity, alignment: .leading)
                  }
                  .switchToggleStyle(controlSize: .mini, font: .callout)

                  Toggle(isOn: binding.mouseFlipY) {
                    Text("Flip mouse Y")
                      .frame(maxWidth: .infinity, alignment: .leading)
                  }
                  .switchToggleStyle(controlSize: .mini, font: .callout)

                  Toggle(isOn: binding.mouseFlipVerticalWheel) {
                    Text("Flip mouse vertical wheel")
                      .frame(maxWidth: .infinity, alignment: .leading)
                  }
                  .switchToggleStyle(controlSize: .mini, font: .callout)

                  Toggle(isOn: binding.mouseFlipHorizontalWheel) {
                    Text("Flip mouse horizontal wheel")
                      .frame(maxWidth: .infinity, alignment: .leading)
                  }
                  .switchToggleStyle(controlSize: .mini, font: .callout)
                }
                .frame(width: 200.0)

                VStack(alignment: .leading, spacing: 2.0) {
                  Toggle(isOn: binding.mouseSwapXY) {
                    Text("Swap mouse X and Y")
                      .frame(maxWidth: .infinity, alignment: .leading)
                  }
                  .switchToggleStyle(controlSize: .mini, font: .callout)

                  Toggle(isOn: binding.mouseSwapWheels) {
                    Text("Swap mouse wheels")
                      .frame(maxWidth: .infinity, alignment: .leading)
                  }
                  .switchToggleStyle(controlSize: .mini, font: .callout)
                }
                .frame(width: 200.0)
              }
            }
          }
        }
      }
      .onAppear {
        setConnectedDeviceSetting()
      }
      .onChange(of: settings.connectedDeviceSettings) { _ in
        setConnectedDeviceSetting()
      }
    }

    private func setConnectedDeviceSetting() {
      connectedDeviceSetting = settings.findConnectedDeviceSetting(connectedDevice)
    }
  }

  struct GamePadSettings: View {
    let connectedDevice: LibKrbn.ConnectedDevice
    @State var showing = false

    var body: some View {
      VStack {
        if connectedDevice.isGamePad {
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
              showing: $showing
            )
          }
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
