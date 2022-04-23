class SimpleModificationDefinitionCategories: Identifiable {
  var categories: [SimpleModificationDefinitionCategory] = []

  func findLabel(jsonString: String) -> String {
    if let compactJsonString = SimpleModification.formatCompactJsonString(string: jsonString) {
      for category in categories {
        for entry in category.entries {
          if entry.json == compactJsonString {
            return entry.label
          }
        }
      }

      return compactJsonString
    }

    return jsonString
  }
}
