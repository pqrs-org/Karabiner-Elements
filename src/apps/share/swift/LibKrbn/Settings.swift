import Combine
import Foundation
import SwiftUI

private func callback(
  _ initializedCoreConfiguration: UnsafeMutableRawPointer?,
  _ context: UnsafeMutableRawPointer?
) {
  if initializedCoreConfiguration == nil { return }
  if context == nil { return }

  let obj: LibKrbn.Settings! = unsafeBitCast(context, to: LibKrbn.Settings.self)

  DispatchQueue.main.async { [weak obj] in
    guard let obj = obj else { return }

    obj.updateProperties(initializedCoreConfiguration)

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

    var libkrbnCoreConfiguration: UnsafeMutableRawPointer?
    private var didSetEnabled = false
    private var saveDebouncer = Debouncer(delay: 0.2)

    @Published var saveErrorMessage = ""

    private init() {
      updateProperties(nil)
      didSetEnabled = true
    }

    func start() {
      let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
      libkrbn_enable_configuration_monitor(callback, obj)

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
        libkrbn_core_configuration_save(self.libkrbnCoreConfiguration)
        self.saveErrorMessage = String(cString: libkrbn_core_configuration_get_save_error_message())
      }

      self.updateProperties(self.libkrbnCoreConfiguration)
    }

    public func updateProperties(_ initializedCoreConfiguration: UnsafeMutableRawPointer?) {
      didSetEnabled = false

      libkrbnCoreConfiguration = initializedCoreConfiguration

      simpleModifications = makeSimpleModifications(nil)
      fnFunctionKeys = makeFnFunctionKeys(nil)

      updateComplexModificationsRules()
      complexModificationsParameterToIfAloneTimeoutMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          libkrbnCoreConfiguration,
          "basic.to_if_alone_timeout_milliseconds"
        ))
      complexModificationsParameterToIfHeldDownThresholdMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          libkrbnCoreConfiguration,
          "basic.to_if_held_down_threshold_milliseconds"
        ))
      complexModificationsParameterToDelayedActionDelayMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          libkrbnCoreConfiguration,
          "basic.to_delayed_action_delay_milliseconds"
        ))
      complexModificationsParameterSimultaneousThresholdMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          libkrbnCoreConfiguration,
          "basic.simultaneous_threshold_milliseconds"
        ))
      complexModificationsParameterMouseMotionToScrollSpeed = Int(
        libkrbn_core_configuration_get_selected_profile_complex_modifications_parameter(
          libkrbnCoreConfiguration,
          "mouse_motion_to_scroll.speed"
        ))

      updateConnectedDeviceSettings()
      delayMillisecondsBeforeOpenDevice = Int(
        libkrbn_core_configuration_get_selected_profile_parameters_delay_milliseconds_before_open_device(
          libkrbnCoreConfiguration))

      virtualHIDKeyboardCountryCode = Int(
        libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_country_code(
          libkrbnCoreConfiguration))
      virtualHIDKeyboardMouseKeyXYScale = Int(
        libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(
          libkrbnCoreConfiguration))
      virtualHIDKeyboardIndicateStickyModifierKeysState =
        libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(
          libkrbnCoreConfiguration)

      updateProfiles()

      checkForUpdatesOnStartup =
        libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(
          libkrbnCoreConfiguration)
      showIconInMenuBar = libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(
        libkrbnCoreConfiguration)
      showProfileNameInMenuBar =
        libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(
          libkrbnCoreConfiguration)
      askForConfirmationBeforeQuitting =
        libkrbn_core_configuration_get_global_configuration_ask_for_confirmation_before_quitting(
          libkrbnCoreConfiguration)
      unsafeUI = libkrbn_core_configuration_get_global_configuration_unsafe_ui(
        libkrbnCoreConfiguration)

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
        libkrbnCoreConfiguration,
        connectedDevice?.libkrbnDeviceIdentifiers)

      for i in 0..<size {
        let simpleModification = SimpleModification(
          index: i,
          fromJsonString: String(
            cString:
              libkrbn_core_configuration_get_selected_profile_simple_modification_from_json_string(
                libkrbnCoreConfiguration,
                i,
                connectedDevice?.libkrbnDeviceIdentifiers)),
          toJsonString: String(
            cString:
              libkrbn_core_configuration_get_selected_profile_simple_modification_to_json_string(
                libkrbnCoreConfiguration,
                i,
                connectedDevice?.libkrbnDeviceIdentifiers)),
          toCategories: SimpleModificationDefinitions.shared.toCategories
        )

        result.append(simpleModification)
      }

      return result
    }

    public func updateSimpleModification(
      index: Int,
      fromJsonString: String,
      toJsonString: String,
      device: ConnectedDevice?
    ) {
      libkrbn_core_configuration_replace_selected_profile_simple_modification(
        libkrbnCoreConfiguration,
        index,
        fromJsonString.cString(using: .utf8),
        toJsonString.cString(using: .utf8),
        device?.libkrbnDeviceIdentifiers)

      save()
    }

    public func appendSimpleModification(device: ConnectedDevice?) {
      libkrbn_core_configuration_push_back_selected_profile_simple_modification(
        libkrbnCoreConfiguration,
        device?.libkrbnDeviceIdentifiers)

      // Do not to call `save()` here because partial settings will be erased at save.
      updateProperties(libkrbnCoreConfiguration)
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
            libkrbnCoreConfiguration,
            device?.libkrbnDeviceIdentifiers)

          let size = libkrbn_core_configuration_get_selected_profile_simple_modifications_size(
            libkrbnCoreConfiguration,
            device?.libkrbnDeviceIdentifiers)

          libkrbn_core_configuration_replace_selected_profile_simple_modification(
            libkrbnCoreConfiguration,
            size - 1,
            fromJsonString.cString(using: .utf8),
            toJsonString.cString(using: .utf8),
            device?.libkrbnDeviceIdentifiers)

          // Do not to call `save()` here because partial settings will be erased at save.
          updateProperties(libkrbnCoreConfiguration)
        }
      }
    }

    public func removeSimpleModification(
      index: Int,
      device: ConnectedDevice?
    ) {
      libkrbn_core_configuration_erase_selected_profile_simple_modification(
        libkrbnCoreConfiguration,
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
        libkrbnCoreConfiguration,
        connectedDevice?.libkrbnDeviceIdentifiers)

      for i in 0..<size {
        let simpleModification = SimpleModification(
          index: i,
          fromJsonString: String(
            cString:
              libkrbn_core_configuration_get_selected_profile_fn_function_key_from_json_string(
                libkrbnCoreConfiguration,
                i,
                connectedDevice?.libkrbnDeviceIdentifiers)),
          toJsonString: String(
            cString:
              libkrbn_core_configuration_get_selected_profile_fn_function_key_to_json_string(
                libkrbnCoreConfiguration,
                i,
                connectedDevice?.libkrbnDeviceIdentifiers)),
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
        libkrbnCoreConfiguration,
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

      let size = libkrbn_core_configuration_get_selected_profile_complex_modifications_rules_size(
        libkrbnCoreConfiguration)
      for i in 0..<size {
        let complexModificationsRule = ComplexModificationsRule(
          i,
          String(
            cString:
              libkrbn_core_configuration_get_selected_profile_complex_modifications_rule_description(
                libkrbnCoreConfiguration, i
              ))
        )
        newComplexModificationsRules.append(complexModificationsRule)
      }

      complexModificationsRules = newComplexModificationsRules
    }

    public func moveComplexModificationsRule(_ sourceIndex: Int, _ destinationIndex: Int) {
      libkrbn_core_configuration_move_selected_profile_complex_modifications_rule(
        libkrbnCoreConfiguration,
        sourceIndex,
        destinationIndex
      )
      save()
    }

    public func removeComplexModificationsRule(_ complexModificationRule: ComplexModificationsRule)
    {
      libkrbn_core_configuration_erase_selected_profile_complex_modifications_rule(
        libkrbnCoreConfiguration,
        complexModificationRule.index
      )
      save()
    }

    public func addComplexModificationRules(
      _ complexModificationsAssetFile: ComplexModificationsAssetFile
    ) {
      for rule in complexModificationsAssetFile.assetRules {
        libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(
          rule.fileIndex,
          rule.ruleIndex,
          libkrbnCoreConfiguration
        )
      }
      save()
    }

    public func addComplexModificationRule(
      _ complexModificationsAssetRule: ComplexModificationsAssetRule
    ) {
      libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(
        complexModificationsAssetRule.fileIndex,
        complexModificationsAssetRule.ruleIndex,
        libkrbnCoreConfiguration
      )
      save()
    }

    @Published var complexModificationsParameterToIfAloneTimeoutMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_complex_modifications_parameter(
            libkrbnCoreConfiguration,
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
            libkrbnCoreConfiguration,
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
            libkrbnCoreConfiguration,
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
            libkrbnCoreConfiguration,
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
            libkrbnCoreConfiguration,
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
      for connectedDeviceSetting in connectedDeviceSettings {
        if connectedDeviceSetting.connectedDevice.id == connectedDevice.id {
          return connectedDeviceSetting
        }
      }

      return nil
    }

    @Published var delayMillisecondsBeforeOpenDevice: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_parameters_delay_milliseconds_before_open_device(
            libkrbnCoreConfiguration, Int32(delayMillisecondsBeforeOpenDevice)
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
            libkrbnCoreConfiguration, UInt8(virtualHIDKeyboardCountryCode)
          )
          save()
        }
      }
    }

    @Published var virtualHIDKeyboardMouseKeyXYScale: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(
            libkrbnCoreConfiguration, Int32(virtualHIDKeyboardMouseKeyXYScale)
          )
          save()
        }
      }
    }

    @Published var virtualHIDKeyboardIndicateStickyModifierKeysState: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(
            libkrbnCoreConfiguration, virtualHIDKeyboardIndicateStickyModifierKeysState
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

      let size = libkrbn_core_configuration_get_profiles_size(libkrbnCoreConfiguration)
      for i in 0..<size {
        let profile = Profile(
          i,
          String(cString: libkrbn_core_configuration_get_profile_name(libkrbnCoreConfiguration, i)),
          libkrbn_core_configuration_get_profile_selected(libkrbnCoreConfiguration, i)
        )
        newProfiles.append(profile)
      }

      profiles = newProfiles
    }

    public func selectProfile(_ profile: Profile) {
      libkrbn_core_configuration_select_profile(libkrbnCoreConfiguration, profile.index)
      save()
    }

    public func updateProfileName(_ profile: Profile, _ name: String) {
      libkrbn_core_configuration_set_profile_name(
        libkrbnCoreConfiguration, profile.index, name.cString(using: .utf8)
      )
      save()
    }

    public func appendProfile() {
      libkrbn_core_configuration_push_back_profile(libkrbnCoreConfiguration)
      save()
    }

    public func removeProfile(_ profile: Profile) {
      libkrbn_core_configuration_erase_profile(libkrbnCoreConfiguration, profile.index)
      save()
    }

    //
    // Misc
    //

    @Published var appIcon: String = "000" {
      didSet {
        if didSetEnabled {
          AppIcons.shared.apply()
        }
      }
    }

    @Published var checkForUpdatesOnStartup: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(
            libkrbnCoreConfiguration, checkForUpdatesOnStartup
          )
          save()
        }
      }
    }

    @Published var showIconInMenuBar: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(
            libkrbnCoreConfiguration, showIconInMenuBar
          )
          save()
          libkrbn_launch_menu()
        }
      }
    }

    @Published var showProfileNameInMenuBar: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(
            libkrbnCoreConfiguration, showProfileNameInMenuBar
          )
          save()
          libkrbn_launch_menu()
        }
      }
    }

    @Published var askForConfirmationBeforeQuitting: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_ask_for_confirmation_before_quitting(
            libkrbnCoreConfiguration, askForConfirmationBeforeQuitting
          )
          save()
        }
      }
    }

    @Published var unsafeUI: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_global_configuration_unsafe_ui(
            libkrbnCoreConfiguration, unsafeUI
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
