import Foundation

@MainActor
struct SimpleModificationDefinitionCategories: Identifiable {
  var id = UUID()

  var categories: [SimpleModificationDefinitionCategory] = []

  func findLabel(jsonString: String) -> String {
    if let canonicalJsonString = CanonicalJSON.string(fromJSONString: jsonString) {
      for category in categories {
        for entry in category.entries where entry.json == canonicalJsonString {
          return entry.label
        }
      }

      if canonicalJsonString == "{}" || canonicalJsonString == "[]" {
        return "---"
      }

      return canonicalJsonString
    }

    return jsonString
  }
}
