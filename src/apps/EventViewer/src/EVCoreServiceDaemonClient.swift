import AsyncAlgorithms
import Combine
import Foundation

func coreServiceConnectionChangedCallback(_ connected: Bool) {
  Task { @MainActor in
    if !connected {
      InputReportHistory.shared.stopCapture()
      EventHistory.shared.stopRawInputEventsCapture()
    }
    TemporarilyIgnoredDeviceManager.shared.coreServiceConnectionChanged(connected)
  }
}

func manipulatorEnvironmentReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let text = String(cString: jsonString)

  Task { @MainActor in
    EVCoreServiceDaemonClient.shared.updateManipulatorEnvironment(text)
  }
}

func connectedDevicesReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let text = String(cString: jsonString)

  Task { @MainActor in
    EVCoreServiceDaemonClient.shared.updateConnectedDevices(text)
  }
}

@MainActor
final class TemporarilyIgnoredDeviceManager {
  enum Owner: Equatable {
    case inputEvents
    case rawInputEvents
    case rawInputRecords
  }

  static let shared = TemporarilyIgnoredDeviceManager()

  private var owner: Owner?
  private var deviceId: UInt64?

  func activate(owner: Owner, deviceId: UInt64?) {
    self.owner = owner
    self.deviceId = deviceId
    apply()
  }

  func update(owner: Owner, deviceId: UInt64?) {
    guard self.owner == owner else {
      return
    }

    self.deviceId = deviceId
    apply()
  }

  func deactivate(owner: Owner) {
    guard self.owner == owner else {
      return
    }

    self.owner = nil
    deviceId = nil
    apply()
  }

  func coreServiceConnectionChanged(_ connected: Bool) {
    if connected {
      if owner != nil {
        // Close any devices opened while CoreService was unavailable before
        // allowing CoreService to seize devices again.
        krbn_set_hid_capture_target(false, 0)
      }
      apply()
    }
  }

  private func apply() {
    krbn_set_hid_capture_target(owner != nil, deviceId ?? 0)
  }
}

@MainActor
final class EVCoreServiceDaemonClient: ObservableObject {
  struct ConnectedDevice: Identifiable, Hashable {
    let id: UInt64
    let manufacturer: String
    let product: String
    let vendorId: UInt64
    let productId: UInt64
    let deviceAddress: String
    let isKeyboard: Bool
    let isPointingDevice: Bool
    let isGamePad: Bool
    let isConsumer: Bool

    var name: String {
      product.isEmpty ? "Unnamed device" : product
    }

    var details: String {
      var kinds: [String] = []
      if isKeyboard { kinds.append("keyboard") }
      if isPointingDevice { kinds.append("pointing device") }
      if isGamePad { kinds.append("game pad") }
      if isConsumer { kinds.append("consumer device") }

      var result = kinds.joined(separator: ", ")
      if vendorId != 0 || productId != 0 {
        let identifiers = String(
          format: "vendor_id: 0x%04x, product_id: 0x%04x",
          vendorId,
          productId)
        result = result.isEmpty ? identifiers : "\(result), \(identifiers)"
      }
      return result
    }
  }

  static let shared = EVCoreServiceDaemonClient()

  private let manipulatorEnvironmentTimer: AsyncTimerSequence<ContinuousClock>
  private var manipulatorEnvironmentTimerTask: Task<Void, Never>?
  private var manipulatorEnvironmentStartCount = 0
  @Published private(set) var manipulatorEnvironmentText = ""

  @Published private(set) var connectedDevicesText = ""
  @Published private(set) var productsByDeviceId: [UInt64: String] = [:]
  @Published private(set) var connectedDevices: [ConnectedDevice] = []
  @Published private(set) var openHIDDeviceIds: Set<UInt64> = []

  init() {
    manipulatorEnvironmentTimer = AsyncTimerSequence(
      interval: .milliseconds(500),
      clock: .continuous
    )
  }

  public func startManipulatorEnvironment() {
    manipulatorEnvironmentStartCount += 1
    if manipulatorEnvironmentStartCount == 1 {
      manipulatorEnvironmentTimerTask = Task { @MainActor in
        krbn_core_service_async_get_manipulator_environment()

        for await _ in manipulatorEnvironmentTimer {
          krbn_core_service_async_get_manipulator_environment()
        }
      }
    }
  }

  public func stopManipulatorEnvironment() {
    manipulatorEnvironmentStartCount -= 1
    if manipulatorEnvironmentStartCount <= 0 {
      manipulatorEnvironmentStartCount = 0
      manipulatorEnvironmentTimerTask?.cancel()
      manipulatorEnvironmentTimerTask = nil
    }
  }

  public func productName(deviceId: UInt64) -> String {
    productsByDeviceId[deviceId] ?? "an unnamed device"
  }

  public func setHIDDevice(_ deviceId: UInt64, opened: Bool) {
    if opened {
      openHIDDeviceIds.insert(deviceId)
    } else {
      openHIDDeviceIds.remove(deviceId)
    }
  }

  public func hidDeviceIsOpen(_ deviceId: UInt64) -> Bool {
    openHIDDeviceIds.contains(deviceId)
  }

  public func updateManipulatorEnvironment(_ text: String) {
    manipulatorEnvironmentText = text
  }

  public func updateConnectedDevices(_ text: String) {
    if connectedDevicesText == text {
      return
    }
    connectedDevicesText = text

    guard let data = text.data(using: .utf8),
      let array = try? JSONSerialization.jsonObject(with: data) as? [[String: Any]]
    else {
      productsByDeviceId = [:]
      connectedDevices = []
      return
    }

    var result: [UInt64: String] = [:]
    var devices: [ConnectedDevice] = []

    for item in array {
      guard
        let idNumber = item["device_id"] as? NSNumber
      else {
        continue
      }

      let identifiers = item["device_identifiers"] as? [String: Any] ?? [:]
      let product = item["product"] as? String ?? ""

      // Keep virtual devices in the name lookup because Main receives events from
      // Karabiner VirtualHIDDevices when All is selected.
      if !product.isEmpty {
        result[idNumber.uint64Value] = product
      }

      // Virtual devices must not be selectable as raw capture targets.
      if identifiers["is_virtual_device"] as? Bool == true {
        continue
      }

      devices.append(
        ConnectedDevice(
          id: idNumber.uint64Value,
          manufacturer: item["manufacturer"] as? String ?? "",
          product: product,
          vendorId: (identifiers["vendor_id"] as? NSNumber)?.uint64Value ?? 0,
          productId: (identifiers["product_id"] as? NSNumber)?.uint64Value ?? 0,
          deviceAddress: identifiers["device_address"] as? String ?? "",
          isKeyboard: identifiers["is_keyboard"] as? Bool ?? false,
          isPointingDevice: identifiers["is_pointing_device"] as? Bool ?? false,
          isGamePad: identifiers["is_game_pad"] as? Bool ?? false,
          isConsumer: identifiers["is_consumer"] as? Bool ?? false
        ))
    }

    productsByDeviceId = result
    connectedDevices = devices

    if let selectedDeviceId = InputReportHistory.shared.selectedDeviceId,
      !devices.contains(where: { $0.id == selectedDeviceId })
    {
      InputReportHistory.shared.deviceDisconnected()
    }

    if let selectedDeviceId = EventHistory.shared.rawInputEventsSelectedDeviceId,
      !devices.contains(where: { $0.id == selectedDeviceId })
    {
      EventHistory.shared.rawInputEventsSelectedDeviceDisconnected()
    }
  }
}
