struct SimpleModification: Identifiable {
  var id = UUID()
  var fromEntry: SimpleModificationDefinitionEntry
  var toEntry: SimpleModificationDefinitionEntry

  init(
    _ fromJsonString: String,
    _ toJsonString: String
  ) {
    do {
      let s = SimpleModification.formatCompactJsonString(string: fromJsonString) ?? ""
      self.fromEntry = SimpleModificationDefinitionEntry(
        SimpleModificationDefinitions.shared.fromCategories.findLabel(jsonString: s),
        s
      )
    }

    do {
      let s = SimpleModification.formatCompactJsonString(string: toJsonString) ?? ""
      self.toEntry = SimpleModificationDefinitionEntry(
        SimpleModificationDefinitions.shared.toCategoriesWithInheritBase.findLabel(jsonString: s),
        s
      )
    }
  }

  static func formatCompactJsonString(string jsonString: String) -> String? {
    if let jsonData = jsonString.data(using: .utf8) {
      if let jsonDict = try? JSONSerialization.jsonObject(with: jsonData, options: []) {
        return SimpleModification.formatCompactJsonString(jsonObject: jsonDict)
      }
    }

    return nil
  }

  static func formatCompactJsonString(jsonObject: Any) -> String? {
    if let compactJsonData = try? JSONSerialization.data(
      withJSONObject: jsonObject,
      options: [.fragmentsAllowed, .sortedKeys, .withoutEscapingSlashes]
    ) {
      return String(data: compactJsonData, encoding: .utf8)
    }

    return nil
  }
}
