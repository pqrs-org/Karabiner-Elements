class ConnectedDevice: Identifiable {
    var id = UUID()
    var index: Int
    var manufacturerName: String
    var productName: String
    var vendorId: UInt64
    var productId: UInt64
    var isKeyboard: Bool
    var isPointingDevice: Bool
    var isBuiltInKeyboard: Bool
    var isBuiltInTrackpad: Bool
    var isBuiltInTouchBar: Bool

    init(index: Int,
         manufacturerName: String,
         productName: String,
         vendorId: UInt64,
         productId: UInt64,
         isKeyboard: Bool,
         isPointingDevice: Bool,
         isBuiltInKeyboard: Bool,
         isBuiltInTrackpad: Bool,
         isBuiltInTouchBar: Bool)
    {
        self.index = index
        self.manufacturerName = manufacturerName
        self.productName = productName
        self.vendorId = vendorId
        self.productId = productId
        self.isKeyboard = isKeyboard
        self.isPointingDevice = isPointingDevice
        self.isBuiltInKeyboard = isBuiltInKeyboard
        self.isBuiltInTrackpad = isBuiltInTrackpad
        self.isBuiltInTouchBar = isBuiltInTouchBar
    }
}
