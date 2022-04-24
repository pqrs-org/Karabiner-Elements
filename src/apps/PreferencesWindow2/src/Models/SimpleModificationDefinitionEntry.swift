class SimpleModificationDefinitionEntry: Identifiable, Equatable {
  var id = UUID()

  var label: String
  var json: String

  init(
    _ label: String,
    _ json: String
  ) {
    self.label = label
    self.json = json
  }

  public static func == (lhs: SimpleModificationDefinitionEntry, rhs: SimpleModificationDefinitionEntry) -> Bool {
    lhs.label == rhs.label && lhs.json == rhs.json
  }
}
