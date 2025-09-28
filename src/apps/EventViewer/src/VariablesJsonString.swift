import AsyncAlgorithms
import SwiftUI

private func manipulatorEnvironmentReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  let text = String(cString: jsonString)

  Task { @MainActor in
    VariablesJsonString.shared.text = text
  }
}

@MainActor
public class VariablesJsonString: ObservableObject {
  public static let shared = VariablesJsonString()

  private let timer: AsyncTimerSequence<ContinuousClock>
  private var timerTask: Task<Void, Never>?

  @Published var text = ""

  init() {
    timer = AsyncTimerSequence(
      interval: .milliseconds(1000),
      clock: .continuous
    )
  }

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_register_grabber_client_manipulator_environment_received_callback(
      manipulatorEnvironmentReceivedCallback)

    timerTask = Task { @MainActor in
      libkrbn_grabber_client_async_get_manipulator_environment()

      for await _ in timer {
        libkrbn_grabber_client_async_get_manipulator_environment()
      }
    }
  }

  public func stop() {
    libkrbn_unregister_grabber_client_manipulator_environment_received_callback(
      manipulatorEnvironmentReceivedCallback)

    timerTask?.cancel()
  }
}
