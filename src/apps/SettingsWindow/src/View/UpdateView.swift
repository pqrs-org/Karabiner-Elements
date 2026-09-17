import SwiftUI

struct UpdateView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var settings = Settings.shared
  let version =
    Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? ""

  private var defaults: SettingsConfiguration.Defaults {
    settings.configuration.defaultConfiguration
  }

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: AppLocalizedText("settings.update.title")) {
          VStack(alignment: .leading, spacing: 12.0) {
            AppLocalizedText("settings.update.version", arguments: ["version": version])

            Toggle(isOn: $settings.configuration.globalConfiguration.checkForUpdates) {
              AppLocalizedText([
                "settings.update.automatic",
                " ",
                .init(
                  "settings.defaults.value",
                  arguments: [
                    "value": localized(
                      defaults.globalConfiguration.checkForUpdates ? "value.on" : "value.off")
                  ]),
              ])
            }
            .switchToggleStyle()

            HStack {
              Button(
                action: {
                  krbn_updater_check_for_updates_stable_only()
                },
                label: {
                  AppLocalizedLabel("menu_bar_extra.check_for_updates", systemImage: "network")
                }
              )

              Spacer()

              Button(
                action: {
                  krbn_updater_check_for_updates_with_beta_version()
                },
                label: {
                  AppLocalizedLabel("menu_bar_extra.check_for_beta_updates", systemImage: "hare")
                }
              )
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.update.websites")) {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack {
              Button(
                action: {
                  NSWorkspace.shared.open(URL(string: "https://karabiner-elements.pqrs.org/")!)
                },
                label: {
                  AppLocalizedLabel("settings.update.official_website", systemImage: "house")
                })

              Button(
                action: {
                  NSWorkspace.shared.open(
                    URL(string: "https://github.com/pqrs-org/Karabiner-Elements")!)
                },
                label: {
                  AppLocalizedLabel("settings.update.github", systemImage: "hammer")
                })
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
