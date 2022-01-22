class ConnectedDeviceSetting: Identifiable {
    private var didSetEnabled = false

    var id = UUID()
    var connectedDevice: ConnectedDevice
    var manipulateCapsLockLed: Bool

    init(_ connectedDevice: ConnectedDevice) {
        self.connectedDevice = connectedDevice

        let ignore = libkrbn_core_configuration_get_selected_profile_device_ignore2(Settings.shared.libkrbnCoreConfiguration,
                                                                                    connectedDevice.vendorId,
                                                                                    connectedDevice.productId,
                                                                                    connectedDevice.isKeyboard,
                                                                                    connectedDevice.isPointingDevice)
        modifyEvents = !ignore

        let manipulateCapsLockLed = libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led2(Settings.shared.libkrbnCoreConfiguration,
                                                                                                                     connectedDevice.vendorId,
                                                                                                                     connectedDevice.productId,
                                                                                                                     connectedDevice.isKeyboard,
                                                                                                                     connectedDevice.isPointingDevice)
        self.manipulateCapsLockLed = manipulateCapsLockLed

        didSetEnabled = true
    }

    @Published var modifyEvents: Bool = false {
        didSet {
            if didSetEnabled {
                libkrbn_core_configuration_set_selected_profile_device_ignore2(Settings.shared.libkrbnCoreConfiguration,
                                                                               connectedDevice.vendorId,
                                                                               connectedDevice.productId,
                                                                               connectedDevice.isKeyboard,
                                                                               connectedDevice.isPointingDevice,
                                                                               !modifyEvents)
                Settings.shared.save()
            }
        }
    }
}
