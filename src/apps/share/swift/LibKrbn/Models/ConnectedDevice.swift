import Foundation

extension LibKrbn {
  @MainActor
  class ConnectedDevice: Identifiable, Equatable, Hashable {
    nonisolated let id: String
    let index: Int
    let manufacturerName: String
    let productName: String
    let transport: String
    let vendorId: UInt64
    let productId: UInt64
    let deviceAddress: String
    let isKeyboard: Bool
    let isPointingDevice: Bool
    let isGamePad: Bool
    let isConsumer: Bool
    let isVirtualDevice: Bool
    let isBuiltInKeyboard: Bool
    let isAppleDevice: Bool

    let libkrbnDeviceIdentifiers: libkrbn_device_identifiers

    static var zero: ConnectedDevice {
      return ConnectedDevice(
        id: "",
        index: -1,
        manufacturerName: "",
        productName: "",
        transport: "",
        vendorId: 0,
        productId: 0,
        deviceAddress: "",
        isKeyboard: false,
        isPointingDevice: false,
        isGamePad: false,
        isConsumer: false,
        isVirtualDevice: false,
        isBuiltInKeyboard: false,
        isAppleDevice: false
      )
    }

    init(
      id: String,
      index: Int,
      manufacturerName: String,
      productName: String,
      transport: String,
      vendorId: UInt64,
      productId: UInt64,
      deviceAddress: String,
      isKeyboard: Bool,
      isPointingDevice: Bool,
      isGamePad: Bool,
      isConsumer: Bool,
      isVirtualDevice: Bool,
      isBuiltInKeyboard: Bool,
      isAppleDevice: Bool
    ) {
      self.id = id
      self.index = index
      self.manufacturerName = manufacturerName
      self.productName = productName
      self.transport = transport
      self.vendorId = vendorId
      self.productId = productId
      self.deviceAddress = deviceAddress
      self.isKeyboard = isKeyboard
      self.isPointingDevice = isPointingDevice
      self.isGamePad = isGamePad
      self.isConsumer = isConsumer
      self.isVirtualDevice = isVirtualDevice
      self.isBuiltInKeyboard = isBuiltInKeyboard
      self.isAppleDevice = isAppleDevice

      libkrbnDeviceIdentifiers = libkrbn_device_identifiers(
        vendorId: vendorId,
        productId: productId,
        deviceAddress: deviceAddress,
        isKeyboard: isKeyboard,
        isPointingDevice: isPointingDevice,
        isGamePad: isGamePad,
        isConsumer: isConsumer,
        isVirtualDevice: isVirtualDevice,
      )
    }

    nonisolated public static func == (lhs: ConnectedDevice, rhs: ConnectedDevice) -> Bool {
      lhs.id == rhs.id
    }

    nonisolated func hash(into hasher: inout Hasher) {
      hasher.combine(id)
    }

    func withDeviceIdentifiersCPointer<Result>(
      _ body: (UnsafePointer<libkrbn_device_identifiers>?) -> Result
    ) -> Result {
      return withUnsafePointer(to: libkrbnDeviceIdentifiers) { body($0) }
    }
  }
}

extension Optional where Wrapped: LibKrbn.ConnectedDevice {
  @MainActor
  func withDeviceIdentifiersCPointer<Result>(
    _ body: (UnsafePointer<libkrbn_device_identifiers>?) -> Result
  ) -> Result {
    switch self {
    case .some(let connectedDevice):
      return connectedDevice.withDeviceIdentifiersCPointer(body)
    case .none:
      return body(nil)
    }
  }
}
