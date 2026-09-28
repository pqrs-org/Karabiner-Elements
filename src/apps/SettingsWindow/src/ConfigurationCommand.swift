import Foundation

enum ConfigurationAction: String, Sendable {
  case reloadAssets = "reload_assets"
  case eraseAsset = "erase_asset"
  case snapshot = "snapshot"
  case syncSave = "sync_save"
  case addRules = "add_rules"
  case patch = "patch"
  case selectProfile = "select_profile"
  case renameProfile = "rename_profile"
  case appendProfile = "append_profile"
  case duplicateProfile = "duplicate_profile"
  case moveProfile = "move_profile"
  case eraseProfile = "erase_profile"
  case replaceSimple = "replace_simple"
  case appendSimple = "append_simple"
  case eraseSimple = "erase_simple"
  case replaceFn = "replace_fn"
  case replaceRule = "replace_rule"
  case prependRule = "prepend_rule"
  case moveRule = "move_rule"
  case enableRule = "enable_rule"
  case eraseRule = "erase_rule"
  case eraseDisconnectedDevices = "erase_disconnected_devices"
  case setFormula = "set_formula"
  case resetFormula = "reset_formula"

  var blocksEditing: Bool {
    switch self {
    case .addRules, .selectProfile, .appendProfile, .duplicateProfile, .moveProfile, .eraseProfile,
      .appendSimple, .eraseSimple, .prependRule, .moveRule, .eraseRule, .eraseDisconnectedDevices:
      return true
    case .reloadAssets, .eraseAsset, .snapshot, .syncSave, .patch, .renameProfile, .replaceSimple,
      .replaceFn, .replaceRule, .enableRule, .setFormula, .resetFormula:
      return false
    }
  }
}

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
  _ action: ConfigurationAction,
  parameters: [String: Any] = [:],
  completion: @escaping @MainActor @Sendable (Data) -> Void
) {
  do {
    var command = parameters
    command["action"] = action.rawValue
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
