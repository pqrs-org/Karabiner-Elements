import SwiftUI

struct ConnectedDeviceSelector: View {
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @Binding var selection: UInt64?
  @FocusState private var deviceListFocused: Bool

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
    // On macOS 27, clicking a row can change selection while focus stays in the other list,
    // leaving the selection highlight inactive. Explicitly focus the clicked list.
    // A simultaneous tap preserves native selection and also handles clicks on the selected row;
    // observing selection changes would miss those clicks and react to programmatic changes.
    .focused($deviceListFocused)
    .simultaneousGesture(
      TapGesture().onEnded {
        deviceListFocused = true
      }
    )
    .overlay {
      if client.connectedDevices.isEmpty {
        AppLocalizedText("event_viewer.devices.empty")
          .foregroundStyle(.secondary)
      }
    }
  }
}
