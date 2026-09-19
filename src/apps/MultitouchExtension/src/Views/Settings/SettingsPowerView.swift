import SwiftUI

struct SettingsPowerView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: AppLocalizedText("multitouch.tab.power")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              Toggle(isOn: $userSettings.allowUserInteractiveActivity) {
                AppLocalizedText([
                  "multitouch.power.enable",
                  " ",
                  .init(
                    "settings.defaults.value",
                    arguments: ["value": localized("value.off")]
                  ),
                ])
                .fixedSize(horizontal: false, vertical: true)
              }
              .switchToggleStyle()
            }

            AppLocalizedLabel(
              "multitouch.power.warning",
              systemImage: WarningBorder.icon
            )
            .fixedSize(horizontal: false, vertical: true)
            .modifier(WarningBorder())
          }

          if userSettings.allowUserInteractiveActivity {
            HStack {
              Toggle(isOn: $userSettings.keepUserInteractiveActivityDuringDisplaySleep) {
                AppLocalizedText([
                  "multitouch.power.display_sleep",
                  " ",
                  .init("settings.defaults.value", arguments: ["value": localized("value.off")]),
                ])
                .fixedSize(horizontal: false, vertical: true)
              }
              .switchToggleStyle()
            }
          }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding()
      }
    }
  }
}
