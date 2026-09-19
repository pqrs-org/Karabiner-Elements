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
                "settings.expert.unsafe_ui",
                " ",
                .init(
                  "settings.general.defaults.value",
                  arguments: [
                    "value": localized(
                      defaults.globalConfiguration.unsafeUi ? "shared.value.on" : "shared.value.off"
                    )
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
                "settings.general.defaults.value",
                arguments: [
                  "value": localized(
                    defaults.selectedProfile.modifyPointingDeviceEventsByDefault
                      ? "shared.value.on" : "shared.value.off")
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
                  "settings.expert.enable_cgeventtap_fallback",
                  " ",
                  .init(
                    "settings.general.defaults.value",
                    arguments: [
                      "value": localized(
                        defaults.globalConfiguration.enableCgeventtapFallback
                          ? "shared.value.on" : "shared.value.off")
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
                  "settings.expert.filter_useless_events_from_specific_devices",
                  " ",
                  .init(
                    "settings.general.defaults.value",
                    arguments: [
                      "value": localized(
                        defaults.globalConfiguration.filterUselessEventsFromSpecificDevices
                          ? "shared.value.on" : "shared.value.off")
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
                  "settings.expert.reorder_same_timestamp_input_events_to_prioritize_modifiers",
                  " ",
                  .init(
                    "settings.general.defaults.value",
                    arguments: [
                      "value": localized(
                        defaults.globalConfiguration
                          .reorderSameTimestampInputEventsToPrioritizeModifiers
                          ? "shared.value.on" : "shared.value.off")
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

        GroupBox(label: AppLocalizedText("settings.expert.delay_milliseconds_before_open_device")) {
          VStack(alignment: .leading, spacing: 4.0) {
            HStack {
              IntTextField(
                value: $settings.configuration.selectedProfile.parameters
                  .delayMillisecondsBeforeOpenDevice,
                range: 0...10000,
                step: 100,
                width: 50)

              AppLocalizedText("settings.general.units.milliseconds")
              AppLocalizedText(
                "settings.general.defaults.value",
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

        GroupBox(
          label: AppLocalizedText("settings.expert.delay_milliseconds_before_sleep_shortcut")
        ) {
          VStack(alignment: .leading, spacing: 4.0) {
            HStack {
              IntTextField(
                value: $settings.configuration.globalConfiguration
                  .delayMillisecondsBeforeSleepShortcut,
                range: 0...10000,
                step: 100,
                width: 50)

              AppLocalizedText("settings.general.units.milliseconds")
              AppLocalizedText(
                "settings.general.defaults.value",
                arguments: [
                  "value": String(defaults.globalConfiguration.delayMillisecondsBeforeSleepShortcut)
                ])
              AppLocalizedText("settings.expert.sleep_delay_disabled_hint")
            }

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
