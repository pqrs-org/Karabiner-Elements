import SwiftUI

struct SettingsAdvancedView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var userSettings = UserSettings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 25.0) {
      GroupBox(label: AppLocalizedText("multitouch.advanced.sleep")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              Toggle(isOn: $userSettings.relaunchAfterWakeUpFromSleep) {
                AppLocalizedText([
                  "multitouch.advanced.relaunch",
                  " ",
                  .init("settings.defaults.value", arguments: ["value": localized("value.on")]),
                ])
              }
              .switchToggleStyle()
            }

            HStack {
              AppLocalizedText("multitouch.advanced.relaunch_wait")

              IntTextField(
                value: $userSettings.relaunchWait,
                range: 0...10,
                step: 1,
                width: 40
              )
              .disabled(!userSettings.relaunchAfterWakeUpFromSleep)

              AppLocalizedText("multitouch.units.seconds")
              AppLocalizedText("settings.defaults.value", arguments: ["value": "3"])
            }
          }
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }

      GroupBox(label: AppLocalizedText("multitouch.advanced.delay")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              AppLocalizedText("multitouch.advanced.touch_delay")

              IntTextField(
                value: $userSettings.delayBeforeTurnOn,
                range: 0...10000,
                step: 100,
                width: 80)

              AppLocalizedText("settings.units.milliseconds")
              AppLocalizedText("settings.defaults.value", arguments: ["value": "0"])
            }

            AppLocalizedText("multitouch.advanced.touch_hint")
              .fixedSize(horizontal: false, vertical: true)
          }

          VStack(alignment: .leading) {
            HStack {
              AppLocalizedText("multitouch.advanced.release_delay")

              IntTextField(
                value: $userSettings.delayBeforeTurnOff,
                range: 0...10000,
                step: 100,
                width: 80)

              AppLocalizedText("settings.units.milliseconds")
              AppLocalizedText("settings.defaults.value", arguments: ["value": "0"])
            }

            AppLocalizedText("multitouch.advanced.release_hint")
              .fixedSize(horizontal: false, vertical: true)
          }
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }

      GroupBox(label: AppLocalizedText("multitouch.advanced.palm")) {
        VStack(alignment: .leading, spacing: 30.0) {
          VStack(alignment: .leading) {
            HStack {
              AppLocalizedText("multitouch.advanced.touch_size_threshold")

              DoubleTextField(
                value: $userSettings.palmThreshold,
                range: 0...10,
                step: 0.5,
                maximumFractionDigits: 3,
                width: 80)

              AppLocalizedText("settings.defaults.value", arguments: ["value": "2"])
            }

            AppLocalizedText("multitouch.advanced.palm_hint")
              .fixedSize(horizontal: false, vertical: true)
          }

        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
  }
}
