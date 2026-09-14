import Foundation

// Shared JSON translations, installed separately from the signed applications.
@MainActor
enum AppLanguage {
  static var catalog: LocalizationCatalog { AppLocalization.shared.catalog }

  static func availableLanguages(catalog: LocalizationCatalog = AppLanguage.catalog) -> [String] {
    catalog.languages
  }

  static func displayName(for language: String) -> String {
    Locale(identifier: language).localizedString(forIdentifier: language) ?? language
  }

  static func locale(
    for selection: String, preferredLanguages: [String] = Locale.preferredLanguages,
    catalog: LocalizationCatalog = AppLanguage.catalog
  ) -> Locale {
    let preferences = selection == "auto" || selection.isEmpty ? preferredLanguages : [selection]
    // Use Foundation's language matching, including regional/script variants,
    // without loading any resource bundles. Unsupported preferences fall back to English.
    let language =
      Bundle.preferredLocalizations(
        from: catalog.languages, forPreferences: preferences + ["en"]
      ).first ?? "en"
    return Locale(identifier: language)
  }

  static func text(
    _ key: String, locale: Locale, catalog: LocalizationCatalog = AppLanguage.catalog,
    arguments: [String: String] = [:]
  ) -> String {
    let selected = Self.locale(for: locale.identifier, catalog: catalog).identifier
    guard let translations = catalog.strings[key] else { return key }
    let template = translations[selected] ?? translations["en"] ?? key
    // Substitute named placeholders once. Values are literal text, never format
    // specifiers, localization keys, or additional placeholders to expand.
    guard !arguments.isEmpty else { return template }
    let result = NSMutableString(string: template)
    let pattern = try! NSRegularExpression(pattern: #"\{([A-Za-z][A-Za-z0-9_]*)\}"#)
    for match in pattern.matches(in: template, range: NSRange(template.startIndex..., in: template))
      .reversed()
    {
      let name = (template as NSString).substring(with: match.range(at: 1))
      if let value = arguments[name] { result.replaceCharacters(in: match.range, with: value) }
    }
    return result as String
  }
}
