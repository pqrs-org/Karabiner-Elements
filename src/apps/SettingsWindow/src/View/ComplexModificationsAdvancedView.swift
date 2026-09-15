import SwiftUI

struct ComplexModificationsAdvancedView: View {
  @ObservedObject private var settings = Settings.shared

  private var defaults: SettingsConfiguration.Defaults {
    settings.configuration.defaultConfiguration
  }

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: AppLocalizedText("settings.parameters.basic")) {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack {
              AppLocalizedText("setting.basic.to_if_alone_timeout_milliseconds")

              IntTextField(
                value: $settings.configuration.selectedProfile.complexModifications.parameters
                  .basicToIfAloneTimeoutMilliseconds,
                range: 0...10000,
                step: 100,
                width: 50)

              AppLocalizedText(
                "settings.defaults.value",
                arguments: [
                  "value": String(
                    defaults.selectedProfile.complexModifications.parameters
                      .basicToIfAloneTimeoutMilliseconds)
                ])
            }

            Divider()

            HStack {
              AppLocalizedText("setting.basic.to_if_held_down_threshold_milliseconds")

              IntTextField(
                value: $settings.configuration.selectedProfile.complexModifications.parameters
                  .basicToIfHeldDownThresholdMilliseconds,
                range: 0...10000,
                step: 100,
                width: 50)

              AppLocalizedText(
                "settings.defaults.value",
                arguments: [
                  "value": String(
                    defaults.selectedProfile.complexModifications.parameters
                      .basicToIfHeldDownThresholdMilliseconds)
                ])
            }

            Divider()

            HStack {
              AppLocalizedText("setting.basic.to_delayed_action_delay_milliseconds")

              IntTextField(
                value: $settings.configuration.selectedProfile.complexModifications.parameters
                  .basicToDelayedActionDelayMilliseconds,
                range: 0...10000,
                step: 100,
                width: 50)

              AppLocalizedText(
                "settings.defaults.value",
                arguments: [
                  "value": String(
                    defaults.selectedProfile.complexModifications.parameters
                      .basicToDelayedActionDelayMilliseconds)
                ])
            }

            Divider()

            HStack {
              AppLocalizedText("setting.basic.simultaneous_threshold_milliseconds")

              IntTextField(
                value: $settings.configuration.selectedProfile.complexModifications.parameters
                  .basicSimultaneousThresholdMilliseconds,
                range: 0...1000,
                step: 20,
                width: 50)

              AppLocalizedText(
                "settings.defaults.value",
                arguments: [
                  "value": String(
                    defaults.selectedProfile.complexModifications.parameters
                      .basicSimultaneousThresholdMilliseconds)
                ])
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.parameters.mouse_motion_to_scroll")) {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack {
              AppLocalizedText("setting.mouse_motion_to_scroll.speed")

              IntTextField(
                value: $settings.configuration.selectedProfile.complexModifications.parameters
                  .mouseMotionToScrollSpeed,
                range: 0...10000,
                step: 10,
                width: 50)

              AppLocalizedText("settings.units.percent")
              AppLocalizedText(
                "settings.defaults.value",
                arguments: [
                  "value": String(
                    defaults.selectedProfile.complexModifications.parameters
                      .mouseMotionToScrollSpeed)
                ])
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
