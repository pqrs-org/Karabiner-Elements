import Combine
import Foundation
import SwiftUI

private func callback(
  _ initializedConnectedDevices: UnsafeMutableRawPointer?,
  _ context: UnsafeMutableRawPointer?
) {
  if initializedConnectedDevices == nil { return }

  Task { @MainActor in
    LibKrbn.ConnectedDevices.shared.update(initializedConnectedDevices)
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

    public func update(_ libkrbnConnectedDevices: UnsafeMutableRawPointer?) {
      var newConnectedDevices: [ConnectedDevice] = []

      let size = libkrbn_connected_devices_get_size(libkrbnConnectedDevices)
      for i in 0..<size {
        let transport = String(
          cString: libkrbn_connected_devices_get_descriptions_transport(
            libkrbnConnectedDevices, i)
        )

        var manufacturerName = String(
          cString: libkrbn_connected_devices_get_descriptions_manufacturer(
            libkrbnConnectedDevices, i)
        )
        .replacingOccurrences(of: "[\r\n]", with: " ", options: .regularExpression)
        if manufacturerName == "" {
          manufacturerName = "No manufacturer name"
        }

        var productName = String(
          cString: libkrbn_connected_devices_get_descriptions_product(libkrbnConnectedDevices, i)
        )
        .replacingOccurrences(of: "[\r\n]", with: " ", options: .regularExpression)
        if productName == "" {
          if transport == "FIFO" {
            productName = "Apple Internal Keyboard / Trackpad"
          } else {
            productName = "No product name"
          }
        }

        let connectedDevice = ConnectedDevice(
          index: i,
          manufacturerName: manufacturerName,
          productName: productName,
          transport: transport,
          vendorId: libkrbn_connected_devices_get_vendor_id(libkrbnConnectedDevices, i),
          productId: libkrbn_connected_devices_get_product_id(libkrbnConnectedDevices, i),
          deviceAddress: libkrbn_connected_devices_get_device_address(libkrbnConnectedDevices, i),
          isKeyboard: libkrbn_connected_devices_get_is_keyboard(libkrbnConnectedDevices, i),
          isPointingDevice: libkrbn_connected_devices_get_is_pointing_device(
            libkrbnConnectedDevices, i),
          isGamePad: libkrbn_connected_devices_get_is_game_pad(
            libkrbnConnectedDevices, i),
          isBuiltInKeyboard: libkrbn_connected_devices_get_is_built_in_keyboard(
            libkrbnConnectedDevices, i),
          isBuiltInTrackpad: libkrbn_connected_devices_get_is_built_in_trackpad(
            libkrbnConnectedDevices, i),
          isBuiltInTouchBar: libkrbn_connected_devices_get_is_built_in_touch_bar(
            libkrbnConnectedDevices, i),
          isAppleDevice: libkrbn_connected_devices_is_apple(libkrbnConnectedDevices, i)
        )

        newConnectedDevices.append(connectedDevice)
      }

      connectedDevices = newConnectedDevices

      NotificationCenter.default.post(
        name: ConnectedDevices.didConnectedDevicesUpdate,
        object: nil
      )
    }
  }
}
