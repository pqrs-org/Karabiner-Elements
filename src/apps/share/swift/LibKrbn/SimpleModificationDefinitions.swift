import SwiftUI

extension LibKrbn {
  @MainActor
  public struct SimpleModificationDefinitions {
    public static let shared = SimpleModificationDefinitions()

    private(set) var fromCategories: SimpleModificationDefinitionCategories
    private(set) var toCategories: SimpleModificationDefinitionCategories
    private(set) var toCategoriesWithInheritBase: SimpleModificationDefinitionCategories

    init() {
      fromCategories = SimpleModificationDefinitionCategories()
      toCategories = SimpleModificationDefinitionCategories()
      toCategoriesWithInheritBase = SimpleModificationDefinitionCategories()

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
                  fromCategories.categories.append(fromCategory)
                }

                if toCategory.entries.count > 0 {
                  toCategories.categories.append(toCategory)
                }

                guard let name = jsonEntry["category"] as? String else { return }

                fromCategory = SimpleModificationDefinitionCategory(name)
                toCategory = SimpleModificationDefinitionCategory(name)
              } else {
                guard let label = jsonEntry["label"] as? String else { return }
                guard let data = jsonEntry["data"] as? [Any] else { return }

                let notFrom = jsonEntry["not_from"] as? Bool ?? false
                let unsafeFrom = jsonEntry["unsafe_from"] as? Bool ?? false
                let notTo = jsonEntry["not_to"] as? Bool ?? false

                if !notFrom {
                  if data.count > 0 {
                    if let compactDataJson = SimpleModification.formatCompactJsonString(
                      jsonObject: data[0])
                    {
                      fromCategory.entries.append(
                        SimpleModificationDefinitionEntry(label, compactDataJson, unsafeFrom))
                    }
                  }
                }
                if !notTo {
                  if let compactDataJson = SimpleModification.formatCompactJsonString(
                    jsonObject: data)
                  {
                    toCategory.entries.append(
                      SimpleModificationDefinitionEntry(label, compactDataJson, false))
                  }
                }
              }
            }

            if fromCategory.entries.count > 0 {
              fromCategories.categories.append(fromCategory)
            }

            if toCategory.entries.count > 0 {
              toCategories.categories.append(toCategory)
            }
          }
        }
      }

      //
      // Make toCategoriesWithInheritBase
      //

      do {
        var inheritBaseKeyCategory = SimpleModificationDefinitionCategory(
          "Inherit the base setting")
        inheritBaseKeyCategory.entries.append(
          SimpleModificationDefinitionEntry("--- (Inherit the base setting)", "[]", false))

        toCategoriesWithInheritBase.categories.append(inheritBaseKeyCategory)
        toCategories.categories.forEach { category in
          toCategoriesWithInheritBase.categories.append(category)
        }
      }
    }
  }
}
