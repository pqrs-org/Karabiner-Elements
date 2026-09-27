import Foundation

private final class ConfigurationCommandCompletion: Sendable {
  let complete: @MainActor @Sendable (Data) -> Void

  init(_ complete: @escaping @MainActor @Sendable (Data) -> Void) {
    self.complete = complete
  }
}

private func configurationCommandCompleted(
  _ json: UnsafePointer<CChar>, _ length: Int, _ context: UnsafeMutableRawPointer
) {
  let completion = Unmanaged<ConfigurationCommandCompletion>.fromOpaque(context).takeRetainedValue()
  let data = Data(bytes: json, count: length)
  Task { @MainActor in
    completion.complete(data)
  }
}

// Enqueue immediately on the main actor, preserving UI edit/save order. C++ owns
// a copy of the command and completes exactly once, even during shutdown.
@MainActor
func sendConfigurationCommand(
  _ command: [String: Any], completion: @escaping @MainActor @Sendable (Data) -> Void
) {
  do {
    let data = try JSONSerialization.data(withJSONObject: command)
    let json = String(decoding: data, as: UTF8.self)
    let context = Unmanaged.passRetained(ConfigurationCommandCompletion(completion)).toOpaque()
    json.withCString {
      krbn_async_configuration_command($0, configurationCommandCompleted, context)
    }
  } catch {
    let data = try! JSONSerialization.data(withJSONObject: ["error": error.localizedDescription])
    completion(data)
  }
}
