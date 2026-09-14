import SwiftUI

@MainActor
public struct SimpleModificationDefinitions {
  public static let shared = SimpleModificationDefinitions()

  private(set) var fromCategories: SimpleModificationDefinitionCategories
  private(set) var toCategories: SimpleModificationDefinitionCategories
  private(set) var toCategoriesWithInheritBase: SimpleModificationDefinitionCategories

  init(data: Data? = nil) {
    fromCategories = SimpleModificationDefinitionCategories()
    toCategories = SimpleModificationDefinitionCategories()
    toCategoriesWithInheritBase = SimpleModificationDefinitionCategories()

    let resourceData =
      data
      ?? Bundle.main.url(forResource: "simple_modifications", withExtension: "json")
      .flatMap { try? Data(contentsOf: $0) }
    if let jsonData = resourceData {
      if let jsonDict =
        try? JSONSerialization.jsonObject(with: jsonData, options: []) as? [[String: Any]]
      {
        var fromCategory = SimpleModificationDefinitionCategory("")
        var toCategory = SimpleModificationDefinitionCategory("")

        jsonDict.forEach { jsonEntry in
          if jsonEntry["category_localization_key"] != nil || jsonEntry["category"] != nil {
            if fromCategory.entries.count > 0 {
              fromCategories.categories.append(fromCategory)
            }

            if toCategory.entries.count > 0 {
              toCategories.categories.append(toCategory)
            }

            guard
              let name = (jsonEntry["category_localization_key"] ?? jsonEntry["category"])
                as? String
            else { return }

            fromCategory = SimpleModificationDefinitionCategory(name)
            toCategory = SimpleModificationDefinitionCategory(name)
          } else {
            guard let label = (jsonEntry["label_localization_key"] ?? jsonEntry["label"]) as? String
            else { return }
            guard let data = jsonEntry["data"] as? [Any] else { return }

            let notFrom = jsonEntry["not_from"] as? Bool ?? false
            let unsafeFrom = jsonEntry["unsafe_from"] as? Bool ?? false
            let notTo = jsonEntry["not_to"] as? Bool ?? false

            if !notFrom {
              if data.count > 0 {
                if let canonicalDataJson = CanonicalJSON.string(fromJSONObject: data[0]) {
                  fromCategory.entries.append(
                    SimpleModificationDefinitionEntry(label, canonicalDataJson, unsafeFrom))
                }
              }
            }
            if !notTo {
              if let canonicalDataJson = CanonicalJSON.string(fromJSONObject: data) {
                toCategory.entries.append(
                  SimpleModificationDefinitionEntry(label, canonicalDataJson, false))
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

    //
    // Make toCategoriesWithInheritBase
    //

    do {
      var inheritBaseKeyCategory = SimpleModificationDefinitionCategory(
        "settings.key_picker.inherit_category")
      inheritBaseKeyCategory.entries.append(
        SimpleModificationDefinitionEntry(
          "settings.key_picker.inherit", "[]", false))

      toCategoriesWithInheritBase.categories.append(inheritBaseKeyCategory)
      toCategories.categories.forEach { category in
        toCategoriesWithInheritBase.categories.append(category)
      }
    }
  }
}
