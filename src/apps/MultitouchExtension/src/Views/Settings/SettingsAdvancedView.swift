import SwiftUI

struct SettingsAdvancedView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: AppLocalizedText("multitouch_extension.advanced.sleep")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              Toggle(isOn: $userSettings.relaunchAfterWakeUpFromSleep) {
                AppLocalizedText([
                  "multitouch_extension.advanced.relaunch",
                  " ",
                  .init(
                    "settings.general.defaults.value",
                    arguments: ["value": localized("shared.value.on")]),
                ])
              }
              .switchToggleStyle()
            }

            HStack {
              AppLocalizedText("multitouch_extension.advanced.relaunch_wait")

              IntTextField(
                value: $userSettings.relaunchWait,
                range: 0...10,
                step: 1,
                width: 40
              )
              .disabled(!userSettings.relaunchAfterWakeUpFromSleep)

              AppLocalizedText("multitouch_extension.units.seconds")
              AppLocalizedText("settings.general.defaults.value", arguments: ["value": "3"])
            }
          }
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }

      GroupBox(label: AppLocalizedText("multitouch_extension.advanced.delay")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              AppLocalizedText("multitouch_extension.advanced.touch_delay")

              IntTextField(
                value: $userSettings.delayBeforeTurnOn,
                range: 0...10000,
                step: 100,
                width: 80)

              AppLocalizedText("settings.general.units.milliseconds")
              AppLocalizedText("settings.general.defaults.value", arguments: ["value": "0"])
            }

            AppLocalizedText("multitouch_extension.advanced.touch_hint")
              .fixedSize(horizontal: false, vertical: true)
          }

          VStack(alignment: .leading) {
            HStack {
              AppLocalizedText("multitouch_extension.advanced.release_delay")

              IntTextField(
                value: $userSettings.delayBeforeTurnOff,
                range: 0...10000,
                step: 100,
                width: 80)

              AppLocalizedText("settings.general.units.milliseconds")
              AppLocalizedText("settings.general.defaults.value", arguments: ["value": "0"])
            }

            AppLocalizedText("multitouch_extension.advanced.release_hint")
              .fixedSize(horizontal: false, vertical: true)
          }
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }

      GroupBox(label: AppLocalizedText("multitouch_extension.advanced.palm")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              AppLocalizedText("multitouch_extension.advanced.touch_size_threshold")

              DoubleTextField(
                value: $userSettings.palmThreshold,
                range: 0...10,
                step: 0.5,
                maximumFractionDigits: 3,
                width: 80)

              AppLocalizedText("settings.general.defaults.value", arguments: ["value": "2"])
            }

            AppLocalizedText("multitouch_extension.advanced.palm_hint")
              .fixedSize(horizontal: false, vertical: true)
          }

        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
  }
}
