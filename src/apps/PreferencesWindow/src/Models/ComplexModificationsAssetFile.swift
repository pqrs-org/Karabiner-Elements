class ComplexModificationsAssetFile: Identifiable {
  var id = UUID()
  var index: Int
  var title: String
  var userFile: Bool
  var importedAt: Date
  var assetRules: [ComplexModificationsAssetRule]

  init(
    _ index: Int,
    _ title: String,
    _ userFile: Bool,
    _ importedAt: Date,
    _ assetRules: [ComplexModificationsAssetRule]
  ) {
    self.index = index
    self.title = title
    self.userFile = userFile
    self.importedAt = importedAt
    self.assetRules = assetRules
  }
}
