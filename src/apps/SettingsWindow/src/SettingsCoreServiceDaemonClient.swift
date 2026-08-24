import Combine
import Foundation

func systemVariablesReceivedCallback(_ jsonString: UnsafePointer<CChar>) {
  struct SystemVariables: Decodable {
    let useFkeysAsStandardFunctionKeys: Bool

    enum CodingKeys: String, CodingKey {
      case useFkeysAsStandardFunctionKeys = "system.use_fkeys_as_standard_function_keys"
    }

    init(from decoder: Decoder) throws {
      let container = try decoder.container(keyedBy: CodingKeys.self)

      useFkeysAsStandardFunctionKeys = try container.decodeFlexibleBool(
        forKey: .useFkeysAsStandardFunctionKeys)
    }
  }

  let data = Data(bytes: jsonString, count: strlen(jsonString))

  let decoder = JSONDecoder()
  do {
    let systemVariables = try decoder.decode(SystemVariables.self, from: data)
    Task { @MainActor in
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

  @Published var useFkeysAsStandardFunctionKeys: Bool = false

  func componentsManagerStopped() {
    useFkeysAsStandardFunctionKeys = false
  }

  public func setAppIcon(_ number: Int32) {
    krbn_core_service_daemon_client_async_set_app_icon(number)
  }
}
