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
    _ key: String, locale: Locale, catalog: LocalizationCatalog = AppLanguage.catalog
  ) -> String {
    let selected = Self.locale(for: locale.identifier, catalog: catalog).identifier
    guard let translations = catalog.strings[key] else { return key }
    return translations[selected] ?? translations["en"] ?? key
  }
}
