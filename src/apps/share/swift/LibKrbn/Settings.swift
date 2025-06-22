import AsyncAlgorithms
import Foundation
import SwiftUI

private func callback() {
  Task { @MainActor in
    LibKrbn.Settings.shared.updateProperties()

    NotificationCenter.default.post(
      name: LibKrbn.Settings.didConfigurationLoad,
      object: nil
    )
  }
}

extension LibKrbn {
  @MainActor
  final class Settings: ObservableObject {
    static let shared = Settings()

    static let didConfigurationLoad = Notification.Name("didConfigurationLoad")

    private var notificationsTask: Task<Void, Never>?
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
          if !libkrbn_core_configuration_save(&errorMessageBuffer, errorMessageBuffer.count) {
            self.saveErrorMessage = String(utf8String: errorMessageBuffer) ?? ""
          }
        }
      }

      updateProperties()
      didSetEnabled = true
    }

    // We register the callback in the `watch` method rather than in `init`.
    // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

    public func watch() {
      if watching {
        return
      }
      watching = true

      libkrbn_enable_configuration_monitor()
      libkrbn_register_core_configuration_updated_callback(callback)
      libkrbn_enqueue_callback(callback)

      ConnectedDevices.shared.watch()

      notificationsTask = Task {
        await withTaskGroup(of: Void.self) { group in
          group.addTask {
            for await _ in await NotificationCenter.default.notifications(
              named: ConnectedDevices.didConnectedDevicesUpdate)
            {
              await self.updateConnectedDeviceSettings()
            }
          }
        }
      }
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
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_if_alone_timeout_milliseconds()
      )
      complexModificationsParameterToIfHeldDownThresholdMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_if_held_down_threshold_milliseconds()
      )
      complexModificationsParameterToDelayedActionDelayMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_to_delayed_action_delay_milliseconds()
      )
      complexModificationsParameterSimultaneousThresholdMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_basic_simultaneous_threshold_milliseconds()
      )
      complexModificationsParameterMouseMotionToScrollSpeed = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter_mouse_motion_to_scroll_speed()
      )

      updateConnectedDeviceSettings()

      delayMillisecondsBeforeOpenDevice = Int(
        libkrbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device()
      )

      do {
        var buffer = [Int8](repeating: 0, count: 32)
        libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_keyboard_type_v2(
          &buffer, buffer.count
        )
        virtualHIDKeyboardKeyboardTypeV2 = String(utf8String: buffer) ?? ""
      }

      virtualHIDKeyboardMouseKeyXYScale = Int(
        libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale())
      virtualHIDKeyboardIndicateStickyModifierKeysState =
        libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state()

      updateProfiles()

      checkForUpdatesOnStartup =
        libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup()
      showIconInMenuBar = libkrbn_core_configuration_get_global_configuration_show_in_menu_bar()
      showProfileNameInMenuBar =
        libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar()
      enableNotificationWindow =
        libkrbn_core_configuration_get_global_configuration_enable_notification_window()
      enableMultitouchExtension =
        libkrbn_core_configuration_get_machine_specific_enable_multitouch_extension()
      askForConfirmationBeforeQuitting =
        libkrbn_core_configuration_get_global_configuration_ask_for_confirmation_before_quitting()
      unsafeUI = libkrbn_core_configuration_get_global_configuration_unsafe_ui()
      filterUselessEventsFromSpecificDevices =
        libkrbn_core_configuration_get_global_configuration_filter_useless_events_from_specific_devices()
      reorderSameTimestampInputEventsToPrioritizeModifiers =
        libkrbn_core_configuration_get_global_configuration_reorder_same_timestamp_input_events_to_prioritize_modifiers()

      updateSystemDefaultProfileExists()

      didSetEnabled = true
    }

    //
    // Simple Modifications
    //

    @Published var simpleModifications: [SimpleModification] = []

    public func makeSimpleModifications(_ connectedDevice: ConnectedDevice?) -> [SimpleModification]
    {
      var result: [SimpleModification] = []

      connectedDevice.withDeviceIdentifiersCPointer {
        let size = libkrbn_core_configuration_get_selected_profile_simple_modifications_size($0)

        for i in 0..<size {
          var buffer = [Int8](repeating: 0, count: 32 * 1024)
          var fromJsonString = ""
          var toJsonString = ""

          if libkrbn_core_configuration_get_selected_profile_simple_modification_from_json_string(
            i, $0, &buffer, buffer.count)
          {
            fromJsonString = String(utf8String: buffer) ?? ""
          }

          if libkrbn_core_configuration_get_selected_profile_simple_modification_to_json_string(
            i, $0, &buffer, buffer.count)
          {
            toJsonString = String(utf8String: buffer) ?? ""
          }

          let simpleModification = SimpleModification(
            index: i,
            fromJsonString: fromJsonString,
            toJsonString: toJsonString,
            toCategories: SimpleModificationDefinitions.shared.toCategories
          )

          result.append(simpleModification)
        }
      }

      return result
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
        libkrbn_core_configuration_replace_selected_profile_simple_modification(
          index,
          fromJsonString.cString(using: .utf8),
          toJsonString.cString(using: .utf8),
          $0)
      }

      reflectSimpleModificationChanges(device)

      save()
    }

    public func appendSimpleModification(device: ConnectedDevice?) {
      device.withDeviceIdentifiersCPointer {
        libkrbn_core_configuration_push_back_selected_profile_simple_modification($0)
      }

      reflectSimpleModificationChanges(device)

      // Do not to call `save()` here because partial settings will be erased at save.
    }

    public func appendSimpleModificationIfEmpty(device: ConnectedDevice?) {
      device.withDeviceIdentifiersCPointer {
        let size = libkrbn_core_configuration_get_selected_profile_simple_modifications_size($0)
        if size == 0 {
          appendSimpleModification(device: device)
        }
      }
    }

    public func removeSimpleModification(
      index: Int,
      device: ConnectedDevice?
    ) {
      device.withDeviceIdentifiersCPointer {
        libkrbn_core_configuration_erase_selected_profile_simple_modification(index, $0)
      }

      reflectSimpleModificationChanges(device)

      save()
    }

    //
    // Fn Function Keys
    //

    @Published var fnFunctionKeys: [SimpleModification] = []

    public func makeFnFunctionKeys(_ connectedDevice: ConnectedDevice?) -> [SimpleModification] {
      var result: [SimpleModification] = []

      connectedDevice.withDeviceIdentifiersCPointer {
        let size = libkrbn_core_configuration_get_selected_profile_fn_function_keys_size($0)

        for i in 0..<size {
          var buffer = [Int8](repeating: 0, count: 32 * 1024)
          var fromJsonString = ""
          var toJsonString = ""

          if libkrbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(
            i, $0, &buffer, buffer.count)
          {
            fromJsonString = String(utf8String: buffer) ?? ""
          }

          if libkrbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(
            i, $0, &buffer, buffer.count)
          {
            toJsonString = String(utf8String: buffer) ?? ""
          }

          let simpleModification = SimpleModification(
            index: i,
            fromJsonString: fromJsonString,
            toJsonString: toJsonString,
            toCategories: connectedDevice == nil
              ? SimpleModificationDefinitions.shared.toCategories
              : SimpleModificationDefinitions.shared.toCategoriesWithInheritBase
          )

          result.append(simpleModification)
        }
      }

      return result
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
        libkrbn_core_configuration_replace_selected_profile_fn_function_key(
          fromJsonString.cString(using: .utf8),
          toJsonString.cString(using: .utf8),
          $0)
      }

      reflectFnFunctionKeyChanges(device)

      save()
    }

    //
    // Complex modifications
    //

    @Published var complexModificationsRules: [ComplexModificationsRule] = []

    private func updateComplexModificationsRules() {
      var newComplexModificationsRules: [ComplexModificationsRule] = []

      let size = libkrbn_core_configuration_get_selected_profile_complex_modifications_rules_size()
      for i in 0..<size {
        var jsonString: String?
        var buffer = [Int8](repeating: 0, count: 1024 * 1024)  // 1MB
        if libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_json_string(
          i, &buffer, buffer.count)
        {
          jsonString = String(utf8String: buffer)
        }

        var ruleDescription = ""
        if libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(
          i, &buffer, buffer.count)
        {
          ruleDescription = String(utf8String: buffer) ?? ""
        }

        let complexModificationsRule = ComplexModificationsRule(
          index: i,
          description: ruleDescription,
          enabled:
            libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_enabled(i),
          jsonString: jsonString
        )
        newComplexModificationsRules.append(complexModificationsRule)
      }

      complexModificationsRules = newComplexModificationsRules
    }

    private func reflectComplexModificationsRuleChanges() {
      updateComplexModificationsRules()
    }

    public func replaceComplexModificationsRule(
      _ complexModificationRule: ComplexModificationsRule,
      _ jsonString: String
    ) -> String? {
      var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
      libkrbn_core_configuration_replace_selected_profile_complex_modifications_rule(
        complexModificationRule.index,
        jsonString.cString(using: .utf8),
        &errorMessageBuffer,
        errorMessageBuffer.count
      )

      let errorMessage = String(utf8String: errorMessageBuffer) ?? ""
      if errorMessage != "" {
        return errorMessage
      }

      reflectComplexModificationsRuleChanges()

      save()
      return nil
    }

    public func pushFrontComplexModificationsRule(
      _ jsonString: String
    ) -> String? {
      var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
      libkrbn_core_configuration_push_front_selected_profile_complex_modifications_rule(
        jsonString.cString(using: .utf8),
        &errorMessageBuffer,
        errorMessageBuffer.count
      )

      let errorMessage = String(utf8String: errorMessageBuffer) ?? ""
      if errorMessage != "" {
        return errorMessage
      }

      reflectComplexModificationsRuleChanges()

      save()
      return nil
    }

    public func moveComplexModificationsRule(_ sourceIndex: Int, _ destinationIndex: Int) {
      libkrbn_core_configuration_move_selected_profile_complex_modifications_rule(
        sourceIndex,
        destinationIndex
      )

      reflectComplexModificationsRuleChanges()

      save()
    }

    public func removeComplexModificationsRule(_ complexModificationRule: ComplexModificationsRule)
    {
      libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(
        complexModificationRule.index
      )

      reflectComplexModificationsRuleChanges()

      save()
    }

    public func addComplexModificationRules(
      _ complexModificationsAssetFile: ComplexModificationsAssetFile
    ) {
      for rule in complexModificationsAssetFile.assetRules.reversed() {
        libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(
          rule.fileIndex, rule.ruleIndex)
      }

      reflectComplexModificationsRuleChanges()

      save()
    }

    public func addComplexModificationRule(
      _ complexModificationsAssetRule: ComplexModificationsAssetRule
    ) {
      libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(
        complexModificationsAssetRule.fileIndex, complexModificationsAssetRule.ruleIndex)

      reflectComplexModificationsRuleChanges()

      save()
    }

    @Published var complexModificationsParameterToIfAloneTimeoutMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_alone_timeout_milliseconds(
            Int32(complexModificationsParameterToIfAloneTimeoutMilliseconds)
          )
          save()
        }
      }
    }

    @Published var complexModificationsParameterToIfHeldDownThresholdMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_if_held_down_threshold_milliseconds(
            Int32(complexModificationsParameterToIfHeldDownThresholdMilliseconds)
          )
          save()
        }
      }
    }

    @Published var complexModificationsParameterToDelayedActionDelayMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_to_delayed_action_delay_milliseconds(
            Int32(complexModificationsParameterToDelayedActionDelayMilliseconds)
          )
          save()
        }
      }
    }

    @Published var complexModificationsParameterSimultaneousThresholdMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_basic_simultaneous_threshold_milliseconds(
            Int32(complexModificationsParameterSimultaneousThresholdMilliseconds)
          )
          save()
        }
      }
    }

    @Published var complexModificationsParameterMouseMotionToScrollSpeed: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter_mouse_motion_to_scroll_speed(
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
    @Published var notConnectedDeviceSettingsCount: Int = 0

    private func updateConnectedDeviceSettings() {
      var newConnectedDeviceSettings: [ConnectedDeviceSetting] = []

      ConnectedDevices.shared.connectedDevices.forEach { connectedDevice in
        newConnectedDeviceSettings.append(ConnectedDeviceSetting(connectedDevice))
      }

      connectedDeviceSettings = newConnectedDeviceSettings

      notConnectedDeviceSettingsCount =
        libkrbn_core_configuration_get_selected_profile_not_connected_devices_count()
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
      libkrbn_core_configuration_erase_selected_profile_not_connected_devices()

      updateConnectedDeviceSettings()

      save()
    }

    @Published var delayMillisecondsBeforeOpenDevice: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(
            Int32(delayMillisecondsBeforeOpenDevice)
          )
          save()
        }
      }
    }

    //
    // Virtual Keybaord
    //

    @Published var virtualHIDKeyboardKeyboardTypeV2 = "" {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_keyboard_type_v2(
            virtualHIDKeyboardKeyboardTypeV2.cString(using: .utf8)
          )
          save()
        }
      }
    }

    @Published var virtualHIDKeyboardMouseKeyXYScale: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(
            Int32(virtualHIDKeyboardMouseKeyXYScale)
          )
          save()
        }
      }
    }

    @Published var virtualHIDKeyboardIndicateStickyModifierKeysState: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(
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
      var newProfiles: [Profile] = []

      let size = libkrbn_core_configuration_get_profiles_size()
      for i in 0..<size {
        var buffer = [Int8](repeating: 0, count: 32 * 1024)
        var name = ""
        if libkrbn_core_configuration_get_profile_name(i, &buffer, buffer.count) {
          name = String(utf8String: buffer) ?? ""
        }

        let profile = Profile(i, name, libkrbn_core_configuration_get_profile_selected(i))
        newProfiles.append(profile)
      }

      profiles = newProfiles
    }

    private func reflectProfileChanges() {
      updateProfiles()
    }

    public func selectedProfileName() -> String {
      var buffer = [Int8](repeating: 0, count: 32 * 1024)
      if libkrbn_core_configuration_get_selected_profile_name(&buffer, buffer.count) {
        return String(utf8String: buffer) ?? ""
      }

      return ""
    }

    public func selectProfile(_ profile: Profile) {
      libkrbn_core_configuration_select_profile(profile.index)

      // To update all settings to the new profile's contents, it is necessary to call `updateProperties`.
      updateProperties()

      save()
    }

    public func updateProfileName(_ profile: Profile, _ name: String) {
      libkrbn_core_configuration_set_profile_name(
        profile.index, name.cString(using: .utf8)
      )

      reflectProfileChanges()

      save()
    }

    public func appendProfile() {
      libkrbn_core_configuration_push_back_profile()

      reflectProfileChanges()

      save()
    }

    public func duplicateProfile(_ profile: Profile) {
      libkrbn_core_configuration_duplicate_profile(profile.index)

      reflectProfileChanges()

      save()
    }

    public func moveProfile(_ sourceIndex: Int, _ destinationIndex: Int) {
      libkrbn_core_configuration_move_profile(
        sourceIndex,
        destinationIndex
      )

      reflectProfileChanges()

      save()
    }

    public func removeProfile(_ profile: Profile) {
      libkrbn_core_configuration_erase_profile(profile.index)

      reflectProfileChanges()

      save()
    }

    //
    // Misc
    //

    @Published var checkForUpdatesOnStartup: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(
            checkForUpdatesOnStartup
          )
          save()
        }
      }
    }

    @Published var showIconInMenuBar: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(
            showIconInMenuBar
          )
          save()
        }
      }
    }

    @Published var showProfileNameInMenuBar: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(
            showProfileNameInMenuBar
          )
          save()
        }
      }
    }

    @Published var enableNotificationWindow: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_enable_notification_window(
            enableNotificationWindow
          )
          save()
        }
      }
    }

    @Published var enableMultitouchExtension: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_machine_specific_enable_multitouch_extension(
            enableMultitouchExtension
          )
          save()
        }
      }
    }

    @Published var askForConfirmationBeforeQuitting: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_ask_for_confirmation_before_quitting(
            askForConfirmationBeforeQuitting
          )
          save()
        }
      }
    }

    @Published var unsafeUI: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_unsafe_ui(
            unsafeUI
          )
          save()
        }
      }
    }

    @Published var filterUselessEventsFromSpecificDevices: Bool = true {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_filter_useless_events_from_specific_devices(
            filterUselessEventsFromSpecificDevices
          )
          save()
        }
      }
    }

    @Published var reorderSameTimestampInputEventsToPrioritizeModifiers: Bool = true {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_reorder_same_timestamp_input_events_to_prioritize_modifiers(
            reorderSameTimestampInputEventsToPrioritizeModifiers
          )
          save()
        }
      }
    }

    @Published var systemDefaultProfileExists: Bool = false
    private func updateSystemDefaultProfileExists() {
      systemDefaultProfileExists = libkrbn_system_core_configuration_file_path_exists()
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
}
