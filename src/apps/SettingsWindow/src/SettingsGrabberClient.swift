import AsyncAlgorithms
import Combine
import Foundation

private func systemVariablesReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  struct SystemVariables: Decodable {
    let useFkeysAsStandardFunctionKeys: Bool

    enum CodingKeys: String, CodingKey {
      case useFkeysAsStandardFunctionKeys = "system.use_fkeys_as_standard_function_keys"
    }
  }

  let data = Data(bytes: jsonString, count: strlen(jsonString))

  let decoder = JSONDecoder()
  if let systemVariables = try? decoder.decode(SystemVariables.self, from: data) {
    Task { @MainActor in
      if SettingsGrabberClient.shared.useFkeysAsStandardFunctionKeys
        != systemVariables.useFkeysAsStandardFunctionKeys
      {
        SettingsGrabberClient.shared.useFkeysAsStandardFunctionKeys =
          systemVariables.useFkeysAsStandardFunctionKeys
      }
    }
  }
}

@MainActor
final class SettingsGrabberClient: ObservableObject {
  static let shared = SettingsGrabberClient()

  @Published var useFkeysAsStandardFunctionKeys: Bool = false

  private let systemVariablesTimer: AsyncTimerSequence<ContinuousClock>
  private var systemVariablesTimerTask: Task<Void, Never>?

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init,
  // there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  init() {
    systemVariablesTimer = AsyncTimerSequence(
      interval: .milliseconds(1000),
      clock: .continuous
    )
  }

  public func start() {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name settings_grabber_client -> settings_grb_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/settings_grb_clnt/18675138fbaed328.sock`

    libkrbn_enable_grabber_client("settings_grb_clnt")

    libkrbn_register_grabber_client_system_variables_received_callback(
      systemVariablesReceivedCallback)

    libkrbn_grabber_client_async_start()
  }

  public func startSystemVariablesMonitoring() {
    systemVariablesTimerTask = Task { @MainActor in
      libkrbn_grabber_client_async_get_system_variables()

      for await _ in systemVariablesTimer {
        libkrbn_grabber_client_async_get_system_variables()
      }
    }
  }

  public func stopSystemVariablesMonitoring() {
    systemVariablesTimerTask?.cancel()
  }
}
