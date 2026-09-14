import Foundation

@main
@MainActor
struct SimpleModificationDefinitionsTests {
  static func main() throws {
    let source = URL(
      fileURLWithPath: "../../../src/apps/SettingsWindow/Resources/simple_modifications.json")
    let data = try Data(contentsOf: source)
    let definitions = SimpleModificationDefinitions(data: data)
    let catalog = try LocalizationCatalog(
      directory: URL(fileURLWithPath: "../../../src/apps/localization/Resources"))
    let en = Locale(identifier: "en")
    let rows = try JSONSerialization.jsonObject(with: data) as! [[String: Any]]

    // Every key referenced by the bundled picker definitions must have a translation.
    for row in rows {
      for field in ["label_localization_key", "category_localization_key"] {
        if let key = row[field] as? String {
          precondition(catalog.strings[key]?["en"] != nil, "Missing translation: \(key)")
        }
      }
    }

    // Moving labels into the catalog must preserve from/to eligibility and unsafe flags.
    for row in rows where row["data"] != nil {
      let values = row["data"] as! [Any]
      let label = (row["label_localization_key"] ?? row["label"]) as! String
      if row["not_from"] as? Bool != true, let first = values.first {
        let json = CanonicalJSON.string(fromJSONObject: first)!
        let entries = definitions.fromCategories.categories.flatMap(\.entries)
        precondition(
          entries.contains {
            $0.label == label && $0.json == json
              && $0.unsafe == (row["unsafe_from"] as? Bool ?? false)
          })
      }
      if row["not_to"] as? Bool != true {
        let json = CanonicalJSON.string(fromJSONObject: values)!
        let entries = definitions.toCategories.categories.flatMap(\.entries)
        precondition(entries.contains { $0.label == label && $0.json == json && !$0.unsafe })
      }
    }

    // Selection uses the stable definition label; translation happens only when displayed.
    let fn = definitions.fromCategories.findLabel(
      jsonString: #"{"apple_vendor_top_case_key_code":"keyboard_fn"}"#)
    precondition(fn == "settings.key_picker.fn_globe")
    precondition(AppLanguage.text(fn, locale: en, catalog: catalog) == "fn (globe)")
    precondition(definitions.fromCategories.categories.contains { $0.include(label: fn) })

    // Literal key names and unrecognized configuration JSON must remain readable.
    let capsLock = definitions.fromCategories.findLabel(jsonString: #"{"key_code":"caps_lock"}"#)
    precondition(capsLock == "caps_lock")
    precondition(AppLanguage.text(capsLock, locale: en, catalog: catalog) == "caps_lock")
    let custom = #"{"key_code":"unknown_custom_key"}"#
    precondition(definitions.fromCategories.findLabel(jsonString: custom) == custom)

    // Inheriting the all-devices setting retains its empty-array value and localized title.
    let inherit = definitions.toCategoriesWithInheritBase.findLabel(jsonString: "[]")
    precondition(inherit == "settings.key_picker.inherit")
    precondition(
      AppLanguage.text(inherit, locale: en, catalog: catalog)
        == "--- (Use the \"For all devices\" setting)")
    print("Simple modification definition tests passed")
  }
}
