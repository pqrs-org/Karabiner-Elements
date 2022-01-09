class ConnectedDevice: Identifiable {
    var id = UUID()
    var index: Int
    var manufacturer: String
    var product: String
    var isBuiltInKeyboard: Bool
    var isBuiltInTrackpad: Bool
    var isBuiltInTouchBar: Bool

    init(index: Int,
         manufacturer: String,
         product: String,
         isBuiltInKeyboard: Bool,
         isBuiltInTrackpad: Bool,
         isBuiltInTouchBar: Bool)
    {
        self.index = index
        self.manufacturer = manufacturer
        self.product = product
        self.isBuiltInKeyboard = isBuiltInKeyboard
        self.isBuiltInTrackpad = isBuiltInTrackpad
        self.isBuiltInTouchBar = isBuiltInTouchBar
    }
}
