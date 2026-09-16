import SwiftUI

// For macOS 13, macOS 14
struct SetupDriverExtensionViewMacOS14: View {
  @State private var showingAdvanced = false

  init(showingAdvanced: Bool = false) {
    _showingAdvanced = State(initialValue: showingAdvanced)
  }

  var body: some View {
    VStack(alignment: .center) {
      AppLocalizedLabel(
        "settings.setup.driver_legacy.permission",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      GroupBox {
        VStack(alignment: .center, spacing: 20.0) {
          VStack(alignment: .center, spacing: 0) {
            AppLocalizedText("settings.setup.driver.not_loaded")
            AppLocalizedText(
              "settings.setup.driver_legacy.allow"
            )
          }

          OpenSystemSettingsButton(
            url: "x-apple.systempreferences:com.apple.preference.security?General",
            label: {
              AppLocalizedLabel(
                "settings.system_settings.open_privacy",
                systemImage: "arrow.forward.circle.fill")
            }
          )

          Image(decorative: "dext-allow-macos14")
            .resizable()
            .scaledToFit()
            .frame(height: 300)
            .border(Color.gray, width: 1)

          if !showingAdvanced {
            Button(
              action: { showingAdvanced = true },
              label: {
                AppLocalizedLabel(
                  "settings.setup.driver_legacy.missing_allow",
                  systemImage: "questionmark.circle")
              })
          }
        }.padding()
      }

      if showingAdvanced {
        GroupBox(label: AppLocalizedText("shared.section.advanced")) {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 0) {
              AppLocalizedText(
                "settings.setup.driver_legacy.missing_allow_description"
              )
              AppLocalizedText(
                "settings.setup.driver_legacy.reinstall_hint"
              )
            }

            AppLocalizedText("settings.setup.driver_legacy.reinstall_steps")

            VStack(alignment: .leading, spacing: 10.0) {
              AppLocalizedText(
                "settings.driver.step_deactivate"
              )
              .fixedSize(horizontal: false, vertical: true)

              DeactivateDriverButton()
                .padding(.vertical, 10)
                .padding(.leading, 20)

              AppLocalizedText("settings.driver.step_restart")
                .fontWeight(.bold)
                .fixedSize(horizontal: false, vertical: true)

              AppLocalizedText("settings.setup.driver_legacy.step_activate")
                .fixedSize(horizontal: false, vertical: true)

              ActivateDriverButton()
                .padding(.vertical, 10)
                .padding(.leading, 20)

              AppLocalizedText("settings.setup.driver_legacy.step_blocked")
                .fixedSize(horizontal: false, vertical: true)

              AppLocalizedText("settings.setup.driver_legacy.step_allow")
                .fixedSize(horizontal: false, vertical: true)
            }
          }.padding()
        }
      }
    }
  }
}
