import Foundation
import SwiftUI

func componentsManagerStoppedCallback(_ lastConfigurationRevision: UInt64) {
  Task { @MainActor in
    guard Settings.shared.componentsManagerStopped(through: lastConfigurationRevision) else {
      return
    }
    ConnectedDevices.shared.componentsManagerStopped()
    SettingsCoreServiceDaemonClient.shared.componentsManagerStopped()
    SettingsConsoleUserServerClient.shared.componentsManagerStopped()
  }
}

func coreConfigurationUpdatedCallback(_ json: UnsafePointer<CChar>, _ length: Int) {
  let data = Data(bytes: json, count: length)

  Task { @MainActor in
    Settings.shared.receiveConfigurationResponse(data)
  }
}

func coreConfigurationLoadStateChangedCallback(_ state: krbn_core_configuration_load_state) {
  Task { @MainActor in
    Settings.shared.coreConfigurationLoadStateChanged(state)
  }
}

private let settingsJSONDecoder: JSONDecoder = {
  let decoder = JSONDecoder()
  decoder.keyDecodingStrategy = .convertFromSnakeCase
  return decoder
}()

private let settingsJSONEncoder: JSONEncoder = {
  let encoder = JSONEncoder()
  encoder.keyEncodingStrategy = .convertToSnakeCase
  return encoder
}()

@MainActor
final class Settings: ObservableObject {
  static let shared = Settings()

  var uiLocale: Locale {
    AppLanguage.locale(
      for: configurationLoaded ? configuration.globalConfiguration.uiLanguage : "auto")
  }

  static let didConfigurationLoad = Notification.Name("didConfigurationLoad")

  private var pendingRequests: [UInt64: (String?) -> Void] = [:]
  private var completedRequests: [() -> Void] = []
  private var lifecycleGeneration: UInt64 = 0
  private var snapshots = ConfigurationSnapshotBuffer<
    (data: Data?, count: UInt64, saveError: String)
  >()
  // Block UI edits while an add, removal, reorder, or profile switch is pending.
  // Otherwise, indices from the old UI could modify a different item after the change.
  // Keep edits blocked until all pending responses are received and the latest snapshot
  // can be applied; also clear this flag when components stop. Ordinary key edits do not set it.
  @Published private(set) var isStructuralChangePending = false

  @Published var saveErrorMessage = ""
  @Published private var loadingState = ConfigurationLoadingState.stopped
  @Published private var configurationStorage: SettingsConfiguration?

  var configurationLoaded: Bool {
    loadingState.isReady
  }

  var configurationLoadState: krbn_core_configuration_load_state? {
    switch loadingState {
    case .stopped, .loading:
      return nil
    case .ready:
      return krbn_core_configuration_load_state_loaded
    case .failed(.permission):
      return krbn_core_configuration_load_state_permission_error
    case .failed(.json):
      return krbn_core_configuration_load_state_json_error
    case .failed(.other):
      return krbn_core_configuration_load_state_other_error
    }
  }

  var configuration: SettingsConfiguration {
    get {
      guard let configurationStorage else {
        preconditionFailure("Settings configuration has not been loaded")
      }
      return configurationStorage
    }
    set {
      // This setter handles UI edits only. Received snapshots update storage directly.
      guard configurationLoaded, !isStructuralChangePending, let oldValue = configurationStorage
      else {
        return
      }
      configurationStorage = newValue
      applyConfigurationPatch(from: oldValue, to: newValue)
    }
  }

  private init() {}

  // Register before enqueueing. Older snapshots are held while edits are pending,
  // so acknowledgements cannot overwrite newer optimistic Swift edits.
  private func performCommand(
    _ action: ConfigurationAction,
    parameters: [String: Any] = [:],
    completion: ((String?) -> Void)? = nil
  ) {
    guard configurationLoaded else {
      completion?("Configuration is not ready")
      return
    }
    // Reject callbacks already queued before SwiftUI disables the controls.
    guard !isStructuralChangePending else {
      completion?("Settings are being updated. Please try again.")
      return
    }

    let requestID = snapshots.beginRequest(blocksEditing: action.blocksEditing)
    isStructuralChangePending = snapshots.isEditingBlocked
    let generation = lifecycleGeneration
    pendingRequests[requestID] =
      completion ?? { [weak self] error in
        if let error { self?.saveErrorMessage = error }
      }

    sendConfigurationCommand(action, parameters: parameters) { [weak self] data in
      guard let self, self.lifecycleGeneration == generation else { return }

      let result = (try? JSONSerialization.jsonObject(with: data)) as? [String: Any]
      self.receiveConfigurationResponse(data)
      self.snapshots.completeRequest(requestID)
      if let completed = self.pendingRequests.removeValue(forKey: requestID) {
        let error = result?["error"] as? String
        self.completedRequests.append { completed(error) }
      }
      self.flushConfigurationResponses()
    }
  }

  private func commandResult(
    _ action: ConfigurationAction, parameters: [String: Any] = [:]
  ) async -> String? {
    await withCheckedContinuation { continuation in
      performCommand(action, parameters: parameters) {
        continuation.resume(returning: $0)
      }
    }
  }

  fileprivate func receiveConfigurationResponse(_ data: Data) {
    guard let result = (try? JSONSerialization.jsonObject(with: data)) as? [String: Any],
      let revision = result["revision"] as? UInt64
    else { return }

    if result["configuration_unavailable"] as? Bool == true {
      snapshots.receive(revision: revision, value: (nil, 0, ""))
      flushConfigurationResponses()
      return
    }

    guard let snapshot = result["snapshot"],
      let snapshotData = try? JSONSerialization.data(withJSONObject: snapshot)
    else { return }

    snapshots.receive(
      revision: revision,
      value: (
        snapshotData,
        result["not_connected_configured_devices_count"] as? UInt64 ?? 0,
        result["save_error"] as? String ?? ""
      ))
    flushConfigurationResponses()
  }

  private func flushConfigurationResponses() {
    guard pendingRequests.isEmpty else { return }

    let readySnapshot = snapshots.takeReadySnapshot()
    isStructuralChangePending = snapshots.isEditingBlocked
    if let snapshot = readySnapshot {
      if let data = snapshot.data {
        if applyConfigurationSnapshot(data) {
          ConnectedDevices.shared.notConnectedConfiguredDevicesCount = snapshot.count
          saveErrorMessage = snapshot.saveError
          NotificationCenter.default.post(name: Self.didConfigurationLoad, object: nil)
        }
      } else {
        transitionLoadingState(.unavailable)
      }
    }
    let completions = completedRequests
    completedRequests.removeAll()
    for completed in completions {
      completed()
    }
  }

  func componentsManagerStopped(through revision: UInt64) -> Bool {
    guard snapshots.reset(through: revision) else { return false }

    isStructuralChangePending = false
    lifecycleGeneration += 1

    let pending = pendingRequests.values
    pendingRequests.removeAll()
    for callback in pending {
      callback("Settings components are stopped")
    }

    let completed = completedRequests
    completedRequests.removeAll()
    for callback in completed {
      callback()
    }

    transitionLoadingState(.stopped)

    return true
  }

  private func transitionLoadingState(_ event: ConfigurationLoadingState.Event) {
    loadingState.handle(event)
    if !configurationLoaded {
      saveErrorMessage = ""
    }
  }

  func coreConfigurationLoadStateChanged(_ state: krbn_core_configuration_load_state) {
    switch state {
    case krbn_core_configuration_load_state_loaded:
      transitionLoadingState(.loadSucceeded)
    case krbn_core_configuration_load_state_permission_error:
      transitionLoadingState(.loadFailed(.permission))
    case krbn_core_configuration_load_state_json_error:
      transitionLoadingState(.loadFailed(.json))
    default:
      transitionLoadingState(.loadFailed(.other))
    }
  }

  private func applyConfigurationSnapshot(_ data: Data) -> Bool {
    var snapshot: SettingsConfiguration
    do {
      snapshot = try settingsJSONDecoder.decode(SettingsConfiguration.self, from: data)
      snapshot.changedSettingsJson = try ChangedSettings.makeJSON(snapshotData: data)
    } catch {
      print("Failed to decode settings configuration snapshot JSON: \(error)")
      transitionLoadingState(.loadFailed(.other))
      return false
    }

    // Retain the last snapshot during loading/stopping for views that are being dismissed.
    // This path must not send the received configuration back as a UI edit.
    configurationStorage = snapshot
    updateSystemDefaultProfileExists()
    transitionLoadingState(.snapshotApplied)
    return true
  }

  //
  // Simple Modifications
  //

  public func simpleModifications(connectedDevice: ConnectedDevice?)
    -> [SettingsConfiguration.SimpleModification]
  {
    if let connectedDevice = connectedDevice {
      return configuration.selectedProfile.devices[connectedDevice.id]?.simpleModifications ?? []
    } else {
      return configuration.selectedProfile.simpleModifications
    }
  }

  public func fnFunctionKeys(connectedDevice: ConnectedDevice?)
    -> [SettingsConfiguration.SimpleModification]
  {
    if let connectedDevice {
      return configuration.selectedProfile.devices[connectedDevice.id]?.fnFunctionKeys ?? []
    } else {
      return configuration.selectedProfile.fnFunctionKeys
    }
  }

  public func updateSimpleModification(
    index: Int,
    fromJsonString: String? = nil,
    toJsonString: String? = nil,
    device: ConnectedDevice?
  ) {
    guard configurationLoaded, !isStructuralChangePending,
      let current = simpleModifications(connectedDevice: device).first(where: { $0.index == index })
    else { return }

    let updated = SettingsConfiguration.SimpleModification(
      index: index,
      fromJsonString: fromJsonString ?? current.fromJsonString,
      toJsonString: toJsonString ?? current.toJsonString)
    // Merge with the latest local row, including edits whose responses are still pending.
    if let device {
      configurationStorage?.selectedProfile.devices[device.id]?.simpleModifications[index] = updated
    } else {
      configurationStorage?.selectedProfile.simpleModifications[index] = updated
    }
    performCommand(
      .replaceSimple,
      parameters: [
        "index": index, "from": updated.fromJsonString,
        "to": updated.toJsonString, "device": device?.id ?? "{}",
      ])
  }

  public func appendSimpleModification(device: ConnectedDevice?) {
    performCommand(
      .appendSimple,
      parameters: ["device": device?.id ?? "{}"])
  }

  public func removeSimpleModification(
    index: Int,
    device: ConnectedDevice?
  ) {
    performCommand(
      .eraseSimple,
      parameters: [
        "index": index, "device": device?.id ?? "{}",
      ])
  }

  //
  // Fn Function Keys
  //

  public func updateFnFunctionKey(
    fromJsonString: String,
    toJsonString: String,
    device: ConnectedDevice?
  ) {
    performCommand(
      .replaceFn,
      parameters: [
        "from": fromJsonString,
        "to": toJsonString, "device": device?.id ?? "{}",
      ])
  }

  //
  // Complex modifications
  //

  public func replaceComplexModificationsRule(
    index: Int,
    codeString: String,
    codeType: SettingsConfiguration.ComplexModificationsRule.CodeType
  ) async -> String? {
    await commandResult(
      .replaceRule,
      parameters: [
        "index": index, "code": codeString,
        "code_type": codeType.rawValue,
        "expected_profile": configuration.profiles.first { $0.selected }?.index ?? 0,
        "expected_rules": configuration.selectedProfile.complexModifications.rules.map(
          \.codeString),
      ])
  }

  public func pushFrontComplexModificationsRule(
    codeString: String,
    codeType: SettingsConfiguration.ComplexModificationsRule.CodeType
  ) async -> String? {
    await commandResult(
      .prependRule,
      parameters: [
        "code": codeString,
        "code_type": codeType.rawValue,
        "expected_profile": configuration.profiles.first { $0.selected }?.index ?? 0,
        "expected_rules": configuration.selectedProfile.complexModifications.rules.map(
          \.codeString),
      ])
  }

  public func moveComplexModificationsRule(_ sourceIndex: Int, _ destinationIndex: Int) {
    performCommand(
      .moveRule,
      parameters: [
        "source": sourceIndex, "destination": destinationIndex,
      ])
  }

  public func setComplexModificationsRuleEnabled(index: Int, enabled: Bool) {
    performCommand(
      .enableRule,
      parameters: [
        "index": index, "enabled": enabled,
      ])
  }

  public func removeComplexModificationsRule(index: Int) {
    performCommand(
      .eraseRule,
      parameters: [
        "index": index
      ])
  }

  public func addComplexModificationRules(
    _ complexModificationsAssetFile: ComplexModificationsAssetFile
  ) {
    let rules = complexModificationsAssetFile.assetRules.reversed().map {
      ["file_path": $0.filePath, "index": $0.ruleIndex] as [String: Any]
    }
    performCommand(
      .addRules,
      parameters: [
        "rules": rules
      ])
  }

  public func addComplexModificationRule(
    _ complexModificationsAssetRule: ComplexModificationsAssetRule
  ) {
    performCommand(
      .addRules,
      parameters: [
        "rules": [
          [
            "file_path": complexModificationsAssetRule.filePath,
            "index": complexModificationsAssetRule.ruleIndex,
          ]
        ]
      ])
  }

  //
  // Devices
  //

  func deviceConfiguration(_ connectedDevice: ConnectedDevice) -> SettingsConfiguration.Device? {
    configurationStorage?.selectedProfile.devices[connectedDevice.id]
  }

  func deviceConfigurationBinding(_ connectedDevice: ConnectedDevice)
    -> Binding<SettingsConfiguration.Device>?
  {
    guard deviceConfiguration(connectedDevice) != nil else { return nil }

    return Binding(
      get: {
        guard let device = self.deviceConfiguration(connectedDevice) else {
          preconditionFailure("Device configuration is not available")
        }
        return device
      },
      set: { device in
        self.configuration.selectedProfile.devices[connectedDevice.id] = device
      })
  }

  enum GamePadStickFormula: String {
    case x
    case y
    case verticalWheel = "vertical_wheel"
    case horizontalWheel = "horizontal_wheel"
  }

  func setGamePadStickFormula(
    _ formula: GamePadStickFormula,
    value: String,
    connectedDevice: ConnectedDevice
  ) async -> Bool {
    await commandResult(
      .setFormula,
      parameters: [
        "formula": formula.rawValue,
        "device": connectedDevice.id, "value": value,
      ]) == nil
  }

  func resetGamePadStickFormula(
    _ formula: GamePadStickFormula,
    connectedDevice: ConnectedDevice
  ) async {
    _ = await commandResult(
      .resetFormula,
      parameters: [
        "formula": formula.rawValue, "device": connectedDevice.id,
      ])
  }

  public func eraseNotConnectedDeviceSettings() {
    performCommand(.eraseDisconnectedDevices)
  }

  //
  // Profiles
  //

  public func selectedProfileName() -> String {
    configuration.profiles.first { $0.selected }?.name ?? ""
  }

  public func selectProfile(_ profile: SettingsConfiguration.Profile) {
    performCommand(
      .selectProfile,
      parameters: [
        "index": profile.index
      ])
  }

  public func updateProfileName(_ profile: SettingsConfiguration.Profile, _ name: String) {
    performCommand(
      .renameProfile,
      parameters: [
        "index": profile.index, "name": name,
      ])
  }

  public func appendProfile() {
    performCommand(.appendProfile)
  }

  public func duplicateProfile(_ profile: SettingsConfiguration.Profile) {
    performCommand(
      .duplicateProfile,
      parameters: [
        "index": profile.index
      ])
  }

  public func moveProfile(_ sourceIndex: Int, _ destinationIndex: Int) {
    performCommand(
      .moveProfile,
      parameters: [
        "source": sourceIndex, "destination": destinationIndex,
      ])
  }

  public func removeProfile(_ profile: SettingsConfiguration.Profile) {
    performCommand(
      .eraseProfile,
      parameters: [
        "index": profile.index
      ])
  }

  //
  // Misc
  //

  private func applyConfigurationPatch(
    from oldConfiguration: SettingsConfiguration,
    to newConfiguration: SettingsConfiguration
  ) {
    do {
      let oldData = try settingsJSONEncoder.encode(SettingsConfigurationUpdate(oldConfiguration))
      let newData = try settingsJSONEncoder.encode(SettingsConfigurationUpdate(newConfiguration))
      let oldJSON = try JSONSerialization.jsonObject(with: oldData)
      let newJSON = try JSONSerialization.jsonObject(with: newData)

      guard let patch = SettingsJSONDiff.make(from: oldJSON, to: newJSON) else {
        return
      }

      performCommand(
        .patch,
        parameters: [
          "patch": patch
        ])
    } catch {
      print("Failed to make settings configuration update JSON: \(error)")
    }
  }

  @Published var systemDefaultProfileExists: Bool = false
  private func updateSystemDefaultProfileExists() {
    systemDefaultProfileExists = krbn_system_core_configuration_file_path_exists()
  }

  func installSystemDefaultProfile() async {
    // The copy must not start until the latest configuration has been written.
    if let error = await commandResult(.syncSave) {
      saveErrorMessage = error
      return
    }

    let url = URL(
      fileURLWithPath:
        "/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/copy_current_profile_to_system_default_profile.applescript"
    )
    guard let script = NSAppleScript(contentsOf: url, error: nil) else { return }
    script.executeAndReturnError(nil)

    updateSystemDefaultProfileExists()
  }

  func removeSystemDefaultProfile() {
    let url = URL(
      fileURLWithPath:
        "/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/remove_system_default_profile.applescript"
    )
    guard let script = NSAppleScript(contentsOf: url, error: nil) else { return }
    script.executeAndReturnError(nil)

    updateSystemDefaultProfileExists()
  }
}
