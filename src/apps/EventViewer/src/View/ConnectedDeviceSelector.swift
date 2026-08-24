import SwiftUI

struct ConnectedDeviceSelector: View {
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @Binding var selection: UInt64?

  var body: some View {
    List(selection: $selection) {
      ForEach(client.connectedDevices) { device in
        ConnectedDeviceLabel(
          title: title(device),
          isKeyboard: device.isKeyboard,
          isPointingDevice: device.isPointingDevice,
          isGamePad: device.isGamePad,
          isConsumer: device.isConsumer
        )
        .tag(device.id)
      }
    }
    .listStyle(.sidebar)
    .overlay {
      if client.connectedDevices.isEmpty {
        Text("No devices connected.")
          .foregroundStyle(.secondary)
      }
    }
  }

  private func title(_ device: EVCoreServiceDaemonClient.ConnectedDevice) -> String {
    var title = device.name
    if !device.manufacturer.isEmpty {
      title += " (\(device.manufacturer))"
    }

    if device.vendorId != 0 || device.productId != 0 {
      title += "\n  [VID: \(device.vendorId), PID: \(device.productId)]"
    }

    return title
  }
}
