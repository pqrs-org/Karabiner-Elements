class ComplexModificationsAssetFile: Identifiable {
    var id = UUID()
    var index: Int
    var title: String
    var userFile: Bool
    var assetRules: [ComplexModificationsAssetRule]

    init(
        _ index: Int,
        _ title: String,
        _ userFile: Bool,
        _ assetRules: [ComplexModificationsAssetRule]
    ) {
        self.index = index
        self.title = title
        self.userFile = userFile
        self.assetRules = assetRules
    }
}
