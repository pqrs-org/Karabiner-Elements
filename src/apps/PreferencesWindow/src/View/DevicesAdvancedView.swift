import SwiftUI

struct DevicesAdvancedView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      GroupBox(label: Text("Delay before open device")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            IntTextField(
              value: $settings.delayMillisecondsBeforeOpenDevice,
              range: 0...10000,
              step: 100,
              width: 50)

            Text("milliseconds (Default value is 1000)")
            Spacer()
          }

          HStack(alignment: .center, spacing: 12.0) {
            Image(systemName: "exclamationmark.triangle")

            VStack(alignment: .leading, spacing: 0.0) {
              Text(
                "Setting insufficient delay (e.g., 0) will result in a device becoming unusable after Karabiner-Elements is quit."
              )
              Text(
                "(This is a macOS problem and can be solved by unplugging the device and plugging it again.)"
              )
            }
          }
        }
        .padding(6.0)
      }

      Spacer()
        .frame(height: 24.0)

      GroupBox(
        label: Text(
          "Disable the built-in keyboard")
      ) {
        VStack(alignment: .leading, spacing: 12.0) {
          Text(
            "Disable the built-in keyboard while one of the following selected devices is connected."
          )

          List {
            VStack(alignment: .leading, spacing: 0.0) {
              ForEach($settings.connectedDeviceSettings) { $connectedDeviceSetting in
                Toggle(isOn: $connectedDeviceSetting.disableBuiltInKeyboardIfExists) {
                  HStack(alignment: .center, spacing: 0) {
                    HStack(spacing: 4.0) {
                      Spacer()
                      if connectedDeviceSetting.connectedDevice.isKeyboard {
                        Image(systemName: "keyboard")
                      }
                      if connectedDeviceSetting.connectedDevice.isPointingDevice {
                        Image(systemName: "capsule.portrait")
                      }
                    }
                    .frame(width: 50.0)

                    Text(
                      "\(connectedDeviceSetting.connectedDevice.productName) (\(connectedDeviceSetting.connectedDevice.manufacturerName)) [\(String(connectedDeviceSetting.connectedDevice.vendorId)),\(String(connectedDeviceSetting.connectedDevice.productId))]"
                    )
                    .padding(.leading, 12.0)

                    Spacer()
                  }
                  .padding(.vertical, 12.0)
                }
                .switchToggleStyle()
                .disabled(
                  connectedDeviceSetting.connectedDevice.isBuiltInKeyboard
                    || connectedDeviceSetting.connectedDevice.isBuiltInTrackpad
                    || connectedDeviceSetting.connectedDevice.isBuiltInTouchBar
                )

                Divider()
              }

              Spacer()
            }
          }
          .background(Color(NSColor.textBackgroundColor))
        }
      }

      Spacer()
    }
    .padding()
  }
}

struct DevicesAdvancediew_Previews: PreviewProvider {
  static var previews: some View {
    DevicesAdvancedView()
  }
}
