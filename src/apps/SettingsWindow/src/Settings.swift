import AsyncAlgorithms
import Foundation
import SwiftUI

func componentsManagerStoppedCallback() {
  Task { @MainActor in
    Settings.shared.componentsManagerStopped()
    ConnectedDevices.shared.componentsManagerStopped()
    SettingsCoreServiceDaemonClient.shared.componentsManagerStopped()
    SettingsConsoleUserServerClient.shared.componentsManagerStopped()
  }
}

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
        guard self.configurationLoaded else { continue }

        print("save")

        self.saveErrorMessage = ""
        var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
        if !krbn_core_configuration_save(&errorMessageBuffer, errorMessageBuffer.count) {
          self.saveErrorMessage = String(utf8String: errorMessageBuffer) ?? ""
        }
      }
    }
  }

  func save() {
    krbn_core_configuration_mark_save_pending()
    saveContinuation.yield(())
  }

  func componentsManagerStopped() {
    didSetEnabled = false
    configurationLoaded = false
    saveErrorMessage = ""
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

    updateSystemDefaultProfileExists()

    didSetEnabled = true
    configurationLoaded = true
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

  private func reflectComplexModificationsRuleChanges() {
    updateProperties()
  }

  private func krbnCodeType(
    _ codeType: SettingsConfiguration.ComplexModificationsRule.CodeType
  ) -> krbn_complex_modifications_rule_code_type {
    switch codeType {
    case .json:
      return krbn_complex_modifications_rule_code_type_json
    case .javascript:
      return krbn_complex_modifications_rule_code_type_javascript
    }
  }

  public func replaceComplexModificationsRule(
    index: Int,
    codeString: String,
    codeType: SettingsConfiguration.ComplexModificationsRule.CodeType
  ) -> String? {
    var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
    if let cString = codeString.cString(using: .utf8) {
      krbn_core_configuration_replace_selected_profile_complex_modifications_rule(
        index: index,
        code: cString,
        codeType: krbnCodeType(codeType),
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
    codeString: String,
    codeType: SettingsConfiguration.ComplexModificationsRule.CodeType
  ) -> String? {
    var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
    if let cString = codeString.cString(using: .utf8) {
      krbn_core_configuration_push_front_selected_profile_complex_modifications_rule(
        code: cString,
        codeType: krbnCodeType(codeType),
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

    // Reorder the currently decoded rules locally so their UUIDs are preserved during the move.
    // Decoding a fresh snapshot here would assign new UUIDs and reset the List scroll state.
    var configuration = configuration
    var rules = configuration.selectedProfile.complexModifications.rules
    if sourceIndex >= 0 && sourceIndex < rules.count {
      let item = rules.remove(at: sourceIndex)
      var destination = destinationIndex
      if sourceIndex < destination {
        destination -= 1
      }
      destination = max(0, min(destination, rules.count))
      rules.insert(item, at: destination)

      for index in rules.indices {
        rules[index].index = index
      }
      configuration.selectedProfile.complexModifications.rules = rules
      self.configuration = configuration
    }

    save()
  }

  public func setComplexModificationsRuleEnabled(index: Int, enabled: Bool) {
    krbn_core_configuration_set_selected_profile_complex_modifications_rule_enabled(index, enabled)

    var configuration = configuration
    if configuration.selectedProfile.complexModifications.rules.indices.contains(index) {
      configuration.selectedProfile.complexModifications.rules[index].enabled = enabled
      self.configuration = configuration
    }

    save()
  }

  public func removeComplexModificationsRule(index: Int) {
    krbn_core_configuration_erase_selected_profile_complex_modifications_rule(index)

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

  enum GamePadStickFormula {
    case x
    case y
    case verticalWheel
    case horizontalWheel
  }

  func setGamePadStickFormula(
    _ formula: GamePadStickFormula,
    value: String,
    connectedDevice: ConnectedDevice
  ) -> Bool {
    let valid = value.withCString { value in
      connectedDevice.withDeviceIdentifiersJSONCString { identifiers in
        switch formula {
        case .x:
          krbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(
            identifiers, value)
        case .y:
          krbn_core_configuration_set_selected_profile_device_game_pad_stick_y_formula(
            identifiers, value)
        case .verticalWheel:
          krbn_core_configuration_set_selected_profile_device_game_pad_stick_vertical_wheel_formula(
            identifiers, value)
        case .horizontalWheel:
          krbn_core_configuration_set_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
            identifiers, value)
        }
      }
    }

    if valid {
      var nextConfiguration = configuration
      switch formula {
      case .x:
        nextConfiguration.selectedProfile.devices[connectedDevice.id]?.gamePadStickXFormula = value
      case .y:
        nextConfiguration.selectedProfile.devices[connectedDevice.id]?.gamePadStickYFormula = value
      case .verticalWheel:
        nextConfiguration.selectedProfile.devices[connectedDevice.id]?
          .gamePadStickVerticalWheelFormula = value
      case .horizontalWheel:
        nextConfiguration.selectedProfile.devices[connectedDevice.id]?
          .gamePadStickHorizontalWheelFormula = value
      }
      didSetEnabled = false
      configuration = nextConfiguration
      didSetEnabled = true
      save()
    }

    return valid
  }

  func resetGamePadStickFormula(
    _ formula: GamePadStickFormula,
    connectedDevice: ConnectedDevice
  ) {
    connectedDevice.withDeviceIdentifiersJSONCString { identifiers in
      switch formula {
      case .x:
        krbn_core_configuration_reset_selected_profile_device_game_pad_stick_x_formula(identifiers)
      case .y:
        krbn_core_configuration_reset_selected_profile_device_game_pad_stick_y_formula(identifiers)
      case .verticalWheel:
        krbn_core_configuration_reset_selected_profile_device_game_pad_stick_vertical_wheel_formula(
          identifiers)
      case .horizontalWheel:
        krbn_core_configuration_reset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
          identifiers)
      }
    }

    save()
    updateProperties()
  }

  public func eraseNotConnectedDeviceSettings() {
    ConnectedDevices.shared.connectedDevicesJSONString.withCString {
      krbn_core_configuration_erase_selected_profile_not_connected_configured_devices($0)
    }

    ConnectedDevices.shared.notConnectedConfiguredDevicesCount = 0

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
  // SettingsConfigurationUpdate does not contain null values, so recursively comparing JSON
  // objects and treating arrays and primitive values as leaves is sufficient here.
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
