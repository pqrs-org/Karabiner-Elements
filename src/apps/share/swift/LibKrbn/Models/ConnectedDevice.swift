import Foundation

extension LibKrbn {
  @MainActor
  class ConnectedDevice: Identifiable, Equatable, Hashable {
    nonisolated let id = UUID()
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

    let libkrbnDeviceIdentifiers: UnsafeMutablePointer<libkrbn_device_identifiers>

    init(
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

      libkrbnDeviceIdentifiers = UnsafeMutablePointer<libkrbn_device_identifiers>.allocate(
        capacity: 1)
      libkrbnDeviceIdentifiers.pointee.vendor_id = vendorId
      libkrbnDeviceIdentifiers.pointee.product_id = productId

      deviceAddress.withCString {
        _ = strlcpy(
          &libkrbnDeviceIdentifiers.pointee.device_address,
          $0,
          MemoryLayout.size(ofValue: libkrbnDeviceIdentifiers.pointee.device_address)
        )
      }

      libkrbnDeviceIdentifiers.pointee.is_keyboard = isKeyboard
      libkrbnDeviceIdentifiers.pointee.is_pointing_device = isPointingDevice
      libkrbnDeviceIdentifiers.pointee.is_game_pad = isGamePad
      libkrbnDeviceIdentifiers.pointee.is_consumer = isConsumer
      libkrbnDeviceIdentifiers.pointee.is_virtual_device = isVirtualDevice
    }

    deinit {
      libkrbnDeviceIdentifiers.deallocate()
    }

    nonisolated public static func == (lhs: ConnectedDevice, rhs: ConnectedDevice) -> Bool {
      lhs.id == rhs.id
    }

    nonisolated func hash(into hasher: inout Hasher) {
      hasher.combine(id)
    }
  }
}
