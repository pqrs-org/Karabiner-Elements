import Combine
import Foundation
import SwiftUI

private func callback(
  _ initializedConnectedDevices: UnsafeMutableRawPointer?,
  _ context: UnsafeMutableRawPointer?
) {
  if initializedConnectedDevices == nil { return }

  //
  // Make connectedDevices
  //

  var newConnectedDevices: [LibKrbn.ConnectedDevice] = []

  let size = libkrbn_connected_devices_get_size(initializedConnectedDevices)
  for i in 0..<size {
    let transport = String(
      cString: libkrbn_connected_devices_get_descriptions_transport(
        initializedConnectedDevices, i)
    )

    var manufacturerName = String(
      cString: libkrbn_connected_devices_get_descriptions_manufacturer(
        initializedConnectedDevices, i)
    )
    .replacingOccurrences(of: "[\r\n]", with: " ", options: .regularExpression)
    if manufacturerName == "" {
      manufacturerName = "No manufacturer name"
    }

    var productName = String(
      cString: libkrbn_connected_devices_get_descriptions_product(initializedConnectedDevices, i)
    )
    .replacingOccurrences(of: "[\r\n]", with: " ", options: .regularExpression)
    if productName == "" {
      if transport == "FIFO" {
        productName = "Apple Internal Keyboard / Trackpad"
      } else {
        productName = "No product name"
      }
    }

    let connectedDevice = LibKrbn.ConnectedDevice(
      index: i,
      manufacturerName: manufacturerName,
      productName: productName,
      transport: transport,
      vendorId: libkrbn_connected_devices_get_vendor_id(initializedConnectedDevices, i),
      productId: libkrbn_connected_devices_get_product_id(initializedConnectedDevices, i),
      deviceAddress: libkrbn_connected_devices_get_device_address(initializedConnectedDevices, i),
      isKeyboard: libkrbn_connected_devices_get_is_keyboard(initializedConnectedDevices, i),
      isPointingDevice: libkrbn_connected_devices_get_is_pointing_device(
        initializedConnectedDevices, i),
      isGamePad: libkrbn_connected_devices_get_is_game_pad(
        initializedConnectedDevices, i),
      isBuiltInKeyboard: libkrbn_connected_devices_get_is_built_in_keyboard(
        initializedConnectedDevices, i),
      isBuiltInTrackpad: libkrbn_connected_devices_get_is_built_in_trackpad(
        initializedConnectedDevices, i),
      isBuiltInTouchBar: libkrbn_connected_devices_get_is_built_in_touch_bar(
        initializedConnectedDevices, i),
      isAppleDevice: libkrbn_connected_devices_is_apple(initializedConnectedDevices, i)
    )

    newConnectedDevices.append(connectedDevice)
  }

  //
  // Update
  //

  Task { @MainActor [newConnectedDevices] in
    LibKrbn.ConnectedDevices.shared.update(newConnectedDevices)
  }
}

extension LibKrbn {
  final class ConnectedDevices: ObservableObject {
    static let shared = ConnectedDevices()
    static let didConnectedDevicesUpdate = Notification.Name("didConnectedDevicesUpdate")

    @Published var connectedDevices: [ConnectedDevice] = []

    private init() {
      libkrbn_enable_connected_devices_monitor(callback, nil)
    }

    public func update(_ newConnectedDevices: [ConnectedDevice]) {
      connectedDevices = newConnectedDevices

      NotificationCenter.default.post(
        name: ConnectedDevices.didConnectedDevicesUpdate,
        object: nil
      )
    }
  }
}
