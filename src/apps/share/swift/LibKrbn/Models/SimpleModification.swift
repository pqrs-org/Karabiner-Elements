import Foundation

extension LibKrbn {
  @MainActor
  struct SimpleModification: Identifiable {
    var id = UUID()
    var index = -1
    var fromEntry: SimpleModificationDefinitionEntry
    var toEntry: SimpleModificationDefinitionEntry

    init(
      index: Int,
      fromJsonString: String,
      toJsonString: String,
      toCategories: SimpleModificationDefinitionCategories
    ) {
      self.index = index

      do {
        let s = SimpleModification.formatCompactJsonString(string: fromJsonString) ?? ""
        let label = SimpleModificationDefinitions.shared.fromCategories.findLabel(jsonString: s)
        self.fromEntry = SimpleModificationDefinitionEntry(label, s, false)
      }

      do {
        let s = SimpleModification.formatCompactJsonString(string: toJsonString) ?? ""
        let label = toCategories.findLabel(jsonString: s)
        self.toEntry = SimpleModificationDefinitionEntry(label, s, false)
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
}
