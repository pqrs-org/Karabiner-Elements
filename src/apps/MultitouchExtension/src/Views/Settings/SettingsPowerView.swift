import SwiftUI

struct SettingsPowerView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: AppLocalizedText("multitouch.tab.power")) {
        VStack(alignment: .leading, spacing: 10.0) {
          HStack {
            Toggle(isOn: $userSettings.allowUserInteractiveActivity) {
              AppLocalizedText("multitouch.power.enable")
            }
            .switchToggleStyle()

            AppLocalizedText(
              "settings.defaults.value", arguments: ["value": localized("value.off")])
          }

          AppLocalizedText("multitouch.power.warning")
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding()
      }

      GroupBox(label: AppLocalizedText("multitouch.power.options")) {
        VStack(alignment: .leading, spacing: 10.0) {
          HStack {
            Toggle(isOn: $userSettings.keepUserInteractiveActivityDuringDisplaySleep) {
              AppLocalizedText("multitouch.power.display_sleep")
            }
            .switchToggleStyle()

            AppLocalizedText(
              "settings.defaults.value", arguments: ["value": localized("value.off")])
          }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding()
      }
      .disabled(!userSettings.allowUserInteractiveActivity)
    }
  }
}
