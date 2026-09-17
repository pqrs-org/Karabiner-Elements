import SwiftUI

struct MiscView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var settings = Settings.shared

  private var defaults: SettingsConfiguration.Defaults {
    settings.configuration.defaultConfiguration
  }

  var body: some View {
    ScrollView {
      VStack(alignment: .leading, spacing: 24.0) {
        GroupBox(label: AppLocalizedText("settings.misc.multitouch_title")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Toggle(isOn: $settings.configuration.machineSpecific.enableMultitouchExtension) {
              AppLocalizedText([
                "setting.enable_multitouch_extension",
                " ",
                .init(
                  "settings.defaults.value",
                  arguments: [
                    "value": localized(
                      defaults.machineSpecific.enableMultitouchExtension ? "value.on" : "value.off")
                  ]),
              ])
            }
            .switchToggleStyle()

            AppLocalizedLabel(
              "settings.misc.multitouch_machine_specific",
              systemImage: InfoBorder.icon
            )
            .modifier(InfoBorder())

            if settings.configuration.machineSpecific.enableMultitouchExtension {
              Button(
                action: {
                  KarabinerAppHelper.shared.openMultitouchExtensionSettings()
                },
                label: {
                  AppLocalizedLabel(
                    "menu_bar_extra.multitouch_settings",
                    systemImage: "rectangle.and.hand.point.up.left.filled")
                }
              )
              .disabled(!settings.configuration.machineSpecific.enableMultitouchExtension)

              AppLocalizedLabel(
                "settings.misc.multitouch_menu_hint",
                systemImage: InfoBorder.icon
              )
              .modifier(InfoBorder())
            }
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.misc.export_import")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Button(
              action: {
                var buffer = [Int8](repeating: 0, count: 32 * 1024)
                krbn_get_user_configuration_directory(&buffer, buffer.count)
                guard let path = String(utf8String: buffer) else { return }

                let url = URL(fileURLWithPath: path, isDirectory: true)
                NSWorkspace.shared.open(url)
              },
              label: {
                AppLocalizedLabel(
                  "settings.misc.open_config_folder", systemImage: "arrow.up.forward.app")
              })

            AppLocalizedLabel(
              "settings.misc.backup_hint",
              systemImage: InfoBorder.icon
            )
            .modifier(InfoBorder())
          }
          .padding()
          .frame(maxWidth: .infinity, alignment: .leading)
        }

        GroupBox(label: AppLocalizedText("settings.misc.system_default")) {
          VStack(alignment: .leading, spacing: 12.0) {
            Button(
              action: {
                settings.installSystemDefaultProfile()
              },
              label: {
                AppLocalizedLabel(
                  "settings.misc.install_system_default",
                  systemImage: "square.and.arrow.down")
              })

            AppLocalizedLabel(
              "settings.misc.system_default_hint",
              systemImage: InfoBorder.icon
            )
            .modifier(InfoBorder())

            if settings.systemDefaultProfileExists {
              Button(
                role: .destructive,
                action: {
                  settings.removeSystemDefaultProfile()
                },
                label: {
                  AppLocalizedLabel("settings.misc.remove_system_default", systemImage: "trash")
                    .buttonLabelStyle()
                }
              )
              .deleteButtonStyle()
            } else {
              AppLocalizedText("settings.misc.no_system_default").foregroundColor(
                Color.primary.opacity(0.5))
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
