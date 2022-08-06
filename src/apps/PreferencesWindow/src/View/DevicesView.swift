import SwiftUI

struct DevicesView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      List {
        VStack(alignment: .leading, spacing: 0.0) {
          ForEach($settings.connectedDeviceSettings) { $connectedDeviceSetting in
            VStack(alignment: .leading, spacing: 8.0) {
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
                  "\(connectedDeviceSetting.connectedDevice.productName) (\(connectedDeviceSetting.connectedDevice.manufacturerName))"
                )
                .padding(.leading, 12.0)

                Spacer()

                VStack(alignment: .trailing, spacing: 0) {
                  HStack(alignment: .center, spacing: 0) {
                    Spacer()

                    Text("Vendor ID: ")
                      .font(.caption)
                    Text(
                      String(
                        format: "%5d (0x%04x)",
                        connectedDeviceSetting.connectedDevice.vendorId,
                        connectedDeviceSetting.connectedDevice.vendorId)
                    )
                    .font(.custom("Menlo", size: 11.0))
                  }
                  HStack(alignment: .center, spacing: 0) {
                    Spacer()

                    Text("Product ID: ")
                      .font(.caption)
                    Text(
                      String(
                        format: "%5d (0x%04x)",
                        connectedDeviceSetting.connectedDevice.productId,
                        connectedDeviceSetting.connectedDevice.productId)
                    )
                    .font(.custom("Menlo", size: 11.0))
                  }
                }
              }

              HStack(alignment: .top, spacing: 0) {
                if connectedDeviceSetting.connectedDevice.isAppleDevice,
                  !connectedDeviceSetting.connectedDevice.isKeyboard,
                  !settings.unsafeUI
                {
                  Text("Apple pointing devices are not supported")
                    .foregroundColor(Color(NSColor.placeholderTextColor))
                  Spacer()
                } else {
                  Toggle(isOn: $connectedDeviceSetting.modifyEvents) {
                    Text("Modify events")
                  }
                  .switchToggleStyle()

                  Spacer()

                  if connectedDeviceSetting.connectedDevice.isKeyboard {
                    VStack(alignment: .trailing, spacing: 6.0) {
                      HStack {
                        Toggle(isOn: $connectedDeviceSetting.manipulateCapsLockLed) {
                          Text("Manipulate caps lock LED")
                        }
                        .switchToggleStyle()
                      }

                      if !connectedDeviceSetting.connectedDevice.isBuiltInKeyboard {
                        Toggle(isOn: $connectedDeviceSetting.treatAsBuiltInKeyboard) {
                          Text("Treat as a built-in keyboard")
                        }
                        .switchToggleStyle()
                      }
                    }
                  }
                }
              }
              .padding(.leading, 62.0)
            }
            .padding(.vertical, 12.0)

            Divider()
          }

          Spacer()
        }
      }
      .background(Color(NSColor.textBackgroundColor))
    }
    .padding()
  }
}

struct DevicesView_Previews: PreviewProvider {
  static var previews: some View {
    DevicesView()
  }
}
