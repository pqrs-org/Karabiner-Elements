import SwiftUI

struct SettingsView: View {
  @AppLocalizationContext private var localized
  @EnvironmentObject private var userSettings: UserSettings

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      GroupBox(label: AppLocalizedText("event_viewer.settings.window_behavior")) {
        VStack(alignment: .leading, spacing: 12.0) {
          Toggle(isOn: $userSettings.forceStayTop) {
            AppLocalizedText([
              "event_viewer.settings.stay_on_top",
              " ",
              .init("settings.general.defaults.value", arguments: ["value": localized("shared.value.off")]),
            ])
          }
          .switchToggleStyle()

          Toggle(isOn: $userSettings.showInAllSpaces) {
            AppLocalizedText([
              "event_viewer.settings.all_spaces",
              " ",
              .init("settings.general.defaults.value", arguments: ["value": localized("shared.value.off")]),
            ])
          }
          .switchToggleStyle()
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
    .padding()
    .frame(maxHeight: .infinity, alignment: .top)
  }
}
