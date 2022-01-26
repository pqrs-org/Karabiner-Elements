import Combine
import Foundation
import SwiftUI

private func callback(
  _ initializedConnectedDevices: UnsafeMutableRawPointer?,
  _ context: UnsafeMutableRawPointer?
) {
  if initializedConnectedDevices == nil { return }
  if context == nil { return }

  let obj: ConnectedDevices! = unsafeBitCast(context, to: ConnectedDevices.self)

  DispatchQueue.main.async { [weak obj] in
    guard let obj = obj else { return }

    obj.update(initializedConnectedDevices)
  }
}

final class ConnectedDevices: ObservableObject {
  static let shared = ConnectedDevices()
  static let didConnectedDevicesUpdate = Notification.Name("didConnectedDevicesUpdate")

  init() {
    let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
    libkrbn_enable_connected_devices_monitor(callback, obj)
  }

  @Published var connectedDevices: [ConnectedDevice] = []

  public func update(_ libkrbnConnectedDevices: UnsafeMutableRawPointer?) {
    var newConnectedDevices: [ConnectedDevice] = []

    let size = libkrbn_connected_devices_get_size(libkrbnConnectedDevices)
    for i in 0..<size {
      var manufacturerName = String(
        cString: libkrbn_connected_devices_get_descriptions_manufacturer(libkrbnConnectedDevices, i)
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
        productName = "No product name"
      }

      let connectedDevice = ConnectedDevice(
        index: i,
        manufacturerName: manufacturerName,
        productName: productName,
        vendorId: libkrbn_connected_devices_get_vendor_id(libkrbnConnectedDevices, i),
        productId: libkrbn_connected_devices_get_product_id(libkrbnConnectedDevices, i),
        isKeyboard: libkrbn_connected_devices_get_is_keyboard(libkrbnConnectedDevices, i),
        isPointingDevice: libkrbn_connected_devices_get_is_pointing_device(
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
