import SwiftUI

struct DeviceSelectorView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @Binding var selectedDevice: LibKrbn.ConnectedDevice?

  var body: some View {
    List {
      VStack(alignment: .leading, spacing: 0) {
        Button(
          action: {
            selectedDevice = nil
          },
          label: {
            HStack {
              Text("For all devices")

              Spacer()
            }
            .sidebarButtonLabelStyle()
          }
        )
        .sidebarButtonStyle(selected: selectedDevice == nil)

        Divider()

        ForEach($settings.connectedDeviceSettings) { $connectedDeviceSetting in
          Button(
            action: {
              selectedDevice = connectedDeviceSetting.connectedDevice
            },
            label: {
              let noId =
                connectedDeviceSetting.connectedDevice.vendorId == 0
                && connectedDeviceSetting.connectedDevice.productId == 0
              let label =
                noId
                ? connectedDeviceSetting.connectedDevice.deviceAddress
                : "\(String(connectedDeviceSetting.connectedDevice.vendorId)),\(String(connectedDeviceSetting.connectedDevice.productId))"
              HStack {
                Text(
                  """
                  \(connectedDeviceSetting.connectedDevice.productName) \
                  (\(connectedDeviceSetting.connectedDevice.manufacturerName)) \
                  \(label != "" ? "[\(label)]" : "")
                  """
                )

                Spacer()

                VStack {
                  if connectedDeviceSetting.connectedDevice.isKeyboard {
                    Image(systemName: "keyboard")
                  }
                  if connectedDeviceSetting.connectedDevice.isPointingDevice {
                    Image(systemName: "capsule.portrait")
                  }
                  if connectedDeviceSetting.connectedDevice.isGamePad {
                    Image(systemName: "gamecontroller")
                  }
                }
              }
              .sidebarButtonLabelStyle()
            }
          )
          .sidebarButtonStyle(
            selected: selectedDevice?.id == connectedDeviceSetting.connectedDevice.id)

          Divider()
        }

        Spacer()
      }
      .background(Color(NSColor.textBackgroundColor))
    }
    .frame(width: 250)
  }
}
