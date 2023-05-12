import Foundation

extension LibKrbn {
  class ConnectedDevice: Identifiable {
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
    let isBuiltInKeyboard: Bool
    let isBuiltInTrackpad: Bool
    let isBuiltInTouchBar: Bool
    let isAppleDevice: Bool

    let libkrbnDeviceIdentifiers: UnsafeMutablePointer<libkrbn_device_identifiers>

    init(
      index: Int,
      manufacturerName: String,
      productName: String,
      transport: String,
      vendorId: UInt64,
      productId: UInt64,
      deviceAddress: UnsafePointer<CChar>,
      isKeyboard: Bool,
      isPointingDevice: Bool,
      isBuiltInKeyboard: Bool,
      isBuiltInTrackpad: Bool,
      isBuiltInTouchBar: Bool,
      isAppleDevice: Bool
    ) {
      self.index = index
      self.manufacturerName = manufacturerName
      self.productName = productName
      self.transport = transport
      self.vendorId = vendorId
      self.productId = productId
      self.deviceAddress = String(cString: deviceAddress)
      self.isKeyboard = isKeyboard
      self.isPointingDevice = isPointingDevice
      self.isBuiltInKeyboard = isBuiltInKeyboard
      self.isBuiltInTrackpad = isBuiltInTrackpad
      self.isBuiltInTouchBar = isBuiltInTouchBar
      self.isAppleDevice = isAppleDevice

      libkrbnDeviceIdentifiers = UnsafeMutablePointer<libkrbn_device_identifiers>.allocate(
        capacity: 1)
      libkrbnDeviceIdentifiers.pointee.vendor_id = vendorId
      libkrbnDeviceIdentifiers.pointee.product_id = productId
      libkrbnDeviceIdentifiers.pointee.device_address = deviceAddress
      libkrbnDeviceIdentifiers.pointee.is_keyboard = isKeyboard
      libkrbnDeviceIdentifiers.pointee.is_pointing_device = isPointingDevice
    }

    deinit {
      libkrbnDeviceIdentifiers.deallocate()
    }
  }
}
