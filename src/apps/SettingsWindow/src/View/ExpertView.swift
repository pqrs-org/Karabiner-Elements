import SwiftUI

struct ExpertView: View {
  @ObservedObject private var settings = Settings.shared

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: AppLocalizedText("settings.expert.title")) {
          VStack(alignment: .leading, spacing: 4.0) {
            Toggle(isOn: $settings.configuration.globalConfiguration.unsafeUi) {
              HStack {
                AppLocalizedText("setting.unsafe_ui")
                AppLocalizedText("settings.defaults.off")
              }
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
            AppLocalizedText("settings.expert.modify_pointing_by_default")
          }
          .switchToggleStyle()
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.expert.cgeventtap_fallback")) {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 4.0) {
              Toggle(isOn: $settings.configuration.globalConfiguration.enableCgeventtapFallback) {
                HStack {
                  AppLocalizedText("setting.enable_cgeventtap_fallback")
                  AppLocalizedText("settings.defaults.off")
                }
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
                HStack {
                  AppLocalizedText("setting.filter_useless_events_from_specific_devices")
                  AppLocalizedText("settings.defaults.on")
                }
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
                HStack {
                  AppLocalizedText(
                    "setting.reorder_same_timestamp_input_events_to_prioritize_modifiers")
                  AppLocalizedText("settings.defaults.on")
                }
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

              AppLocalizedText("settings.defaults.milliseconds_1000")
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

              AppLocalizedText("settings.defaults.sleep_delay")
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
