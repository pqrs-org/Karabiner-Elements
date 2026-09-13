import SwiftUI

// Use explicit external-JSON lookup for application-owned translations. The
// system's own controls and ordinary user-provided strings keep their behavior.
struct AppLocalizedText: View {
  @ObservedObject private var localization = AppLocalization.shared
  @Environment(\.locale) private var locale
  let key: String

  init(_ key: String) { self.key = key }

  var body: some View {
    Text(verbatim: AppLanguage.text(key, locale: locale))
  }
}

struct AppLocalizedLabel: View {
  let key: String
  let systemImage: String

  init(_ key: String, systemImage: String) {
    self.key = key
    self.systemImage = systemImage
  }

  var body: some View {
    Label {
      AppLocalizedText(key)
    } icon: {
      Image(systemName: systemImage)
    }
  }
}
