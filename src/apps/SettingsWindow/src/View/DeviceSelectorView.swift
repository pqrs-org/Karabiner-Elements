import SwiftUI

struct DeviceSelectorView: View {
  @Binding var selectedDevice: LibKrbn.ConnectedDevice?
  @ObservedObject private var connectedDevices = LibKrbn.ConnectedDevices.shared
  @ObservedObject private var settings = LibKrbn.Settings.shared

  @State var selected: LibKrbn.ConnectedDevice?

  var body: some View {
    // Build a combined array with `ConnectedDevice.zero` for "For all devices"
    let filtered = connectedDevices.connectedDevices.filter {
      (settings.findConnectedDeviceSetting($0)?.modifyEvents ?? false)
        && !$0.isVirtualDevice
    }
    let targets = [LibKrbn.ConnectedDevice.zero] + filtered

    List(targets, selection: $selected) { device in
      Label(
        title: {
          Text(deviceLabelTitle(device))
            .lineLimit(nil)
            .fixedSize(horizontal: false, vertical: true)
        },
        icon: {
          if device.index < 0 {
            Image(systemName: "circle.grid.2x2")
          } else {
            VStack {
              if device.isKeyboard { Image(systemName: "keyboard") }
              if device.isPointingDevice { Image(systemName: "computermouse") }
              if device.isGamePad { Image(systemName: "gamecontroller") }
              if device.isConsumer { Image(systemName: "headphones") }
            }
            .frame(width: 20.0)
          }
        }
      )
      .frame(maxWidth: .infinity, alignment: .leading)
      .padding(.vertical, 8)
      .listRowSeparator(.visible, edges: .bottom)
      .tag(Optional(device))
    }
    .listStyle(.sidebar)
    .frame(width: 250)
    .onAppear {
      if selectedDevice == nil {
        selected = LibKrbn.ConnectedDevice.zero
      } else {
        selected = selectedDevice
      }
    }
    .onChange(of: selected) { newValue in
      if let selected = selected {
        if selected.index >= 0 {
          selectedDevice = selected
          return
        }
      }
      selectedDevice = nil
    }
  }

  private func deviceLabelTitle(_ device: LibKrbn.ConnectedDevice) -> String {
    if device.index < 0 {
      return "For all devices"
    } else {
      var title = "\(device.productName) (\(device.manufacturerName))"

      if device.vendorId == 0 && device.productId == 0 {
        if !device.deviceAddress.isEmpty {
          title += "\n  \(device.deviceAddress)"
        }
      } else {
        title += "\n  [VID: \(device.vendorId), PID: \(device.productId)]"
      }

      return title
    }
  }
}
