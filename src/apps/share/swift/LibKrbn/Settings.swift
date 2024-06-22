import Combine
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
  final class Settings: ObservableObject {
    static let shared = Settings()

    static let didConfigurationLoad = Notification.Name("didConfigurationLoad")

    private var watching = false
    private var didSetEnabled = false
    private var saveDebouncer = Debouncer(delay: 0.2)

    @Published var saveErrorMessage = ""

    private init() {
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

      NotificationCenter.default.addObserver(
        forName: ConnectedDevices.didConnectedDevicesUpdate,
        object: nil,
        queue: .main
      ) { [weak self] _ in
        guard let self = self else { return }

        self.updateConnectedDeviceSettings()
      }
    }

    func save() {
      saveDebouncer.debounce { [weak self] in
        guard let self = self else { return }

        print("save")

        self.saveErrorMessage = ""
        var errorMessageBuffer = [Int8](repeating: 0, count: 4 * 1024)
        if !libkrbn_core_configuration_save(&errorMessageBuffer, errorMessageBuffer.count) {
          self.saveErrorMessage = String(cString: errorMessageBuffer)
        }
      }
    }

    public func updateProperties() {
      didSetEnabled = false

      simpleModifications = makeSimpleModifications(nil)
      fnFunctionKeys = makeFnFunctionKeys(nil)

      updateComplexModificationsRules()
      complexModificationsParameterToIfAloneTimeoutMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          "basic.to_if_alone_timeout_milliseconds"
        ))
      complexModificationsParameterToIfHeldDownThresholdMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          "basic.to_if_held_down_threshold_milliseconds"
        ))
      complexModificationsParameterToDelayedActionDelayMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          "basic.to_delayed_action_delay_milliseconds"
        ))
      complexModificationsParameterSimultaneousThresholdMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          "basic.simultaneous_threshold_milliseconds"
        ))
      complexModificationsParameterMouseMotionToScrollSpeed = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          "mouse_motion_to_scroll.speed"
        ))

      updateConnectedDeviceSettings()
      delayMillisecondsBeforeOpenDevice = Int(
        libkrbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device()
      )

      virtualHIDKeyboardCountryCode = Int(
        libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_country_code())
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

      let size = libkrbn_core_configuration_get_selected_profile_simple_modifications_size(
        connectedDevice?.libkrbnDeviceIdentifiers)

      for i in 0..<size {
        var buffer = [Int8](repeating: 0, count: 32 * 1024)
        var fromJsonString = ""
        var toJsonString = ""

        if libkrbn_core_configuration_get_selected_profile_simple_modification_from_json_string(
          i, connectedDevice?.libkrbnDeviceIdentifiers,
          &buffer, buffer.count)
        {
          fromJsonString = String(cString: buffer)
        }

        if libkrbn_core_configuration_get_selected_profile_simple_modification_to_json_string(
          i, connectedDevice?.libkrbnDeviceIdentifiers,
          &buffer, buffer.count)
        {
          toJsonString = String(cString: buffer)
        }

        let simpleModification = SimpleModification(
          index: i,
          fromJsonString: fromJsonString,
          toJsonString: toJsonString,
          toCategories: SimpleModificationDefinitions.shared.toCategories
        )

        result.append(simpleModification)
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

    public func updateSimpleModification(
      index: Int,
      fromJsonString: String,
      toJsonString: String,
      device: ConnectedDevice?
    ) {
      libkrbn_core_configuration_replace_selected_profile_simple_modification(
        index,
        fromJsonString.cString(using: .utf8),
        toJsonString.cString(using: .utf8),
        device?.libkrbnDeviceIdentifiers)

      save()
    }

    public func appendSimpleModification(device: ConnectedDevice?) {
      libkrbn_core_configuration_push_back_selected_profile_simple_modification(
        device?.libkrbnDeviceIdentifiers)

      // Do not to call `save()` here because partial settings will be erased at save.
      updateProperties()
    }

    public func appendSimpleModification(
      jsonString: String,
      device: ConnectedDevice?
    ) {
      if let jsonData = jsonString.data(using: .utf8) {
        if let jsonDict = try? JSONSerialization.jsonObject(with: jsonData, options: [])
          as? [String: Any]
        {
          let fromJsonString =
            SimpleModification.formatCompactJsonString(jsonObject: jsonDict["from"] ?? "") ?? "{}"
          let toJsonString =
            SimpleModification.formatCompactJsonString(jsonObject: jsonDict["to"] ?? "") ?? "[]"

          libkrbn_core_configuration_push_back_selected_profile_simple_modification(
            device?.libkrbnDeviceIdentifiers)

          let size = libkrbn_core_configuration_get_selected_profile_simple_modifications_size(
            device?.libkrbnDeviceIdentifiers)

          libkrbn_core_configuration_replace_selected_profile_simple_modification(
            size - 1,
            fromJsonString.cString(using: .utf8),
            toJsonString.cString(using: .utf8),
            device?.libkrbnDeviceIdentifiers)

          // Do not to call `save()` here because partial settings will be erased at save.
          updateProperties()
        }
      }
    }

    public func removeSimpleModification(
      index: Int,
      device: ConnectedDevice?
    ) {
      libkrbn_core_configuration_erase_selected_profile_simple_modification(
        index,
        device?.libkrbnDeviceIdentifiers)

      save()
    }

    //
    // Fn Function Keys
    //

    @Published var fnFunctionKeys: [SimpleModification] = []

    public func makeFnFunctionKeys(_ connectedDevice: ConnectedDevice?) -> [SimpleModification] {
      var result: [SimpleModification] = []

      let size = libkrbn_core_configuration_get_selected_profile_fn_function_keys_size(
        connectedDevice?.libkrbnDeviceIdentifiers)

      for i in 0..<size {
        var buffer = [Int8](repeating: 0, count: 32 * 1024)
        var fromJsonString = ""
        var toJsonString = ""

        if libkrbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(
          i, connectedDevice?.libkrbnDeviceIdentifiers,
          &buffer, buffer.count)
        {
          fromJsonString = String(cString: buffer)
        }

        if libkrbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(
          i, connectedDevice?.libkrbnDeviceIdentifiers,
          &buffer, buffer.count)
        {
          toJsonString = String(cString: buffer)
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

      return result
    }

    public func updateFnFunctionKey(
      fromJsonString: String,
      toJsonString: String,
      device: ConnectedDevice?
    ) {
      libkrbn_core_configuration_replace_selected_profile_fn_function_key(
        fromJsonString.cString(using: .utf8),
        toJsonString.cString(using: .utf8),
        device?.libkrbnDeviceIdentifiers)

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
          jsonString = String(cString: buffer)
        }

        var ruleDescription = ""
        if libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(
          i, &buffer, buffer.count)
        {
          ruleDescription = String(cString: buffer)
        }

        let complexModificationsRule = ComplexModificationsRule(
          i,
          ruleDescription,
          jsonString
        )
        newComplexModificationsRules.append(complexModificationsRule)
      }

      complexModificationsRules = newComplexModificationsRules
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

      let errorMessage = String(cString: errorMessageBuffer)
      if errorMessage != "" {
        return errorMessage
      }

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

      let errorMessage = String(cString: errorMessageBuffer)
      if errorMessage != "" {
        return errorMessage
      }

      save()
      return nil
    }

    public func moveComplexModificationsRule(_ sourceIndex: Int, _ destinationIndex: Int) {
      libkrbn_core_configuration_move_selected_profile_complex_modifications_rule(
        sourceIndex,
        destinationIndex
      )
      save()
    }

    public func removeComplexModificationsRule(_ complexModificationRule: ComplexModificationsRule)
    {
      libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(
        complexModificationRule.index
      )
      save()
    }

    public func addComplexModificationRules(
      _ complexModificationsAssetFile: ComplexModificationsAssetFile
    ) {
      for rule in complexModificationsAssetFile.assetRules.reversed() {
        libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(
          rule.fileIndex, rule.ruleIndex)
      }
      save()
    }

    public func addComplexModificationRule(
      _ complexModificationsAssetRule: ComplexModificationsAssetRule
    ) {
      libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(
        complexModificationsAssetRule.fileIndex, complexModificationsAssetRule.ruleIndex)
      save()
    }

    @Published var complexModificationsParameterToIfAloneTimeoutMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(
            "basic.to_if_alone_timeout_milliseconds",
            Int32(complexModificationsParameterToIfAloneTimeoutMilliseconds)
          )
          save()
        }
      }
    }

    @Published var complexModificationsParameterToIfHeldDownThresholdMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(
            "basic.to_if_held_down_threshold_milliseconds",
            Int32(complexModificationsParameterToIfHeldDownThresholdMilliseconds)
          )
          save()
        }
      }
    }

    @Published var complexModificationsParameterToDelayedActionDelayMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(
            "basic.to_delayed_action_delay_milliseconds",
            Int32(complexModificationsParameterToDelayedActionDelayMilliseconds)
          )
          save()
        }
      }
    }

    @Published var complexModificationsParameterSimultaneousThresholdMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(
            "basic.simultaneous_threshold_milliseconds",
            Int32(complexModificationsParameterSimultaneousThresholdMilliseconds)
          )
          save()
        }
      }
    }

    @Published var complexModificationsParameterMouseMotionToScrollSpeed: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(
            "mouse_motion_to_scroll.speed",
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

      ConnectedDevices.shared.connectedDevices.forEach { connectedDevice in
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

    @Published var virtualHIDKeyboardCountryCode: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_country_code(
            UInt8(virtualHIDKeyboardCountryCode)
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
          name = String(cString: buffer)
        }

        let profile = Profile(i, name, libkrbn_core_configuration_get_profile_selected(i))
        newProfiles.append(profile)
      }

      profiles = newProfiles
    }

    public func selectProfile(_ profile: Profile) {
      libkrbn_core_configuration_select_profile(profile.index)
      save()
    }

    public func updateProfileName(_ profile: Profile, _ name: String) {
      libkrbn_core_configuration_set_profile_name(
        profile.index, name.cString(using: .utf8)
      )
      save()
    }

    public func appendProfile() {
      libkrbn_core_configuration_push_back_profile()
      save()
    }

    public func duplicateProfile(_ profile: Profile) {
      libkrbn_core_configuration_duplicate_profile(profile.index)
      save()
    }

    public func moveProfile(_ sourceIndex: Int, _ destinationIndex: Int) {
      libkrbn_core_configuration_move_profile(
        sourceIndex,
        destinationIndex
      )
      save()
    }

    public func removeProfile(_ profile: Profile) {
      libkrbn_core_configuration_erase_profile(profile.index)
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
