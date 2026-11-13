import Foundation

struct ConnectedDeviceCapabilities: Decodable {
  struct LogitechHidpp: Decodable {
    let divertableControlIds: [UInt16]
  }

  let logitechHidpp: LogitechHidpp?
}

struct DeviceIdentifierValueForUI: Decodable {
  let vendorId: UInt64?
  let productId: UInt64?
  let locationId: UInt64?
  let deviceAddress: String?
  let serialNumber: String?
  let transport: String?
}

@MainActor
class ConnectedDevice: Identifiable, Equatable, Hashable {
  nonisolated let id: String
  let index: Int
  let manufacturerName: String
  let productName: String
  let locationId: UInt64
  let transport: String
  let serialNumber: String
  let vendorId: UInt64
  let productId: UInt64
  let deviceAddress: String
  let deviceIdentifierValuesForUI: [DeviceIdentifierValueForUI]
  let isKeyboard: Bool
  let isPointingDevice: Bool
  let isGamePad: Bool
  let isConsumer: Bool
  let isVirtualDevice: Bool
  let isBuiltInKeyboard: Bool
  let isAppleDevice: Bool
  let capabilities: ConnectedDeviceCapabilities

  func localizedProductName(_ localized: AppLocalizationContext.Lookup) -> String {
    guard productName.isEmpty else { return productName }
    return localized(
      transport == "FIFO"
        ? "settings.devices.internal_keyboard_trackpad" : "settings.devices.no_product_name")
  }

  func localizedManufacturerName(_ localized: AppLocalizationContext.Lookup) -> String {
    manufacturerName.isEmpty ? localized("settings.devices.no_manufacturer_name") : manufacturerName
  }

  static var zero: ConnectedDevice {
    return ConnectedDevice(
      id: "",
      index: -1,
      manufacturerName: "",
      productName: "",
      locationId: 0,
      transport: "",
      serialNumber: "",
      vendorId: 0,
      productId: 0,
      deviceAddress: "",
      deviceIdentifierValuesForUI: [],
      isKeyboard: false,
      isPointingDevice: false,
      isGamePad: false,
      isConsumer: false,
      isVirtualDevice: false,
      isBuiltInKeyboard: false,
      isAppleDevice: false,
      capabilities: ConnectedDeviceCapabilities(logitechHidpp: nil)
    )
  }

  init(
    id: String,
    index: Int,
    manufacturerName: String,
    productName: String,
    locationId: UInt64,
    transport: String,
    serialNumber: String,
    vendorId: UInt64,
    productId: UInt64,
    deviceAddress: String,
    deviceIdentifierValuesForUI: [DeviceIdentifierValueForUI],
    isKeyboard: Bool,
    isPointingDevice: Bool,
    isGamePad: Bool,
    isConsumer: Bool,
    isVirtualDevice: Bool,
    isBuiltInKeyboard: Bool,
    isAppleDevice: Bool,
    capabilities: ConnectedDeviceCapabilities
  ) {
    self.id = id
    self.index = index
    self.manufacturerName = manufacturerName
    self.productName = productName
    self.locationId = locationId
    self.transport = transport
    self.serialNumber = serialNumber
    self.vendorId = vendorId
    self.productId = productId
    self.deviceAddress = deviceAddress
    self.deviceIdentifierValuesForUI = deviceIdentifierValuesForUI
    self.isKeyboard = isKeyboard
    self.isPointingDevice = isPointingDevice
    self.isGamePad = isGamePad
    self.isConsumer = isConsumer
    self.isVirtualDevice = isVirtualDevice
    self.isBuiltInKeyboard = isBuiltInKeyboard
    self.isAppleDevice = isAppleDevice
    self.capabilities = capabilities
  }

  nonisolated public static func == (lhs: ConnectedDevice, rhs: ConnectedDevice) -> Bool {
    lhs.id == rhs.id
  }

  nonisolated func hash(into hasher: inout Hasher) {
    hasher.combine(id)
  }

  func withDeviceIdentifiersJSONCString<Result>(
    _ body: (UnsafePointer<CChar>?) -> Result
  ) -> Result {
    id.withCString(body)
  }
}

extension Optional where Wrapped: ConnectedDevice {
  @MainActor
  func withDeviceIdentifiersJSONCString<Result>(
    _ body: (UnsafePointer<CChar>?) -> Result
  ) -> Result {
    switch self {
    case .some(let connectedDevice):
      return connectedDevice.withDeviceIdentifiersJSONCString(body)
    case .none:
      return body(nil)
    }
  }
}
