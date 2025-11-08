import AsyncAlgorithms
import Combine
import Foundation

private func systemVariablesReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  struct SystemVariables: Decodable {
    let temporarilyIgnoreAllDevices: Bool
    let useFkeysAsStandardFunctionKeys: Bool

    enum CodingKeys: String, CodingKey {
      case temporarilyIgnoreAllDevices = "system.temporarily_ignore_all_devices"
      case useFkeysAsStandardFunctionKeys = "system.use_fkeys_as_standard_function_keys"
    }

    init(from decoder: Decoder) throws {
      let container = try decoder.container(keyedBy: CodingKeys.self)

      temporarilyIgnoreAllDevices = try container.decodeFlexibleBool(
        forKey: .temporarilyIgnoreAllDevices)

      useFkeysAsStandardFunctionKeys = try container.decodeFlexibleBool(
        forKey: .useFkeysAsStandardFunctionKeys)
    }
  }

  let data = Data(bytes: jsonString, count: strlen(jsonString))

  let decoder = JSONDecoder()
  do {
    let systemVariables = try decoder.decode(SystemVariables.self, from: data)
    Task { @MainActor in
      if SettingsCoreServiceClient.shared.temporarilyIgnoreAllDevices
        != systemVariables.temporarilyIgnoreAllDevices
      {
        SettingsCoreServiceClient.shared.temporarilyIgnoreAllDevices =
          systemVariables.temporarilyIgnoreAllDevices
      }

      if SettingsCoreServiceClient.shared.useFkeysAsStandardFunctionKeys
        != systemVariables.useFkeysAsStandardFunctionKeys
      {
        SettingsCoreServiceClient.shared.useFkeysAsStandardFunctionKeys =
          systemVariables.useFkeysAsStandardFunctionKeys
      }
    }
  } catch let err {
    print("Failed to decode system variables JSON: \(err)")
  }
}

extension KeyedDecodingContainer {
  fileprivate func decodeFlexibleBool(forKey key: Key) throws -> Bool {
    if let value = try? decode(Bool.self, forKey: key) {
      return value
    }

    if let value = try? decode(Int.self, forKey: key) {
      return value != 0
    }

    throw DecodingError.typeMismatch(
      Bool.self,
      DecodingError.Context(
        codingPath: codingPath + [key],
        debugDescription: "Expected Bool or Int that can be coerced to Bool."
      )
    )
  }
}

@MainActor
final class SettingsCoreServiceClient: ObservableObject {
  static let shared = SettingsCoreServiceClient()

  @Published var temporarilyIgnoreAllDevices: Bool = false
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
    // So we use the shorten name settings_core_service_client -> settings_cs_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/settings_cs_clnt/18675138fbaed328.sock`

    libkrbn_enable_core_service_client("settings_cs_clnt")

    libkrbn_register_core_service_client_system_variables_received_callback(
      systemVariablesReceivedCallback)

    libkrbn_core_service_client_async_start()
  }

  public func startSystemVariablesMonitoring() {
    systemVariablesTimerTask = Task { @MainActor in
      libkrbn_core_service_client_async_get_system_variables()

      for await _ in systemVariablesTimer {
        libkrbn_core_service_client_async_get_system_variables()
      }
    }
  }

  public func stopSystemVariablesMonitoring() {
    systemVariablesTimerTask?.cancel()
  }
}
