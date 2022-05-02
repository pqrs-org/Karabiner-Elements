class ConnectedDeviceSetting: Identifiable {
  private var didSetEnabled = false

  var id = UUID()
  var connectedDevice: ConnectedDevice

  init(_ connectedDevice: ConnectedDevice) {
    self.connectedDevice = connectedDevice

    let ignore = libkrbn_core_configuration_get_selected_profile_device_ignore2(
      Settings.shared.libkrbnCoreConfiguration,
      connectedDevice.vendorId,
      connectedDevice.productId,
      connectedDevice.isKeyboard,
      connectedDevice.isPointingDevice)
    modifyEvents = !ignore

    let manipulateCapsLockLed =
      libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led2(
        Settings.shared.libkrbnCoreConfiguration,
        connectedDevice.vendorId,
        connectedDevice.productId,
        connectedDevice.isKeyboard,
        connectedDevice.isPointingDevice)
    self.manipulateCapsLockLed = manipulateCapsLockLed

    let disableBuiltInKeyboardIfExists =
      libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists2(
        Settings.shared.libkrbnCoreConfiguration,
        connectedDevice.vendorId,
        connectedDevice.productId,
        connectedDevice.isKeyboard,
        connectedDevice.isPointingDevice)
    self.disableBuiltInKeyboardIfExists = disableBuiltInKeyboardIfExists

    simpleModifications = Settings.shared.makeSimpleModifications(connectedDevice)
    fnFunctionKeys = Settings.shared.makeFnFunctionKeys(connectedDevice)

    didSetEnabled = true
  }

  @Published var modifyEvents: Bool = false {
    didSet {
      if didSetEnabled {
        libkrbn_core_configuration_set_selected_profile_device_ignore2(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.vendorId,
          connectedDevice.productId,
          connectedDevice.isKeyboard,
          connectedDevice.isPointingDevice,
          !modifyEvents)
        Settings.shared.save()
      }
    }
  }

  @Published var manipulateCapsLockLed: Bool = false {
    didSet {
      if didSetEnabled {
        libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led2(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.vendorId,
          connectedDevice.productId,
          connectedDevice.isKeyboard,
          connectedDevice.isPointingDevice,
          manipulateCapsLockLed)
        Settings.shared.save()
      }
    }
  }

  @Published var disableBuiltInKeyboardIfExists: Bool = false {
    didSet {
      if didSetEnabled {
        libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists2(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.vendorId,
          connectedDevice.productId,
          connectedDevice.isKeyboard,
          connectedDevice.isPointingDevice,
          disableBuiltInKeyboardIfExists)
        Settings.shared.save()
      }
    }
  }

  @Published var simpleModifications: [SimpleModification] = []
  @Published var fnFunctionKeys: [SimpleModification] = []
}
