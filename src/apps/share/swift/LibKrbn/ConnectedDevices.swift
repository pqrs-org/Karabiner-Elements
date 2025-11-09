import AsyncAlgorithms
import Combine
import Foundation
import SwiftUI

private func connectedDevicesReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let s = String(cString: jsonString)

  Task { @MainActor in
    LibKrbn.ConnectedDevices.shared.update(s)
  }
}

// device_properties.hpp
private struct ConnectedDevicePayload: Decodable {
  // device_identifiers.hpp
  struct DeviceIdentifiers: Codable {
    let vendorId: UInt64?
    let productId: UInt64?
    let isKeyboard: Bool?
    let isPointingDevice: Bool?
    let isGamePad: Bool?
    let isConsumer: Bool?
    let isVirtualDevice: Bool?
    let deviceAddress: String?
  }

  let deviceId: UInt64
  let deviceIdentifiers: DeviceIdentifiers
  let locationId: UInt64?
  let manufacturer: String?
  let product: String?
  let serialNumber: String?
  let transport: String?
  let isBuiltInKeyboard: Bool?
  let isBuiltInPointingDevice: Bool?
  let isBuiltInTouchBar: Bool?
  let isApple: Bool?
}

extension LibKrbn {
  @MainActor
  final class ConnectedDevices: ObservableObject {
    static let shared = ConnectedDevices()

    private let timer: AsyncTimerSequence<ContinuousClock>
    private var timerTask: Task<Void, Never>?

    var connectedDevicesJSONString = ""
    @Published var connectedDevices: [ConnectedDevice] = []

    init() {
      timer = AsyncTimerSequence(
        interval: .milliseconds(1000),
        clock: .continuous
      )
    }

    // We register the callback in the `watch` method rather than in `init`.
    // If libkrbn_register_*_callback is called within init,
    // there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

    public func watch() {
      if timerTask != nil {
        return
      }

      libkrbn_register_core_service_client_connected_devices_received_callback(
        connectedDevicesReceivedCallback)

      timerTask = Task { @MainActor in
        libkrbn_core_service_client_async_get_connected_devices()

        for await _ in timer {
          libkrbn_core_service_client_async_get_connected_devices()
        }
      }
    }

    public func update(_ jsonString: String) {
      if connectedDevicesJSONString == jsonString {
        return
      }
      connectedDevicesJSONString = jsonString

      guard let data = jsonString.data(using: .utf8) else { return }
      let decoder = JSONDecoder()
      decoder.keyDecodingStrategy = .convertFromSnakeCase
      let encoder = JSONEncoder()
      encoder.keyEncodingStrategy = .convertToSnakeCase

      do {
        let payloads = try decoder.decode([ConnectedDevicePayload].self, from: data)

        connectedDevices = try payloads.enumerated().map { index, payload in
          let deviceIdentifiersJSONData = try encoder.encode(payload.deviceIdentifiers)
          let deviceIdentifiersJSONString =
            String(data: deviceIdentifiersJSONData, encoding: .utf8) ?? ""

          var manufacturer = (payload.manufacturer ?? "")
            .replacingOccurrences(of: "[\r\n]", with: " ", options: .regularExpression)
          if manufacturer.isEmpty {
            manufacturer = "No manufacturer name"
          }

          var product = (payload.product ?? "")
            .replacingOccurrences(of: "[\r\n]", with: " ", options: .regularExpression)
          if product.isEmpty {
            if payload.transport == "FIFO" {
              product = "Apple Internal Keyboard / Trackpad"
            } else {
              product = "No product name"
            }
          }

          return LibKrbn.ConnectedDevice(
            id: deviceIdentifiersJSONString,
            index: index,
            manufacturerName: manufacturer,
            productName: product,
            transport: payload.transport ?? "",
            vendorId: payload.deviceIdentifiers.vendorId ?? 0,
            productId: payload.deviceIdentifiers.productId ?? 0,
            deviceAddress: payload.deviceIdentifiers.deviceAddress ?? "",
            isKeyboard: payload.deviceIdentifiers.isKeyboard ?? false,
            isPointingDevice: payload.deviceIdentifiers.isPointingDevice ?? false,
            isGamePad: payload.deviceIdentifiers.isGamePad ?? false,
            isConsumer: payload.deviceIdentifiers.isConsumer ?? false,
            isVirtualDevice: payload.deviceIdentifiers.isVirtualDevice ?? false,
            isBuiltInKeyboard: payload.isBuiltInKeyboard ?? false,
            isAppleDevice: payload.isApple ?? false
          )
        }
      } catch {
        print(error.localizedDescription)
        return
      }
    }
  }
}
