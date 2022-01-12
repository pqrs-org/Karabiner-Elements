import Combine
import Foundation
import SwiftUI

private func callback(_ initializedCoreConfiguration: UnsafeMutableRawPointer?,
                      _ context: UnsafeMutableRawPointer?)
{
    if initializedCoreConfiguration == nil { return }
    if context == nil { return }

    let obj: Settings! = unsafeBitCast(context, to: Settings.self)

    DispatchQueue.main.async { [weak obj] in
        guard let obj = obj else { return }

        obj.updateProperties(initializedCoreConfiguration)
    }
}

final class Settings: ObservableObject {
    static let shared = Settings()

    private var libkrbnCoreConfiguration: UnsafeMutableRawPointer?
    private var didSetEnabled = false

    init() {
        updateProperties(nil)
        didSetEnabled = true

        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_configuration_monitor(callback, obj)

        NotificationCenter.default.addObserver(forName: ConnectedDevices.didConnectedDevicesUpdate,
                                               object: nil,
                                               queue: .main)
        { [weak self] _ in
            guard let self = self else { return }

            self.updateConnectedDeviceSettings()
        }
    }

    private func save() {
        print("save")
        libkrbn_core_configuration_save(libkrbnCoreConfiguration)
    }

    public func updateProperties(_ initializedCoreConfiguration: UnsafeMutableRawPointer?) {
        didSetEnabled = false

        libkrbnCoreConfiguration = initializedCoreConfiguration

        updateConnectedDeviceSettings()

        virtualHIDKeyboardCountryCode = Int(libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_country_code(libkrbnCoreConfiguration))
        virtualHIDKeyboardMouseKeyXYScale = Int(libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(libkrbnCoreConfiguration))
        virtualHIDKeyboardIndicateStickyModifierKeysState = libkrbn_core_configuration_get_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(libkrbnCoreConfiguration)

        updateProfiles()

        checkForUpdatesOnStartup = libkrbn_core_configuration_get_global_configuration_check_for_updates_on_startup(libkrbnCoreConfiguration)
        showIconInMenuBar = libkrbn_core_configuration_get_global_configuration_show_in_menu_bar(libkrbnCoreConfiguration)
        showProfileNameInMenuBar = libkrbn_core_configuration_get_global_configuration_show_profile_name_in_menu_bar(libkrbnCoreConfiguration)

        updateSystemDefaultProfileExists()

        didSetEnabled = true
    }

    //
    // Connected Device Settings
    //

    @Published var connectedDeviceSettings: [ConnectedDeviceSetting] = []

    private func updateConnectedDeviceSettings() {
        var newConnectedDeviceSettings: [ConnectedDeviceSetting] = []

        ConnectedDevices.shared.connectedDevices.forEach { connectedDevice in
            let ignore = libkrbn_core_configuration_get_selected_profile_device_ignore2(libkrbnCoreConfiguration,
                                                                                        connectedDevice.vendorId,
                                                                                        connectedDevice.productId,
                                                                                        connectedDevice.isKeyboard,
                                                                                        connectedDevice.isPointingDevice)
            let manipulateCapsLockLed = libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led2(libkrbnCoreConfiguration,
                                                                                                                         connectedDevice.vendorId,
                                                                                                                         connectedDevice.productId,
                                                                                                                         connectedDevice.isKeyboard,
                                                                                                                         connectedDevice.isPointingDevice)
            newConnectedDeviceSettings.append(
                ConnectedDeviceSetting(connectedDevice: connectedDevice,
                                       ignore: ignore,
                                       manipulateCapsLockLed: manipulateCapsLockLed))
        }

        connectedDeviceSettings = newConnectedDeviceSettings
    }

    //
    // Virtual Keybaord
    //

    @Published var virtualHIDKeyboardCountryCode: Int = 0 {
        didSet {
            if didSetEnabled {
                libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_country_code(libkrbnCoreConfiguration, UInt8(virtualHIDKeyboardCountryCode))
                save()
            }
        }
    }

    @Published var virtualHIDKeyboardMouseKeyXYScale: Int = 0 {
        didSet {
            if didSetEnabled {
                libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_mouse_key_xy_scale(libkrbnCoreConfiguration, Int32(virtualHIDKeyboardMouseKeyXYScale))
                save()
            }
        }
    }

    @Published var virtualHIDKeyboardIndicateStickyModifierKeysState: Bool = false {
        didSet {
            if didSetEnabled {
                libkrbn_core_configuration_set_selected_profile_virtual_hid_keyboard_indicate_sticky_modifier_keys_state(libkrbnCoreConfiguration, virtualHIDKeyboardIndicateStickyModifierKeysState)
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
        for i in 0 ..< size {
            let profile = Profile(i,
                                  String(cString: libkrbn_core_configuration_get_profile_name(libkrbnCoreConfiguration, i)),
                                  libkrbn_core_configuration_get_profile_selected(libkrbnCoreConfiguration, i))
            newProfiles.append(profile)
        }

        profiles = newProfiles
    }

    public func selectProfile(_ profile: Profile) {
        libkrbn_core_configuration_select_profile(libkrbnCoreConfiguration, profile.index)
        save()

        updateProfiles()
    }

    public func updateProfileName(_ profile: Profile, _ name: String) {
        libkrbn_core_configuration_set_profile_name(libkrbnCoreConfiguration, profile.index, name.cString(using: .utf8))
        save()

        updateProfiles()
    }

    public func appendProfile() {
        libkrbn_core_configuration_push_back_profile(libkrbnCoreConfiguration)
        save()

        updateProfiles()
    }

    public func removeProfile(_ profile: Profile) {
        libkrbn_core_configuration_erase_profile(libkrbnCoreConfiguration, profile.index)
        save()

        updateProfiles()
    }

    //
    // Misc
    //

    @Published var checkForUpdatesOnStartup: Bool = false {
        didSet {
            if didSetEnabled {
                libkrbn_core_configuration_set_global_configuration_check_for_updates_on_startup(libkrbnCoreConfiguration, checkForUpdatesOnStartup)
                save()
            }
        }
    }

    @Published var showIconInMenuBar: Bool = false {
        didSet {
            if didSetEnabled {
                libkrbn_core_configuration_set_global_configuration_show_in_menu_bar(libkrbnCoreConfiguration, showIconInMenuBar)
                save()
                libkrbn_launch_menu()
            }
        }
    }

    @Published var showProfileNameInMenuBar: Bool = false {
        didSet {
            if didSetEnabled {
                libkrbn_core_configuration_set_global_configuration_show_profile_name_in_menu_bar(libkrbnCoreConfiguration, showProfileNameInMenuBar)
                save()
                libkrbn_launch_menu()
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

        let url = URL(fileURLWithPath: "/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/copy_current_profile_to_system_default_profile.applescript")
        guard let script = NSAppleScript(contentsOf: url, error: nil) else { return }
        script.executeAndReturnError(nil)

        updateSystemDefaultProfileExists()
    }

    func removeSystemDefaultProfile() {
        let url = URL(fileURLWithPath: "/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/remove_system_default_profile.applescript")
        guard let script = NSAppleScript(contentsOf: url, error: nil) else { return }
        script.executeAndReturnError(nil)

        updateSystemDefaultProfileExists()
    }
}
