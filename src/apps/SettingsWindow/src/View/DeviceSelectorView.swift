import SwiftUI

struct DeviceSelectorView: View {
  @Binding var selectedDevice: LibKrbn.ConnectedDevice?
  @ObservedObject private var connectedDevices = LibKrbn.ConnectedDevices.shared
  @ObservedObject private var settings = LibKrbn.Settings.shared

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

        ForEach(connectedDevices.connectedDevices) { connectedDevice in
          if settings.findConnectedDeviceSetting(connectedDevice)?.modifyEvents ?? false,
            !connectedDevice.isKarabinerVirtualHidDevice
          {
            Button(
              action: {
                selectedDevice = connectedDevice
              },
              label: {
                let noId = connectedDevice.vendorId == 0 && connectedDevice.productId == 0
                let label =
                  noId
                  ? connectedDevice.deviceAddress
                  : "\(String(connectedDevice.vendorId)),\(String(connectedDevice.productId))"
                HStack {
                  Text(
                    """
                    \(connectedDevice.productName) \
                    (\(connectedDevice.manufacturerName)) \
                    \(label != "" ? "[\(label)]" : "")
                    """
                  )

                  Spacer()

                  VStack {
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
                }
                .sidebarButtonLabelStyle()
              }
            )
            .sidebarButtonStyle(
              selected: selectedDevice?.id == connectedDevice.id)

            Divider()
          }
        }

        Spacer()
      }
      .background(Color(NSColor.textBackgroundColor))
    }
    .frame(width: 250)
  }
}
