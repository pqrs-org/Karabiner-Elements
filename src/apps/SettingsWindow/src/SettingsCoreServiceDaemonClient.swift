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
      if SettingsCoreServiceDaemonClient.shared.temporarilyIgnoreAllDevices
        != systemVariables.temporarilyIgnoreAllDevices
      {
        SettingsCoreServiceDaemonClient.shared.temporarilyIgnoreAllDevices =
          systemVariables.temporarilyIgnoreAllDevices
      }

      if SettingsCoreServiceDaemonClient.shared.useFkeysAsStandardFunctionKeys
        != systemVariables.useFkeysAsStandardFunctionKeys
      {
        SettingsCoreServiceDaemonClient.shared.useFkeysAsStandardFunctionKeys =
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
final class SettingsCoreServiceDaemonClient: ObservableObject {
  static let shared = SettingsCoreServiceDaemonClient()

  @Published var temporarilyIgnoreAllDevices: Bool = false
  @Published var useFkeysAsStandardFunctionKeys: Bool = false

  private let systemVariablesTimer: AsyncTimerSequence<ContinuousClock>
  private var systemVariablesTimerTask: Task<Void, Never>?

  init() {
    systemVariablesTimer = AsyncTimerSequence(
      interval: .milliseconds(1000),
      clock: .continuous
    )
  }

  public func start() {
    krbn_enable_core_service_daemon_client()

    krbn_set_core_service_daemon_client_system_variables_received_callback(
      systemVariablesReceivedCallback)

    krbn_core_service_daemon_client_async_start()
  }

  public func startSystemVariablesMonitoring() {
    systemVariablesTimerTask = Task { @MainActor in
      krbn_core_service_daemon_client_async_get_system_variables()

      for await _ in systemVariablesTimer {
        krbn_core_service_daemon_client_async_get_system_variables()
      }
    }
  }

  public func stopSystemVariablesMonitoring() {
    systemVariablesTimerTask?.cancel()
  }

  public func setAppIcon(_ number: Int32) {
    krbn_core_service_daemon_client_async_set_app_icon(number)
  }
}
