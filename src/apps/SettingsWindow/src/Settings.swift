import AsyncAlgorithms
import Combine
import Foundation
import SwiftUI

func coreConfigurationUpdatedCallback(_ json: UnsafePointer<CChar>, _ length: Int) {
  let data = Data(bytes: json, count: length)

  Task { @MainActor in
    Settings.shared.updateProperties(data)

    NotificationCenter.default.post(
      name: Settings.didConfigurationLoad,
      object: nil
    )
  }
}

private func settingsJSONOutputCallback(
  _ json: UnsafePointer<CChar>,
  _ length: Int,
  _ context: UnsafeMutableRawPointer
) {
  context.assumingMemoryBound(to: Data.self).pointee = Data(bytes: json, count: length)
}

private func dataFromJSONOutput(_ body: (UnsafeMutableRawPointer) -> Void) -> Data {
  var data = Data()
  withUnsafeMutablePointer(to: &data) { context in
    body(UnsafeMutableRawPointer(context))
  }
  return data
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

  static let didConfigurationLoad = Notification.Name("didConfigurationLoad")

  @ObservedObject private var connectedDevices = ConnectedDevices.shared
  private var connectedDevicesCancellable: AnyCancellable?
  private var watching = false
  private var didSetEnabled = false

  private let saveStream: AsyncStream<Void>
  private let saveContinuation: AsyncStream<Void>.Continuation
  private var saveTask: Task<Void, Never>?

  @Published var saveErrorMessage = ""
  @Published private(set) var configurationLoaded = false
  @Published private var configurationStorage: SettingsConfiguration?

  var configuration: SettingsConfiguration {
    get {
      guard let configurationStorage else {
        preconditionFailure("Settings configuration has not been loaded")
      }
      return configurationStorage
    }
    set {
      let oldValue = configurationStorage
      configurationStorage = newValue

      if didSetEnabled, let oldValue {
        applyConfigurationPatch(from: oldValue, to: newValue)
      }
    }
  }

  private init() {
    var continuation: AsyncStream<Void>.Continuation!
    self.saveStream = AsyncStream<Void> { continuation = $0 }
    self.saveContinuation = continuation

    self.saveTask = Task { @MainActor in
      for await _ in self.saveStream.debounce(for: .seconds(0.2)) {
        print("save")

        self.saveErrorMessage = ""
        var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
        if !krbn_core_configuration_save(&errorMessageBuffer, errorMessageBuffer.count) {
          self.saveErrorMessage = String(utf8String: errorMessageBuffer) ?? ""
        }
      }
    }
  }

  public func watch() {
    if watching {
      return
    }
    watching = true

    connectedDevicesCancellable = connectedDevices.$connectedDevices
      .sink { [weak self] _ in
        Task { @MainActor in
          self?.updateConnectedDeviceSettingsFromConnectedDevices()
        }
      }
    connectedDevices.watch()
  }

  func save() {
    saveContinuation.yield(())
  }

  public func updateProperties() {
    let data = dataFromJSONOutput { context in
      krbn_core_configuration_get_settings_configuration_snapshot_json(
        settingsJSONOutputCallback,
        context)
    }

    updateProperties(data)
  }

  fileprivate func updateProperties(_ data: Data) {
    let snapshot: SettingsConfiguration
    do {
      snapshot = try settingsJSONDecoder.decode(SettingsConfiguration.self, from: data)
    } catch {
      print("Failed to decode settings configuration snapshot JSON: \(error)")
      return
    }

    didSetEnabled = false
    configuration = snapshot

    let selectedProfile = snapshot.selectedProfile
    simpleModifications = makeSimpleModifications(
      selectedProfile.simpleModifications,
      toCategories: SimpleModificationDefinitions.shared.toCategories
    )
    fnFunctionKeys = makeSimpleModifications(
      selectedProfile.fnFunctionKeys,
      toCategories: SimpleModificationDefinitions.shared.toCategories
    )
    complexModificationsRules = makeComplexModificationsRules(
      selectedProfile.complexModifications.rules
    )

    updateConnectedDeviceSettingsFromSnapshot()

    updateSystemDefaultProfileExists()

    didSetEnabled = true
    configurationLoaded = true
  }

  //
  // Simple Modifications
  //

  @Published var simpleModifications: [SimpleModification] = []

  private func makeSimpleModifications(
    _ payloads: [SettingsConfiguration.SimpleModification],
    toCategories: SimpleModificationDefinitionCategories
  ) -> [SimpleModification] {
    payloads.map {
      SimpleModification(
        index: $0.index,
        fromJsonString: $0.fromJsonString,
        toJsonString: $0.toJsonString,
        toCategories: toCategories)
    }
  }

  public func simpleModifications(connectedDevice: ConnectedDevice?) -> [SimpleModification] {
    if let connectedDevice = connectedDevice {
      return findConnectedDeviceSetting(connectedDevice)?.simpleModifications ?? []
    } else {
      return simpleModifications
    }
  }

  private func reflectSimpleModificationChanges() {
    updateProperties()
  }

  public func updateSimpleModification(
    index: Int,
    fromJsonString: String,
    toJsonString: String,
    device: ConnectedDevice?
  ) {
    device.withDeviceIdentifiersJSONCString {
      if let fromJson = fromJsonString.cString(using: .utf8),
        let toJson = toJsonString.cString(using: .utf8)
      {
        krbn_core_configuration_replace_selected_profile_simple_modification(
          index: index,
          fromJSON: fromJson,
          toJSON: toJson,
          deviceIdentifiersJSON: $0)
      }
    }

    reflectSimpleModificationChanges()

    save()
  }

  public func appendSimpleModification(device: ConnectedDevice?) {
    device.withDeviceIdentifiersJSONCString {
      krbn_core_configuration_push_back_selected_profile_simple_modification($0)
    }

    reflectSimpleModificationChanges()

    // Do not to call `save()` here because partial settings will be erased at save.
  }

  public func appendSimpleModificationIfEmpty(device: ConnectedDevice?) {
    guard configurationLoaded else { return }

    if simpleModifications(connectedDevice: device).isEmpty {
      appendSimpleModification(device: device)
    }
  }

  public func removeSimpleModification(
    index: Int,
    device: ConnectedDevice?
  ) {
    device.withDeviceIdentifiersJSONCString {
      krbn_core_configuration_erase_selected_profile_simple_modification(index, $0)
    }

    reflectSimpleModificationChanges()

    save()
  }

  //
  // Fn Function Keys
  //

  @Published var fnFunctionKeys: [SimpleModification] = []

  private func reflectFnFunctionKeyChanges() {
    updateProperties()
  }

  public func updateFnFunctionKey(
    fromJsonString: String,
    toJsonString: String,
    device: ConnectedDevice?
  ) {
    device.withDeviceIdentifiersJSONCString {
      if let fromJson = fromJsonString.cString(using: .utf8),
        let toJson = toJsonString.cString(using: .utf8)
      {
        krbn_core_configuration_replace_selected_profile_fn_function_key(
          fromJSON: fromJson,
          toJSON: toJson,
          deviceIdentifiersJSON: $0)
      }
    }

    reflectFnFunctionKeyChanges()

    save()
  }

  //
  // Complex modifications
  //

  @Published var complexModificationsRules: [ComplexModificationsRule] = []

  private func makeComplexModificationsRules(
    _ payloads: [SettingsConfiguration.ComplexModificationsRule]
  ) -> [ComplexModificationsRule] {
    payloads.map {
      ComplexModificationsRule(
        index: $0.index,
        description: $0.description,
        enabled: $0.enabled,
        codeString: $0.codeString,
        searchText: $0.searchText,
        codeType: $0.codeType == "javascript"
          ? krbn_complex_modifications_rule_code_type_javascript
          : krbn_complex_modifications_rule_code_type_json)
    }
  }

  private func reflectComplexModificationsRuleChanges() {
    updateProperties()
  }

  public func replaceComplexModificationsRule(
    _ complexModificationRule: ComplexModificationsRule,
    _ codeString: String,
    _ codeType: krbn_complex_modifications_rule_code_type,
  ) -> String? {
    var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
    if let cString = codeString.cString(using: .utf8) {
      krbn_core_configuration_replace_selected_profile_complex_modifications_rule(
        index: complexModificationRule.index,
        code: cString,
        codeType: codeType,
        errorMessageBuffer: &errorMessageBuffer,
        errorMessageBufferLength: errorMessageBuffer.count
      )

      let errorMessage = String(utf8String: errorMessageBuffer) ?? ""
      if errorMessage != "" {
        return errorMessage
      }

      reflectComplexModificationsRuleChanges()

      save()
    }

    return nil
  }

  public func pushFrontComplexModificationsRule(
    _ codeString: String,
    _ codeType: krbn_complex_modifications_rule_code_type,
  ) -> String? {
    var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
    if let cString = codeString.cString(using: .utf8) {
      krbn_core_configuration_push_front_selected_profile_complex_modifications_rule(
        code: cString,
        codeType: codeType,
        errorMessageBuffer: &errorMessageBuffer,
        errorMessageBufferLength: errorMessageBuffer.count
      )

      let errorMessage = String(utf8String: errorMessageBuffer) ?? ""
      if errorMessage != "" {
        return errorMessage
      }

      reflectComplexModificationsRuleChanges()

      save()
    }

    return nil
  }

  public func moveComplexModificationsRule(_ sourceIndex: Int, _ destinationIndex: Int) {
    krbn_core_configuration_move_selected_profile_complex_modifications_rule(
      sourceIndex,
      destinationIndex
    )

    // Avoid reflectComplexModificationsRuleChanges here because rebuilding the array can reset List scroll state.
    var rules = complexModificationsRules
    if sourceIndex >= 0 && sourceIndex < rules.count {
      let item = rules.remove(at: sourceIndex)
      var destination = destinationIndex
      if sourceIndex < destination {
        destination -= 1
      }
      destination = max(0, min(destination, rules.count))
      rules.insert(item, at: destination)

      for (index, rule) in rules.enumerated() {
        rule.index = index
      }
      complexModificationsRules = rules
    }

    save()
  }

  public func removeComplexModificationsRule(_ complexModificationRule: ComplexModificationsRule) {
    krbn_core_configuration_erase_selected_profile_complex_modifications_rule(
      complexModificationRule.index
    )

    reflectComplexModificationsRuleChanges()

    save()
  }

  public func addComplexModificationRules(
    _ complexModificationsAssetFile: ComplexModificationsAssetFile
  ) {
    for rule in complexModificationsAssetFile.assetRules.reversed() {
      krbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(
        rule.fileIndex, rule.ruleIndex)
    }

    reflectComplexModificationsRuleChanges()

    save()
  }

  public func addComplexModificationRule(
    _ complexModificationsAssetRule: ComplexModificationsAssetRule
  ) {
    krbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(
      complexModificationsAssetRule.fileIndex, complexModificationsAssetRule.ruleIndex)

    reflectComplexModificationsRuleChanges()

    save()
  }

  //
  // Devices
  //

  @Published var connectedDeviceSettings: [ConnectedDeviceSetting] = []

  private func updateConnectedDeviceSettingsFromConnectedDevices() {
    updateConnectedDeviceSettings(updateProperties: false)
  }

  private func updateConnectedDeviceSettingsFromSnapshot() {
    updateConnectedDeviceSettings(updateProperties: true)
  }

  private func updateConnectedDeviceSettings(updateProperties: Bool) {
    guard let deviceSettings = configurationStorage?.selectedProfile.devices else {
      return
    }

    var existingConnectedDeviceSettings = connectedDeviceSettings
    var newConnectedDeviceSettings: [ConnectedDeviceSetting] = []

    connectedDevices.connectedDevices.forEach { connectedDevice in
      guard let deviceSetting = deviceSettings[connectedDevice.id] else {
        return
      }

      if let index = existingConnectedDeviceSettings.firstIndex(where: {
        $0.connectedDevice.id == connectedDevice.id
      }) {
        let connectedDeviceSetting = existingConnectedDeviceSettings.remove(at: index)
        connectedDeviceSetting.connectedDevice = connectedDevice

        if updateProperties {
          connectedDeviceSetting.updateProperties(deviceSetting)
        }

        newConnectedDeviceSettings.append(connectedDeviceSetting)
      } else {
        newConnectedDeviceSettings.append(ConnectedDeviceSetting(connectedDevice, deviceSetting))
      }
    }

    connectedDeviceSettings = newConnectedDeviceSettings
  }

  public func findConnectedDeviceSetting(_ connectedDevice: ConnectedDevice)
    -> ConnectedDeviceSetting?
  {
    for connectedDeviceSetting in connectedDeviceSettings
    where connectedDeviceSetting.connectedDevice == connectedDevice {
      return connectedDeviceSetting
    }

    return nil
  }

  public func eraseNotConnectedDeviceSettings() {
    connectedDevices.connectedDevicesJSONString.withCString {
      krbn_core_configuration_erase_selected_profile_not_connected_configured_devices($0)
    }

    connectedDevices.notConnectedConfiguredDevicesCount = 0

    updateProperties()

    save()
  }

  //
  // Profiles
  //

  private func reflectProfileChanges() {
    updateProperties()
  }

  public func selectedProfileName() -> String {
    configuration.profiles.first { $0.selected }?.name ?? ""
  }

  public func selectProfile(_ profile: SettingsConfiguration.Profile) {
    krbn_core_configuration_select_profile(profile.index)

    // To update all settings to the new profile's contents, it is necessary to call `updateProperties`.
    updateProperties()

    save()
  }

  public func updateProfileName(_ profile: SettingsConfiguration.Profile, _ name: String) {
    if let cString = name.cString(using: .utf8) {
      krbn_core_configuration_set_profile_name(profile.index, cString)

      reflectProfileChanges()

      save()
    }
  }

  public func appendProfile() {
    krbn_core_configuration_push_back_profile()

    reflectProfileChanges()

    save()
  }

  public func duplicateProfile(_ profile: SettingsConfiguration.Profile) {
    krbn_core_configuration_duplicate_profile(profile.index)

    reflectProfileChanges()

    save()
  }

  public func moveProfile(_ sourceIndex: Int, _ destinationIndex: Int) {
    krbn_core_configuration_move_profile(
      sourceIndex,
      destinationIndex
    )

    // Avoid reflectProfileChanges here because rebuilding the array can reset List scroll state.
    var nextProfiles = configuration.profiles
    if sourceIndex >= 0 && sourceIndex < nextProfiles.count {
      let item = nextProfiles.remove(at: sourceIndex)
      var destination = destinationIndex
      if sourceIndex < destination {
        destination -= 1
      }
      destination = max(0, min(destination, nextProfiles.count))
      nextProfiles.insert(item, at: destination)

      for index in nextProfiles.indices {
        nextProfiles[index].index = index
      }
      configuration.profiles = nextProfiles
    }

    save()
  }

  public func removeProfile(_ profile: SettingsConfiguration.Profile) {
    krbn_core_configuration_erase_profile(profile.index)

    reflectProfileChanges()

    save()
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

      guard let patch = makeJSONMergePatch(from: oldJSON, to: newJSON) else {
        return
      }

      let data = try JSONSerialization.data(withJSONObject: patch)
      guard let jsonString = String(data: data, encoding: .utf8) else { return }

      if jsonString.withCString({ krbn_core_configuration_apply_settings_configuration_update($0) })
      {
        save()
      }
    } catch {
      print("Failed to make settings configuration update JSON: \(error)")
    }
  }

  // Returns a JSON Merge Patch that contains only values changed in `newValue`.
  // SettingsConfigurationUpdate does not contain arrays or null values, so recursively comparing
  // JSON objects and treating every other value as a leaf is sufficient here.
  private func makeJSONMergePatch(from oldValue: Any, to newValue: Any) -> Any? {
    if let oldObject = oldValue as? [String: Any],
      let newObject = newValue as? [String: Any]
    {
      var patch: [String: Any] = [:]

      for (key, newChild) in newObject {
        if let oldChild = oldObject[key] {
          if let childPatch = makeJSONMergePatch(from: oldChild, to: newChild) {
            patch[key] = childPatch
          }
        } else {
          patch[key] = newChild
        }
      }

      return patch.isEmpty ? nil : patch
    }

    if let oldObject = oldValue as? NSObject,
      oldObject.isEqual(newValue)
    {
      return nil
    }

    return newValue
  }

  @Published var systemDefaultProfileExists: Bool = false
  private func updateSystemDefaultProfileExists() {
    systemDefaultProfileExists = krbn_system_core_configuration_file_path_exists()
  }

  func installSystemDefaultProfile() {
    // Ensure karabiner.json exists before copy.
    save()

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
