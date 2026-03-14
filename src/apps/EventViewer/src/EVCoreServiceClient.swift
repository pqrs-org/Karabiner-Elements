import AsyncAlgorithms
import Combine
import Foundation

private func statusChangedCallback() {
  Task { @MainActor in
    EVCoreServiceClient.shared.temporarilyIgnoreAllDevices = false
  }
}

private func manipulatorEnvironmentReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let text = String(cString: jsonString)

  Task { @MainActor in
    EVCoreServiceClient.shared.manipulatorEnvironmentStream.setText(text)
  }
}

private func connectedDevicesReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let text = String(cString: jsonString)

  Task { @MainActor in
    EVCoreServiceClient.shared.connectedDevicesStream.setText(text)
    EVCoreServiceClient.shared.updateConnectedDevices(text)
  }
}

@MainActor
final class EVCoreServiceClient: ObservableObject {
  static let shared = EVCoreServiceClient()

  private let manipulatorEnvironmentTimer: AsyncTimerSequence<ContinuousClock>
  private var manipulatorEnvironmentTimerTask: Task<Void, Never>?
  private var manipulatorEnvironmentStartCount = 0
  let manipulatorEnvironmentStream = RealtimeTextStream()

  private let connectedDevicesTimer: AsyncTimerSequence<ContinuousClock>
  private var connectedDevicesTimerTask: Task<Void, Never>?
  private var connectedDevicesStartCount = 0
  let connectedDevicesStream = RealtimeTextStream()
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

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init,
  // there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name event_viewer_core_service_client -> ev_cs_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/ev_cs_clnt/186745e8160a7b98.sock`

    libkrbn_enable_core_service_client("ev_cs_clnt")

    libkrbn_register_core_service_client_status_changed_callback(statusChangedCallback)

    libkrbn_register_core_service_client_manipulator_environment_received_callback(
      manipulatorEnvironmentReceivedCallback)

    libkrbn_register_core_service_client_connected_devices_received_callback(
      connectedDevicesReceivedCallback)

    libkrbn_core_service_client_async_start()
  }

  public func startManipulatorEnvironment() {
    manipulatorEnvironmentStartCount += 1
    if manipulatorEnvironmentStartCount == 1 {
      manipulatorEnvironmentTimerTask = Task { @MainActor in
        libkrbn_core_service_client_async_get_manipulator_environment()

        for await _ in manipulatorEnvironmentTimer {
          libkrbn_core_service_client_async_get_manipulator_environment()
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
        libkrbn_core_service_client_async_get_connected_devices()

        for await _ in connectedDevicesTimer {
          libkrbn_core_service_client_async_get_connected_devices()
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

  public func updateConnectedDevices(_ text: String) {
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
      libkrbn_core_service_client_async_temporarily_ignore_all_devices(temporarilyIgnoreAllDevices)
    }
  }
}
