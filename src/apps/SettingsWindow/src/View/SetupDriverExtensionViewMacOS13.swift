import SwiftUI

// For macOS 13, macOS 14
struct SetupDriverExtensionViewMacOS13: View {
  @State private var showingAdvanced = false
  private let driverExtensionImage: String

  init(
    showingAdvanced: Bool = false,
    presentation: SetupDriverPresentation = .macOS13
  ) {
    _showingAdvanced = State(initialValue: showingAdvanced)
    driverExtensionImage = presentation.imageName
  }

  var body: some View {
    VStack(alignment: .leading) {
      AppLocalizedLabel(
        "settings.setup.driver_legacy.permission",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      GroupBox {
        VStack(alignment: .leading, spacing: 20.0) {
          VStack(alignment: .leading, spacing: 0) {
            AppLocalizedText("settings.setup.driver.not_loaded")
            AppLocalizedText(
              "settings.setup.driver_legacy.allow"
            )
          }

          OpenSystemSettingsButton(
            url: "x-apple.systempreferences:com.apple.preference.security?General",
            label: {
              AppLocalizedConstrainedLabel(
                "settings.setup.system_settings.open_privacy",
                systemImage: "arrow.forward.circle.fill")
            }
          )

          Image(decorative: driverExtensionImage)
            .resizable()
            .scaledToFit()
            .frame(height: 300)
            .border(Color.gray, width: 1)

          if !showingAdvanced {
            Button(
              action: { showingAdvanced = true },
              label: {
                AppLocalizedConstrainedLabel(
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
                "settings.setup.driver_legacy.reinstallation_step_001_deactivate"
              )

              DeactivateDriverButton()
                .padding(.vertical, 10)
                .padding(.leading, 20)

              AppLocalizedText("settings.setup.driver_legacy.reinstallation_step_002_restart")
                .fontWeight(.bold)

              AppLocalizedText("settings.setup.driver_legacy.reinstallation_step_003_activate")

              ActivateDriverButton()
                .padding(.vertical, 10)
                .padding(.leading, 20)

              AppLocalizedText("settings.setup.driver_legacy.reinstallation_step_004_blocked")

              AppLocalizedText("settings.setup.driver_legacy.reinstallation_step_005_allow")
            }
          }.padding()
        }
      }
    }
  }
}
