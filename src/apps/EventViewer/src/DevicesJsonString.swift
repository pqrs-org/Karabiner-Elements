import AsyncAlgorithms
import SwiftUI

private func connectedDevicesReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let text = String(cString: jsonString)

  Task { @MainActor in
    DevicesJsonString.shared.stream.setText(text)
  }
}

@MainActor
public class DevicesJsonString {
  public static let shared = DevicesJsonString()

  private let timer: AsyncTimerSequence<ContinuousClock>
  private var timerTask: Task<Void, Never>?

  let stream = RealtimeTextStream()

  init() {
    timer = AsyncTimerSequence(
      interval: .milliseconds(1000),
      clock: .continuous
    )
  }

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init,
  // there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_register_core_service_client_connected_devices_received_callback(
      connectedDevicesReceivedCallback)

    timerTask = Task { @MainActor in
      libkrbn_core_service_client_async_get_connected_devices()

      for await _ in timer {
        libkrbn_core_service_client_async_get_connected_devices()
      }
    }
  }

  public func stop() {
    libkrbn_unregister_core_service_client_connected_devices_received_callback(
      connectedDevicesReceivedCallback)

    timerTask?.cancel()
  }
}
