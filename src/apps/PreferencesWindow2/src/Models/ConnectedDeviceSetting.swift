class ConnectedDeviceSetting: Identifiable {
    var id = UUID()
    var connectedDevice: ConnectedDevice
    var ignore: Bool
    var manipulateCapsLockLed: Bool

    init(connectedDevice: ConnectedDevice,
         ignore: Bool,
         manipulateCapsLockLed: Bool)
    {
        self.connectedDevice = connectedDevice
        self.ignore = ignore
        self.manipulateCapsLockLed = manipulateCapsLockLed
    }
}
