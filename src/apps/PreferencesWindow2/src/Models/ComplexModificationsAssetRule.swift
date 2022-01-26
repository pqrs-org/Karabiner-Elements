class ComplexModificationsAssetRule: Identifiable {
  var id = UUID()
  var fileIndex: Int
  var ruleIndex: Int
  var description: String

  init(_ fileIndex: Int, _ ruleIndex: Int, _ description: String) {
    self.fileIndex = fileIndex
    self.ruleIndex = ruleIndex
    self.description = description
  }
}
