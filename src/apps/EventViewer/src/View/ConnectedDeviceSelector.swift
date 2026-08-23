import SwiftUI

struct ConnectedDeviceSelector: View {
  @ObservedObject private var client = EVCoreServiceDaemonClient.shared
  @Binding var selection: UInt64?

  var body: some View {
    List(selection: $selection) {
      ForEach(client.connectedDevices) { device in
        Label {
          Text(title(device))
            .lineLimit(nil)
            .fixedSize(horizontal: false, vertical: true)
        } icon: {
          VStack {
            if device.isKeyboard { Image(systemName: "keyboard") }
            if device.isPointingDevice { Image(systemName: "computermouse") }
            if device.isGamePad { Image(systemName: "gamecontroller") }
            if device.isConsumer { Image(systemName: "headphones") }
          }
          .frame(width: 20)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.vertical, 8)
        .listRowSeparator(.visible, edges: .bottom)
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
