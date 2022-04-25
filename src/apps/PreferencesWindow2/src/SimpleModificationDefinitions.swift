import SwiftUI

public struct SimpleModificationDefinitions {
  public static let shared = SimpleModificationDefinitions()

  private(set) var fromCategories: SimpleModificationDefinitionCategories
  private(set) var toCategories: SimpleModificationDefinitionCategories
  private(set) var toCategoriesWithInheritDefault: SimpleModificationDefinitionCategories

  init() {
    fromCategories = SimpleModificationDefinitionCategories()
    toCategories = SimpleModificationDefinitionCategories()
    toCategoriesWithInheritDefault = SimpleModificationDefinitionCategories()

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
              let notTo = jsonEntry["not_to"] as? Bool ?? false

              if !notFrom {
                if data.count > 0 {
                  if let compactDataJson = SimpleModification.formatCompactJsonString(
                    jsonObject: data[0])
                  {
                    fromCategory.entries.append(
                      SimpleModificationDefinitionEntry(label, compactDataJson))
                  }
                }
              }
              if !notTo {
                if let compactDataJson = SimpleModification.formatCompactJsonString(
                  jsonObject: data)
                {
                  toCategory.entries.append(
                    SimpleModificationDefinitionEntry(label, compactDataJson))
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
    // Make toCategoriesWithInheritDefault
    //

    do {
      var inheritDefaultKeyCategory = SimpleModificationDefinitionCategory("Inherit default key")
      inheritDefaultKeyCategory.entries.append(
        SimpleModificationDefinitionEntry("--- (Inherit default key)", "[]"))

      toCategoriesWithInheritDefault.categories.append(inheritDefaultKeyCategory)
      toCategories.categories.forEach { category in
        toCategoriesWithInheritDefault.categories.append(category)
      }
    }
  }
}
