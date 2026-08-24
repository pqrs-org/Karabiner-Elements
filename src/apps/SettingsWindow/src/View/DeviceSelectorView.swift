import SwiftUI

struct DeviceSelectorView: View {
  @Binding var selectedDevice: ConnectedDevice?
  @ObservedObject private var connectedDevices = ConnectedDevices.shared
  @ObservedObject private var settings = Settings.shared

  @State var selected = ConnectedDevice.zero

  var body: some View {
    // Build a combined array with `ConnectedDevice.zero` for "For all devices"
    let filtered = connectedDevices.connectedDevices.filter {
      (settings.deviceConfiguration($0)?.modifyEvents ?? false)
        && !$0.isVirtualDevice
    }
    let targets = [ConnectedDevice.zero] + filtered

    List(targets, selection: $selected) { device in
      ConnectedDeviceLabel(
        title: deviceLabelTitle(device),
        isKeyboard: device.isKeyboard,
        isPointingDevice: device.isPointingDevice,
        isGamePad: device.isGamePad,
        isConsumer: device.isConsumer,
        fallbackSystemImageName: device.index < 0 ? "circle.grid.2x2" : nil
      )
      .tag(device)
    }
    .listStyle(.sidebar)
    .onAppear {
      if let selectedDevice = selectedDevice {
        selected = selectedDevice
      } else {
        selected = ConnectedDevice.zero
      }
    }
    .onChange(of: selected) { newValue in
      if newValue.index < 0 {
        selectedDevice = nil
      } else {
        selectedDevice = newValue
      }
    }
  }

  private func deviceLabelTitle(_ device: ConnectedDevice) -> String {
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
