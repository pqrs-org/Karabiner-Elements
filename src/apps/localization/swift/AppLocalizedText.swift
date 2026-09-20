import SwiftUI

// Use explicit external-JSON lookup for application-owned translations. The
// system's own controls and ordinary user-provided strings keep their behavior.
// Prefer the full text height for descriptions and settings labels, including on macOS 13.
// Use AppLocalizedConstrainedText in controls that should follow their parent's sizing.
struct AppLocalizedText: View {
  typealias Segment = AppLocalizedConstrainedText.Segment

  private let text: AppLocalizedConstrainedText

  init(_ key: String, arguments: [String: String] = [:]) {
    text = AppLocalizedConstrainedText(key, arguments: arguments)
  }

  init(_ segments: [Segment]) {
    text = AppLocalizedConstrainedText(segments)
  }

  func localizedString(locale: Locale, catalog: LocalizationCatalog) -> String {
    text.localizedString(locale: locale, catalog: catalog)
  }

  var body: some View {
    text.fixedSize(horizontal: false, vertical: true)
  }
}

// Keeps Text's normal sizing behavior without imposing a line limit or truncation mode.
struct AppLocalizedConstrainedText: View {
  @ObservedObject private var localization = AppLocalization.shared
  @Environment(\.locale) private var locale
  struct Segment: ExpressibleByStringLiteral {
    let key: String
    let arguments: [String: String]

    init(_ key: String, arguments: [String: String] = [:]) {
      self.key = key
      self.arguments = arguments
    }

    init(stringLiteral value: String) {
      self.init(value)
    }
  }

  private let segments: [Segment]

  init(_ key: String, arguments: [String: String] = [:]) {
    self.init([Segment(key, arguments: arguments)])
  }

  init(_ segments: [Segment]) {
    self.segments = segments
  }

  func localizedString(locale: Locale, catalog: LocalizationCatalog) -> String {
    segments.map {
      AppLanguage.text($0.key, locale: locale, catalog: catalog, arguments: $0.arguments)
    }.joined()
  }

  var body: some View {
    Text(verbatim: localizedString(locale: locale, catalog: localization.catalog))
  }
}

struct AppLocalizedLabel: View {
  private let label: AppLocalizedConstrainedLabel

  init(_ key: String, systemImage: String, arguments: [String: String] = [:]) {
    label = AppLocalizedConstrainedLabel(key, systemImage: systemImage, arguments: arguments)
  }

  var body: some View {
    label.fixedSize(horizontal: false, vertical: true)
  }
}

// Use for buttons, menus, pickers and other size-constrained control labels.
struct AppLocalizedConstrainedLabel: View {
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
      AppLocalizedConstrainedText(key, arguments: arguments)
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
