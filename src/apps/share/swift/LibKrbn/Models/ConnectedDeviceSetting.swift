import Foundation

extension LibKrbn {
  final class ConnectedDeviceSetting: Identifiable {
    private var didSetEnabled = false

    var id = UUID()
    var connectedDevice: ConnectedDevice

    init(_ connectedDevice: ConnectedDevice) {
      self.connectedDevice = connectedDevice

      let ignore = libkrbn_core_configuration_get_selected_profile_device_ignore(
        Settings.shared.libkrbnCoreConfiguration,
        connectedDevice.libkrbnDeviceIdentifiers)
      modifyEvents = !ignore

      self.manipulateCapsLockLed =
        libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.treatAsBuiltInKeyboard =
        libkrbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.disableBuiltInKeyboardIfExists =
        libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      simpleModifications = LibKrbn.Settings.shared.makeSimpleModifications(connectedDevice)
      fnFunctionKeys = LibKrbn.Settings.shared.makeFnFunctionKeys(connectedDevice)

      didSetEnabled = true
    }

    @Published var modifyEvents: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_ignore(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            !modifyEvents)
          Settings.shared.save()
        }
      }
    }

    @Published var manipulateCapsLockLed: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            manipulateCapsLockLed)
          Settings.shared.save()
        }
      }
    }

    @Published var treatAsBuiltInKeyboard: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            treatAsBuiltInKeyboard)
          Settings.shared.save()
        }
      }
    }

    @Published var disableBuiltInKeyboardIfExists: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            disableBuiltInKeyboardIfExists)
          Settings.shared.save()
        }
      }
    }

    @Published var simpleModifications: [SimpleModification] = []
    @Published var fnFunctionKeys: [SimpleModification] = []
  }
}
