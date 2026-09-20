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
    // With the device list in an HStack, sidebar styling can leave the previously selected row
    // bold after switching away and back on macOS 27. Use regular inset-list selection styling.
    .listStyle(.inset)
    .modifier(FocusOnClick())
    .overlay {
      if client.connectedDevices.isEmpty {
        AppLocalizedText("event_viewer.devices.empty")
          .foregroundStyle(.secondary)
      }
    }
  }
}
