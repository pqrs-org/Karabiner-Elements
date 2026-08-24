import SwiftUI

struct ConnectedDeviceSelector: View {
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @Binding var selection: UInt64?

  var body: some View {
    List(selection: $selection) {
      ForEach(client.connectedDevices) { device in
        ConnectedDeviceLabel(
          title: connectedDeviceLabelTitle(
            productName: device.name,
            manufacturerName: device.manufacturer,
            vendorId: device.vendorId,
            productId: device.productId,
            deviceAddress: device.deviceAddress
          ),
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
}
