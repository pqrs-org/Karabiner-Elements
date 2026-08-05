import AsyncAlgorithms
import Combine
import Foundation

func coreServiceConnectionChangedCallback(_: Bool) {
  Task { @MainActor in
    EVCoreServiceDaemonClient.shared.temporarilyIgnoreAllDevices = false
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
final class EVCoreServiceDaemonClient: ObservableObject {
  static let shared = EVCoreServiceDaemonClient()

  private let manipulatorEnvironmentTimer: AsyncTimerSequence<ContinuousClock>
  private var manipulatorEnvironmentTimerTask: Task<Void, Never>?
  private var manipulatorEnvironmentStartCount = 0
  @Published private(set) var manipulatorEnvironmentText = ""

  private let connectedDevicesTimer: AsyncTimerSequence<ContinuousClock>
  private var connectedDevicesTimerTask: Task<Void, Never>?
  private var connectedDevicesStartCount = 0
  @Published private(set) var connectedDevicesText = ""
  @Published private(set) var productsByDeviceId: [UInt64: String] = [:]

  init() {
    manipulatorEnvironmentTimer = AsyncTimerSequence(
      interval: .milliseconds(500),
      clock: .continuous
    )

    connectedDevicesTimer = AsyncTimerSequence(
      interval: .milliseconds(1000),
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

  public func startConnectedDevices() {
    connectedDevicesStartCount += 1
    if connectedDevicesStartCount == 1 {
      connectedDevicesTimerTask = Task { @MainActor in
        krbn_core_service_async_get_connected_devices()

        for await _ in connectedDevicesTimer {
          krbn_core_service_async_get_connected_devices()
        }
      }
    }
  }

  public func stopConnectedDevices() {
    connectedDevicesStartCount -= 1
    if connectedDevicesStartCount <= 0 {
      connectedDevicesStartCount = 0
      connectedDevicesTimerTask?.cancel()
      connectedDevicesTimerTask = nil
    }
  }

  public func productName(deviceId: UInt64) -> String {
    productsByDeviceId[deviceId] ?? "an unnamed device"
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
      return
    }

    var result: [UInt64: String] = [:]

    for item in array {
      guard
        let idNumber = item["device_id"] as? NSNumber,
        let product = item["product"] as? String
      else {
        continue
      }

      if !product.isEmpty {
        result[idNumber.uint64Value] = product
      }
    }

    productsByDeviceId = result
  }

  @Published var temporarilyIgnoreAllDevices: Bool = false {
    didSet {
      krbn_core_service_async_temporarily_ignore_all_devices(
        temporarilyIgnoreAllDevices)
    }
  }
}
