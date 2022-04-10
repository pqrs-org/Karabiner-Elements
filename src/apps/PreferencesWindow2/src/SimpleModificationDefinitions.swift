import SwiftUI

public class SimpleModificationDefinitions: ObservableObject {
  public static let shared = SimpleModificationDefinitions()

  @Published var fromCategories: [SimpleModificationDefinitionCategory] = []
  @Published var toCategories: [SimpleModificationDefinitionCategory] = []

  init() {
    if let path = Bundle.main.path(forResource: "simple_modifications", ofType: "json") {
      if let jsonData = try? Data(contentsOf: URL(fileURLWithPath: path)) {
        if let jsonDict =
          try? JSONSerialization.jsonObject(with: jsonData, options: []) as? [[String: Any]]
        {
          var fromCategory = SimpleModificationDefinitionCategory("")
          var toCategory = SimpleModificationDefinitionCategory("")

          jsonDict.forEach { jsonEntry in
            if jsonEntry["category"] != nil {
              if fromCategory.entries.count > 0 {
                fromCategories.append(fromCategory)
              }

              if toCategory.entries.count > 0 {
                toCategories.append(toCategory)
              }

              guard let name = jsonEntry["category"] as? String else { return }

              fromCategory = SimpleModificationDefinitionCategory(name)
              toCategory = SimpleModificationDefinitionCategory(name)
            } else {
              guard let label = jsonEntry["label"] as? String else { return }
              guard let data = jsonEntry["data"] else { return }
              guard
                let compactDataJson = SimpleModification.formatCompactJsonString(jsonObject: data)
              else {
                return
              }

              let notFrom = jsonEntry["not_from"] as? Bool ?? false
              let notTo = jsonEntry["not_to"] as? Bool ?? false

              if !notFrom {
                fromCategory.entries.append(
                  SimpleModificationDefinitionEntry(label, compactDataJson))
              }
              if !notTo {
                toCategory.entries.append(SimpleModificationDefinitionEntry(label, compactDataJson))
              }
            }
          }

          if fromCategory.entries.count > 0 {
            fromCategories.append(fromCategory)
          }

          if toCategory.entries.count > 0 {
            toCategories.append(toCategory)
          }
        }
      }
    }
  }
}
