import AsyncAlgorithms
import Combine
import Foundation
import SwiftUI

private func callback() {
  Task { @MainActor in
    Settings.shared.updateProperties()

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

private struct SimpleModificationPayload: Decodable {
  let index: Int
  let fromJsonString: String
  let toJsonString: String
}

private struct ComplexModificationsRulePayload: Decodable {
  let index: Int
  let description: String
  let enabled: Bool
  let codeString: String
  let searchText: String
  let codeType: String
}

private struct ProfilePayload: Decodable {
  let index: Int
  let name: String
  let selected: Bool
}

private let settingsJSONDecoder: JSONDecoder = {
  let decoder = JSONDecoder()
  decoder.keyDecodingStrategy = .convertFromSnakeCase
  return decoder
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

    updateProperties()
    didSetEnabled = true
  }

  public func watch() {
    if watching {
      return
    }
    watching = true

    krbn_enable_configuration_monitor()
    krbn_set_core_configuration_updated_callback(callback)
    krbn_enqueue_callback(callback)

    connectedDevicesCancellable = connectedDevices.$connectedDevices
      .sink { [weak self] _ in
        Task { @MainActor in
          self?.updateConnectedDeviceSettings()
        }
      }
    connectedDevices.watch()
  }

  func save() {
    saveContinuation.yield(())
  }

  public func updateProperties() {
    didSetEnabled = false

    simpleModifications = makeSimpleModifications(nil)
    fnFunctionKeys = makeFnFunctionKeys(nil)

    updateComplexModificationsRules()

    complexModificationsParameterToIfAloneTimeoutMilliseconds = Int(
      krbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_if_alone_timeout_milliseconds()
    )
    complexModificationsParameterToIfHeldDownThresholdMilliseconds = Int(
      krbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_if_held_down_threshold_milliseconds()
    )
    complexModificationsParameterToDelayedActionDelayMilliseconds = Int(
      krbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_delayed_action_delay_milliseconds()
    )
    complexModificationsParameterSimultaneousThresholdMilliseconds = Int(
      krbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_simultaneous_threshold_milliseconds()
    )
    complexModificationsParameterMouseMotionToScrollSpeed = Int(
      krbn_core_configuration_get_selected_profile_complex_modifications_parameter_mouse_motion_to_scroll_speed()
    )

    updateConnectedDeviceSettings()

    delayMillisecondsBeforeOpenDevice = Int(
      krbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device()
    )

    do {
      var buffer = [Int8](repeating: 0, count: 32)
      krbn_core_configuration_get_selected_profile_virtual_hid_keyboard_keyboard_type_v2(
        &buffer, buffer.count
      )
      virtualHIDKeyboardKeyboardTypeV2 = String(utf8String: buffer) ?? ""
    }

    virtualHIDKeyboardMouseKeyXYScale = Int(
      krbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale())
    virtualHIDKeyboardIndicateStickyModifierKeysState =
      krbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state()

    updateProfiles()

    checkForUpdates =
      krbn_core_configuration_get_global_configuration_check_for_updates()
    showIconInMenuBar = krbn_core_configuration_get_global_configuration_show_in_menu_bar()
    showProfileNameInMenuBar =
      krbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar()
    showAdditionalMenuItems =
      krbn_core_configuration_get_global_configuration_show_additional_menu_items()
    enableNotificationWindow =
      krbn_core_configuration_get_global_configuration_enable_notification_window()
    enableMultitouchExtension =
      krbn_core_configuration_get_machine_specific_enable_multitouch_extension()
    do {
      var buffer = [Int8](repeating: 0, count: 4 * 1024)
      krbn_core_configuration_get_machine_specific_external_editor_path(
        &buffer, buffer.count
      )
      externalEditorPath = String(utf8String: buffer) ?? ""
    }
    unsafeUI = krbn_core_configuration_get_global_configuration_unsafe_ui()
    filterUselessEventsFromSpecificDevices =
      krbn_core_configuration_get_global_configuration_filter_useless_events_from_specific_devices()
    reorderSameTimestampInputEventsToPrioritizeModifiers =
      krbn_core_configuration_get_global_configuration_reorder_same_timestamp_input_events_to_prioritize_modifiers()
    enableCGEventTapFallback =
      krbn_core_configuration_get_global_configuration_enable_cgeventtap_fallback()

    updateSystemDefaultProfileExists()

    didSetEnabled = true
  }

  //
  // Simple Modifications
  //

  @Published var simpleModifications: [SimpleModification] = []

  public func makeSimpleModifications(_ connectedDevice: ConnectedDevice?) -> [SimpleModification] {
    let data = connectedDevice.withDeviceIdentifiersCPointer { deviceIdentifiers in
      dataFromJSONOutput { context in
        krbn_core_configuration_get_selected_profile_simple_modifications_json(
          deviceIdentifiers,
          settingsJSONOutputCallback,
          context)
      }
    }

    do {
      return try settingsJSONDecoder.decode([SimpleModificationPayload].self, from: data).map {
        SimpleModification(
          index: $0.index,
          fromJsonString: $0.fromJsonString,
          toJsonString: $0.toJsonString,
          toCategories: SimpleModificationDefinitions.shared.toCategories)
      }
    } catch {
      print("Failed to decode simple modifications JSON: \(error)")
      return []
    }
  }

  public func simpleModifications(connectedDevice: ConnectedDevice?) -> [SimpleModification] {
    if let connectedDevice = connectedDevice {
      return findConnectedDeviceSetting(connectedDevice)?.simpleModifications ?? []
    } else {
      return simpleModifications
    }
  }

  private func reflectSimpleModificationChanges(_ connectedDevice: ConnectedDevice?) {
    if connectedDevice == nil {
      simpleModifications = makeSimpleModifications(nil)
    } else {
      updateConnectedDeviceSettings()
    }
  }

  public func updateSimpleModification(
    index: Int,
    fromJsonString: String,
    toJsonString: String,
    device: ConnectedDevice?
  ) {
    device.withDeviceIdentifiersCPointer {
      if let fromJson = fromJsonString.cString(using: .utf8),
        let toJson = toJsonString.cString(using: .utf8)
      {
        krbn_core_configuration_replace_selected_profile_simple_modification(
          index, fromJson, toJson, $0)
      }
    }

    reflectSimpleModificationChanges(device)

    save()
  }

  public func appendSimpleModification(device: ConnectedDevice?) {
    device.withDeviceIdentifiersCPointer {
      krbn_core_configuration_push_back_selected_profile_simple_modification($0)
    }

    reflectSimpleModificationChanges(device)

    // Do not to call `save()` here because partial settings will be erased at save.
  }

  public func appendSimpleModificationIfEmpty(device: ConnectedDevice?) {
    device.withDeviceIdentifiersCPointer {
      if krbn_core_configuration_selected_profile_simple_modifications_empty($0) {
        appendSimpleModification(device: device)
      }
    }
  }

  public func removeSimpleModification(
    index: Int,
    device: ConnectedDevice?
  ) {
    device.withDeviceIdentifiersCPointer {
      krbn_core_configuration_erase_selected_profile_simple_modification(index, $0)
    }

    reflectSimpleModificationChanges(device)

    save()
  }

  //
  // Fn Function Keys
  //

  @Published var fnFunctionKeys: [SimpleModification] = []

  public func makeFnFunctionKeys(_ connectedDevice: ConnectedDevice?) -> [SimpleModification] {
    let data = connectedDevice.withDeviceIdentifiersCPointer { deviceIdentifiers in
      dataFromJSONOutput { context in
        krbn_core_configuration_get_selected_profile_fn_function_keys_json(
          deviceIdentifiers,
          settingsJSONOutputCallback,
          context)
      }
    }

    do {
      return try settingsJSONDecoder.decode([SimpleModificationPayload].self, from: data).map {
        SimpleModification(
          index: $0.index,
          fromJsonString: $0.fromJsonString,
          toJsonString: $0.toJsonString,
          toCategories: connectedDevice == nil
            ? SimpleModificationDefinitions.shared.toCategories
            : SimpleModificationDefinitions.shared.toCategoriesWithInheritBase)
      }
    } catch {
      print("Failed to decode fn function keys JSON: \(error)")
      return []
    }
  }

  private func reflectFnFunctionKeyChanges(_ connectedDevice: ConnectedDevice?) {
    if connectedDevice == nil {
      fnFunctionKeys = makeFnFunctionKeys(nil)
    } else {
      updateConnectedDeviceSettings()
    }
  }

  public func updateFnFunctionKey(
    fromJsonString: String,
    toJsonString: String,
    device: ConnectedDevice?
  ) {
    device.withDeviceIdentifiersCPointer {
      if let fromJson = fromJsonString.cString(using: .utf8),
        let toJson = toJsonString.cString(using: .utf8)
      {
        krbn_core_configuration_replace_selected_profile_fn_function_key(fromJson, toJson, $0)
      }
    }

    reflectFnFunctionKeyChanges(device)

    save()
  }

  //
  // Complex modifications
  //

  @Published var complexModificationsRules: [ComplexModificationsRule] = []

  private func updateComplexModificationsRules() {
    let data = dataFromJSONOutput { context in
      krbn_core_configuration_get_selected_profile_complex_modifications_rules_json(
        settingsJSONOutputCallback,
        context)
    }

    do {
      complexModificationsRules =
        try settingsJSONDecoder.decode([ComplexModificationsRulePayload].self, from: data).map {
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
    } catch {
      print("Failed to decode complex modifications rules JSON: \(error)")
      complexModificationsRules = []
    }
  }

  private func reflectComplexModificationsRuleChanges() {
    updateComplexModificationsRules()
  }

  public func replaceComplexModificationsRule(
    _ complexModificationRule: ComplexModificationsRule,
    _ codeString: String,
    _ codeType: krbn_complex_modifications_rule_code_type,
  ) -> String? {
    var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
    if let cString = codeString.cString(using: .utf8) {
      krbn_core_configuration_replace_selected_profile_complex_modifications_rule(
        complexModificationRule.index,
        cString,
        codeType,
        &errorMessageBuffer,
        errorMessageBuffer.count
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
        cString,
        codeType,
        &errorMessageBuffer,
        errorMessageBuffer.count
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

  @Published var complexModificationsParameterToIfAloneTimeoutMilliseconds: Int = 0 {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_alone_timeout_milliseconds(
          Int32(complexModificationsParameterToIfAloneTimeoutMilliseconds)
        )
        save()
      }
    }
  }

  @Published var complexModificationsParameterToIfHeldDownThresholdMilliseconds: Int = 0 {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_held_down_threshold_milliseconds(
          Int32(complexModificationsParameterToIfHeldDownThresholdMilliseconds)
        )
        save()
      }
    }
  }

  @Published var complexModificationsParameterToDelayedActionDelayMilliseconds: Int = 0 {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_delayed_action_delay_milliseconds(
          Int32(complexModificationsParameterToDelayedActionDelayMilliseconds)
        )
        save()
      }
    }
  }

  @Published var complexModificationsParameterSimultaneousThresholdMilliseconds: Int = 0 {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_simultaneous_threshold_milliseconds(
          Int32(complexModificationsParameterSimultaneousThresholdMilliseconds)
        )
        save()
      }
    }
  }

  @Published var complexModificationsParameterMouseMotionToScrollSpeed: Int = 0 {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_selected_profile_complex_modifications_parameter_mouse_motion_to_scroll_speed(
          Int32(complexModificationsParameterMouseMotionToScrollSpeed)
        )
        save()
      }
    }
  }

  //
  // Devices
  //

  @Published var connectedDeviceSettings: [ConnectedDeviceSetting] = []

  private func updateConnectedDeviceSettings() {
    var newConnectedDeviceSettings: [ConnectedDeviceSetting] = []

    connectedDevices.connectedDevices.forEach { connectedDevice in
      newConnectedDeviceSettings.append(ConnectedDeviceSetting(connectedDevice))
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

    updateConnectedDeviceSettings()

    save()
  }

  @Published var delayMillisecondsBeforeOpenDevice: Int = 0 {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(
          Int32(delayMillisecondsBeforeOpenDevice)
        )
        save()
      }
    }
  }

  //
  // Virtual keyboard
  //

  @Published var virtualHIDKeyboardKeyboardTypeV2 = "" {
    didSet {
      if didSetEnabled {
        if let cString = virtualHIDKeyboardKeyboardTypeV2.cString(using: .utf8) {
          krbn_core_configuration_set_selected_profile_virtual_hid_keyboard_keyboard_type_v2(
            cString
          )
          save()
        }
      }
    }
  }

  @Published var virtualHIDKeyboardMouseKeyXYScale: Int = 0 {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(
          Int32(virtualHIDKeyboardMouseKeyXYScale)
        )
        save()
      }
    }
  }

  @Published var virtualHIDKeyboardIndicateStickyModifierKeysState: Bool = false {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(
          virtualHIDKeyboardIndicateStickyModifierKeysState
        )
        save()
      }
    }
  }

  //
  // Profiles
  //

  @Published var profiles: [Profile] = []

  private func updateProfiles() {
    let data = dataFromJSONOutput { context in
      krbn_core_configuration_get_profiles_json(settingsJSONOutputCallback, context)
    }

    do {
      profiles = try settingsJSONDecoder.decode([ProfilePayload].self, from: data).map {
        Profile($0.index, $0.name, $0.selected)
      }
    } catch {
      print("Failed to decode profiles JSON: \(error)")
      profiles = []
    }
  }

  private func reflectProfileChanges() {
    updateProfiles()
  }

  public func selectedProfileName() -> String {
    var buffer = [Int8](repeating: 0, count: 32 * 1024)
    if krbn_core_configuration_get_selected_profile_name(&buffer, buffer.count) {
      return String(utf8String: buffer) ?? ""
    }

    return ""
  }

  public func selectProfile(_ profile: Profile) {
    krbn_core_configuration_select_profile(profile.index)

    // To update all settings to the new profile's contents, it is necessary to call `updateProperties`.
    updateProperties()

    save()
  }

  public func updateProfileName(_ profile: Profile, _ name: String) {
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

  public func duplicateProfile(_ profile: Profile) {
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
    var nextProfiles = profiles
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
      profiles = nextProfiles
    }

    save()
  }

  public func removeProfile(_ profile: Profile) {
    krbn_core_configuration_erase_profile(profile.index)

    reflectProfileChanges()

    save()
  }

  //
  // Misc
  //

  @Published var checkForUpdates: Bool = false {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_global_configuration_check_for_updates(
          checkForUpdates
        )
        save()
      }
    }
  }

  @Published var showIconInMenuBar: Bool = false {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_global_configuration_show_in_menu_bar(
          showIconInMenuBar
        )
        save()
      }
    }
  }

  @Published var showProfileNameInMenuBar: Bool = false {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(
          showProfileNameInMenuBar
        )
        save()
      }
    }
  }

  @Published var showAdditionalMenuItems: Bool = false {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_global_configuration_show_additional_menu_items(
          showAdditionalMenuItems
        )
        save()
      }
    }
  }

  @Published var enableNotificationWindow: Bool = false {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_global_configuration_enable_notification_window(
          enableNotificationWindow
        )
        save()
      }
    }
  }

  @Published var enableMultitouchExtension: Bool = false {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_machine_specific_enable_multitouch_extension(
          enableMultitouchExtension
        )
        save()
      }
    }
  }

  @Published var externalEditorPath: String = "" {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_machine_specific_external_editor_path(
          externalEditorPath
        )
        save()
      }
    }
  }

  @Published var unsafeUI: Bool = false {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_global_configuration_unsafe_ui(
          unsafeUI
        )
        save()
      }
    }
  }

  @Published var filterUselessEventsFromSpecificDevices: Bool = true {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_global_configuration_filter_useless_events_from_specific_devices(
          filterUselessEventsFromSpecificDevices
        )
        save()
      }
    }
  }

  @Published var reorderSameTimestampInputEventsToPrioritizeModifiers: Bool = true {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_global_configuration_reorder_same_timestamp_input_events_to_prioritize_modifiers(
          reorderSameTimestampInputEventsToPrioritizeModifiers
        )
        save()
      }
    }
  }

  @Published var enableCGEventTapFallback: Bool = false {
    didSet {
      if didSetEnabled {
        krbn_core_configuration_set_global_configuration_enable_cgeventtap_fallback(
          enableCGEventTapFallback
        )
        save()
      }
    }
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
