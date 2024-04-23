import Foundation

extension LibKrbn {
  class ConnectedDevice: Identifiable, Equatable {
    let id = UUID()
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
    let isBuiltInKeyboard: Bool
    let isBuiltInTrackpad: Bool
    let isBuiltInTouchBar: Bool
    let isAppleDevice: Bool
    let isKarabinerVirtualHidDevice: Bool

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
      isBuiltInKeyboard: Bool,
      isBuiltInTrackpad: Bool,
      isBuiltInTouchBar: Bool,
      isAppleDevice: Bool,
      isKarabinerVirtualHidDevice: Bool
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
      self.isBuiltInKeyboard = isBuiltInKeyboard
      self.isBuiltInTrackpad = isBuiltInTrackpad
      self.isBuiltInTouchBar = isBuiltInTouchBar
      self.isAppleDevice = isAppleDevice
      self.isKarabinerVirtualHidDevice = isKarabinerVirtualHidDevice

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
    }

    deinit {
      libkrbnDeviceIdentifiers.deallocate()
    }

    public static func == (lhs: ConnectedDevice, rhs: ConnectedDevice) -> Bool {
      lhs.id == rhs.id
    }
  }
}
