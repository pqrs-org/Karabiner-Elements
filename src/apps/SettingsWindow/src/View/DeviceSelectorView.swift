import SwiftUI

struct DeviceSelectorView: View {
  @AppLocalizationContext private var localized
  @Binding var selectedDevice: ConnectedDevice?
  @ObservedObject private var connectedDevices = ConnectedDevices.shared
  @ObservedObject private var settings = Settings.shared

  @State var selected = ConnectedDevice.zero
  @FocusState private var deviceListFocused: Bool

  var body: some View {
    // Build a combined array with `ConnectedDevice.zero` for localized("settings.devices.all")
    let filtered = connectedDevices.connectedDevices.filter {
      (settings.deviceConfiguration($0)?.modifyEvents ?? false)
        && !$0.isVirtualDevice
    }
    let targets = [ConnectedDevice.zero] + filtered

    List(targets, selection: $selected) { device in
      ConnectedDeviceLabel(
        title: device.index < 0
          ? localized("settings.devices.all")
          : connectedDeviceLabelTitle(
            productName: device.localizedProductName(localized),
            manufacturerName: device.localizedManufacturerName(localized),
            vendorId: device.vendorId,
            productId: device.productId,
            deviceAddress: device.deviceAddress
          ),
        isKeyboard: device.isKeyboard,
        isPointingDevice: device.isPointingDevice,
        isGamePad: device.isGamePad,
        isConsumer: device.isConsumer,
        fallbackSystemImageName: device.index < 0 ? "circle.grid.2x2" : nil
      )
      .tag(device)
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
}
