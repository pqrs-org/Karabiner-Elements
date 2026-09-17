import SwiftUI

struct ExpertView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var settings = Settings.shared

  private var defaults: SettingsConfiguration.Defaults {
    settings.configuration.defaultConfiguration
  }

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: AppLocalizedText("settings.expert.title")) {
          VStack(alignment: .leading, spacing: 4.0) {
            Toggle(isOn: $settings.configuration.globalConfiguration.unsafeUi) {
              AppLocalizedText([
                "setting.unsafe_ui",
                " ",
                .init(
                  "settings.defaults.value",
                  arguments: [
                    "value": localized(
                      defaults.globalConfiguration.unsafeUi ? "value.on" : "value.off")
                  ]),
              ])
            }
            .switchToggleStyle()

            AppLocalizedLabel(
              "settings.expert.unsafe_description",
              systemImage: WarningBorder.icon
            )
            .modifier(WarningBorder())
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.expert.pointing_devices")) {
          Toggle(
            isOn: $settings.configuration.selectedProfile
              .modifyPointingDeviceEventsByDefault
          ) {
            AppLocalizedText([
              "settings.expert.modify_pointing_by_default",
              " ",
              .init(
                "settings.defaults.value",
                arguments: [
                  "value": localized(
                    defaults.selectedProfile.modifyPointingDeviceEventsByDefault
                      ? "value.on" : "value.off")
                ]),
            ])
          }
          .switchToggleStyle()
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.expert.cgeventtap_fallback")) {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(isOn: $settings.configuration.globalConfiguration.enableCgeventtapFallback) {
                AppLocalizedText([
                  "setting.enable_cgeventtap_fallback",
                  " ",
                  .init(
                    "settings.defaults.value",
                    arguments: [
                      "value": localized(
                        defaults.globalConfiguration.enableCgeventtapFallback
                          ? "value.on" : "value.off")
                    ]),
                ])
              }
              .switchToggleStyle()

              AppLocalizedLabel(
                "settings.expert.fallback_description",
                systemImage: InfoBorder.icon
              )
              .modifier(InfoBorder())
              .fixedSize(horizontal: false, vertical: true)
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.expert.options")) {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(
                isOn: $settings.configuration.globalConfiguration
                  .filterUselessEventsFromSpecificDevices
              ) {
                AppLocalizedText([
                  "setting.filter_useless_events_from_specific_devices",
                  " ",
                  .init(
                    "settings.defaults.value",
                    arguments: [
                      "value": localized(
                        defaults.globalConfiguration.filterUselessEventsFromSpecificDevices
                          ? "value.on" : "value.off")
                    ]),
                ])
              }
              .switchToggleStyle()

              AppLocalizedLabel(
                "settings.expert.filter_description",
                systemImage: InfoBorder.icon
              )
              .modifier(InfoBorder())
              .fixedSize(horizontal: false, vertical: true)
            }

            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(
                isOn: $settings.configuration.globalConfiguration
                  .reorderSameTimestampInputEventsToPrioritizeModifiers
              ) {
                AppLocalizedText([
                  "setting.reorder_same_timestamp_input_events_to_prioritize_modifiers",
                  " ",
                  .init(
                    "settings.defaults.value",
                    arguments: [
                      "value": localized(
                        defaults.globalConfiguration
                          .reorderSameTimestampInputEventsToPrioritizeModifiers
                          ? "value.on" : "value.off")
                    ]),
                ])
              }
              .switchToggleStyle()

              AppLocalizedLabel(
                "settings.expert.modifier_order_hint",
                systemImage: InfoBorder.icon
              )
              .modifier(InfoBorder())
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("setting.delay_milliseconds_before_open_device")) {
          VStack(alignment: .leading, spacing: 4.0) {
            HStack {
              IntTextField(
                value: $settings.configuration.selectedProfile.parameters
                  .delayMillisecondsBeforeOpenDevice,
                range: 0...10000,
                step: 100,
                width: 50)

              AppLocalizedText("settings.units.milliseconds")
              AppLocalizedText(
                "settings.defaults.value",
                arguments: [
                  "value": String(
                    defaults.selectedProfile.parameters.delayMillisecondsBeforeOpenDevice)
                ])
            }

            AppLocalizedLabel(
              "settings.expert.device_delay_warning",
              systemImage: WarningBorder.icon
            )
            .modifier(WarningBorder())
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("setting.delay_milliseconds_before_sleep_shortcut")) {
          VStack(alignment: .leading, spacing: 4.0) {
            HStack {
              IntTextField(
                value: $settings.configuration.globalConfiguration
                  .delayMillisecondsBeforeSleepShortcut,
                range: 0...10000,
                step: 100,
                width: 50)

              AppLocalizedText("settings.units.milliseconds")
              AppLocalizedText(
                "settings.defaults.value",
                arguments: [
                  "value": String(defaults.globalConfiguration.delayMillisecondsBeforeSleepShortcut)
                ])
            }

            AppLocalizedText("settings.expert.sleep_delay_disabled_hint")

            AppLocalizedLabel(
              "settings.expert.sleep_delay_description",
              systemImage: InfoBorder.icon
            )
            .modifier(InfoBorder())
            .fixedSize(horizontal: false, vertical: true)
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
      .padding()
    }
  }
}
