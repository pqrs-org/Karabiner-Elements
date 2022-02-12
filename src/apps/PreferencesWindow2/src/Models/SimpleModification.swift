class SimpleModification: Identifiable {
  var id = UUID()
  var fromJsonString: String
  var toJsonString: String

  init(
    _ fromJsonString: String,
    _ toJsonString: String
  ) {
    self.fromJsonString = fromJsonString
    self.toJsonString = toJsonString
  }
}
