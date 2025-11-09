import AsyncAlgorithms
import Combine

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
  }
}

@MainActor
final class EVCoreServiceClient: ObservableObject {
  static let shared = EVCoreServiceClient()

  private let manipulatorEnvironmentTimer: AsyncTimerSequence<ContinuousClock>
  private var manipulatorEnvironmentTimerTask: Task<Void, Never>?
  let manipulatorEnvironmentStream = RealtimeTextStream()

  private let connectedDevicesTimer: AsyncTimerSequence<ContinuousClock>
  private var connectedDevicesTimerTask: Task<Void, Never>?
  let connectedDevicesStream = RealtimeTextStream()

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

  public func manipulatorEnvironmentStart() {
    manipulatorEnvironmentTimerTask = Task { @MainActor in
      libkrbn_core_service_client_async_get_manipulator_environment()

      for await _ in manipulatorEnvironmentTimer {
        libkrbn_core_service_client_async_get_manipulator_environment()
      }
    }
  }

  public func manipulatorEnvironmentStop() {
    manipulatorEnvironmentTimerTask?.cancel()
  }

  public func startConnectedDevices() {
    connectedDevicesTimerTask = Task { @MainActor in
      libkrbn_core_service_client_async_get_connected_devices()

      for await _ in connectedDevicesTimer {
        libkrbn_core_service_client_async_get_connected_devices()
      }
    }
  }

  public func stopConnectedDevices() {
    connectedDevicesTimerTask?.cancel()
  }

  @Published var temporarilyIgnoreAllDevices: Bool = false {
    didSet {
      libkrbn_core_service_client_async_temporarily_ignore_all_devices(temporarilyIgnoreAllDevices)
    }
  }
}
