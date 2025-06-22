import Darwin

extension libkrbn_device_identifiers {
  public init(
    vendorId: UInt64,
    productId: UInt64,
    deviceAddress: String,
    isKeyboard: Bool,
    isPointingDevice: Bool,
    isGamePad: Bool,
    isConsumer: Bool,
    isVirtualDevice: Bool
  ) {
    self = libkrbn_device_identifiers()

    self.vendor_id = vendorId
    self.product_id = productId

    deviceAddress.withCString { srcPtr in
      let size = MemoryLayout.size(ofValue: self.device_address)
      withUnsafeMutableBytes(of: &self.device_address) { destBuf in
        let destPtr = destBuf.baseAddress!.assumingMemoryBound(to: CChar.self)
        _ = strlcpy(destPtr, srcPtr, size)
      }
    }

    self.is_keyboard = isKeyboard
    self.is_pointing_device = isPointingDevice
    self.is_game_pad = isGamePad
    self.is_consumer = isConsumer
    self.is_virtual_device = isVirtualDevice
  }
}
