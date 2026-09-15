import SwiftUI

// UserDefaults uses each application's bundle identifier, keeping this preference
// independent of other apps and of karabiner.json.
struct StoredAppLanguage: ViewModifier {
  enum PickerPlacement {
    case toolbar
    case contentTop
    case none
  }

  var pickerPlacement: PickerPlacement = .toolbar

  @AppStorage("uiLanguage") private var selection = "auto"
  @ObservedObject private var localization = AppLocalization.shared

  func body(content: Content) -> some View {
    Group {
      switch pickerPlacement {
      case .toolbar:
        content.toolbar {
          ToolbarItem(placement: .primaryAction) {
            languagePicker
          }
        }
      case .contentTop:
        // Keep the selector within the tab's content width, outside the
        // Settings scene's toolbar, which is reserved for tab navigation.
        VStack(alignment: .trailing, spacing: 12) {
          languagePicker
          content
        }
      case .none:
        content
      }
    }
    .environment(\.locale, AppLanguage.locale(for: selection))
    .onAppear { resetUnavailableLanguage() }
    .onChange(of: selection) { _ in resetUnavailableLanguage() }
    .onChange(of: localization.catalog.languages) { _ in resetUnavailableLanguage() }
  }

  private var languagePicker: some View {
    LanguagePicker(selection: $selection, languages: localization.catalog.languages)
      .fixedSize()
  }

  private func resetUnavailableLanguage() {
    guard !localization.catalog.strings.isEmpty else { return }
    if selection != "auto" && !localization.catalog.languages.contains(selection) {
      selection = "auto"
    }
  }
}
