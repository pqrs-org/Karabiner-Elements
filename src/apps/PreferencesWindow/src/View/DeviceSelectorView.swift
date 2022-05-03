import SwiftUI

struct DeviceSelectorView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
    @Binding var selectedDevice: LibKrbn.ConnectedDevice?

  var body: some View {
    List {
      VStack(alignment: .leading, spacing: 0) {
        Button(action: {
          selectedDevice = nil
        }) {
          HStack {
            Text("For all devices")

            Spacer()
          }
          .sidebarButtonLabelStyle()
        }
        .sidebarButtonStyle(selected: selectedDevice == nil)

        Divider()

        ForEach($settings.connectedDeviceSettings) { $connectedDeviceSetting in
          Button(action: {
            selectedDevice = connectedDeviceSetting.connectedDevice
          }) {
            HStack {
              Text(
                """
                \(connectedDeviceSetting.connectedDevice.productName) \
                (\(connectedDeviceSetting.connectedDevice.manufacturerName)) \
                [\(String(connectedDeviceSetting.connectedDevice.vendorId)),\(String(connectedDeviceSetting.connectedDevice.productId))]
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
              }
            }
            .sidebarButtonLabelStyle()
          }
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
