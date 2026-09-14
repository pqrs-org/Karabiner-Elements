import SwiftUI

// Use explicit external-JSON lookup for application-owned translations. The
// system's own controls and ordinary user-provided strings keep their behavior.
struct AppLocalizedText: View {
  @ObservedObject private var localization = AppLocalization.shared
  @Environment(\.locale) private var locale
  let key: String

  let arguments: [String: String]

  init(_ key: String, arguments: [String: String] = [:]) {
    self.key = key
    self.arguments = arguments
  }

  var body: some View {
    Text(verbatim: AppLanguage.text(key, locale: locale, arguments: arguments))
  }
}

struct AppLocalizedLabel: View {
  let key: String
  let systemImage: String

  let arguments: [String: String]

  init(_ key: String, systemImage: String, arguments: [String: String] = [:]) {
    self.key = key
    self.systemImage = systemImage
    self.arguments = arguments
  }

  var body: some View {
    Label {
      AppLocalizedText(key, arguments: arguments)
    } icon: {
      Image(systemName: systemImage)
    }
  }
}

// For APIs that require a String (search fields, tooltips, and custom controls).
// Keeping locale and catalog as dynamic properties refreshes these strings too.
@MainActor
@propertyWrapper
struct AppLocalizationContext: DynamicProperty {
  @ObservedObject private var localization = AppLocalization.shared
  @Environment(\.locale) private var locale

  var wrappedValue: Lookup { Lookup(locale: locale, catalog: localization.catalog) }

  @MainActor
  struct Lookup {
    let locale: Locale
    let catalog: LocalizationCatalog

    func callAsFunction(_ key: String, arguments: [String: String] = [:]) -> String {
      AppLanguage.text(key, locale: locale, catalog: catalog, arguments: arguments)
    }
  }
}
