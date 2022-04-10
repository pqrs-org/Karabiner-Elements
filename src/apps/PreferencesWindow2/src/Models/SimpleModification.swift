class SimpleModification: Identifiable {
  var id = UUID()
  var fromJsonString: String?
  var toJsonString: String?

  init(
    _ fromJsonString: String,
    _ toJsonString: String
  ) {
      self.fromJsonString = SimpleModification.formatCompactJsonString(fromJsonString)
      self.toJsonString = SimpleModification.formatCompactJsonString(toJsonString)
  }

  static func formatCompactJsonString(_ jsonString: String) -> String? {
    if let jsonData = jsonString.data(using: .utf8) {
      if let jsonDict = try? JSONSerialization.jsonObject(with: jsonData, options: []) {
        if let compactJsonData = try? JSONSerialization.data(
          withJSONObject: jsonDict,
          options: [.sortedKeys, .withoutEscapingSlashes]
        ) {
          return String(data: compactJsonData, encoding: .utf8)
        }
      }
    }

    return nil
  }
}
