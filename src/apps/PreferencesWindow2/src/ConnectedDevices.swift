import Combine
import Foundation
import SwiftUI

private func callback(_ initializedConnectedDevices: UnsafeMutableRawPointer?,
                      _ context: UnsafeMutableRawPointer?)
{
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

    init() {
        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_connected_devices_monitor(callback, obj)
    }

    @Published var connectedDevices: [ConnectedDevice] = []

    public func update(_ libkrbnConnectedDevices: UnsafeMutableRawPointer?) {
        var newConnectedDevices: [ConnectedDevice] = []

        let size = libkrbn_connected_devices_get_size(libkrbnConnectedDevices)
        for i in 0 ..< size {
            var manufacturer = String(cString: libkrbn_connected_devices_get_descriptions_manufacturer(libkrbnConnectedDevices, i))
                .replacingOccurrences(of: "[\r\n]", with: " ", options: .regularExpression)
            if manufacturer == "" {
                manufacturer = "No manufacturer name"
            }

            var product = String(cString: libkrbn_connected_devices_get_descriptions_product(libkrbnConnectedDevices, i))
                .replacingOccurrences(of: "[\r\n]", with: " ", options: .regularExpression)
            if product == "" {
                product = "No product name"
            }

            let connectedDevice = ConnectedDevice(
                index: i,
                manufacturer: manufacturer,
                product: product,
                isBuiltInKeyboard: libkrbn_connected_devices_get_is_built_in_keyboard(libkrbnConnectedDevices, i),
                isBuiltInTrackpad: libkrbn_connected_devices_get_is_built_in_trackpad(libkrbnConnectedDevices, i),
                isBuiltInTouchBar: libkrbn_connected_devices_get_is_built_in_touch_bar(libkrbnConnectedDevices, i)
            )

            newConnectedDevices.append(connectedDevice)
        }

        connectedDevices = newConnectedDevices
    }
}
