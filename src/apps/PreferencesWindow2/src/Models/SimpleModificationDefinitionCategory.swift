class SimpleModificationDefinitionCategory: Identifiable {
  var id = UUID()

  var name: String
  var entries: [SimpleModificationDefinitionEntry] = []

  init(
    _ name: String
  ) {
    self.name = name
  }
}
